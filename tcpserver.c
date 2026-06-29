#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h> //for signal handling
#include <errno.h> //for EINTR
#include "threadpool.h"

#define PORT 8080
#define BUFSIZE 2048

#define MAX_BACKLOGGED_CONNECTIONS 5

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig){
    (void)sig;
    keep_running = 0;
}

int main(){
    int welcome_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(welcome_sock < 0){
        perror("Creating welcome socket");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);
    socklen_t serverAddr_len = sizeof(serverAddr);

    int opt = 1;
    setsockopt(welcome_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if((bind(welcome_sock, (struct sockaddr*)&serverAddr, serverAddr_len)) < 0){
        perror("Binding welcome socket to port");
        exit(1);
    }

    //before listening, let's create the threadpool
    Threadpool* pool = pool_create(10); //creating 10 threads for now

    listen(welcome_sock, MAX_BACKLOGGED_CONNECTIONS);

    struct sockaddr_in clientAddr;
    socklen_t clientAddr_len = sizeof(clientAddr);

    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; //not SA_RESTART
    sigaction(SIGINT, &sa, NULL);

    while(keep_running){
        int conn_sock = accept(welcome_sock, (struct sockaddr*)&clientAddr, &clientAddr_len);
        if(conn_sock < 0){
            if(errno == EINTR){
                continue;
            }

            perror("Creating connection socket");
            exit(1);
        }
        pool_submit(pool, conn_sock);
    }
    //handle signal here, since now keep_running = 0 and u are out of loop
    close(welcome_sock);
    pool_destroy(pool);
    printf("\nServer closed. Bye!\n");
    return 0;
}