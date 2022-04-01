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
#include <signal.h>

#define NUMBER_OF_USERS MAX_FILE_BUFFER_SIZE / (MAX_USERNAME_PASSWORD_SIZE * 2)
#define BACKLOG 10 // how many pending connections queue will hold
#define MAX_CLIENTS 20
#define MAX_HEADER_UNIT_SIZE 20
#define MAX_USERNAME_PASSWORD_SIZE 50
#define MAX_HEADER_BUILDER_SIZE 200
#define MAX_CONTROL_MESSAGE_SIZE 200
#define MAX_HEADER_EXTRACTOR_SIZE 200
#define MAX_NAME 200
#define MAX_DATA 1000
#define MAX_BUFFER_SIZE 1200
#define MAX_FILE_BUFFER_SIZE 2000

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

#define FILENAME "Users.txt"

struct message {
   
   unsigned int type;
   unsigned int size;
   unsigned char source[MAX_NAME];
   unsigned char data[MAX_DATA];
};

struct user {

   char username[MAX_USERNAME_PASSWORD_SIZE];
   char password[MAX_USERNAME_PASSWORD_SIZE];
   
   int sockfd;
   int loggedIn;
};

struct session {

   char sessionName[MAX_NAME];
   char adminName[MAX_USERNAME_PASSWORD_SIZE];

   int active;
   int numberOfUsers;

   int userIndexSessionLookup[NUMBER_OF_USERS];
};

struct listHead {

   struct listNode* head;
};

struct listNode {

   struct listNode* nextNode;
   struct listNode* previousNode;

   struct user* userProfile;
};

void ctrlCHandler(int signum);
int loadFileIntoList(struct listHead* listHead);
struct listHead* createList();
void insertNode(struct listHead* listHead, char* username, char* password);
int findSockfdWithUsername(struct listHead* listHead, char* username);
struct user* findUserProfileWithSockfd(struct listHead* listHead, int sockfd);
struct user* findUserProfileWithUsername(struct listHead* listHead, char* username);
int validLoginCheck(struct listHead* listHead, char* username, char* password);
int usernameExitsCheck(struct listHead* listHead, char* username);
int findListIndexWithSockfd(struct listHead* listHead, int sockfd);
int findListIndexWithUsername(struct listHead* listHead, char* username);
void leaveSession(struct listHead* listHead, int sockfd);
int findSessionIndexWithSockfd(struct listHead* listHead, int sockfd);
int findSessionIndexWithName(char* sessionName);
int findInactiveSession();
void deconstructPacket(unsigned char* formattedPacket, struct message* clientMessage);
int formatPacket(unsigned int type, unsigned int size, unsigned char* source, unsigned char* data, unsigned char* formattedPacket);

int noSessionList[NUMBER_OF_USERS];

struct session listOfSessions[NUMBER_OF_USERS];

struct listHead* listOfUsers;

int main(int argc, char *argv[]) {

   int masterSockfd = 0;
   int newSockfd = 0;
   int maxSockfd = 0;
   int currentSockfd = 0;
   int opt = 1;
   int numByteSent = 0;
   int numByteReceived = 0;
   int validLoginCheckReturn = 0;
   int addressPortReaderSize = 0;
   int inactiveSession = 0;
   int findSessionWithNameReturn = 0;
   int usernameExitsCheckReturn = 0;
   int firstIteration = 0;
   int inSessionCount = 0;
   int sessionIndex = 0;
   int numberOfuserProfiles = 0;

   fd_set readerfd;

   int clientSocket[MAX_CLIENTS];

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
   struct listNode* currentNode;

   socklen_t addressLength;

   struct user userAlpha;
   struct user userBeta;
   struct user userDelta;
   struct user userGamma;
   struct user userOmega;

   listOfUsers = createList();

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      memset(listOfSessions[i].sessionName, 0, MAX_NAME * sizeof(char));
      memset(listOfSessions[i].userIndexSessionLookup, 0, NUMBER_OF_USERS * sizeof(int));

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

   signal(SIGINT, ctrlCHandler);

   // Resolves a hostname into an address
   if (getaddrinfo(NULL, argv[1], &serverInfo, &serverInfoPtr) != 0) {

      printf("Invalid Port Number: Exiting Program\n");
      return 0;
   }

   // Create a socket
   masterSockfd = socket(AF_INET, SOCK_STREAM, 0);

   setsockopt(masterSockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));

   // Bind it to the port we passed in to getaddrinfo()
   bind(masterSockfd, serverInfoPtr->ai_addr, serverInfoPtr->ai_addrlen);

   listen(masterSockfd, BACKLOG);

   if (access(FILENAME, F_OK) != 0) {

      printf("\"%s\" Does Not Exist: Exiting Program\n", FILENAME);
      return 0;
   }

   numberOfuserProfiles = loadFileIntoList(listOfUsers);

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

         printf("New Connection With IP: %s\n", clientIP);

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

               leaveSession(listOfUsers, currentSockfd);

               getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
               clientIP = inet_ntoa(addressPortReader.sin_addr);

               printf("User \"%s\" Disconnected With IP: %s\n", findUserProfileWithSockfd(listOfUsers, currentSockfd)->username, clientIP);

               noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 0;
               findUserProfileWithSockfd(listOfUsers, currentSockfd)->loggedIn = 0;
               listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].userIndexSessionLookup[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 0;
               findUserProfileWithSockfd(listOfUsers, currentSockfd)->sockfd = -1;

               close(currentSockfd);

               clientSocket[i] = 0;
            }
            else {

               deconstructPacket(buffer, &clientMessage);

               if (clientMessage.type == LOGIN) {

                  validLoginCheckReturn = validLoginCheck(listOfUsers, clientMessage.source, clientMessage.data);

                  if (validLoginCheckReturn == INVALID_USERNAME) {

                     sprintf(controlMessage, "%d", INVALID_USERNAME);
                     numByteSent = formatPacket(LO_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);

                     getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                     clientIP = inet_ntoa(addressPortReader.sin_addr);

                     printf("Invalid Username, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else if (validLoginCheckReturn == INVALID_PASSWORD) {

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

                     if (findUserProfileWithUsername(listOfUsers, clientMessage.source)->loggedIn == 0) {

                        findUserProfileWithUsername(listOfUsers, clientMessage.source)->loggedIn = 1;
                        findUserProfileWithUsername(listOfUsers, clientMessage.source)->sockfd = currentSockfd; 
                        noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 1;

                        sprintf(controlMessage, "%d", VALID);
                        numByteSent = formatPacket(LO_ACK, strlen(controlMessage), "Server", controlMessage, buffer);
                        
                        write(currentSockfd, buffer, numByteSent);

                        getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                        clientIP = inet_ntoa(addressPortReader.sin_addr);

                        printf("User \"%s\" Logged In With IP: %s\n", clientMessage.source, clientIP);
                     }
                     else if (findUserProfileWithUsername(listOfUsers, clientMessage.source)->loggedIn == 1) {

                        sprintf(controlMessage, "%d", ALREADY_LOGGED_IN);
                        numByteSent = formatPacket(LO_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);

                        getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                        clientIP = inet_ntoa(addressPortReader.sin_addr);

                        printf("User \"%s\" Already Logged In, Client Disconnected: %s\n", clientMessage.source, clientIP);

                        close(currentSockfd);

                        clientSocket[i] = 0;
                     }
                  }
               }
               else if (clientMessage.type == REGISTER) {

                  usernameExitsCheckReturn = usernameExitsCheck(listOfUsers, clientMessage.source);

                  if (usernameExitsCheckReturn == 1) {

                     sprintf(controlMessage, "%d", INVALID_USERNAME);
                     numByteSent = formatPacket(REGISTER_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);

                     getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                     clientIP = inet_ntoa(addressPortReader.sin_addr);

                     printf("Username Already In Use, Client Disconnected: %s\n", clientIP);

                     close(currentSockfd);

                     clientSocket[i] = 0;
                  }
                  else if (usernameExitsCheckReturn == 0) {

                     if (numberOfuserProfiles < NUMBER_OF_USERS) {
                        
                        insertNode(listOfUsers, clientMessage.source, clientMessage.data);

                        numberOfuserProfiles += 1;

                        findUserProfileWithUsername(listOfUsers, clientMessage.source)->loggedIn = 1;
                        findUserProfileWithUsername(listOfUsers, clientMessage.source)->sockfd = currentSockfd; 
                        noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 1;

                        sprintf(controlMessage, "%d", VALID);
                        numByteSent = formatPacket(REGISTER_ACK, strlen(controlMessage), "Server", controlMessage, buffer);
                        
                        write(currentSockfd, buffer, numByteSent);

                        getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                        clientIP = inet_ntoa(addressPortReader.sin_addr);

                        printf("New User \"%s\" Logged In With IP: %s\n", clientMessage.source, clientIP);
                     }
                     else {

                        sprintf(controlMessage, "%d", ACCOUNT_LIMIT_REACHED);
                        numByteSent = formatPacket(REGISTER_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);

                        getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                        clientIP = inet_ntoa(addressPortReader.sin_addr);

                        printf("Account Limit Reached, Client Disconnected: %s\n", clientIP);

                        close(currentSockfd);

                        clientSocket[i] = 0;
                     }
                  }
               }
               else if (clientMessage.type == EXIT) {

                  leaveSession(listOfUsers, currentSockfd);

                  getpeername(currentSockfd, (struct sockaddr*)&addressPortReader, &addressPortReaderSize);
                  clientIP = inet_ntoa(addressPortReader.sin_addr);

                  printf("Client \"%s\" Logged Out With IP: %s\n", findUserProfileWithSockfd(listOfUsers, currentSockfd)->username, clientIP);

                  noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 0;
                  findUserProfileWithSockfd(listOfUsers, currentSockfd)->loggedIn = 0;
                  listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].userIndexSessionLookup[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 0;
                  findUserProfileWithSockfd(listOfUsers, currentSockfd)->sockfd = -1;

                  close(currentSockfd);

                  clientSocket[i] = 0;
               }
               else if (clientMessage.type == JOIN) {

                  leaveSession(listOfUsers, currentSockfd);

                  findSessionWithNameReturn = findSessionIndexWithName(clientMessage.data);

                  if (findSessionWithNameReturn != -1) {
                     
                     noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 2;
                     listOfSessions[findSessionWithNameReturn].numberOfUsers += 1;
                     listOfSessions[findSessionWithNameReturn].userIndexSessionLookup[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 1;

                     printf("User \"%s\" Joined Session \"%s\"\n", findUserProfileWithSockfd(listOfUsers, currentSockfd)->username, listOfSessions[findSessionWithNameReturn].sessionName);
                  
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
                  
                  printf("User \"%s\" Left Session \"%s\"\n", findUserProfileWithSockfd(listOfUsers, currentSockfd)->username, listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].sessionName);

                  leaveSession(listOfUsers, currentSockfd);
               }
               else if (clientMessage.type == NEW_SESS) {
               
                  leaveSession(listOfUsers, currentSockfd);

                  findSessionWithNameReturn = findSessionIndexWithName(clientMessage.data);

                  if (findSessionWithNameReturn == -1) {

                     inactiveSession = findInactiveSession();

                     if (inactiveSession != -1) {

                        strcpy(listOfSessions[inactiveSession].sessionName, clientMessage.data);

                        noSessionList[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 2;
                        listOfSessions[inactiveSession].active = 1;
                        listOfSessions[inactiveSession].numberOfUsers = 1;
                        listOfSessions[inactiveSession].userIndexSessionLookup[findListIndexWithSockfd(listOfUsers, currentSockfd)] = 1;
                        
                        strcpy(listOfSessions[inactiveSession].adminName, clientMessage.source);

                        printf("User \"%s\" Created Session \"%s\"\n", findUserProfileWithSockfd(listOfUsers, currentSockfd)->username, clientMessage.data);

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

                     printf("A Session With The Name \"%s\" Already Exists:\n", clientMessage.data);

                     strcpy(controlMessage, "SESSION_WITH_SAME_NAME");
                     numByteSent = formatPacket(NS_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                     write(currentSockfd, buffer, numByteSent);
                  }
               }
               else if (clientMessage.type == KICK) {

                  if (strcmp(listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].adminName, clientMessage.source) == 0) {

                     if (usernameExitsCheck(listOfUsers, clientMessage.data) == 0) {

                        strcpy(controlMessage, "USER_DOES_NOT_EXIST");
                        numByteSent = formatPacket(KICK_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                        write(currentSockfd, buffer, numByteSent);
                     }
                     else if (usernameExitsCheck(listOfUsers, clientMessage.data) == 1) {

                        if (listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].userIndexSessionLookup[findListIndexWithUsername(listOfUsers, clientMessage.data)] == 0) {

                           strcpy(controlMessage, "USER_NOT_IN_SESSION");
                           numByteSent = formatPacket(KICK_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

                           write(currentSockfd, buffer, numByteSent);
                        }
                        else if (listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].userIndexSessionLookup[findListIndexWithUsername(listOfUsers, clientMessage.data)] == 1) {

                           leaveSession(listOfUsers, findSockfdWithUsername(listOfUsers, clientMessage.data));

                           strcpy(controlMessage, listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].sessionName);
                           numByteSent = formatPacket(KICK_ACK, strlen(controlMessage), clientMessage.source, controlMessage, buffer);

                           write(currentSockfd, buffer, numByteSent);
                           write(findSockfdWithUsername(listOfUsers, clientMessage.data), buffer, numByteSent);
                        }
                     }
                  }
                  else if (strcmp(listOfSessions[findSessionIndexWithSockfd(listOfUsers, currentSockfd)].adminName, clientMessage.source) != 0) {

                     strcpy(controlMessage, "NOT_ADMIN");
                     numByteSent = formatPacket(KICK_NAK, strlen(controlMessage), "Server", controlMessage, buffer);

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
                        };
                        
                        strcat(packetData, listOfSessions[i].sessionName);
                        strcat(packetData, "|");

                        currentNode = listOfUsers->head;

                        for (int j = 0; currentNode != NULL; j++) {

                           if (listOfSessions[i].userIndexSessionLookup[j] == 1) {

                              if (strcmp(currentNode->userProfile->username, listOfSessions[i].adminName) == 0) {

                                 strcat(packetData, "(Admin) ");
                              }

                              strcat(packetData, currentNode->userProfile->username);
                              strcat(packetData, ",");
                           }

                           currentNode = currentNode->nextNode;
                        }
                        
                        strcat(packetData, "\n");
                     }
                  }
                  
                  inSessionCount = 0;

                  currentNode = listOfUsers->head;

                  for (int i = 0; currentNode != NULL; i++) {

                     if(noSessionList[i] == 1) {

                        strcat(queryBuilder, currentNode->userProfile->username);
                        strcat(queryBuilder, ",");

                        inSessionCount += 1;
                     }

                     currentNode = currentNode->nextNode;
                  }

                  if (inSessionCount > 0) {

                     strcat(packetData, "Online/Not In Session|");
                     strcat(packetData, queryBuilder);
                     strcat(packetData, "\n");
                  }

                  firstIteration = 0;

                  numByteSent = formatPacket(QU_ACK, strlen(packetData), "Server", packetData, buffer);
                  
                  write(currentSockfd, buffer, numByteSent);
               }
               else if (clientMessage.type == MESSAGE) {

                  sessionIndex = findSessionIndexWithSockfd(listOfUsers, currentSockfd);

                  numByteSent = formatPacket(MESSAGE, strlen(clientMessage.data), clientMessage.source, clientMessage.data, buffer);

                  currentNode = listOfUsers->head;

                  for (int i = 0; currentNode != NULL; i++) {

                     if(listOfSessions[sessionIndex].userIndexSessionLookup[i] == 1) {

                        if (currentSockfd != currentNode->userProfile->sockfd) {
                           
                           write(currentNode->userProfile->sockfd, buffer, numByteSent);
                        }
                     }

                     currentNode = currentNode->nextNode;
                  }
               }
            }
         }
      }
   }

   return 0;
}

void ctrlCHandler(int signum) {

   struct listNode* currentNode;

   char fileBuffer[MAX_FILE_BUFFER_SIZE];

   FILE* usersFile;

   memset(fileBuffer, 0, MAX_FILE_BUFFER_SIZE * sizeof(char));

   usersFile = fopen(FILENAME, "w");

   printf("\n");

   if (usersFile == NULL) {

      printf("\"%s\" Failed To Open: Exiting Program\n", FILENAME);
      exit(signum);
   }

   if (listOfUsers->head == NULL) {

      printf("The List Of Users Is Empty: Exiting Program\n");
      exit(signum);
   }

   currentNode = listOfUsers->head;

   while (currentNode != NULL) {

      strcat(fileBuffer, currentNode->userProfile->username);
      strcat(fileBuffer, ":");
      strcat(fileBuffer, currentNode->userProfile->password);

      if (currentNode->nextNode != NULL) {

         strcat(fileBuffer, "\n");
      }
   
      currentNode = currentNode->nextNode;
   }

   fwrite(fileBuffer, 1, strlen(fileBuffer), usersFile);

   fclose(usersFile);
   
   exit(signum);
}

int loadFileIntoList(struct listHead* listHead) {

   int usernamePlaceMark = 0;
   int passwordPlaceMark = 0;
   int fileSize = 0;
   int numberOfuserProfiles = 0;
   int scanUsernamePassword = 1;

   char fileBuffer[MAX_FILE_BUFFER_SIZE];
   char usernameFromFile[MAX_USERNAME_PASSWORD_SIZE];
   char passwordFromFile[MAX_USERNAME_PASSWORD_SIZE];

   FILE* usersFile;

   usersFile = fopen(FILENAME, "r");

   if (usersFile == NULL) {

      printf("\"%s\" Failed To Open: Exiting Program\n", FILENAME);
      exit(0);
   }

   fileSize = fread(fileBuffer, 1, MAX_FILE_BUFFER_SIZE, usersFile);
   fileBuffer[fileSize] = '\0';

   fclose(usersFile);

   for (int i = 0; i < fileSize; i++) {

      if ((fileBuffer[i] != ':') && (fileBuffer[i] != '\n')) {
         
         if (scanUsernamePassword % 3 == 1) {

            usernameFromFile[i - usernamePlaceMark] = fileBuffer[i];
            usernameFromFile[i - usernamePlaceMark + 1] = '\0';
         }
         else if (scanUsernamePassword % 3 == 2) {

            passwordFromFile[i - passwordPlaceMark] = fileBuffer[i];
            passwordFromFile[i - passwordPlaceMark + 1] = '\0';
         }
      }

      if ((fileBuffer[i] == ':') || (fileBuffer[i] == '\n') || (i == fileSize - 1)) {

         scanUsernamePassword += 1;

         if (scanUsernamePassword % 3 == 0) {

            insertNode(listOfUsers, usernameFromFile, passwordFromFile);
            
            numberOfuserProfiles += 1;
            scanUsernamePassword += 1;
         }
         
         if (scanUsernamePassword % 3 == 1) {

            usernamePlaceMark = i + 1;
         }
         else if (scanUsernamePassword % 3 == 2) {

            passwordPlaceMark = i + 1;
         }
      }
   }

   return numberOfuserProfiles;
}

struct listHead* createList() {

   struct listHead* newList = NULL;

   newList = (struct listHead*)malloc(sizeof(struct listHead));

   newList->head = NULL;

   return newList;
}

void insertNode(struct listHead* listHead, char* username, char* password) {

   struct listNode* newNode = NULL;
   struct listNode* currentNode = NULL;

   struct user* newUserProfile = NULL;

   newNode = (struct listNode*)malloc(sizeof(struct listNode));
   newUserProfile = (struct user*)malloc(sizeof(struct user));

   strcpy(newUserProfile->username, username);
   strcpy(newUserProfile->password, password);

   newUserProfile->sockfd = -1;
   newUserProfile->loggedIn = 0;

   newNode->userProfile = newUserProfile;

   if (listHead->head == NULL) {

      listHead->head = newNode;

      newNode->nextNode = NULL;
      newNode->previousNode = NULL;

      return;
   }

   currentNode = listHead->head;

   while (currentNode->nextNode != NULL) {

      currentNode = currentNode->nextNode;
   }

   currentNode->nextNode = newNode;

   newNode->nextNode = NULL;
   newNode->previousNode = currentNode;
}

int findSockfdWithUsername(struct listHead* listHead, char* username) {
   
   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (strcmp(currentNode->userProfile->username, username) == 0) {

         return currentNode->userProfile->sockfd;
      }

      currentNode = currentNode->nextNode;
   }

   return -1;
}

struct user* findUserProfileWithSockfd(struct listHead* listHead, int sockfd) {

   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (currentNode->userProfile->sockfd == sockfd) {

         return currentNode->userProfile;
      }

      currentNode = currentNode->nextNode;
   }

   return NULL;
}

struct user* findUserProfileWithUsername(struct listHead* listHead, char* username) {

   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (strcmp(currentNode->userProfile->username, username) == 0) {

         return currentNode->userProfile;
      }

      currentNode = currentNode->nextNode;
   }

   return NULL;
}

int validLoginCheck(struct listHead* listHead, char* username, char* password) {

   struct listNode* currentNode = NULL;

   if (listHead->head == NULL) {

      return LIST_EMPTY;
   }

   currentNode = listHead->head;
   
   while (strcmp(currentNode->userProfile->username, username) != 0) {

      currentNode = currentNode->nextNode;

      if (currentNode == NULL) {

         return INVALID_USERNAME;
      }
   }

   currentNode = listHead->head;
   
   while ((strcmp(currentNode->userProfile->username, username) != 0) || (strcmp(currentNode->userProfile->password, password) != 0)) {

      currentNode = currentNode->nextNode;

      if (currentNode == NULL) {

         return INVALID_PASSWORD;
      }
   }

   return VALID;
}

int usernameExitsCheck(struct listHead* listHead, char* username) {

   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (strcmp(currentNode->userProfile->username, username) == 0) {

         return 1;
      }

      currentNode = currentNode->nextNode;
   }

   return 0;
}

int findListIndexWithSockfd(struct listHead* listHead, int sockfd) {
   
   int index = 0;

   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (currentNode->userProfile->sockfd == sockfd) {

         return index;
      }

      index += 1;

      currentNode = currentNode->nextNode;
   }

   return -1;
}

int findListIndexWithUsername(struct listHead* listHead, char* username) {
   
   int index = 0;

   struct listNode* currentNode = NULL;

   currentNode = listHead->head;

   while (currentNode != NULL) {

      if (strcmp(currentNode->userProfile->username, username) == 0) {

         return index;
      }

      index += 1;

      currentNode = currentNode->nextNode;
   }

   return -1;
}

void leaveSession(struct listHead* listHead, int sockfd) {

   int findSessionIndexWithSockfdReturn = 0;

   findSessionIndexWithSockfdReturn = findSessionIndexWithSockfd(listHead, sockfd);

   if (findSessionIndexWithSockfdReturn != -1) {

      listOfSessions[findSessionIndexWithSockfdReturn].userIndexSessionLookup[findListIndexWithSockfd(listHead, sockfd)] = 0;
      listOfSessions[findSessionIndexWithSockfdReturn].numberOfUsers -= 1;

      if (listOfSessions[findSessionIndexWithSockfdReturn].numberOfUsers == 0) {

         memset(listOfSessions[findSessionIndexWithSockfdReturn].sessionName, 0, MAX_NAME * sizeof(char));
         listOfSessions[findSessionIndexWithSockfdReturn].active = 0;
      }
   }

   noSessionList[findListIndexWithSockfd(listHead, sockfd)] = 1;
}

int findSessionIndexWithSockfd(struct listHead* listHead, int sockfd) {

   int findUserIndexWithSockfdReturn = 0;

   findUserIndexWithSockfdReturn = findListIndexWithSockfd(listHead, sockfd);

   for (int i = 0; i < NUMBER_OF_USERS; i++) {

      if(listOfSessions[i].userIndexSessionLookup[findUserIndexWithSockfdReturn] == 1) {

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