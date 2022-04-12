/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *								 *
 * Sogang University						 *
 * Computer Science and Engineering				 *
 * System Programming						 *
 *								 *
 * Project name : MyShell					 *
 * FIle name    : myshell.c     				 *
 * Author       : 20160051 Park JungHwan				 *
 * Date         : 2022 - 04 - 13				 *
 *								 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */



#include "myshell.h"
#define MAXLINE 8192
#define MAXARGS 128
#define MAXPIP 20 
#define MAXJOB 100 
#define READEND 0
#define WRITEEND 1
#define SUSPENDED 0  
#define RUNNING 1    



volatile pid_t reaped_id[MAXPIP] = { 0 }; // terminate된 프로세스가 reap됐는지를 알려주는 Array 
//volatile pid_t pid[MAXPIP] = { 0 }; // 생성된 자식프로세스의 pid를 저장하는 Array
//volatile int pcount; // 생성된 자식 프로세스의 개수 
pid_t pid[MAXPIP] = {0}; 
int pcount;

volatile int reap_count; // reap 된 자식 프로세스의 개수 
volatile int foreground_proc_id = - 1; 
char foreground_proc_name[128]; 
volatile bool sigtstp_flag = false; 
volatile int jobCnt = 0; // joblist에 있는 job의 개수 
volatile int bg, child_status; 

struct Job joblist[100]; 
struct Job null_job = {NULL, NULL, NULL, NULL}; 


int main(void){
    int idx = 0; 
    bool pipe_start_flag = false; 

    signal(SIGINT, sigint_handler); 
    signal(SIGCHLD, sigchld_handler); 
    signal(SIGTERM, sigterm_handler); // ignore sigterm 
   signal(SIGTSTP, sigtstp_handler); 

    initJobList(); 


    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
        // init global variable(pcount, reap_count)
        pcount = 0; reap_count = 0; 
        memset((pid_t*)reaped_id, 0, sizeof(pid_t) * MAXPIP);
        memset((pid_t*)pid, 0, sizeof(pid_t) * MAXPIP);  

	    printf("CSE4100-SP-P1> ");                   
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
        int fd[MAXPIP][2]; 
        
                    bool temp_flag = false; 

        sigset_t blocking_mask; 

        bool pipe_start_flag = false; // this argv print output to the WriteEnd of Pipe; 
        bool pipe_end_flag = false; // this argv input from ReadEnd of Pipe
        
        
        Sigfillset(&blocking_mask); 
        Sigdelset(&blocking_mask, SIGTSTP); 
        Sigdelset(&blocking_mask, SIGINT);
        Sigdelset(&blocking_mask, SIGCHLD); 
        Sigdelset(&blocking_mask, SIGCONT); 


        bg = preproc_command(cmdline, &idx); 
        strcpy(buffer, cmdline);  

        while(buffer[idx] != '\0'){
                
            /*check if it is piped process*/
            pipe_start_flag = pipe_end_flag;  // if last process was piping process, then this process is a piped process

            parse_line(&buffer[idx], argv, &idx, &pipe_end_flag); // parse until it meets '\0' or '|'(assign its (index-1) on idx)
            if(pipe_end_flag) 
                pipe(fd[pcount]); 
            if(argv[1] != NULL) quote_mark_check(&argv[1]); 

            if(!builtin_command(argv)){
                pid[pcount] = Fork(); 
                if(pid[pcount] == 0){
                    int big_bro_idx = pcount - 1; // 직전 프로세스의 인덱스
                    if(pipe_end_flag){
                       dup2(fd[pcount][WRITEEND],STDOUT_FILENO); 
                    }
                    if(pipe_start_flag){
                        dup2(fd[pcount-1][READEND] ,STDIN_FILENO); 
                    }

                    for(int i = 0; i < pcount; i++){
                        close(fd[i][READEND]); 
                        //close(fd[i][WRITEEND]); 
                    }
                    if(!pipe_start_flag || pipe_end_flag){ // if it is not the last process
                        close(fd[pcount][READEND]);
                        close(fd[pcount][WRITEEND]); 
                    }

                    if(execvp(argv[0], argv) < 0){
                        printf("%s: command not found\n", argv[0]);
                        exit(1);              
                    }    
        
                }
                else{ // parent proce
                    if(pipe_end_flag){
                        close(fd[pcount][WRITEEND]); 
                    }
                    if(!bg){
                        foreground_proc_id = pid[pcount]; // 현재 fg에서 돌아가고 있는 프로세스 ID
                        strcpy(foreground_proc_name, argv[0]); 
                        
                        pidx = waitpid(-1, NULL, WUNTRACED);
                        if(pidx == foreground_proc_id)
                            foreground_proc_id = -1; // no foreground job
                        // while(reaped_id[pcount] <= 0 && sigtstp_flag == false){
                        //     Sigsuspend(&blocking_mask); // wait for sigchld 
                        sigtstp_flag = false; // reset flag to false(it changed at sigtstp_flag)
                    }
                    else{ // background process
                        struct Job J = getJob(pid[pcount], argv[0], RUNNING);  
                        addJob(J); 
                    }
                    ++pcount; 
                }
            }
            ++idx; 
        }   
    if(pipe_start_flag){
        for(int i = 1; i < pcount ; i++){
            close(fd[i-1][READEND]);
         //   close(fd[i-1][WRITEEND]);
        }


    }
    
} 

int builtin_command(char * argv[]){
    
    if(!strcmp(argv[0], "cd")){
        if(chdir(argv[1]) < 0){
            printf("no such file or directory\n");
        }
        return 1; 
    }

    if(!strcmp(argv[0], "exit")){
        /*
            프로세스 그룹 내 모든 프로세스를 종료시킴
        */
        Kill(-getpid(), SIGTERM); 
        exit(0); 
    }

    if(!strcmp(argv[0], "jobs")){
        printjob(); 
        return 1; 
    }

    if(!strcmp(argv[0], "kill")){
        int idx = get_job_idx(argv[1]);
        if(is_valid(idx))
            killJob(idx); 
        return 1; 
    }

    if(!strcmp(argv[0], "bg")){
        int idx = get_job_idx(argv[1]); 
        if(is_valid(idx))
           change_bg(idx); 
        return 1; 
    }

    if(!strcmp(argv[0], "fg")){
        int idx = get_job_idx(argv[1]); 
        if(is_valid(idx))
            change_fg(idx); 
        return 1; 
    }


    return 0; 
}

void quote_mark_check(char **argv){ //argv[1]이 들어올 거임 ("cat")
    if(*argv[0] == '\"' || *argv[0] == '\''){
        *argv = *argv + 1; 
        char mark = (*(*argv -1) == '\"')? '\"' : '\'';
        char* mark_pos = *argv, *last; 
        while((mark_pos = strchr(mark_pos, mark)) != NULL){
            mark_pos = mark_pos + 1; 
            last = mark_pos; 
        }
        last = last - 1; 
        *last = '\0';

    }
}

int preproc_command(char * buffer, int *idx){
    
    // delete unnecessary blank from front and end of the command 
    // and check whether it is a background process

    while(buffer[*idx] == ' '){
        ++(*idx); 
    }
    
    int end_idx = strlen(buffer)-1; 

    while(buffer[end_idx] == ' ' || buffer[end_idx] == '\n')
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
        bool inside_quot_mark = false; 


        while((c = buffer[i]) != '\0' && c != '|' && c != '\n'){
            if(c == '\'' || c == '\"'){
                if(!inside_quot_mark){
                    argv[arg_idx++] = &buffer[i]; 
                    inside_quot_mark = true;
                    isLastLetter = true; 
                }
                else{
                    isLastLetter = true;
                }
            }
            else if (c != ' ' && isLastLetter == false){ // 현재문자: 문자 && 직전문자: 공백
                if(inside_quot_mark == true);
                else{
                    argv[arg_idx++] = &buffer[i]; 
                    isLastLetter = true;
                }
            }
            else if (c != ' '){ // 현재문자: 문자 && 직전문자: 문자 
                isLastLetter = true;
            } 
            else if(c == ' ' &&  isLastLetter){ // 현재문자: 공백 && 직저문자: 문자
                if(inside_quot_mark == true);
                else{
                    buffer[i] = '\0'; 
                    isLastLetter = false;
                }
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
/*********************************`************
 *Buiiltin Command 
 ********************************************/

int get_job_idx(char *arg){
    return atoi(&arg[1]); // '%'를 제외한 숫자를 반환한다. 
}
bool is_valid(int idx){
    if(joblist[idx].pid == NULL){
        write(STDOUT_FILENO, "No Such Job\n", 12); 
        return 0;
    }
    else
        return 1; 
}

void killJob(int idx){ 
   
    pid_t pid = joblist[idx].pid;

    Kill(pid, SIGTERM); 
    deleteJob(pid);
}

void change_bg(int idx){
    int pid = joblist[idx].pid;
    joblist[idx].state = RUNNING; 
    Kill(pid, SIGCONT); 
}

void change_fg(int idx){
    int pid = joblist[idx].pid;
    foreground_proc_id = joblist[idx].pid;
    strcpy(foreground_proc_name, joblist[idx].pname);
    bg = 0; 
    deleteJob(pid); 
    Kill(pid, SIGCONT); 
    Waitpid(pid, NULL, WUNTRACED); 
} 


struct Job getJob(pid_t pid, char* name, int state){
    struct Job P; 
    strcpy(P.pname, name); 
    P.pid = pid; 
    P.state = state;
    P.idx = NULL; 

    return P; 
}

void initJobList()
{
    for(int i = 1; i < MAXJOB; i++){
        joblist[i] = null_job; 
    }
    jobCnt = 0; 
}

void printjob()  // jobCnt 개수만큼의 job을 joblist에서 포맷에 맞게 출력한다.
{
    int cnt = jobCnt; 
    for(int i = 1; i < MAXJOB; i++){
        if(joblist[i].pid != NULL){
            printf("[%d] %s %s\n", i, (joblist[i].state)? "running":"suspended", joblist[i].pname); 
            cnt--; 
        }
        if(cnt == 0) break; 
    }
}

void addJob(struct Job P) // 1번부터 빈(NULL) 인덱스를 찾아서 정보를 집어넣는다. 
{   
    int i; 
    for(i = 1; i < MAXJOB; i++){
        if(joblist[i].pid == NULL)
            break; 
    }
    if(i == MAXJOB){
        printf("JOB LIST FULL");
        exit(1); 
    }
    P.idx = i; 

    joblist[i] = P; 
    jobCnt ++; 
}

void deleteJob(pid_t pid){
    for(int i = 1; i < MAXJOB; i++){
        if(joblist[i].pid == pid){
            joblist[i] = null_job;
            jobCnt --;
            break; 
        } 
    }
}
/*********************************`************
 *Signal handlers
 ********************************************/
 
 void sigchld_handler(int sig){
     sigset_t mask, prev; 
     int status; 
     int olderrno = errno; 


     Sigfillset(&mask); 
     Sigprocmask(SIG_BLOCK, &mask, &prev); 

    while((reaped_id[reap_count] = waitpid(-1, &status, WNOHANG)) > 0)
    {
        if(foreground_proc_id == reaped_id[reap_count])
            foreground_proc_id = - 1; 
        deleteJob(reaped_id[reap_count]); 
        reap_count ++; 
    }
    
    if(pid[reap_count] > 0 && WIFSTOPPED(status)){ // 동생 프로세스가 있으면 && stopped된 상태라면
        Kill(pid[reap_count], SIGCONT);  // 블럭 되어 있던 동생 프로세스에게 SIGCONT 시그널을 날림 
        write(STDOUT_FILENO, "I alarmed my sibling", 20); 
    }

    Sigprocmask(SIG_SETMASK, &prev, NULL); 
    errno = olderrno; 
}
void sigterm_handler(int sig){
    return ;
}

void sigint_handler(int sig){
    if(foreground_proc_id != -1) Kill(foreground_proc_id, SIGINT); 
    write(STDIN_FILENO, "\n> ", 3); 
    return; // ignore SIGI
}

void sigtstp_handler(int sig){
    sigset_t mask_all, prev_all;
    int olderrno = errno; 
    Sigfillset(&mask_all); 
    Sigprocmask(SIG_BLOCK, &mask_all, &prev_all); 

    write(STDOUT_FILENO, "\n> ", 3);
    if(!bg){
        if(foreground_proc_id != -1) {
            Kill(foreground_proc_id, SIGTSTP); 
            struct Job J = getJob(foreground_proc_id, foreground_proc_name, SUSPENDED); 
            addJob(J); 
        }
    }

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