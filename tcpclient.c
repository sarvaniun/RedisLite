#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <string.h>

#define PORT 8080
#define BUFSIZE 2048

int main(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0){
        perror("Creating client socket");
        exit(1);
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);
    socklen_t serverAddr_len = sizeof(serverAddr);

    if((connect(sock, (struct sockaddr*) &serverAddr, serverAddr_len)) < 0){
        perror("Connecting to server");
        exit(1);
    }

    //connected now
    char input[1000];
    while(1){
        printf("> ");
        fgets(input, sizeof(input), stdin);
        //printf("'%s'\n", input);
        //printf("About to send: '%s'\n", input);
        write(sock, input, strlen(input)+1);
        char buffer[BUFSIZE];
        int recvlen = read(sock, buffer, BUFSIZE-1);
        if(recvlen < 0){
            perror("Reading reply from server");
            continue;
        }
        buffer[recvlen] = '\0';
        printf("%s\n", buffer);   
    }
}