#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <math.h>

#define MAX_HEADER_UNIT_SIZE 20
#define MAX_USERNAME_PASSWORD_SIZE 50
#define MAX_COMMAND_SIZE 100
#define MAX_HEADER_BUILDER_SIZE 200
#define MAX_HEADER_EXTRACTOR_SIZE 200
#define MAX_CONTROL_MESSAGE_SIZE 200

#define MAX_NAME 200
#define MAX_DATA 1000
#define MAX_BUFFER_SIZE 1200

#define INVALID_USERNAME -1
#define INVALID_PASSWORD -2
#define ALREADY_LOGGED_IN -3
#define VALID -4
#define LIST_EMPTY -5
#define ACCOUNT_LIMIT_REACHED -6

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
#define REGISTER 14
#define REGISTER_ACK 15
#define REGISTER_NAK 16
#define KICK 17
#define KICK_ACK 18
#define KICK_NAK 19

struct message {
   
   unsigned int type;
   unsigned int size;
   unsigned char source[MAX_NAME];
   unsigned char data[MAX_DATA];
};

void printList(char* packetData, int packetSize);
void deconstructPacket(unsigned char* formattedPacket, struct message* clientMessage);
int formatPacket(unsigned int type, unsigned int size, unsigned char* source, unsigned char* data, unsigned char* buffer);

int main() {

   int sockfd = 0;
   int opt = 1;
   int firstIteration = 0;
   int numByteSent = 0;
   int numByteReceived = 0;
   int loggedIn = 0;
   int validCommand = 1;
   int inSession = 0;
   int clearCommandCheck = 0;
   int sscanfReturn = 0;
   int selectReturn = 0;
   int stdinfd = 0;
   int processed = 0;
      
   char enterCheck = 0;

   char commandString[MAX_COMMAND_SIZE];
   char inputClientID[MAX_COMMAND_SIZE];
   char inputPassword[MAX_COMMAND_SIZE];
   char inputServerIP[MAX_COMMAND_SIZE];
   char inputServerPort[MAX_COMMAND_SIZE];

   char buffer[MAX_BUFFER_SIZE];
   char controlMessage[MAX_CONTROL_MESSAGE_SIZE];
   char packetData[MAX_BUFFER_SIZE];
   char sessionID[MAX_NAME];
   char targetClientID[MAX_USERNAME_PASSWORD_SIZE];
   char inputBuffer[MAX_DATA];

   struct addrinfo serverInfo;
   struct addrinfo* serverInfoPtr;
   struct sockaddr_storage senderInfo;
   struct message serverMessage;
   struct timeval tv;

   socklen_t addressLength;

   fd_set IOreaderfd;
   fd_set socketReaderfd;

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

   stdinfd = fileno(stdin);

   while (1) {

      if (loggedIn == 0) {
         
         printf("Please Login Or Register An Account:\n");
         scanf("%s", commandString);

         while ((strcmp(commandString, "/login") != 0) && (strcmp(commandString, "/register") != 0)) {

            if (strcmp(commandString, "/quit") == 0) {

               printf("Exiting Program\n");
               return 0;
            }
            else if (strcmp(commandString, "/clear") == 0) {

               clearCommandCheck = 1;

               system("clear");
            }

            enterCheck = 0;

            if (firstIteration == 0) {

               firstIteration = 1;
            }
            else {

               while(enterCheck != '\n') {
                  
                  scanf("%c", &enterCheck);
               }
               
               if (clearCommandCheck == 0) {
                  
                  printf("Try Entering: /login or /register <Client ID> <Password> <Server-IP> <Server-Port>\n");
               }
               else if (clearCommandCheck == 1) {

                  clearCommandCheck = 0;

                  printf("Please Login Or Register An Account:\n");
               }

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

         if (strcmp(commandString, "/login") == 0) {
            
            numByteSent = formatPacket(LOGIN, strlen(inputPassword), inputClientID, inputPassword, buffer);
            write(sockfd, buffer, numByteSent);
         }
         else if (strcmp(commandString, "/register") == 0) {

            numByteSent = formatPacket(REGISTER, strlen(inputPassword), inputClientID, inputPassword, buffer);
            write(sockfd, buffer, numByteSent);
         }

         numByteReceived = read(sockfd, buffer, MAX_BUFFER_SIZE);
         deconstructPacket(buffer, &serverMessage);

         if (serverMessage.type == LO_ACK) {

            printf("Please Enter A Command:\n");

            memset(commandString, 0, MAX_COMMAND_SIZE * sizeof(char));
            loggedIn = 1;
         }
         else if (serverMessage.type == LO_NAK) {

            if (atoi(serverMessage.data) == INVALID_USERNAME) {
         
               printf("Invalid Username:\n");
            }
            else if (atoi(serverMessage.data) == INVALID_PASSWORD) {

               printf("Invalid Password:\n");
            }
            else if (atoi(serverMessage.data) == ALREADY_LOGGED_IN) {

               printf("Already Logged In:\n");
            }
         }
         else if (serverMessage.type == REGISTER_ACK) {

            printf("Please Enter A Command:\n");

            memset(commandString, 0, MAX_COMMAND_SIZE * sizeof(char));
            loggedIn = 1;
         }
         else if (serverMessage.type == REGISTER_NAK) {

            if (atoi(serverMessage.data) == INVALID_USERNAME) {
         
               printf("Username Already In Use:\n");
            }
            else if (atoi(serverMessage.data) == ACCOUNT_LIMIT_REACHED) {

               printf("Account Limit Reached:\n");
            }
         }
      }
      else if (loggedIn == 1) {

         FD_ZERO(&IOreaderfd);
         FD_SET(fileno(stdin), &IOreaderfd);

         FD_ZERO(&socketReaderfd);
         FD_SET(sockfd, &socketReaderfd);

         tv.tv_sec = 0;
         tv.tv_usec = 1000;

         fflush(stdout);

         selectReturn = select(stdinfd + 1, &IOreaderfd, NULL, NULL, &tv);
         select(sockfd + 1, &socketReaderfd, NULL, NULL, &tv);

         if (selectReturn == 0) {

            if (processed == 0) {

               processed = 1;

               sscanf(inputBuffer, "%s", commandString);

               if (strcmp(commandString, "/logout") == 0) {

                  validCommand = 1;

                  inSession = 0;

                  sprintf(controlMessage, "%d", EXIT);
                  numByteSent = formatPacket(EXIT, strlen(controlMessage), inputClientID, controlMessage, buffer);

                  write(sockfd, buffer, numByteSent);

                  loggedIn = 0;
               }
               else if (strcmp(commandString, "/joinsession") == 0) {
                  
                  validCommand = 1;

                  if (strchr(inputBuffer, ' ') != NULL) {

                     sscanfReturn = sscanf(strchr(inputBuffer, ' '), "%s", sessionID);

                     if (sscanfReturn != EOF) {

                        strcpy(packetData, sessionID);
                        numByteSent = formatPacket(JOIN, strlen(packetData), inputClientID, packetData, buffer);

                        write(sockfd, buffer, numByteSent);
                     }
                     else if (sscanfReturn == EOF) {

                        printf("Invalid Session ID:\n");
                     }
                  }
                  else if (strchr(inputBuffer, ' ') == NULL) {
                     
                     printf("Try Entering: /joinsession <Session ID>\n");
                  }
               }
               else if (strcmp(commandString, "/leavesession") == 0) {

                  validCommand = 1;

                  inSession = 0;

                  sprintf(controlMessage, "%d", LEAVE_SESS);
                  numByteSent = formatPacket(LEAVE_SESS, strlen(controlMessage), inputClientID, controlMessage, buffer);

                  write(sockfd, buffer, numByteSent);

                  printf("Please Enter A Command:\n");
               }
               else if (strcmp(commandString, "/createsession") == 0) {

                  validCommand = 1;

                  if (strchr(inputBuffer, ' ') != NULL) {

                     sscanfReturn = sscanf(strchr(inputBuffer, ' '), "%s", sessionID);

                     if (sscanfReturn != EOF) {

                        strcpy(packetData, sessionID);
                        numByteSent = formatPacket(NEW_SESS, strlen(packetData), inputClientID, packetData, buffer);

                        write(sockfd, buffer, numByteSent);
                     }
                     else if (sscanfReturn == EOF) {

                        printf("Invalid Session ID:\n");
                     }
                  }
                  else if (strchr(inputBuffer, ' ') == NULL) {
                     
                     printf("Try Entering: /createsession <Session ID>\n");
                  }
               }
               else if (strcmp(commandString, "/list") == 0) {

                  validCommand = 1;

                  sprintf(controlMessage, "%d", QUERY);
                  numByteSent = formatPacket(QUERY, strlen(controlMessage), inputClientID, controlMessage, buffer);

                  write(sockfd, buffer, numByteSent);
               }
               else if (strcmp(commandString, "/quit") == 0) {

                  printf("Exiting Program\n");
                  return 0;
               }
               else if (strcmp(commandString, "/clear") == 0) {

                  system("clear");

                  if (inSession == 1) {

                     printf("%s: ", inputClientID);
                  }
                  else if (inSession == 0) {

                     printf("Please Enter A Command:\n");
                  }
               }
               else if (strcmp(commandString, "/kick") == 0) {

                  validCommand = 1;

                  if (inSession == 1) {

                     if (strchr(inputBuffer, ' ') != NULL) {

                        sscanfReturn = sscanf(strchr(inputBuffer, ' '), "%s", targetClientID);

                        if (sscanfReturn != EOF) {

                           strcpy(packetData, targetClientID);
                           numByteSent = formatPacket(KICK, strlen(packetData), inputClientID, packetData, buffer);

                           write(sockfd, buffer, numByteSent);
                        }
                        else if (sscanfReturn == EOF) {

                           printf("Invalid Client ID:\n");
                        }
                     }
                     else if (strchr(inputBuffer, ' ') == NULL) {
                        
                        printf("Try Entering: /kick <Client ID>\n");
                     }
                  }
                  else if (inSession == 0) {

                     printf("This Command Can Only Be Used In A Session:\n");
                  }
               }
               else if (commandString[0] == '/') {

                  validCommand = 0;

                  printf("Please Enter A Valid Command:\n");

                  if (inSession == 1) {

                     printf("%s: ", inputClientID);
                  }
               }
               else if (strcmp(commandString, "") != 0) {

                  validCommand = 1;

                  if (inSession == 1) {
                     
                     strcpy(packetData, inputBuffer);
                     numByteSent = formatPacket(MESSAGE, strlen(inputBuffer), inputClientID, inputBuffer, buffer);

                     write(sockfd, buffer, numByteSent);

                     printf("%s: ", inputClientID);
                  }
                  else if (inSession == 0) {

                     printf("Please Enter A Command:\n");
                  }
               }
            }

            if (FD_ISSET(sockfd, &socketReaderfd)) {

               numByteReceived = read(sockfd, buffer, MAX_BUFFER_SIZE);
               deconstructPacket(buffer, &serverMessage);

               if (numByteReceived == 0) {

                  printf("Server Disconnected: Exiting Program\n");
                  return 0;
               }

               if (serverMessage.type == JN_ACK) {

                  inSession = 1;

                  printf("Joined Session:\n");

                  printf("%s: ", inputClientID);
               }
               else if (serverMessage.type == JN_NAK) {

                  printf("Session Does Not Exist:\n");
               }
               else if (serverMessage.type == NS_ACK) {

                  inSession = 1;

                  printf("Session Created:\n");

                  printf("%s: ", inputClientID);
               }
               else if (serverMessage.type == NS_NAK) {

                  if (strcmp(serverMessage.data, "MAX_SESSION_LIMIT_REACHED") == 0) {

                     printf("Maximum Session Limit Has Been Reached:\n");
                  }
                  else if (strcmp(serverMessage.data, "SESSION_WITH_SAME_NAME") == 0) {

                     printf("A Session With The Same Name Already Exists:\n");
                  }
               }
               else if (serverMessage.type == KICK_ACK) {

                  if (strcmp(serverMessage.source, inputClientID) == 0) {

                     printf("User \"%s\" Has Been Kicked:\n", targetClientID);
                     printf("%s: ", inputClientID);
                  }
                  else if (strcmp(serverMessage.source, inputClientID) != 0) {

                     inSession = 0;

                     printf("\rYou Have Been Kick From Session: %s\n", serverMessage.data);
                     printf("Please Enter A Command:\n");
                  }
               }
               else if (serverMessage.type == KICK_NAK) {

                  if (strcmp(serverMessage.data, "NOT_ADMIN") == 0) {

                     printf("Only The Admin Can Use: /kick\n");
                  }
                  else if (strcmp(serverMessage.data, "USER_DOES_NOT_EXIST") == 0) {

                     printf("User \"%s\" Does Not Exist:\n", targetClientID);
                  }
                  else if (strcmp(serverMessage.data, "USER_NOT_IN_SESSION") == 0) {

                     printf("User \"%s\" Is Not In Session:\n", targetClientID);
                  }
               }
               else if (serverMessage.type == QU_ACK) {

                  printf("---------- List ----------\n");

                  printList(serverMessage.data, serverMessage.size);

                  printf("--------------------------\n");

                  if (inSession == 0) {
                     
                     printf("Please Enter A Command:\n");
                  }
                  else if (inSession == 1) {

                     printf("%s: ", inputClientID);
                  }
               }
               else if (serverMessage.type == MESSAGE) {

                  printf("\33[2K\r");

                  printf("\r%s: %s", serverMessage.source, serverMessage.data);

                  printf("%s: ", inputClientID);
               }
            }
         }
         else if (selectReturn != 0) {

            memset(inputBuffer, 0, MAX_DATA * sizeof(char));
            memset(commandString, 0, MAX_COMMAND_SIZE * sizeof(char));

            read(stdinfd, inputBuffer, MAX_DATA);

            processed = 0;
         }
      }
   }

   return 0;
}

void printList(char* packetData, int packetSize) {

   for (int i = 0; i <  packetSize; i++) {

      if (packetData[i] == '|') {
         
         printf(": ");
      }
      else if (packetData[i] == ',') {

         if (packetData[i + 1] != '\n') {
            
            printf(", ");
         }
      }
      else {

         printf("%c", packetData[i]);
      }
   }
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