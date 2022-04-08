#include "myshell.h"
#define MAXLINE 8192
#define MAXARGS 128
#define MAXPIP 20 
#define READEND 0
#define WRITEEND 1 
    
volatile pid_t reaped_id[MAXPIP] = { 0 }; // terminate된 프로세스가 reap됐는지를 알려주는 Array 
volatile pid_t pid[MAXPIP] = { 0 }; // 생성된 자식프로세스의 pid를 저장하는 Array
volatile int pcount; // 생성된 자식 프로세스의 개수 
volatile int reap_count; // reap 된 자식 프로세스의 개수 
volatile int foreground_proc_id; 
volatile bool sigtstp_flag = false; 

int main(void){
    int idx = 0; 
    bool pipe_start_flag = false; 
    signal(SIGINT, sigint_handler); 
    signal(SIGCHLD, sigchld_handler); 
    signal(SIGTSTP, sigtstp_handler); 

    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
        // init global variable(pcount, reap_count)

        pcount = 0; reap_count = 0; 
        memset((pid_t*)reaped_id, 0, sizeof(pid_t) * MAXPIP);
        memset((pid_t*)pid, 0, sizeof(pid_t) * MAXPIP);  

	    printf("> ");                   
	    fgets(cmdline, MAXLINE, stdin); 
	    if (feof(stdin))
	        exit(0);

	    /* Evaluate */
	    eval(cmdline);
    }     
}



void eval(char *cmdline){
        int idx = 0, pidx = 0;
        char * argv[MAXARGS];
        char buffer[MAXARGS];
        char * envp[2] = {"/usr/bin/", "0"}; 
        int fd[2]; 
        pid_t pid[MAXPIP]; 
        sigset_t blocking_mask; 
        int bg, child_status; 

        bool pipe_start_flag = false; // this argv print output to the WriteEnd of Pipe; 
        bool pipe_end_flag = false; // this argv input from ReadEnd of Pipe
        
        
        Sigfillset(&blocking_mask); 
        Sigdelset(&blocking_mask, SIGTSTP); 
        Sigdelset(&blocking_mask, SIGINT);
        Sigdelset(&blocking_mask, SIGCHLD); 

        bg = preproc_command(cmdline, &idx); 
        strcpy(buffer, cmdline);  


        pipe(fd); 
        while(buffer[idx] != '\0'){
                
            /*check if it is piped process*/
            pipe_start_flag = pipe_end_flag;  // if last process was piping process, then this process is a piped process

            parse_line(&buffer[idx], argv, &idx, &pipe_end_flag); // parse until it meets '\0' or '|'(assign its (index-1) on idx)

        
            if((pid[pcount] = Fork()) == 0){
                int big_bro_idx = pcount - 1; // 직전 프로세스의 인덱스 
                if(!pipe_start_flag && pipe_end_flag){
                    dup2(fd[WRITEEND],STDOUT_FILENO); 
                    close(fd[READEND]); 
                }
                else if(pipe_start_flag && !pipe_end_flag){
                    dup2(fd[READEND], STDIN_FILENO);
                    close(fd[WRITEEND]);  

                    while(reaped_id[big_bro_idx] == 0){ // 직전 프로세스가 실행중인 동안 
                        printf("this line works"); 
                        Sigsuspend(&blocking_mask);
                    }
                }   
                else if(pipe_start_flag && pipe_end_flag){
                    dup2(fd[WRITEEND],STDOUT_FILENO); 
                    dup2(fd[READEND], STDIN_FILENO);
                
                    while(reaped_id[big_bro_idx] == 0){ // 직전 프로세스가 실행중인 동안 
                        Sigsuspend(&blocking_mask); // wait for SIGCONT
                    }
                }
                // we should get input from fd[READEND] and put it on argv 
                if(execvp(argv[0], argv) < 0){
                    printf("%s\n", strerror(errno)); 
                    printf("exe error");
                    exit(1);              
                }    
    
            }
            else{ // parent process"
                if(!bg){
                    foreground_proc_id = pid[pcount]; // 현재 fg에서 돌아가고 있는 프로세스 ID
                    printf("%d\n", foreground_proc_id); 
                    while(reaped_id[pcount] == 0 && sigtstp_flag == false){
                        Sigsuspend(&blocking_mask); // wait for sigchld 
                    }
                    sigtstp_flag = false; // reset flag to false(it changed at sigtstp_flag)
                }
                ++pcount; 
            }
            ++idx; 
        } 
}
    
int preproc_command(char * buffer, int *idx){
    
    // delete unnecessary blank from front and end of the command 
    // and check whether it is background process

    while(buffer[*idx] == ' '){
        ++(*idx); 
    }
    
    int end_idx = strlen(buffer)-1; 

    while(buffer[end_idx] == ' ' )
        -- end_idx; 
    buffer[end_idx + 1] = '\0'; 
    
    if(buffer[end_idx] == '&'){
        buffer[end_idx] = '\0';
        return 1;   
    } 
    else{
        buffer[end_idx + 1] = '\0';
        return 0;         
    }
}
void parse_line(char *buffer, char *argv[], int* idx, bool* pipe_start_flag){
/***********
Parse-line: buffer 에 입력된 커맨드를 ArgV에 저장 | ls -al | grep -n "txt"
***********/

        int i = 0; 
        // int i = *idx; 
        int bg; 
        int arg_idx = 0; 
        int end_idx ; 
        char c;
        bool isLastLetter = false; 

        while((c = buffer[i]) != '\0' && c != '|' && c != '\n'){
            if (c != ' ' && isLastLetter == false){
                argv[arg_idx++] = &buffer[i]; 
                isLastLetter = true;  
            }
            else if (c != ' '){
                isLastLetter = true;
            } 
            else if(c == ' ' &&  isLastLetter){
                buffer[i] = '\0'; 
                isLastLetter = false;
            }
            ++i; 
        }
        if(c == '|'){
            *pipe_start_flag = true;  // 이러고서 while문 초입에서 pipe_end_flag = pipe_start_flag 시전
            buffer[i] = '\0'; // eval function 의 while 문 끝에서 idx ++를 한다. 이때 c == '|' 였을 경우 더 읽어야 하므로 i를 증가시키지 않는다. 
        }
        else{ //last character was '\0'; 
            buffer[i] = '\0';
            *pipe_start_flag = false; 
            --i;  // string의 끝에서 eval은 멈춰야 하므로 idx ++ 했을 때 '\0'을 만나도록 idx를 조정한다. 
        }
        argv[arg_idx] = NULL;   // set last index of argument vector as zero  
        *idx = *idx + i;  // 
        return; 
}

/*********************************************
 *Signal handlers
 ********************************************/
 
 void sigchld_handler(int sig){
     sigset_t mask, prev; 
     int olderrno = errno; 

     Sigfillset(&mask); 
     Sigprocmask(SIG_BLOCK, &mask, &prev); 

    reaped_id[reap_count] = Waitpid(pid[reap_count], NULL, 0); 
    printf("%d", reap_count); 
    printf("process %d terminated", reaped_id[reap_count]); 
    
    ++ reap_count; 

    if(pid[reap_count] > 0)
        Kill(pid[reap_count], SIGCONT); 


    Sigprocmask(SIG_SETMASK, &prev, NULL); 
    errno = olderrno; 
}

void sigint_handler(int sig){
    Kill(foreground_proc_id, SIGINT); 
    write(STDIN_FILENO, "\n> ", 3); 
    return; // ignore SIGI
}

void sigtstp_handler(int sig){
    sigset_t mask_all, prev_all;
    int olderrno = errno; 
    Sigfillset(&mask_all); 
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); 

    Kill(foreground_proc_id, SIGTSTP); 
    sigtstp_flag = true; 
    write(STDOUT_FILENO, "hello world\n", 12); 
    Kill(getpid(), SIGCONT); 

    Sigprocmask(SIG_BLOCK, &prev_all, NULL); 
    errno = olderrno; 
}

/*********************************************
 *Wrapper function
 ********************************************/

void unix_error(char *msg) /* Unix-style error */
{
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
}

pid_t Fork(void) 
{
    pid_t pid;

    if ((pid = fork()) < 0)
	unix_error("Fork error");
    return pid;
}
/* $end forkwrapper */

void Execve(const char *filename, char *const argv[], char *const envp[]) 
{
    if (execve(filename, argv, envp) < 0)
	unix_error("Execve error");
}

/* $begin wait */
pid_t Wait(int *status) 
{
    pid_t pid;

    if ((pid  = wait(status)) < 0)
	unix_error("Wait error");
    return pid;
}
/* $end wait */

pid_t Waitpid(pid_t pid, int *iptr, int options) 
{
    pid_t retpid;

    if ((retpid  = waitpid(pid, iptr, options)) < 0) 
	unix_error("Waitpid error");
    return(retpid);
}

/* $begin kill */
void Kill(pid_t pid, int signum) 
{
    int rc;

    if ((rc = kill(pid, signum)) < 0)
	unix_error("Kill error");
}

void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset)
{
    if (sigprocmask(how, set, oldset) < 0)
	unix_error("Sigprocmask error");
    return;
}

void Sigemptyset(sigset_t *set)
{
    if (sigemptyset(set) < 0)
	unix_error("Sigemptyset error");
    return;
}

void Sigfillset(sigset_t *set)
{ 
    if (sigfillset(set) < 0)
	unix_error("Sigfillset error");
    return;
}

void Sigdelset(sigset_t *set, int signum)
{
    if (sigdelset(set, signum) < 0)
	unix_error("Sigdelset error");
    return;
}


int Sigsuspend(const sigset_t *set)
{
    int rc = sigsuspend(set); /* always returns -1 */
    if (errno != EINTR)
        unix_error("Sigsuspend error");
    return rc;
}