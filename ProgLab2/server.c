#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 100

int main(int argc, char *argv[]) {
   
   int sockfd = 0;
   int numByteSent = 0;
   int numByteReceived = 0;

   char message[MAX_BUFFER_SIZE];
   //char buffer[MAX_BUFFER_SIZE];

   struct addrinfo serverInfo, *serverInfoPtr;

   // We use sockaddr_storage and typecast it to sockaddr when passing it to a function.
   // You'd typically create a struct sockaddr_in or a struct sockaddr_in6 
   // depending on what IP version you're using. In order to avoid trying to know what 
   // IP version you will be using, you can use a struct sockaddr_storage which can hold either.
   struct sockaddr_storage clientInfo;

   socklen_t addressLength = sizeof(clientInfo);

   // Initialize serverInfo
   serverInfo.ai_flags = AI_PASSIVE;
   serverInfo.ai_family = AF_INET;
   serverInfo.ai_socktype = SOCK_DGRAM;

   serverInfo.ai_protocol = 0;
   serverInfo.ai_addrlen = 0;
   serverInfo.ai_addr = NULL;
   serverInfo.ai_canonname = NULL;
   serverInfo.ai_next = NULL;

   // Resolves a hostname into an address
   getaddrinfo(NULL, argv[1], &serverInfo, &serverInfoPtr);

   // Create a socket
   sockfd = socket(AF_INET, SOCK_DGRAM, 0);

   // Bind it to the port we passed in to getaddrinfo()
   bind(sockfd, serverInfoPtr->ai_addr, sizeof(*(serverInfoPtr->ai_addr)));

   printf("Server Listening On Port: %s\n", argv[1]);
   int buf = 0;   // Receives a message from the client
   numByteReceived = recvfrom(sockfd, &buf, sizeof(buf), 0, (struct sockaddr *)&clientInfo, &addressLength);
   uint32_t total_frag = ntohl(buf);
   
   //free(buffer);
   //buffer[numByteReceived] = '\0';
   printf("Server Received: \"%d\"\n", ntohl(buf));

   //if (strcmp(buffer, "ftp") == 0) {

     // strcpy(message, "yes");
   //}
   //else {

     // strcpy(message, "no");
   //}

   // Sends a message to the client
   //numByteSent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&clientInfo, serverInfoPtr->ai_addrlen);
   //printf("Server Sent: \"%s\"\n", message);

   close(sockfd);

   return 0;
}