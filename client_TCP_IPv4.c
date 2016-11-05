


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>



void DieWithUserMessage(const char *msg, const char *detail);

void DieWithSystemMessage(const char *msg);


int main(int argc, char *argv[]) {
	char *command = argv[1]; // First arg: list or get command
	char *ipv6server, *filename; // Middle args
	in_port_t port_server;
	int bufsize;

	if (argc < 4 || argc > 5) // Test for correct number of arguments
		DieWithUserMessage("Parameter(s)",
		"<Server Address> <Echo Word> [<Server Port>]");

	// checking if we have a list or get command
	if(!strcmp("list", command)){
		ipv6server = argv[2];
		port_server = atoi(argv[3]);
		bufsize = atoi(argv[4]);
	}else if (!strcmp("get", command)){
		filename = argv[2];
		ipv6server = argv[3];
		port_server = atoi(argv[4]);
		bufsize = atoi(argv[5]);
	}

	// Create a reliable, stream socket using TCP
	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	if (sock < 0){
		DieWithSystemMessage("socket() failed");
	}else{
		printf("Sock() - Criando o socket!\n");
	}

	// Construct the server address structure
	struct sockaddr_in servAddr; // Server address

	memset(&servAddr, 0, sizeof(servAddr)); // Zero out structure
	servAddr.sin_family = AF_INET; // IPv4 address family


	
	// Converte endereço IPv4 e IPv6 do formato texto para binário
	int rtnVal = inet_pton(AF_INET, ipv6server, &servAddr.sin_addr.s_addr);
	
	if (rtnVal == 0)
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	else if (rtnVal < 0)
		DieWithSystemMessage("inet_pton() failed");

	//convert values between host and network byte order
	servAddr.sin_port = htons(port_server); // Server port

	// Establish the connection to the echo server
	if (connect(sock, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
		DieWithSystemMessage("connect() failed");
	}else{
		printf("connect() - Estabelecendo conexão...\n");
	}



	// Send the string to the server
	ssize_t numBytes;
	if(!strcmp("list", command)){
		numBytes = send(sock, "./\0", 3, 0);
	}
	
	if (numBytes < 0)
		DieWithSystemMessage("send() failed");
	else
		printf("send() - Enviando mensangem para o servidor...\n");
	

	// Receive list of files
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	fputs("Received: \n", stdout); // Setup to print the echoed string
	char buffer[bufsize]; // I/O buffer
	while (1) {
		
		/* Receive up to the buffer size (minus 1 to leave space for
		a null terminator) bytes from the sender */
		
		printf("Antes receber: \n");
		numBytes = recv(sock, buffer, bufsize - 1, 0);
		printf("Depois receber: \n");
		//PRECISA CRIAR LISTA ENCADEADA PRA PARTIR OS NOMES CORRETAMENTE
		//nao necessariamente lista
//		printf("%s\n", buffer);
		int i;
		printf("(Bytes received: %li) ", numBytes);
		for (i = 0; i < bufsize; i++) printf("%c", buffer[i]);
		printf("\n======\n");
		buffer[0] = '\0';

		if (numBytes < 0)
			break;
		else if (numBytes == 0)
			break;
		
		totalBytesRcvd += numBytes; // Keep tally of total bytes
		//buffer[numBytes] = '\0'; // Terminate the string!
		//fputs(buffer, stdout); // Print the echo buffer
		//printf("%s\n", buffer);
	}


	fputc('\n', stdout); // Print a final linefeed
	close(sock);
	exit(0);
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

