#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 100

struct packet {
   
   unsigned int total_frag;
   unsigned int frag_no;
   unsigned int size;
   char* filename;
   char filedata[1000];
};

int main(int argc, char *argv[]) {
   
   int sockfd = 0;
   int numByteSent = 0;
   int numByteReceived = 0;

   char buffer[MAX_BUFFER_SIZE];
   char message[MAX_BUFFER_SIZE];

   char commandString[100];
   char filePathName[100];
   char* fileName;

   FILE* transferFile;
   int fileSize = 0;
   char formattedPacket[1200];
   
   struct packet filePacket;

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

   if (access(filePathName, F_OK) == 0) {

      printf("File Exists: Sending \"%s\" To Server\n", commandString);
   }
   else {

      printf("File Does Not Exist: Exiting Program\n");
      return 0;
   }
   
   // Sends a message to the server
   numByteSent = sendto(sockfd, commandString, strlen(commandString), 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);
   printf("Client Sent: \"%s\"\n", commandString);

   // Receives a message from the server
   numByteReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&senderInfo, &addressLength);
  
   buffer[numByteReceived] = '\0';
   printf("Client Received: \"%s\"\n", buffer);

   if (strcmp(buffer, "yes") == 0) {

      printf("A file transfer can start.\n");
   }
   else if (strcmp(buffer, "no") == 0) {

      printf("Server Returned \"no\": Exiting Program\n");
   }

   //////////////////// Section 2 ////////////////////

   transferFile = fopen(filePathName, "rb");

   if (transferFile == NULL) {

      printf("File failed to open!\n");
      return 0;
   }
   else {

      printf("File successfully opened!\n");
   }

   fseek(transferFile, 0, SEEK_END);
   fileSize = ftell(transferFile);

   fseek(transferFile, 0, SEEK_SET);

   printf("Size of the file = %d bytes\n", fileSize);

   filePacket.total_frag = (fileSize / 1000) + 1;
   filePacket.size = fileSize;
   
   fileName = strrchr(filePathName, '/');
   fileName = strrchr(filePathName, fileName[1]);

   filePacket.filename = (char *)malloc(strlen(fileName) * sizeof(char *));
   strcpy(filePacket.filename, fileName);

   printf("\nfileName = %s\n\n", fileName);

   for (int k = 0; k < filePacket.total_frag + 1; k++) {

      filePacket.frag_no = 1;

      fread(filePacket.filedata, 1, 1000, transferFile);

      char temp[20];
      char headerBuilder[200];

      int charCount = 0;
      int formattedPacketSize = 0;

      sprintf(temp, "%d", filePacket.total_frag);
      strcpy(headerBuilder, temp);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      sprintf(temp, "%d", filePacket.frag_no);
      strcat(headerBuilder, temp);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      sprintf(temp, "%d", filePacket.size);
      strcat(headerBuilder, temp);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      strcat(headerBuilder, filePacket.filename);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      charCount += 1;

      //formattedPacket = (char *)malloc((charCount + fileSize) * sizeof(char *));

      strcpy(formattedPacket, headerBuilder);
      
      for (int i = 0; i < 1000; i++) {

         formattedPacket[i + charCount] = filePacket.filedata[i];
      }

      //formattedPacketSize = charCount + fileSize - 1;
      formattedPacketSize = charCount + 1000 - 1;

      //formattedPacket[formattedPacketSize] = '\0'; 
      printf("filePathName size = %d\n", strlen(filePathName));
      printf("formattedPacketSize = %d\n", formattedPacketSize);
      //printf("%s\n", formattedPacket);

      numByteSent = sendto(sockfd, formattedPacket, 1200, 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);
      printf("Client Sent Packet\n");
   }

   fclose(transferFile);
   close(sockfd);

   return 0;
}