
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <unistd.h>


#define BUFSIZE 256

static const int MAXPENDING = 5; // Maximum outstanding connection requests

void DieWithUserMessage(const char *msg, const char *detail);

void DieWithSystemMessage(const char *msg);

void HandleTCPClient(int clntSocket);

int main(int argc, char *argv[]) {

	if (argc != 2) // Test for correct number of arguments
	DieWithUserMessage("Parameter(s)", "<Server Port>");

	in_port_t port_server = atoi(argv[1]); // First arg: local port

	// Create socket for incoming connections
	int servSock; // Socket descriptor for server
	if ((servSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
		DieWithSystemMessage("socket() failed");
	}else{
		printf("Socket() - Criando o socket!\n");
	}

	// Construct local address structure
	struct sockaddr_in servAddr; // Local address
	memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	
	//config values of socket
	servAddr.sin_family = AF_INET; // IPv4 address family
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface
	servAddr.sin_port = htons(port_server); // Local port

	// Bind to the local address
	if (bind(servSock, (struct sockaddr*) &servAddr, sizeof(servAddr)) < 0){
		DieWithSystemMessage("bind() failed");
	}else{
		printf("Bind() - Atribuindo IP e porta ao descritor de arquivo!\n");
	}

	// Mark the socket so it will listen for incoming connections
	if (listen(servSock, MAXPENDING) < 0){
		DieWithSystemMessage("listen() failed");
	}else{
		printf("Listen() - Permitindo que socket ouça requisições de conexões!\nAguardando conexão...\n");
	}

	for (;;) { // Run forever
		struct sockaddr_in clntAddr; // Client address
		// Set length of client address structure (in-out parameter)
		socklen_t clntAddrLen = sizeof(clntAddr);

		// Wait for a client to connect
		int clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntAddrLen);
		if (clntSock < 0){
			DieWithSystemMessage("accept() failed");
		}else{
			printf("Accept() - Conexão recebida e aceita!\n");
		}

		// clntSock is connected to a client!
		char clntName[INET_ADDRSTRLEN]; // String to contain client address
		
		if (inet_ntop(AF_INET, &clntAddr.sin_addr.s_addr, clntName,
			sizeof(clntName)) != NULL)
			printf("IP/Port: %s:%d\n", clntName, ntohs(clntAddr.sin_port));
		else
			puts("Unable to get client address");

		HandleTCPClient(clntSock);
	}
// NOT REACHED
}



void HandleTCPClient(int clntSocket) {
  char buffer[BUFSIZE]; // Buffer for echo string
  DIR *pd = 0;
  struct dirent *pdirent = 0;

	// Receive message from client
	ssize_t numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
	
	if (numBytesRcvd < 0)
		DieWithSystemMessage("recv() failed");
	
	// Send received string and receive again until end of stream
	while (numBytesRcvd > 0) { // 0 indicates end of stream
    pd = opendir(buffer);
    if(pd == NULL) {
      // ERROR: wrong path
      return ; // define a erro CODE
    }  
    while (pdirent=readdir(pd)){    
      //printf("%s\n", pdirent->d_name);
      pdirent=readdir(pd);
      ssize_t numBytesSent = send(clntSocket, pdirent->d_name, sizeof(pdirent->d_name), 0);
      if (numBytesSent < 0){
        DieWithSystemMessage("send() failed");
      }else{
        printf("send() - Enviando mensagem para o cliente!\n");
      }
    }
    closedir(pd);
		// See if there is more data to receive
		numBytesRcvd = recv(clntSocket, buffer, BUFSIZE, 0);
		
		if (numBytesRcvd < 0){
			DieWithSystemMessage("recv() failed");
		}else{
			printf("recv() - Recebendo a resposta do cliente!\n");
		}
	}

	close(clntSocket); // Close client socket
}

void DieWithUserMessage(const char *msg, const char *detail) {
	fputs(msg, stderr);
	fputs(": ", stderr);
	fputs(detail, stderr);
	fputc('\n', stderr);
	exit(1);
}

void DieWithSystemMessage(const char *msg) {
	perror(msg);
	exit(1);
}