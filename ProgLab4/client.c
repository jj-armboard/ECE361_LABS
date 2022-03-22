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

#define MAX_HEADER_UNIT_SIZE 20
#define MAX_HEADER_BUILDER_SIZE 200
#define MAX_COMMAND_SIZE 100

#define MAX_NAME 200
#define MAX_DATA 1000
#define MAX_BUFFER_SIZE 1200

#define INVALID_USERNAME -1
#define INVALID_PASSWORD -2
#define ALREADY_LOGGED_IN -3
#define VALID -4

#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12

struct message {
   
   unsigned int type;
   unsigned int size;
   unsigned char source[MAX_NAME];
   unsigned char data[MAX_DATA];
};

int formatPacket(unsigned int type, unsigned int size, unsigned char* source, unsigned char* data, unsigned char* buffer);

int main() {

   int sockfd = 0;
   int opt = 1;
   int firstIteration = 0;
   int numByteSent = 0;
   int numByteReceived = 0;
   int loggedIn = 0;
      
   char enterCheck = 0;

   char commandString[MAX_COMMAND_SIZE];
   char inputClientID[MAX_COMMAND_SIZE];
   char inputPassword[MAX_COMMAND_SIZE];
   char inputServerIP[MAX_COMMAND_SIZE];
   char inputServerPort[MAX_COMMAND_SIZE];

   char buffer[MAX_BUFFER_SIZE];

   struct addrinfo serverInfo;
   struct addrinfo* serverInfoPtr;
   struct sockaddr_storage senderInfo;

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

   addressLength = sizeof(serverInfoPtr->ai_addr);

   while (1) {

      if (loggedIn == 0) {
         
         printf("Please Login:\n");
         scanf("%s", commandString);

         while (strcmp(commandString, "/login") != 0) {

            enterCheck = 0;

            if (firstIteration == 0) {

               firstIteration = 1;
            }
            else {

               while(enterCheck != '\n') {
                  
                  scanf("%c", &enterCheck);
               }
               
               printf("Try Entering: /login <Client ID> <Password> <Server-IP> <Server-Port>\n");
               scanf("%s", commandString);
            }
         }

         scanf("%s %s %s %s", inputClientID, inputPassword, inputServerIP, inputServerPort);

         // Resolves a hostname into an address
         if (getaddrinfo(inputServerIP, inputServerPort, &serverInfo, &serverInfoPtr) != 0) {

            printf("Connection To The Server Failed: Exiting Program\n");
            return 0;
         }

         // Create a socket
         sockfd = socket(AF_INET, SOCK_STREAM, 0);

         setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

         if (connect(sockfd, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen) == -1) {

            printf("Connection To The Server Failed: Exiting Program\n");
            return 0;
         }

         numByteSent = formatPacket(LOGIN, strlen(inputPassword), inputClientID, inputPassword, buffer);

         write(sockfd, buffer, numByteSent);

         numByteReceived = read(sockfd, buffer, MAX_BUFFER_SIZE);

         buffer[numByteReceived] = '\0';

         if (atoi(buffer) == INVALID_USERNAME) {
         
            printf("Invalid Username:\n");
         }
         else if (atoi(buffer) == INVALID_PASSWORD) {

            printf("Invalid Password:\n");
         }
         else if (atoi(buffer) == ALREADY_LOGGED_IN) {

            printf("Already Logged In:\n");
         }
         else if (atoi(buffer) == VALID) {

            printf("Valid:\n");
            loggedIn = 1;
         }
      }
   }

   return 0;
}

int formatPacket(unsigned int type, unsigned int size, unsigned char* source, unsigned char* data, unsigned char* formattedPacket) {

   int charCount = 0;

   char headerUnit[MAX_HEADER_UNIT_SIZE];
   char headerBuilder[MAX_HEADER_BUILDER_SIZE];

   sprintf(headerUnit, "%d", type);
   strcpy(headerBuilder, headerUnit);

   charCount = strlen(headerBuilder);

   headerBuilder[charCount] = ':';
   headerBuilder[charCount + 1] = '\0';

   sprintf(headerUnit, "%d", size);
   strcat(headerBuilder, headerUnit);

   charCount = strlen(headerBuilder);

   headerBuilder[charCount] = ':';
   headerBuilder[charCount + 1] = '\0';

   strcat(headerBuilder, source);

   charCount = strlen(headerBuilder);

   headerBuilder[charCount] = ':';
   headerBuilder[charCount + 1] = '\0';

   strcpy(formattedPacket, headerBuilder);

   headerBuilder[charCount] = ':';
   headerBuilder[charCount + 1] = '\0';

   charCount += 1;

   for (int i = 0; i < MAX_DATA; i++) {

      formattedPacket[i + charCount] = data[i];
   }

   return charCount + size;
}