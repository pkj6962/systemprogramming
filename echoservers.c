#include "csapp.h"

typedef struct{
    int maxfd; // max file descriptor 
    fd_set read_set;
    fd_set ready_set; 
    int nready; // number of ready(unused) descriptors (FD_SETSIZE - used descriptor)
    int maxi; // max index of client fd array          
    int clientfd[FD_SETSIZE];
    rio_t clientrio[FD_SETSIZE];

}pool; /* a pool of connected descriptors */

int byte_cnt = 0;

void init_pool(int listenfd, pool *p);
void check_clients(pool *p);
void add_client(int connfd, pool *p);

int main(int argc, char**argv){
    int listenfd, connfd; 
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; 
    static pool pool;
    char client_hostname[MAXLINE], client_port[MAXLINE];  
    // pool pool; 

    if(argc != 2){
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);

    int cnt = 0; 
    while(1){
        pool.ready_set = pool.read_set;
        pool.nready = Select(pool.maxfd+1, &pool.ready_set, NULL, NULL, NULL);

        if(FD_ISSET(listenfd, &pool.ready_set)){
            clientlen = sizeof(struct sockaddr_storage); 
            connfd = Accept(listenfd, (SA*)&clientaddr, &clientlen); 
            Getnameinfo((SA*)&clientaddr, clientlen, client_hostname, MAXLINE, client_port, MAXLINE, 0); 
            printf("connected to (%s, %s)\n", client_hostname, client_port); 
            add_client(connfd, &pool);
        }
        check_clients(&pool); 
        printf("%dth iteration Done\n", cnt++);
    }

}
void init_pool(int listenfd, pool *p){
    int i;
    p->maxi = -1; 
    for(i = 0; i < FD_SETSIZE; i++){
        p->clientfd[i] = -1; 
    }

    p->maxfd = listenfd; 
    FD_ZERO(&p->read_set);
    FD_SET(listenfd, &p->read_set); 
}
void add_client(int connfd, pool *p){
    int i; 
    p->nready --; // number of unsed descriptor decreased

    for(i = 0; i < FD_SETSIZE; i++){
        if(p->clientfd[i] < 0){ // unused descriptor
            p->clientfd[i] = connfd; 
            Rio_readinitb(&p->clientrio[i], connfd);  // then message sent to clientrio is sent to connfd

            FD_SET(connfd, &p->read_set); 

            if(connfd > p->maxfd)
                p->maxfd = connfd; 
            if(i > p->maxi)
                p->maxi = i ;
            break; 
        }
    }
    if(i == FD_SETSIZE){
        app_error("add_client error: Too many Clients"); 
    }
}

void check_clients(pool *p){

    int i = 0, connfd, n; 
    char buf[MAXLINE];
    rio_t rio; 

    for(i = 0; i <= p->maxi && p->nready> 0; i++){
        connfd = p->clientfd[i]; 
        rio = p->clientrio[i];

        if((connfd > 0) && (FD_ISSET(connfd, &p->ready_set))){
            p->nready --; 
            if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
                byte_cnt += n; 
                printf("Server received %d (%d total) bytes on fd %d\n", n, byte_cnt, connfd); 
                Rio_writen(connfd, buf, n); 
                printf("fd %d: Wrote back Successfully\n", connfd); 
            }
            else{ // EOF detected 
                Close(connfd);
                p->clientfd[i] = -1; 
                FD_CLR(connfd, &p->read_set); 
            }
        }

    }
}
