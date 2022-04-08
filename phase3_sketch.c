/* Phase3 

/************************** 
 * Description
 **************************/
/*

job: display running or stopped background job
[number] suspended(running) 

/************************** 
 * Idea
 **************************
/************************** 
 * JOB ADT 
 **************************
1) 어떤 프로그램이 백그라운드에서 실행되면, 해당 프로그램을 자료구조에 저장한다.
2) 그 프로그램이 어떠한 이유로든 terminate되면(sigchld_handler에서 deletejob), 자료구조에서 해당 프로그램을 삭제한다. 



void initJobList{
    1. Array 모든 원소 NULL로 초기화
    2. pcount = 0 으로 초기화 
}

struct process getProcess(pid_t pid, char* name){
    1. Fork()  전후로 JobList에 넣기 위해 Process Struct를 만들어서 넣을 것이다. 
    2. pid와 argv[0]으로써 struct Process의 뼈대를 구축할 것이다. 

}

void addjob(struct process) // 1번부터 빈(NULL) 인덱스를 찾아서 정보를 집어넣는다. 
{
    실행조건
    1. 백그라운드로 프로세스가 실행됐을 때(Fork()후) 
    2. bg 명령어를 받았을 때 
}


void deletejob(pid_t pid); // joblist에서 pid를 가지는 원소를 찾아서 NULL로 만든다. 
{   
    실행조건 
    1. 해당 pid가 bg에서 fg로 바꼈을 때(fg 명령어를 받았을 때)

}

void printjob // jobCnt 개수만큼의 job을 joblist에서 포맷에 맞게 출력한다. 

void changeJobStatus(pid_t pid); // joblist에서 pid를 가진 process를 찾아서 process의 status를 바꾼다. 
{
    실행조건: 
    1. suspended가 걸렸을 때(CIRL + Z)
    --> foreground가 돌아가고 있을 때 CIRL + Z 또는 CIRL + C 를 입력해도, fg는 멈추지(종료되지) 않는다. myshell이 SIGNAL을 직접 주어야 한다. 

}

/************************** 
 * Implementation
 **************************
typedef enum {suspended, running}; 
typedef struct process{
    char pname[128];
    pid_t pid; 
    int state;
    int idx; 
 }

자료구조 

struct process joblist[100]; 
int jobCnt; // joblist에 있는 job의 개수 

struct process getProcess(pid_t pid, char* name){
    struct process P; 
    P.pname = name; 
    P.pid = pid; 
    P.state = running;
    P.idx = NULL; 

    return P; 
}

void addjob(struct process P) // 1번부터 빈(NULL) 인덱스를 찾아서 정보를 집어넣는다. 
{   
    int i; 
    for(i = 1; i < MAXJOB; i++){
        if(joblist[i] == NULL)
            break; 
    }
    if(i == MAXJOB){
        printf("JOB LIST FULL");
        exit(1); 
    }
    P.idx = i; 

    joblist[i] = P; 
    jobcnt ++; 
}
void deletejob(pid_t pid); // joblist에서 pid를 가지는 원소를 찾아서 NULL로 만든다. 
void printjob // jobCnt 개수만큼의 job을 joblist에서 포맷에 맞게 출력한다. 

void changeJobStatus(pid_t pid); // joblist에서 pid를 가진 process를 찾아서 process의 status를 바꾼다. 



*/


/************************** 
 * bg, fg 
 **************************

void bg(int idx)
{
    1. joblist에서 해당 idx를 참조해서 pid를 찾아낸다. 
    1. pid에게 SIGCONT 시그널을 날린다.
    3. joblist 해당 idx의 status 를 suspended 에서 running으로 바꾼다.  
}

void fg(int idx)
{
    1. joblist에서 해당 idx를 참조해서 pid를 찾아낸다.
    2. 해당 프로세스가 block되어 있는 상태였다면 SIGCONT 시그널을 날린다. 
    2. joblist에서 해당 idx를 NULL값으로 만든다(pcount -= 1)
    3. myshell이 waitpid(pid) 해야 함. 
    (what if the process get CIRL + Z again? --> deadlock?)


}


