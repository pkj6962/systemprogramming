#include "csapp.h"
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#define MAXARGS 128
#define MAXPIP 20 
#define READEND 0
#define WRITEEND 1 
    
void eval(char *cmdline);
int preproc_command(char * buffer, int *idx);
void parse_line(char * buffer, char *argv[], int* idx, bool *pipe_start_flag); 


int main(void){
    int idx = 0; 
    bool pipe_start_flag = false; 

    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	/* Read */
	    printf("> ");                   
	    fgets(cmdline, MAXLINE, stdin); 
	    if (feof(stdin))
	        exit(0);

	    /* Evaluate */
	    eval(cmdline);
    }     
}

 

/*
void eval(char *cmdline){
      */ 

    /*

    ************

    1. cmdline 의 명령어를 parse_line 해서 char * argv[] 에 저장
    2. 자식 프로세스를 생성(fork)해서 실행(execve)
    
    ************

    pid_t pid[MAXPIP]; 
    int process_cnt;  // counted in parse_line function

    char buffer[MAX_LINE]; //cmd_line상 string을 buffer에 copy 
    char* argv[MAX_ARGS];

    int bg; // process가 bg인지 fg인지에 따라 다른 값 할당

    strcpy(cmdline, buffer); 
    parse_line(buffer, arv); 

    if(!bg)
        wait(); 

    */
   

    /*

    ***********
    How to Implement Pipe
    ***********

    ***********
    Idea 
    ***********
    - 파이프는 재귀로
        dup(fd, stdout)를 통해 stdout으로 출력하던 ls의 출구를 새로운 fd로 보낼 수 있다.
        dup(fd ,stdin)를 통해 stdin으로 입력받던? grep의 입구를 새로운 입구로 대신 설정할 수 있다. 

    따라서 파이프입구쪽 프로세스를 실행하기 전  fd:stdout 을 fd[WRITE_END]로 바꾸어 놓고 프로세스를 실행한다.
    마찬가지로 파이프출구쪽 프로세스를 실행하기전 fd:stdin을 fd[READ_END]로 바꾸어 놓은 다음 출력한다.

    
    dup() --> execvpe(ls) --> eval(buf[i+1]) 



    parse_line(cmdline, argv); 
    - argv에서 각 process가  시작하는 위치를 저장하는 배열을 하나 선언한다. 

   
   
    ***********
    Eval() FUnction: Psudo-Code 
    ***********
    
    eval(cmdline){

        bool    pipe_start_flag = false; 
        bool    pipe_end_flag = false; 

        UNTIL(buffer meets EndOfString)
            eval()
            parse_line until encountering '|' or '\0'
            if(buffer[i] == '|') 
                pipe_end_flag = true; 
        
            if(마지막args == '|')
                pipe_start_flag = true; 


            if((fid = fork()) == 0){
                if(pipe_end_flag) dup(fd[WRITE_END],stdout)
                execvpe(); 
                exit(); // this line will not be executed 
            }
        
    }
    ************/

void eval(char *cmdline){
        int idx = 0, pidx = 0;
        char * argv[MAXARGS];
        char buffer[MAXARGS];
        char * envp[2] = {"/usr/bin/", "0"}; 
        int fd[2]; 
        pid_t pid[MAXPIP]; 
        int bg, child_status; 

        bool pipe_start_flag = false; // this argv print output to the WriteEnd of Pipe; 
        bool pipe_end_flag = false; // this argv input from ReadEnd of Pipe

        bg = preproc_command(cmdline, &idx); 
        strcpy(buffer, cmdline);  


        pipe(fd); 
        while(buffer[idx] != '\0'){
                
            /*check if it is piped process*/
            pipe_start_flag = pipe_end_flag;  // if last process was piping process, then this process is a piped process

            parse_line(&buffer[idx], argv, &idx, &pipe_end_flag); // parse until it meets '\0' or '|'(assign its (index-1) on idx)

        
            if((pid[pidx] = fork()) == 0){
                if(!pipe_start_flag && pipe_end_flag){
                    dup2(fd[WRITEEND],STDOUT_FILENO); 
                    close(fd[READEND]); 
                }
                else if(pipe_start_flag && !pipe_end_flag){
                    dup2(fd[READEND], STDIN_FILENO);
                    close(fd[WRITEEND]);  
                    // waitpid(pid[pidx-1], &child_status, NULL); 
                }   
                else if(pipe_start_flag && pipe_end_flag){
                    dup2(fd[WRITEEND],STDOUT_FILENO); 
                    dup2(fd[READEND], STDIN_FILENO);
                }
                // we should get input from fd[READEND] and put it on argv 
                if(execvp(argv[0], argv) < 0){
                    printf("%s\n", strerror(errno)); 
                    printf("exe error");
                    exit(1);              
                }    
    
            }
            else if(pid[pidx] < 0){
                printf("Error");
                exit(1); 
            } 
            else{ // parent process
                // if(!bg){
                //     wait(&child_status); 
                // }
            }
            ++idx; 
        }
}
    

/*
    ***********
    Remaining Problem To Solve 
    ***********
    1. piped process 는 piping process가 terminate 될 때까지 기다려야 한다. 
        1) piping process가 부모에게 SIGCHLD를 날린다. 부모는 그 Signal을 받고 이 소식을 piped proccess 에게 날린다(: 어떤 Signal? UserDefined Signal?). 
    2. can process reap its sibling process?
    



*/

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


    /*
    0. buffer를 처음부터 끝('\0')까지 쭉 봐야 함
        2) 문자 끝 '공백' 또는 '|'에 '\0' 할당
            i) Encounter ' ' when IsLastLetter == true : 
                buffer[i] = '\0';
                IsLastLetter = false;  

            공백(&, |)을 만났는데 이전 char가 문자였으면(flag=true) '\0'으로
            ii) encounter '|': 
                if(IsLestLetter) buffer[i] = '\0';
                pidx +=1; argIdx = 0;  
                IsLastLetter = false;  


        3) 문자를 만났는데 이전 char가 ' '였으면 해당 주소를 argv상 원소로
                if(ISLastLetter == False)  
            i) argv[pidx][argIdx] = &buffer[i]; 
                argIdx += 1; 
                IsLastLetter = true; 




    }
    
    ***********
    Parse_line Psudo Code
    ***********
    - What Shoulld be passed by reference from Eval()
        - buffer
        - argv
        - 프로세스 별 arg 개수? (Is it needed?)

    Parse_line(buffer, argv);


    case)

    1) buffer= '     ls -al | grep 'rwx'&' 
    2) ''
    3) 'ls -al|grep 'rwx'
    argv[0] = 'ls' '-al' '\0'
    argv[1] = 'grep' ''rwx'



    - Flag: 
    bool lastLatter: - 직전 char가 글자였나요(T), 공백 또는 '|' 였나요(F). 
                     - declare: False 

    - 다 끝낸 다음, 
    만약 마지막이 '&'였으면 이것은 마지막 프로세스의 마지막 arg로 들어간다. 
    따라서 마지막 프로세스 마지막 arg를 체크해서 '&'이면 이를 '\0'으로 바꾸고 
    bg = 1로 할당해준다.

    - 만약 '&'가 이전 커맨드에 붙여서 입력되면? 
        마지막 args를 '\0'을 만날 때까지 스캔한다, say,
        ! 이는 파이프를 고려하지 않는 방식이다. 

        i = 0;
        lastArg = &argv[pidx][argIdx-1] 
        while(*(last + i)) != '\0' && *(lastArg + i) != '&')
            i += 1; 
        if(*(lastArg + i) == '&'){
            *(lastArg + i) = '\0';
            bg = 1; 
        }

        

    - 인덱스 관리 
        1) buffer index: i 
        3) argv index: argIdx - 공백 직후 문자를 만날 때 마다 증가
     
    */




