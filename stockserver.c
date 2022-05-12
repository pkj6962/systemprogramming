/*
 * echoserveri.c - An iterative echo server
 */
/* $begin echoserverimain */
#include "csapp.h"

#define BUY 0
#define SELL 1
#define SHOW 2
#define EXIT 3

struct Node
{
    int ID;
    int left_stock;
    int price;
    int readcnt;
    sem_t mutex;
    struct Node *left;
    struct Node *right;
    int height;
};

void echo(int connfd, struct Node *root);
void readStockFile(struct Node **root);
void getStockData(char *buffer, int stock_data[]);
void saveData(struct Node *root, FILE *fp);
void updateStock(char *response, struct Node *root, int option, int ID, int cnt);
struct Node *findNode(struct Node *root, int ID);
void decodeCommand(char *command, int order[]);
void bufferStockList(char *response, struct Node *root);
void printPreOrder(struct Node *root);

struct Node *insertNode(struct Node *node, int stock_data[]);

int main(int argc, char **argv)
{
    struct Node *root = NULL;
    int listenfd, connfd;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr; /* Enough space for any address */ // line:netp:echoserveri:sockaddrstorage
    char client_hostname[MAXLINE], client_port[MAXLINE];
    int order[3] = {0, 0, 0};

    if (argc != 2)
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(0);
    }

    readStockFile(&root);

    listenfd = Open_listenfd(argv[1]);

    while (1)
    {
        clientlen = sizeof(struct sockaddr_storage);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, client_hostname, MAXLINE,
                    client_port, MAXLINE, 0);
        printf("Connected to (%s, %s)\n", client_hostname, client_port);
        echo(connfd, root);
        Close(connfd);
    }

    FILE *fp = fopen("stock1.txt", "w");
    saveData(root, fp);
    fclose(fp);

    exit(0);
}
/* $end echoserverimain */

void echo(int connfd, struct Node *root)
{
    int n;
    int order[3];
    char message[MAXLINE];
    char response[MAXLINE] = {0};
    rio_t rio;

    Rio_readinitb(&rio, connfd);
    while ((n = Rio_readlineb(&rio, message, MAXLINE)) != 0)
    {
        printf("server received %d bytes\n", n);

        decodeCommand(message, order);

        /*
        order[0]에 저장된 명령의 종류에 따라 명령을 다르게 처리해야 한다.
        오늘은 show 명령을 받았을 때 response에 주식 목록을 저장해서 전송하는 것까지 해보자.

        */
        switch (order[0])
        {
        case (BUY):
        case (SELL):
            updateStock(response, root, order[0], order[1], order[2]);
            break;
        case (SHOW):
            response[0] = '\0';
            bufferStockList(response, root);
            break;
        case (EXIT):
            break;
        }

        Rio_writen(connfd, response, MAXLINE);
    }
}
void bufferStockList(char response[], struct Node *root)
{
    int MAXLEN = 100;
    char tmp[MAXLEN];

    if (root != NULL)
    {
        bufferStockList(response, root->left);
        sprintf(tmp, "%d %d %d\n", root->ID, root->left_stock, root->price);
        strcat(response, tmp);
        bufferStockList(response, root->right);
    }
}
int max(int a, int b);

void readStockFile(struct Node **root)
{

    // 파일의 데이터를 읽어서 트리에 저장

    char buffer[MAXLINE];
    int stock_data[3];
    FILE *fp = fopen("stock.txt", "r");

    while (fgets(buffer, MAXLINE, fp) != 0)
    {
        getStockData(buffer, stock_data);
        *root = insertNode(*root, stock_data);
    }
    fclose(fp);
}
void getStockData(char *buffer, int stock_data[])
{

    char *temp = buffer;

    stock_data[0] = atoi(temp);

    for (int i = 1; i < 3; i++)
    {
        temp = strchr(temp, ' ');
        temp += 1;
        stock_data[i] = atoi(temp);
    }
}
void saveData(struct Node *root, FILE *fp)
{
    // 전위 탐색하면서 파일에 노드의 정보를 저장

    if (root != NULL)
    {
        saveData(root->left, fp);
        fprintf(fp, "%d %d %d\n", root->ID, root->left_stock, root->price);
        saveData(root->right, fp);
    }
}
void updateStock(char *response, struct Node *root, int option, int ID, int cnt)
{

    // option == 0: buy | option == 1: sell

    struct Node *node = findNode(root, ID);
    if (option == BUY)
    {
        if (node->left_stock < cnt)
            strcpy(response, "Not enough left stocks\n"); // 클라이언트에게 보내는 함수로 수정해야함.
        else
        {
            node->left_stock -= cnt;
            strcpy(response, "[buy] success\n");
        }
    }
    else
    {
        node->left_stock += cnt;
        strcpy(response, "[sell] success\n");
    }
}
struct Node *findNode(struct Node *root, int ID)
{
    if (root->ID == ID)
        return root;
    else if (ID > root->ID)
        return findNode(root->right, ID);
    else
        return findNode(root->left, ID);
}
void decodeCommand(char *command, int order[])
{
    // "buy 1 6" "show" "sell 2 3" "exit"

    if (strstr(command, "buy") || strstr(command, "sell"))
    {
        order[0] = (command[0] == 'b') ? BUY : SELL;
        char *temp = command;
        for (int i = 1; i < 3; i++)
        {
            temp = strchr(temp, ' ');
            temp += 1;
            order[i] = atoi(temp);
        }
    }
    else
    {
        order[0] = (command[0] == 's') ? SHOW : EXIT;
    }
}

/**********************
 * AVL Tree Function
 * ********************/

int height(struct Node *N)
{
    if (N == NULL)
        return 0;
    else
        return N->height;
}
int max(int a, int b)
{
    return (a > b) ? a : b;
}

// create a New Node
struct Node *newNode(int stock_data[])
{
    struct Node *node = (struct Node *)malloc(sizeof(struct Node));

    node->left = node->right = NULL;
    node->height = 1;
    node->readcnt = 0;

    node->ID = stock_data[0];
    node->left_stock = stock_data[1];
    node->price = stock_data[2];

    Sem_init(&node->mutex, 0, 1);

    return node;
}

int getBalance(struct Node *N)
{
    if (N == NULL)
        return 0;
    return height(N->left) - height(N->right);
}

struct Node *rightRotate(struct Node *y)
{
    struct Node *x = y->left;
    struct Node *T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}
struct Node *leftRotate(struct Node *x)
{
    struct Node *y = x->right;
    struct Node *T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}
struct Node *insertNode(struct Node *node, int stock_data[])
{
    int ID = stock_data[0];

    if (node == NULL)
        return newNode(stock_data);
    if (ID < node->ID)
        node->left = insertNode(node->left, stock_data);
    else if (ID > node->ID)
        node->right = insertNode(node->right, stock_data);
    else
        return node;

    node->height = 1 + max(height(node->left), height(node->right));
    int balance = getBalance(node);

    if (balance > 1 && ID < node->left->ID) // 왼쪽으로 치우침 && 새로 삽입한 노드가 자식의 왼쪽 자식으로 삽입됨
        return rightRotate(node);

    if (balance < -1 && ID > node->right->ID)
        return leftRotate(node);

    if (balance > 1 && ID > node->left->ID)
    {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && ID < node->right->ID)
    {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }
    return node; // well balanced
}
void printPreOrder(struct Node *root)
{
    if (root != NULL)
    {
        printPreOrder(root->left);
        printf("%d %d %d\n", root->ID, root->left_stock, root->price);
        printPreOrder(root->right);
    }
}
