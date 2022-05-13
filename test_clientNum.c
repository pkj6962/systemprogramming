// 성능 분석 프로그램
/*

동시 처리율: 처리량(client * 요청수) / elapsed_time (초당 처리 주문건수)

1. 쓰레드 개수와 처리율의 상관관계
2. 버퍼 사이즈와 처리율의 상관관계


3. 클라이언트 수와 처리율의 상관관계
4. 클라이언트 요청 타입과 처리율의 상관관계
    show only(recursive traversal): 51137522 (100 thread, 100 clients, 50 orders)
    write only: 50379843

가설: read 요청(show)이 write 요청(buy, sell) 보다 덜 비쌀 것이다.
근거: read 요청은 동시에 여러 쓰레드가 동시에 처리할 수 있는 반면,
write 요청은 한 시점에 하나의 노드를 한 쓰레드만 처리할 수 있다.

*/

// 클라이언트 개수별 처리율 분석.
#include "csapp.h"
#define MAXNUM 100

int main(void)
{

    int ClientNum = 1, idx = 0, status;
    pid_t pid[MAXNUM];
    char TNum[5];
    char *argv[5] = {"./multiclient", "127.0.1.1", "12345", TNum, NULL};
    for (; ClientNum <= MAXNUM; ClientNum += 3)
    {
        pid[idx] = Fork();
        if (pid[idx] == 0)
        {
            sprintf(TNum, "%d", ClientNum);
            Execve(argv[0], argv, NULL);
        }
        else
        {
            Waitpid(pid[idx], &status, 0);
            idx++;
        }
    }

    /*

1. Fork
2. multiclient 프로그램 실행
3. multiclient 가 실행되고부터 죽을 때까지의 시간 측정: multiclient.c에서 코드 수정

4. reap 잘해주기




    */
}