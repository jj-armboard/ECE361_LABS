#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>

int main(int argc, char *argv[]) {

   int sockfd = 0;
   int yes = 1;

   struct addrinfo serverInfo, *serverInfoPtr;
   struct sockaddr_storage senderInfo;

   socklen_t addressLength = sizeof(senderInfo);

   // Initialize serverInfo
   serverInfo.ai_flags = AI_PASSIVE;
   serverInfo.ai_family = AF_INET;
   serverInfo.ai_socktype = SOCK_STREAM;

   serverInfo.ai_protocol = 0;
   serverInfo.ai_addrlen = 0;
   serverInfo.ai_addr = NULL;
   serverInfo.ai_canonname = NULL;
   serverInfo.ai_next = NULL;

   // Resolves a hostname into an address
   getaddrinfo(argv[1], argv[2], &serverInfo, &serverInfoPtr);

   // Create a socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   connect(sockfd, serverInfoPtr->ai_addr,  sizeof(*(serverInfoPtr->ai_addr)));

   char buff[100] = "This is a test";

   write(sockfd, buff, sizeof(buff));

   return 0;
}
