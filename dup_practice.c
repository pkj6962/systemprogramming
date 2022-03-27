/*
DUP() Function Practice.

I will call two Child Process from Parent Process. And First Process will redirect its stdout, Second Process will redirect its stdin so that they share the output.  

*/

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

int main(void){

    pid_t pid[2]; 
    int fd1; 
    char *envp[2] = {"usr/bin/", '\0'}; 
    char *args[2] = {"/usr/bin/ls", NULL}; 

    char *args_grep[4] = {"/usr/bin/grep", "h", "csapp.h", NULL}; 


    


    if((fd1 = open("temp", O_RDWR|O_CREAT, S_IRUSR|S_IWUSR)) < 0){
        printf("error");
        exit(1); // temp 라는 파일이 생길까? 생겼다.
    }

    if(dup2(fd1, STDOUT_FILENO) < 0){
        printf("dup error"); 
        exit(1); 
    } // 부모 process의 file descriptor를 자식이 상속받으므로 자식 프로세스도 fd1으로 출력  
    printf("where will it be printed?");
    
    if((pid[0] = fork()) == 0)
        execve(args[0], args, envp); 

    // if((pid[1] = fork()) == 0)
    //     execvpe(args_grep[0], args_grep, envp); 


}