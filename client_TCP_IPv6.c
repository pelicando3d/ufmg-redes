


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

int create_socket() {
  // Create a reliable, stream socket using TCP
	int sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	
	if (sock < 0){
		DieWithSystemMessage("socket() failed");
	}else{
		printf("Sock() - Criando o socket!\n");
	}

	return sock;
}

struct sockaddr_in6 setup_addr(char *ip_server, in_port *port_server) {
  // Construct the server address structure
	struct sockaddr_in6 serv_addr; // Server address
  serv_addr.sin6_family = AF_INET6;
  serv_addr.sin6_port = htons(port_server); // Server port
  serv_addr.sin6_flowinfo = 0; // nao sei se tem que setar, checar depois
  
  // Converte endereço IPv4 e IPv6 do formato texto para binário
	int return_code = inet_pton(AF_INET6, ip_server, &serv_addr.sin6_addr.s6_add);
  if (return_code == 0)
		DieWithUserMessage("inet_pton() failed", "invalid address string");
	else if (return_code < 0)
		DieWithSystemMessage("inet_pton() failed");

  return serv_addr;
}

void prepare_message_get (char *filename, char *message) {
  char *final_msg;
  strcpy(final_msg, "get ");
  strcat(final_msg, filename);
  strcpy(message, final_msg);
}

void prepare_message_list (char *message) {
  strcpy(message, "list");
}

int main(int argc, char *argv[]) {
	char *command = argv[1];
	char *filename;
	char *ip_server;
	in_port port_server;
	int buffer_size;

  // confirming the correct number of parameters was sent
  if (!strcmp(command, "get") && argc != 6) {
    DieWithUserMessage("Parameter(s)",
		  "get <Filename> <Server Address> <Server Port> <Buffer Size>");
  }
  if (!strcmp(command, "list") && argc != 5) {
    DieWithUserMessage("Parameter(s)",
		  "list <Server Address> <Server Port> <Buffer Size>");
  }

  // command 'get' has an extra parameter after 'get'
  int argv_offset = !strcmp(command, "get") ? 1 : 0;
  
  // reading parameters
  if (!strcmp(command, "get")) filename = argv[2];
  ip_server = argv[argv_offset + 2];
  port_server = atoi(argv[argv_offset + 3]);
  buffer_size = atoi(argv[argv_offset + 4]);
  
  // creating sockets
  int sock = create_socket();
  struct sockaddr_in6 serv_addr = setup_addr();

  // preparing message
  char *message;
  if (!strcmp(command, "get")) 
    prepare_message_get(filename, message);
  else if (!strcmp(command, "list"))
    prepare_message_list(message);
  
	// establishing connection 
	if (connect(sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
		DieWithSystemMessage("connect() failed");
	else
		printf("connect() - Estabelecendo conexão...\n");
	
  
  // sending message to server
	size_t messageLen = strlen(message); // Determine input length
	ssize_t nbytes_sent = send(sock, message, messageLen, 0);
	
	if (nbytes_sent < 0)
		DieWithSystemMessage("send() failed");
	else if (nbytes_sent != messageLen)
		DieWithUserMessage("send()", "sent unexpected number of bytes");
	else
		printf("send() - Enviando mensangem para o servidor...\n");
	






	/////////////////////////////////////
	// nao modificado a partir daqui
	////////////////////////////////////

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

