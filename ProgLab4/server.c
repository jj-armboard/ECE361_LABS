#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>

#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_CLIENTS 20
#define MAX_BUFFER_SIZE 1200

int main(int argc, char *argv[]) {

   int masterSockfd = 0;
   int newSockfd = 0;
   int maxSockfd = 0;
   int currentSockfd = 0;
   int opt = 1;
   int numByteReceived = 0;

   fd_set readerfd;

   int clientSocket[MAX_CLIENTS];

   char *clientIP;
   char buffer[MAX_BUFFER_SIZE];

   struct addrinfo serverInfo;
   struct addrinfo* serverInfoPtr;
   struct sockaddr_storage clientInfo;

   socklen_t addressLength;

   // Initialize serverInfo
   serverInfo.ai_flags = AI_PASSIVE;
   serverInfo.ai_family = AF_INET;
   serverInfo.ai_socktype = SOCK_STREAM;

   serverInfo.ai_protocol = 0;
   serverInfo.ai_addrlen = 0;
   serverInfo.ai_addr = NULL;
   serverInfo.ai_canonname = NULL;
   serverInfo.ai_next = NULL;

   for (int i = 0; i < MAX_CLIENTS; i++) {

      clientSocket[i] = 0;
   }

   addressLength = sizeof(serverInfoPtr->ai_addr);

   // Resolves a hostname into an address
   getaddrinfo(NULL, argv[1], &serverInfo, &serverInfoPtr);

   // Create a socket
   masterSockfd = socket(AF_INET, SOCK_STREAM, 0);

   setsockopt(masterSockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

   // Bind it to the port we passed in to getaddrinfo()
   bind(masterSockfd, serverInfoPtr->ai_addr, sizeof(*(serverInfoPtr->ai_addr)));

   listen(masterSockfd, BACKLOG);

   printf("Server Listening On Port: %s\n", argv[1]);

   while(1) {

      // Clear socket set
      FD_ZERO(&readerfd);

      // Add master socket to set 
      FD_SET(masterSockfd, &readerfd); 

      maxSockfd = masterSockfd;

      for (int i = 0; i < MAX_CLIENTS; i++) {

         currentSockfd = clientSocket[i];

         if (currentSockfd > 0) {

            FD_SET(currentSockfd, &readerfd);
         }

         if (currentSockfd > maxSockfd) {

            maxSockfd = currentSockfd;
         }
      }

      select(maxSockfd + 1, &readerfd, NULL, NULL, NULL);

      if (FD_ISSET(masterSockfd, &readerfd)) {

         newSockfd = accept(masterSockfd, (struct sockaddr *)&clientInfo, &addressLength);

         clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);

         printf("New Connection IP: %s\n", clientIP);

         for (int i = 0; i < MAX_CLIENTS; i++) {

            if (clientSocket[i] == 0) {

               clientSocket[i] = newSockfd;

               break;
            }
         }
      } 

      for (int i = 0; i < MAX_CLIENTS; i++) {

         currentSockfd = clientSocket[i];

         if (FD_ISSET(currentSockfd, &readerfd)) {

            numByteReceived = read(currentSockfd, buffer, MAX_BUFFER_SIZE);

            if (numByteReceived == 0) {

               clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);

               printf("Client Disconnected IP: %s\n", clientIP);

               close(currentSockfd);

               clientSocket[i] = 0;
            }
            else {

               buffer[numByteReceived] = '\0';

               clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);

               printf("Client %s sent: %s\n", clientIP, buffer);
            }
         }
      }
   }

   return 0;
}

