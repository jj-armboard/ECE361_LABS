#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define SERVERPORT "4950"
#define MAX_BUFFER_SIZE 100

int main(int argc, char *argv[]) {
   
   int sockfd = 0;
   int bytes_sent = 0;
   int len = 0; 

   char message[] = "John is gay";
   char buffer[MAX_BUFFER_SIZE];

   struct addrinfo serverInfo, *serverInfoPtr;

   struct sockaddr address;

   struct sockaddr_storage senderInfo;

   socklen_t addressLength = sizeof(senderInfo);

   len = strlen(message);

   serverInfo.ai_flags = AI_PASSIVE;
   serverInfo.ai_family = AF_INET;
   serverInfo.ai_socktype = SOCK_DGRAM;

   serverInfo.ai_protocol = 0;
   serverInfo.ai_addrlen = 0;
   serverInfo.ai_addr = NULL;
   serverInfo.ai_canonname = NULL;
   serverInfo.ai_next = NULL;

   address.sa_family = htonl(INADDR_ANY);
   strcpy(address.sa_data, SERVERPORT);

   getaddrinfo(NULL, SERVERPORT, &serverInfo, &serverInfoPtr);

   // socket(AF_INET, SOCK_DGRAM, 0) will also work
   sockfd = socket(serverInfoPtr->ai_family, serverInfoPtr->ai_socktype, serverInfoPtr->ai_protocol);
   printf("sockfd = %d\n", sockfd);

   // bind it to the port we passed in to getaddrinfo():
   //bind(sockfd, &address, sizeof(address));
   bind(sockfd, serverInfoPtr->ai_addr, sizeof(address));

   recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&senderInfo, &addressLength);

   bytes_sent = sendto(sockfd, message, len, 0, (struct sockaddr *)&senderInfo, serverInfoPtr->ai_addrlen);
   printf("bytes_sent = %d\n", bytes_sent);

   //printf("IP of sender = %s\n", ((struct sockaddr *)&senderInfo)->sa_data);

   //bytes_sent = sendto(sockfd, message, len, 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);
   //printf("bytes_sent = %d\n", bytes_sent);

   close(sockfd);

   return 0;
}