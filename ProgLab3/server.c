#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>

#define MAX_MESSAGE_SIZE 200
#define MAX_PAYLOAD_SIZE 1000
#define MAX_BUFFER_SIZE 1200

#define PACKET_DROP_RATE 0.1

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
	int fileNameSize = 0;

	int nextBaseTen = 1;

	char message[MAX_MESSAGE_SIZE];
	char buffer[MAX_BUFFER_SIZE];
	char headerExtractor[200];

	char* fileBuffer;

	struct packet filePacket;

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

	memset(message, 0, MAX_MESSAGE_SIZE);
	memset(buffer, 0, MAX_BUFFER_SIZE);

	// Resolves a hostname into an address
	getaddrinfo(NULL, argv[1], &serverInfo, &serverInfoPtr);

	// Create a socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	// Bind it to the port we passed in to getaddrinfo()
	bind(sockfd, serverInfoPtr->ai_addr, sizeof(*(serverInfoPtr->ai_addr)));

	printf("Server Listening On Port: %s\n", argv[1]);

	// Receives a message from the client
	numByteReceived = recvfrom(sockfd, message, MAX_MESSAGE_SIZE, 0, (struct sockaddr *)&clientInfo, &addressLength);

	message[numByteReceived] = '\0';
	printf("Server Received: \"%s\"\n", message);

	if (strcmp(message, "ftp") == 0) {

		strcpy(message, "yes");
	}
	else {

		strcpy(message, "no");
	}

	// Sends a message to the client
	numByteSent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&clientInfo, serverInfoPtr->ai_addrlen);
	printf("Server Sent: \"%s\"\n", message);

//////////////////// ProgLab 2 ////////////////////

	numByteReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&clientInfo, &addressLength);

	printf("File Transfer Started\n");

	strcpy(message, "Ack");

	numByteSent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&clientInfo, serverInfoPtr->ai_addrlen);

	memset(headerExtractor, 0, 200);

	for (int i = 0; ; i++) {

		if (buffer[i] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = buffer[i];
	}

	filePacket.total_frag = atoi(headerExtractor);

	charCount = strlen(headerExtractor) + 1;

	memset(headerExtractor, 0, 200);

	for (int i = 0; ; i++) {

		if (buffer[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = buffer[i + charCount];
	}

	filePacket.frag_no = atoi(headerExtractor);

	charCount = charCount + strlen(headerExtractor) + 1;

	memset(headerExtractor, 0, 200);

	for (int i = 0; ; i++) {

		if (buffer[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = buffer[i + charCount];
	}

	filePacket.size = atoi(headerExtractor);

	charCount = charCount + strlen(headerExtractor) + 1;

	memset(headerExtractor, 0, 200);

	for (int i = 0; ; i++) {

		fileNameSize += 1;

		if (buffer[i + charCount] == ':') {
			
			headerExtractor[i] = '\0';

			break;
		}

		headerExtractor[i] = buffer[i + charCount];
	}

	filePacket.filename = (char *)malloc(fileNameSize * sizeof(char));

	strcpy(filePacket.filename, headerExtractor);

	printf("Filename: %s | Size: %d Bytes | Number Of Packets: %d\n", filePacket.filename, filePacket.size, filePacket.total_frag);

	charCount = charCount + strlen(filePacket.filename) + 1;

	fileBuffer = (char *)malloc(filePacket.size);

	for (int k = 0; k < filePacket.total_frag; k++) {
		
		int byteNumerator = 0;

		if (filePacket.size - (k * MAX_PAYLOAD_SIZE) >= MAX_PAYLOAD_SIZE) {

			byteNumerator = MAX_PAYLOAD_SIZE;
		}
		else {

			byteNumerator = filePacket.size % MAX_PAYLOAD_SIZE;
		}

		if (k > 0) {
			
			// Simulated Packet Drop
			while (1) {
					
				numByteReceived = recvfrom(sockfd, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr *)&clientInfo, &addressLength);

				if ((double)rand() / (double)((unsigned)RAND_MAX + 1) > PACKET_DROP_RATE) {

					strcpy(message, "Ack");

					numByteSent = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&clientInfo, serverInfoPtr->ai_addrlen);

					break;
				}
				else {

					printf("Packet %d: DROPPED\n", k);
				}
			}
		}

		for (int i = 0; i < byteNumerator; i++) {

			filePacket.filedata[i] = buffer[i + charCount];

			fileBuffer[(k * MAX_PAYLOAD_SIZE) + i] = buffer[i + charCount];
		}

		if (nextBaseTen == (int)log10(k + 2)) {

			charCount += 1;
			nextBaseTen += 1;
		}
	}

	FILE* transferFile = fopen(filePacket.filename, "w");

	fwrite(fileBuffer, 1, filePacket.size, transferFile);

	fclose(transferFile);
	close(sockfd);

	return 0;
}