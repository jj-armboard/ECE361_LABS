#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 100

struct packet {  
uint32_t total_frag;  
unsigned int frag_no; 
unsigned int size; 
char* filename; 
char filedata[1000];  
};

int main(int argc, char *argv[]) {
   
   int sockfd = 0;
   int numByteSent = 0;
   int numByteReceived = 0;

   //char buffer[MAX_BUFFER_SIZE];
   char message[MAX_BUFFER_SIZE];

   char commandString[100];
   char filePathName[100];

   struct addrinfo serverInfo, *serverInfoPtr;

   // We use sockaddr_storage and typecast it to sockaddr when passing it to a function.
   // You'd typically create a struct sockaddr_in or a struct sockaddr_in6 
   // depending on what IP version you're using. In order to avoid trying to know what 
   // IP version you will be using, you can use a struct sockaddr_storage which can hold either.
   struct sockaddr_storage senderInfo;

   socklen_t addressLength = sizeof(senderInfo);

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
   getaddrinfo(argv[1], argv[2], &serverInfo, &serverInfoPtr);

   // Create a socket
   sockfd = socket(AF_INET, SOCK_DGRAM, 0);

   // Bind it to the port we passed in to getaddrinfo()
   bind(sockfd, serverInfoPtr->ai_addr, sizeof(*(serverInfoPtr->ai_addr)));

   printf("Please Input: ftp <file name>\n");
   scanf("%s %[^\n]", commandString, filePathName);
   
//   if (access(filePathName, F_OK) == 0) {

//      printf("File Exists: Sending \"%s\" To Server\n", commandString);
//   }
//   else {

//      printf("File Does Not Exist: Exiting Program\n");
//      return 0;
//   }
   struct packet pack;
   pack.total_frag = 1;
   pack.frag_no = 1;
   pack.size = 3123;
   //strcpy(pack.filename,&commandString);
   
   //code to serialize the packets
   
   //basic code to serialize an integer
   int serialized_total_frag = htonl(pack.total_frag);
   int serialized_frag_no = htonl(pack.frag_no)
   
   //basic code to copy to a buffer
   
   unsigned char * buffer = malloc(1*sizeof(struct packet));
   memcpy(buffer,&serialized_total_frag,sizeof(int));
   
   //char buffer[1024];
   
   
   // Sends a message to the server
   numByteSent = sendto(sockfd, buffer, sizeof(buffer), 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);
   printf("Client Sent: \"%d\"\n", &total_frag);
   free(buffer);
   // Receives a message from the server
   //numByteReceived = recvfrom(sockfd, buffer,  - 1, 0, (struct sockaddr *)&senderInfo, &addressLength);
  
   //buffer[numByteReceived] = '\0';
   //printf("Client Received: \"%s\"\n", buffer);

   //if (strcmp(buffer, "yes") == 0) {

//      printf("A file transfer can start.\n");
  // }
   //else if (strcmp(buffer, "no") == 0) {

     // printf("Server Returned \"no\": Exiting Program\n");
   //}

   close(sockfd);

   return 0;
}