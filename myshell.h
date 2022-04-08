#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

 void sigchld_handler(int sig);
 void sigint_handler(int sig);
void sigtstp_handler(int sig); 

void eval(char *cmdline);
int preproc_command(char * buffer, int *idx);
void parse_line(char * buffer, char *argv[], int* idx, bool *pipe_start_flag);


 pid_t Fork(void) ;
 void Execve(const char *filename, char *const argv[], char *const envp[]) ;
 pid_t Wait(int *status) ;
 pid_t Waitpid(pid_t pid, int *iptr, int options) ;
 void Kill(pid_t pid, int signum) ;


int Sigsuspend(const sigset_t *set); 
void Sigprocmask(int how, const sigset_t *set, sigset_t *oldset);
void Sigemptyset(sigset_t *set);
void Sigfillset(sigset_t *set);
void Sigdelset(sigset_t *set, int signum);
void unix_error(char *msg);/* Unix-style error */