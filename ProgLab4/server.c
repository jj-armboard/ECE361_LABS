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

#define NUMBER_OF_USERS 5
#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_CLIENTS 20
#define MAX_CONTROL_MESSAGE_SIZE 200
#define MAX_HEADER_EXTRACTOR_SIZE 200
#define MAX_NAME 200
#define MAX_DATA 1200
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

struct user {

   char* username;
   char* password;
   char* ip;

   int loggedIn;
};

int findUserIndex(char* ip);
int usernamePasswordCheck(unsigned char* username, unsigned char* password);
void deconstructPacket(unsigned char* formattedPacket, struct message* clientMessage);

struct user* listOfUsers[NUMBER_OF_USERS];

int main(int argc, char *argv[]) {

   int masterSockfd = 0;
   int newSockfd = 0;
   int maxSockfd = 0;
   int currentSockfd = 0;
   int opt = 1;
   int numByteReceived = 0;
   int usernamePasswordReturn = 0;

   fd_set readerfd;

   int clientSocket[MAX_CLIENTS];

   char* clientIP;
   char buffer[MAX_BUFFER_SIZE];
   char controlMessage[MAX_CONTROL_MESSAGE_SIZE];

   struct addrinfo serverInfo;
   struct addrinfo* serverInfoPtr;
   struct sockaddr_storage clientInfo;
   struct message clientMessage;

   socklen_t addressLength;

   struct user userAlpha;
   struct user userBeta;
   struct user userDelta;
   struct user userGamma;
   struct user userOmega;

   userAlpha.username = "Alpha";
   userAlpha.password = "password1";
   userAlpha.loggedIn = 0;

   userBeta.username = "Beta";
   userBeta.password = "password2";
   userBeta.loggedIn = 0;
   
   userDelta.username = "Delta";
   userDelta.password = "password3";
   userDelta.loggedIn = 0;

   userGamma.username = "Gamma";
   userGamma.password = "password4";
   userGamma.loggedIn = 0;

   userOmega.username = "Omega";
   userOmega.password = "password5";
   userOmega.loggedIn = 0;

   listOfUsers[0] = &userAlpha;
   listOfUsers[1] = &userBeta;
   listOfUsers[2] = &userDelta;
   listOfUsers[3] = &userGamma;
   listOfUsers[4] = &userOmega;

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
   bind(masterSockfd, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);

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

               listOfUsers[findUserIndex(clientIP)]->loggedIn = 0;

               printf("Client Disconnected IP: %s\n", clientIP);

               close(currentSockfd);

               clientSocket[i] = 0;
            }
            else {

               buffer[numByteReceived] = '\0';

               deconstructPacket(buffer, &clientMessage);

               if (clientMessage.type == LOGIN) {

                  usernamePasswordReturn = usernamePasswordCheck(clientMessage.source, clientMessage.data);

                  if (usernamePasswordReturn == INVALID_USERNAME) {

                     sprintf(controlMessage, "%d", INVALID_USERNAME);

                     write(currentSockfd, controlMessage, strlen(controlMessage));

                     clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);
                     printf("Invalid Username, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else if (usernamePasswordReturn == INVALID_PASSWORD) {

                     sprintf(controlMessage, "%d", INVALID_PASSWORD);
                     write(currentSockfd, controlMessage, strlen(controlMessage));

                     clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);
                     printf("Invalid Password, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else {

                     if (listOfUsers[usernamePasswordReturn]->loggedIn == 0) {

                        clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);

                        listOfUsers[usernamePasswordReturn]->loggedIn = 1;
                        listOfUsers[usernamePasswordReturn]->ip = clientIP;

                        sprintf(controlMessage, "%d", VALID);
                        write(currentSockfd, controlMessage, strlen(controlMessage));

                        printf("Valid:\n");
                     }
                     else if (listOfUsers[usernamePasswordReturn]->loggedIn == 1) {

                        sprintf(controlMessage, "%d", ALREADY_LOGGED_IN);
                        write(currentSockfd, controlMessage, strlen(controlMessage));

                        clientIP = inet_ntoa(((struct sockaddr_in *)&clientInfo)->sin_addr);
                        printf("Already Logged In, Client Disconnected: %s\n", clientIP);

                        close(currentSockfd);

                        clientSocket[i] = 0;
                     }
                  }
               }
            }
         }
      }
   }

   return 0;
}

int findUserIndex(char* ip) {

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if (listOfUsers[i]->ip == ip) {

         return i;
      }
   }
}

int usernamePasswordCheck(unsigned char* username, unsigned char* password) {

   int validUsernameIndex = -1;
   int validPasswordIndex = -1;

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if (strcmp(username, listOfUsers[i]->username) == 0) {

         validUsernameIndex = i;
         break;
      }
   }

   if (validUsernameIndex == -1) {

      return INVALID_USERNAME;
   }

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if (strcmp(password, listOfUsers[i]->password) == 0) {

         if (i == validUsernameIndex) {

            validPasswordIndex = i;
            break;
         }
      }
   }

   if (validPasswordIndex == -1) {

      return INVALID_PASSWORD;
   }

   return validPasswordIndex;
}

void deconstructPacket(unsigned char* formattedPacket, struct message* clientMessage) {

   int charCount = 0;

   char headerExtractor[MAX_HEADER_EXTRACTOR_SIZE];

   memset(headerExtractor, 0, 200);

   for (int i = 0; ; i++) {

		if (formattedPacket[i] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = formattedPacket[i];
	}

   clientMessage->type = atoi(headerExtractor);

   charCount = strlen(headerExtractor) + 1;

   memset(headerExtractor, 0, 200);

   for (int i = 0; ; i++) {

		if (formattedPacket[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = formattedPacket[i + charCount];
	}

   clientMessage->size = atoi(headerExtractor);

   charCount = charCount + strlen(headerExtractor) + 1;

	memset(headerExtractor, 0, 200);

   for (int i = 0; ; i++) {

		if (formattedPacket[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = formattedPacket[i + charCount];
	}

   strcpy(clientMessage->source, headerExtractor);

   charCount = charCount + strlen(clientMessage->source) + 1;

   for (int i = 0; i < clientMessage->size; i++) {

      clientMessage->data[i] = formattedPacket[charCount + i];
   }
}

