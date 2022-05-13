#include "csapp.h"
#define MAXLEN 100

int main(void)
{
    int num_thread = 1, idx = 0, status;
    pid_t server_id[MAXLEN];
    pid_t client_id[MAXLEN];
    char *localhost = "127.0.1.1";
    char *portNum = "12345";
    char *num_client = "50";
    char Tnum[5];
    char *server_argv[] = {"./stockserver", portNum, Tnum, NULL};
    char *client_argv[] = {"./multiclient", localhost, portNum, num_client, Tnum, NULL};

    for (; num_thread <= MAXLEN; num_thread += 3)
    {
        sprintf(Tnum, "%d", num_thread);
        server_id[idx] = Fork();
        if (server_id[idx] == 0)
        {
            Execve(server_argv[0], server_argv, NULL);
        }
        else
        {
            sleep(3);
            client_id[idx] = Fork();
            if (client_id[idx] == 0)
            {
                Execve(client_argv[0], client_argv, NULL);
            }
            else
            {
                if (Waitpid(client_id[idx], &status, 0) > 0)
                { // 실험 서버에 대응되는 클라이언트 프로세스가 종료되면 실험 서버를 종료한다.
                    Kill(server_id[idx], SIGINT);
                    Waitpid(server_id[idx], &status, 0);
                }
            }
            idx++;
        }
    }
}
