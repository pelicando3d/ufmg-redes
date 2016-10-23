


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 256

void DieWithUserMessage(const char *msg, const char *detail);

void DieWithSystemMessage(const char *msg);


int main(int argc, char *argv[]) {
	char *ip_server = argv[1]; // First arg: server IP address (dotted quad)
	char *message = argv[2]; // Second arg: string to echo

	if (argc < 3 || argc > 4) // Test for correct number of arguments
		DieWithUserMessage("Parameter(s)",
		"<Server Address> <Echo Word> [<Server Port>]");

	// Third arg (optional): server port (numeric). 7 is well-known echo port
	in_port_t port_server = (argc == 4) ? atoi(argv[3]) : 7;

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
	int rtnVal = inet_pton(AF_INET, ip_server, &servAddr.sin_addr.s_addr);
	
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

	size_t messageLen = strlen(message); // Determine input length

	// Send the string to the server
	ssize_t numBytes = send(sock, message, messageLen, 0);
	
	if (numBytes < 0)
		DieWithSystemMessage("send() failed");
	else if (numBytes != messageLen){
		DieWithUserMessage("send()", "sent unexpected number of bytes");
	}else{
		printf("send() - Enviando mensangem para o servidor...\n");
	}

	// Receive list of files
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	fputs("Received: ", stdout); // Setup to print the echoed string
	while (1) {
		char buffer[BUFSIZE]; // I/O buffer
		/* Receive up to the buffer size (minus 1 to leave space for
		a null terminator) bytes from the sender */
	
		numBytes = recv(sock, buffer, BUFSIZE - 1, 0);
		printf("%s\n", buffer);
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

