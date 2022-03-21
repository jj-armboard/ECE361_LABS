#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>

#define BACKLOG 10 // how many pending connections queue will hold

int main(int argc, char *argv[]) {

   int sockfd = 0;
   int newSockfd = 0;
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
   getaddrinfo(NULL, argv[1], &serverInfo, &serverInfoPtr);

   // Create a socket
   sockfd = socket(AF_INET, SOCK_STREAM, 0);

   //setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

   // Bind it to the port we passed in to getaddrinfo()
   bind(sockfd, serverInfoPtr->ai_addr, sizeof(*(serverInfoPtr->ai_addr)));

   printf("Server Listening On Port: %s\n", argv[1]);

   listen(sockfd, BACKLOG);

   addressLength = sizeof(senderInfo);

   newSockfd = accept(sockfd, (struct sockaddr*)&senderInfo, &addressLength);

   //----- Testing -----//

   char buff[100];

   read(newSockfd, buff, sizeof(buff));

   printf("Server Received: \"%s\"\n", buff);

   return 0;
}

