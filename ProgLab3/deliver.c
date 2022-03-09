#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#define ALPHA 0.125
#define MAX_MESSAGE_SIZE 200
#define MAX_PAYLOAD_SIZE 1000
#define FORMATTED_PACKET_SIZE 1200

struct packet {
   
   unsigned int total_frag;
   unsigned int frag_no;
   unsigned int size;
   char* filename;
   char filedata[MAX_PAYLOAD_SIZE];
};

int main(int argc, char *argv[]) {
   
   int sockfd = 0;
   int numByteSent = 0;
   int numByteReceived = 0;
   int charCount = 0;
   int fileSize = 0;

   char message[MAX_MESSAGE_SIZE];

   char commandString[100];
   char filePathName[100];
   char* fileName;

   char headerUnit[20];
   char headerBuilder[200];

   char formattedPacket[FORMATTED_PACKET_SIZE];

   FILE* transferFile;

   clock_t startClock;
   clock_t endClock;

   double averageRTT;
   
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
   
   startClock = clock();

   // Sends a message to the server
   numByteSent = sendto(sockfd, commandString, strlen(commandString), 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);
   printf("Client Sent: \"%s\"\n", commandString);

   // Receives a message from the server
   numByteReceived = recvfrom(sockfd, message, MAX_MESSAGE_SIZE, 0, (struct sockaddr *)&senderInfo, &addressLength);
  
   endClock = clock();

   averageRTT = ((double)(endClock - startClock) / CLOCKS_PER_SEC);

   message[numByteReceived] = '\0';
   printf("Client Received: \"%s\"\n", message);

   if (strcmp(message, "yes") == 0) {

      printf("A File Transfer Can Start\n");
   }
   else if (strcmp(message, "no") == 0) {

      printf("Server Returned \"no\": Exiting Program\n");
   }

   struct timeval timeOut;

   timeOut.tv_sec = 0;
   timeOut.tv_usec = averageRTT * 1e6 * 5; 

   setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut));

//////////////////// ProgLab 2 ////////////////////

   transferFile = fopen(filePathName, "rb");

   if (transferFile == NULL) {

      printf("File failed to open\n");
      return 0;
   }
   else {

      printf("File successfully opened\n");
   }

   fseek(transferFile, 0, SEEK_END);
   fileSize = ftell(transferFile);

   fseek(transferFile, 0, SEEK_SET);

   filePacket.total_frag = (fileSize / MAX_PAYLOAD_SIZE) + 1;
   filePacket.size = fileSize;
   
   fileName = strrchr(filePathName, '/');
   fileName = strchr(filePathName, fileName[1]);

   filePacket.filename = (char *)malloc(strlen(fileName) * sizeof(char *));
   strcpy(filePacket.filename, fileName);

   printf("Filename: %s | Size: %d Bytes | Number Of Packets: %d\n", filePacket.filename, filePacket.size, filePacket.total_frag);

   filePacket.frag_no = 0;

   for (int i = 0; i < filePacket.total_frag; i++) {

      charCount = 0;

      filePacket.frag_no += 1;

      fread(filePacket.filedata, 1, MAX_PAYLOAD_SIZE, transferFile);

      sprintf(headerUnit, "%d", filePacket.total_frag);
      strcpy(headerBuilder, headerUnit);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      sprintf(headerUnit, "%d", filePacket.frag_no);
      strcat(headerBuilder, headerUnit);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      sprintf(headerUnit, "%d", filePacket.size);
      strcat(headerBuilder, headerUnit);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      strcat(headerBuilder, filePacket.filename);

      charCount = strlen(headerBuilder);

      headerBuilder[charCount] = ':';
      headerBuilder[charCount + 1] = '\0';

      charCount += 1;

      strcpy(formattedPacket, headerBuilder);
      
      for (int j = 0; j < MAX_PAYLOAD_SIZE; j++) {

         formattedPacket[j + charCount] = filePacket.filedata[j];
      }

      startClock = clock();

      numByteSent = sendto(sockfd, formattedPacket, FORMATTED_PACKET_SIZE, 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);

      //////////////////// ProgLab 3 ////////////////////

      int timeOutCounter = 0;

      while (1) {

         numByteReceived = recvfrom(sockfd, message, MAX_MESSAGE_SIZE, 0, (struct sockaddr *)&senderInfo, &addressLength);      
         
         message[numByteReceived] = '\0';

         if (numByteReceived == -1 || (strcmp(message, "Ack") != 0)) {

            if (timeOutCounter < 5) {

               startClock = clock();

               numByteSent = sendto(sockfd, formattedPacket, FORMATTED_PACKET_SIZE, 0, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);

               timeOutCounter += 1;
               printf("Packet %d: Timed Out | Average RTT: %.4f ms | Retransmitting\n", i, averageRTT * 1000);
            }
            else {

               printf("Too Many Timeouts: Exiting Program\n");
               return 0;
            }
         }
         else {

            endClock = clock();

            averageRTT = ((1 - ALPHA) * averageRTT) + (ALPHA * ((double)(endClock - startClock) / CLOCKS_PER_SEC));

            timeOut.tv_sec = 0;
            timeOut.tv_usec = averageRTT * 1e6 * 5;

            break;
         }
      }
   }

   fclose(transferFile);
   close(sockfd);

   return 0;
}