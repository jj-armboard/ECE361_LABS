#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_BUFFER_SIZE 1200

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

   char message[MAX_BUFFER_SIZE];
   char buffer[MAX_BUFFER_SIZE];

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

   // Receives a message from the client
   numByteReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientInfo, &addressLength);
  
   buffer[numByteReceived] = '\0';
   printf("Server Received: \"%s\"\n", buffer);

   if (strcmp(buffer, "ftp") == 0) {

      strcpy(message, "yes");
   }
   else {

      strcpy(message, "no");
   }

   // Sends a message to the client
   numByteSent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&clientInfo, serverInfoPtr->ai_addrlen);
   printf("Server Sent: \"%s\"\n", message);
   /*while(1){
    
    * 
    * 
    * 
    * 
    * 
    * process fragments
    * if(frag_no == total_frag){
    * break;
    * }
   
   }*/
   while(1){
   numByteReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&clientInfo, &addressLength);
   
	buffer[numByteReceived] = '\0';
	printf("%s\n", buffer);

	struct packet filePacket;

	char temp[200];

	int charCount = 0;
	int fileNameSize = 0;

	for (int i = 0; ; i++) {

		if (buffer[i] == ':') {
			
			temp[i] == '\0';

			break;
		}

		temp[i] = buffer[i];
	}

	filePacket.total_frag = atoi(temp);

	printf("filePacket.total_frag = %d\n", filePacket.total_frag);

	charCount = strlen(temp) + 1;

	for (int i = 0; ; i++) {

		if (buffer[i + charCount] == ':') {
			
			temp[i] == '\0';

			break;
		}

		temp[i] = buffer[i + charCount];
	}

	filePacket.frag_no = atoi(temp);

	printf("filePacket.frag_no = %d\n", filePacket.frag_no);
        
	charCount = charCount + strlen(temp) + 1;

	for (int i = 0; ; i++) {

		if (buffer[i + charCount] == ':') {
			
			temp[i] == '\0';

			break;
		}

		temp[i] = buffer[i + charCount];
	}

	filePacket.size = atoi(temp);

	printf("filePacket.size = %d\n", filePacket.size);

	charCount = charCount + strlen(temp) + 1;

	for (int i = 0; ; i++) {

		fileNameSize += 1;

		if (buffer[i + charCount] == ':') {
			
			temp[i] == '\0';

			break;
		}

		temp[i] = buffer[i + charCount];
	}

	filePacket.filename = (char *)malloc(fileNameSize*sizeof(char));

	strcpy(filePacket.filename, temp);

	printf("filePacket.filename = %s\n", filePacket.filename);

	charCount = charCount + strlen(filePacket.filename) + 1;

	for (int i = 0; i < filePacket.size; i++) {

		filePacket.filedata[i] = buffer[i + charCount];
	}

	filePacket.filedata[filePacket.size] = '\0';

	//strcat(filePacket.filedata, " IT FUCKING ARIRRIVED!!!");

	

	FILE* transferFile = fopen(filePacket.filename, "w");

	fwrite(filePacket.filedata, 1, 1000, transferFile);

	fclose(transferFile);
        if(filePacket.frag_no == filePacket.total_frag){
            break;
        }
   }
   close(sockfd);
   
   
   

   return 0;
}