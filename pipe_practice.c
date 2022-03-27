/*
I will pipe two child process so that one child's output can be the input of the other. 
*/

#include <unistd.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#define READ 0
#define WRITE 1 

int main(void){

    int fd[2]; 
    pid_t pid[2]; 
    char *envp[2] = {"usr/bin/", '\0'}; 
    char *args_grep[4] = {"/usr/bin/grep", "w", "csapp.h", NULL}; 
    char *args[2] = {"/usr/bin/ls", NULL}; 

    char readBuffer[100]; 
    pipe(fd); // 0th is for read, 1st is for write 

    char *msg = "Hello World.\0"; 
    if((pid[0] = fork()) == 0){
        close(fd[READ]); 
        if(dup2(fd[WRITE], STDOUT_FILENO) < 0){
            printf("error"); 
            exit(1); 
        }
        execve(args[0], args, envp); 
        
        //its output are sent to fd[1]. 

    }


    if((pid[1] = fork()) == 0){
        close(fd[WRITE]); 
        dup2(fd[READ], STDIN_FILENO); 
        read(fd[READ], readBuffer, sizeof(readBuffer)); 

        printf("%s", readBuffer); 

        // args_grep[2] = readBuffer;
        // execve(args_grep[0], args_grep, envp);  
    }
    // next mission: we will send one child's output 
    // to the other child's new process(execve) argument

}