#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define SERVERPORT "4950" // the port users will be connecting to
#define MAX_BUFFER_SIZE 100

int main(int argc, char *argv[]) {
   
   int sockfd;
   struct addrinfo hints, *servinfo, *p;
   struct sockaddr_storage senderInfo;
   int rv;
   int numbytes;

   char buffer[MAX_BUFFER_SIZE];

   socklen_t addressLength = sizeof(senderInfo);

   if (argc != 3) {
      fprintf(stderr,"usage: talker hostname message\n");
      exit(1);
   }

   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET; // set to AF_INET to use IPv4
   hints.ai_socktype = SOCK_DGRAM;

   if ((rv = getaddrinfo(argv[1], SERVERPORT, &hints, &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
   }

   // loop through all the results and make a socket
   for(p = servinfo; p != NULL; p = p->ai_next) {
      if ((sockfd = socket(p->ai_family, p->ai_socktype,
      p->ai_protocol)) == -1) {
         perror("talker: socket");
         continue;
      }

      break;
   }

   if (p == NULL) {
      fprintf(stderr, "talker: failed to create socket\n");
      return 2;
   }

   if ((numbytes = sendto(sockfd, argv[2], strlen(argv[2]), 0,
   p->ai_addr, p->ai_addrlen)) == -1) {
      perror("talker: sendto");
      exit(1);
   }

   freeaddrinfo(servinfo);

   printf("talker: sent %d bytes to %s\n", numbytes, argv[1]);

   numbytes = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE - 1, 0, (struct sockaddr *)&senderInfo, &addressLength);
  
   printf("listener: packet is %d bytes long\n", numbytes);
   buffer[numbytes] = '\0';
   printf("listener: packet contains \"%s\"\n", buffer);
   close(sockfd);

   return 0;
   
}
