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
#define MAX_HEADER_UNIT_SIZE 20
#define MAX_HEADER_BUILDER_SIZE 200
#define MAX_CONTROL_MESSAGE_SIZE 200
#define MAX_HEADER_EXTRACTOR_SIZE 200
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
#define NS_NAK 10
#define MESSAGE 11
#define QUERY 12
#define QU_ACK 13

struct message {
   
   unsigned int type;
   unsigned int size;
   unsigned char source[MAX_NAME];
   unsigned char data[MAX_DATA];
};

struct user {

   char* username;
   char* password;
   
   int sockfd;
   int loggedIn;
};

struct session {

   char sessionName[MAX_NAME];

   int active;
   int numberOfUsers;
   int UserIndexSessionLookup[NUMBER_OF_USERS];
};

int findSessionIndexWithSockfd(int sockfd);
int findSessionIndexWithName(char* sessionName);
int findInactiveSession();
int findUserIndexWithSockfd(int sockfd);
int usernamePasswordCheck(unsigned char* username, unsigned char* password);
void deconstructPacket(unsigned char* formattedPacket, struct message* clientMessage);
int formatPacket(unsigned int type, unsigned int size, unsigned char* source, unsigned char* data, unsigned char* formattedPacket);

struct user* listOfUsers[NUMBER_OF_USERS];
struct session listOfSessions[NUMBER_OF_USERS];

int main(int argc, char *argv[]) {

   int masterSockfd = 0;
   int newSockfd = 0;
   int maxSockfd = 0;
   int currentSockfd = 0;
   int opt = 1;
   int numByteSent = 0;
   int numByteReceived = 0;
   int usernamePasswordReturn = 0;
   int addressPortReaderSize = 0;
   int inactiveSession = 0;
   int findSessionWithNameReturn = 0;
   int firstIteration = 0;
   int inSessionCount = 0;
   int sessionIndex = 0;

   fd_set readerfd;

   int clientSocket[MAX_CLIENTS];
   int noSessionList[NUMBER_OF_USERS];

   char* clientIP;
   char buffer[MAX_BUFFER_SIZE];
   char controlMessage[MAX_CONTROL_MESSAGE_SIZE];
   char packetData[MAX_DATA];
   char queryBuilder[MAX_HEADER_BUILDER_SIZE];

   struct addrinfo serverInfo;
   struct addrinfo* serverInfoPtr;
   struct sockaddr_storage clientInfo;
   struct message clientMessage;
   struct sockaddr_in addressPortReader;

   socklen_t addressLength;

   struct user userAlpha;
   struct user userBeta;
   struct user userDelta;
   struct user userGamma;
   struct user userOmega;

   userAlpha.username = "Alpha";
   userAlpha.password = "password1";
   userAlpha.sockfd = -1;
   userAlpha.loggedIn = 0;

   userBeta.username = "Beta";
   userBeta.password = "password2";
   userBeta.sockfd = -1;
   userBeta.loggedIn = 0;
   
   userDelta.username = "Delta";
   userDelta.password = "password3";
   userDelta.sockfd = -1;
   userDelta.loggedIn = 0;

   userGamma.username = "Gamma";
   userGamma.password = "password4";
   userGamma.sockfd = -1;
   userGamma.loggedIn = 0;

   userOmega.username = "Omega";
   userOmega.password = "password5";
   userOmega.sockfd = -1;
   userOmega.loggedIn = 0;

   listOfUsers[0] = &userAlpha;
   listOfUsers[1] = &userBeta;
   listOfUsers[2] = &userDelta;
   listOfUsers[3] = &userGamma;
   listOfUsers[4] = &userOmega;

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      memset(listOfSessions[i].sessionName, 0, MAX_NAME * sizeof(char));
      memset(listOfSessions[i].UserIndexSessionLookup, 0, NUMBER_OF_USERS * sizeof(int));

      listOfSessions[i].active = 0;
      listOfSessions[i].numberOfUsers = 0;
   }

   memset(noSessionList, 0, NUMBER_OF_USERS * sizeof(int));

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
   addressPortReaderSize = sizeof(addressPortReader);

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

               getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
               clientIP = inet_ntoa(addressPortReader.sin_addr);

               noSessionList[findUserIndexWithSockfd(currentSockfd)] = 0;
               listOfUsers[findUserIndexWithSockfd(currentSockfd)]->loggedIn = 0;
               listOfSessions[findSessionIndexWithSockfd(currentSockfd)].UserIndexSessionLookup[findUserIndexWithSockfd(currentSockfd)] = 0;
               listOfUsers[findUserIndexWithSockfd(currentSockfd)]->sockfd = -1;

               printf("Client Disconnected IP: %s\n", clientIP);

               close(currentSockfd);

               clientSocket[i] = 0;
            }
            else {

               deconstructPacket(buffer, &clientMessage);

               if (clientMessage.type == LOGIN) {

                  usernamePasswordReturn = usernamePasswordCheck(clientMessage.source, clientMessage.data);

                  if (usernamePasswordReturn == INVALID_USERNAME) {

                     sprintf(controlMessage, "%d", INVALID_USERNAME);
                     numByteSent = formatPacket(LO_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);

                     getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                     clientIP = inet_ntoa(addressPortReader.sin_addr);

                     printf("Invalid Username, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else if (usernamePasswordReturn == INVALID_PASSWORD) {

                     sprintf(controlMessage, "%d", INVALID_PASSWORD);
                     numByteSent = formatPacket(LO_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);

                     getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                     clientIP = inet_ntoa(addressPortReader.sin_addr);

                     printf("Invalid Password, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else {

                     if (listOfUsers[usernamePasswordReturn]->loggedIn == 0) {

                        listOfUsers[usernamePasswordReturn]->loggedIn = 1;
                        listOfUsers[usernamePasswordReturn]->sockfd = currentSockfd;
                        noSessionList[findUserIndexWithSockfd(currentSockfd)] = 1;

                        sprintf(controlMessage, "%d", VALID);
                        numByteSent = formatPacket(LO_ACK, strlen(controlMessage), "Server", controlMessage, buffer);
                        
                        write(currentSockfd, buffer, numByteSent);

                        printf("Valid:\n");
                     }
                     else if (listOfUsers[usernamePasswordReturn]->loggedIn == 1) {

                        sprintf(controlMessage, "%d", ALREADY_LOGGED_IN);
                        numByteSent = formatPacket(LO_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);

                        getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                        clientIP = inet_ntoa(addressPortReader.sin_addr);

                        printf("Already Logged In, Client Disconnected: %s\n", clientIP);

                        close(currentSockfd);

                        clientSocket[i] = 0;
                     }
                  }
               }
               else if (clientMessage.type == EXIT) {

                  getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                  clientIP = inet_ntoa(addressPortReader.sin_addr);

                  noSessionList[findUserIndexWithSockfd(currentSockfd)] = 0;
                  listOfUsers[findUserIndexWithSockfd(currentSockfd)]->loggedIn = 0;
                  listOfSessions[findSessionIndexWithSockfd(currentSockfd)].UserIndexSessionLookup[findUserIndexWithSockfd(currentSockfd)] = 0;
                  listOfUsers[findUserIndexWithSockfd(currentSockfd)]->sockfd = -1;

                  printf("Client Logged Out IP: %s\n", clientIP);

                  close(currentSockfd);

                  clientSocket[i] = 0;
               }
               else if (clientMessage.type == JOIN) {

                  findSessionWithNameReturn = findSessionIndexWithName(clientMessage.data);

                  if (findSessionWithNameReturn != -1) {
                     
                     noSessionList[findUserIndexWithSockfd(currentSockfd)] = 2;
                     listOfSessions[findSessionWithNameReturn].numberOfUsers += 1;
                     listOfSessions[findSessionWithNameReturn].UserIndexSessionLookup[findUserIndexWithSockfd(currentSockfd)] = 1;

                     printf("Client \"%s\" Joined Session \"%s\"\n", listOfUsers[findUserIndexWithSockfd(currentSockfd)]->username, listOfSessions[findSessionWithNameReturn].sessionName);
                  
                     sprintf(controlMessage, "%d", JN_ACK);
                     numByteSent = formatPacket(JN_ACK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);
                  }
                  else if (findSessionWithNameReturn == -1) {
                     
                     printf("No Session With The Name \"%s\" Exists\n", clientMessage.data);
                  
                     sprintf(controlMessage, "%d", JN_NAK);
                     numByteSent = formatPacket(JN_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);
                  }
               }
               else if (clientMessage.type == LEAVE_SESS) {

               }
               else if (clientMessage.type == NEW_SESS) {
               
                  findSessionWithNameReturn = findSessionIndexWithName(clientMessage.data);

                  if (findSessionWithNameReturn == -1) {

                     inactiveSession = findInactiveSession();

                     if (inactiveSession != -1) {

                        strcpy(listOfSessions[inactiveSession].sessionName, clientMessage.data);

                        //printf("currentSockfd = %d | findUserIndexWithSockfd(currentSockfd) = %d\n", currentSockfd, findUserIndexWithSockfd(currentSockfd));

                        noSessionList[findUserIndexWithSockfd(currentSockfd)] = 2;
                        listOfSessions[inactiveSession].active = 1;
                        listOfSessions[inactiveSession].numberOfUsers = 1;
                        listOfSessions[inactiveSession].UserIndexSessionLookup[findUserIndexWithSockfd(currentSockfd)] = 1;

                        printf("Session Created:\n");

                        sprintf(controlMessage, "%d", NS_ACK);
                        numByteSent = formatPacket(NS_ACK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);
                     }
                     else if (inactiveSession == -1) {

                        printf("Maximum Session Limit Has Been Reached:\n");

                        strcpy(controlMessage, "MAX_SESSION_LIMIT_REACHED");
                        numByteSent = formatPacket(NS_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);
                     }
                  }
                  else if (findSessionWithNameReturn != -1) {

                     printf("A Session With The Same Name Already Exists:\n");

                     strcpy(controlMessage, "SESSION_WITH_SAME_NAME");
                     numByteSent = formatPacket(NS_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);
                  }
               }
               else if (clientMessage.type == QUERY) {

                  memset(queryBuilder, 0, MAX_HEADER_BUILDER_SIZE * sizeof(char));
                  memset(packetData, 0, MAX_DATA * sizeof(char));

                  for (int i = 0; i < NUMBER_OF_USERS; i++) {

                     if (listOfSessions[i].active == 1) {

                        if (firstIteration == 0) {

                           firstIteration = 1;
                           
                           //printf("------------------------------\n");
                        };
                        
                        strcat(packetData, listOfSessions[i].sessionName);
                        strcat(packetData, "|");

                        printf("%s: ", listOfSessions[i].sessionName);

                        for (int j = 0; j < NUMBER_OF_USERS; j++) {

                           if (listOfSessions[i].UserIndexSessionLookup[j] == 1) {

                              strcat(packetData, listOfUsers[j]->username);
                              strcat(packetData, ",");

                              //printf("%s ", listOfUsers[j]->username);
                           }
                        }
                        
                        strcat(packetData, "\n");

                        //printf("\n------------------------------\n");
                     }
                  }

                  //printf("%s\n", packetData);

                  /////////////////////////////////
                  //for(int TEST = 0; TEST < NUMBER_OF_USERS; TEST++) {

                  //   printf("%d | ", noSessionList[TEST]);
                  //}
                  //printf("\n");
                  /////////////////////////////////
                  
                  inSessionCount = 0;

                  for (int i = 0; i < NUMBER_OF_USERS; i++) {

                     if(noSessionList[i] == 1) {

                        strcat(queryBuilder, listOfUsers[i]->username);
                        strcat(queryBuilder, ",");

                        inSessionCount += 1;
                     }
                  }

                  if (inSessionCount > 0) {

                     strcat(packetData, "Online But Not In Session|");
                     strcat(packetData, queryBuilder);
                     strcat(packetData, "\n");
                  }

                  firstIteration = 0;

                  numByteSent = formatPacket(QU_ACK, strlen(packetData), "Server", packetData, buffer);
                  
                  write(currentSockfd, buffer, numByteSent);
               }
               else if (clientMessage.type == MESSAGE) {

                  sessionIndex = findSessionIndexWithSockfd(currentSockfd);

                  numByteSent = formatPacket(MESSAGE, strlen(clientMessage.data), "Server", clientMessage.data, buffer);

                  for (int i = 0; i < NUMBER_OF_USERS; i++) {

                     if(listOfSessions[sessionIndex].UserIndexSessionLookup[i] == 1) {

                        write(listOfUsers[i]->sockfd, buffer, numByteSent);
                     }
                  }
               }
            }
         }
      }
   }

   return 0;
}

int findSessionIndexWithSockfd(int sockfd) {

   int findUserIndexWithSockfdReturn = 0;

   findUserIndexWithSockfdReturn = findUserIndexWithSockfd(sockfd);

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if(listOfSessions[i].UserIndexSessionLookup[findUserIndexWithSockfdReturn] == 1) {

         return i;
      }
   }

   return -1;
}

int findSessionIndexWithName(char* sessionName) {

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if((strcmp(listOfSessions[i].sessionName, sessionName) == 0) && (listOfSessions[i].active == 1)) {

         return i;
      }
   }

   return -1;
}

int findInactiveSession() {

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if (listOfSessions[i].active == 0) {

         return i;
      }
   }

   return -1;
}

int findUserIndexWithSockfd(int sockfd) {

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if (listOfUsers[i]->sockfd == sockfd) {

         return i;
      }
   }

   return -1;
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

   clientMessage->type = 0;
   clientMessage->size = 0;
   memset(clientMessage->source , 0, MAX_NAME * sizeof(char));
   memset(clientMessage->data, 0, MAX_DATA * sizeof(char));

   memset(headerExtractor, 0, MAX_HEADER_EXTRACTOR_SIZE * sizeof(char));

   for (int i = 0; ; i++) {

		if (formattedPacket[i] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = formattedPacket[i];
	}

   clientMessage->type = atoi(headerExtractor);

   charCount = strlen(headerExtractor) + 1;

   memset(headerExtractor, 0, MAX_HEADER_EXTRACTOR_SIZE * sizeof(char));

   for (int i = 0; ; i++) {

		if (formattedPacket[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = formattedPacket[i + charCount];
	}

   clientMessage->size = atoi(headerExtractor);

   charCount = charCount + strlen(headerExtractor) + 1;

	memset(headerExtractor, 0, MAX_HEADER_EXTRACTOR_SIZE * sizeof(char));

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