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

//////////////////////////////////////////////////
// SEÇÃO REFERENTE A COMUNICACAO COM O SERVIDOR //
//////////////////////////////////////////////////

typedef struct srv_comm {
  int sock;
  struct sockaddr_in6 serv_addr;
} srv_comm;

// aloca a estrutura e cria o socket
srv_comm* create_servcomm () {
  srv_comm *scomm = (srv_comm*) malloc(sizeof(srv_comm));
  
  // Create a reliable, stream socket using TCP
	int sock = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
	
	if (sock < 0){
		DieWithSystemMessage("socket() failed");
	}else{
		printf("Sock() - Criando o socket!\n");
	}

  scomm->sock = sock;
	return scomm;
}

// fecha scoket criado
void close_servcomm(srv_comm *scomm) {
  close(scomm->sock);
}

// seta valores da estrutura com o endereco de destino
void setup_addr(srv_comm *scomm, char *ip_server, in_port_t port_server) {
  
  // seta valores passados por parametro
  scomm->serv_addr.sin6_family = AF_INET6;
  scomm->serv_addr.sin6_port = htons(port_server); // Server port
  scomm->serv_addr.sin6_flowinfo = 0; // nao sei se tem que setar, checar depois
  
  // converte endereço IPv4 e IPv6 do formato texto para binário
	int return_code = inet_pton(AF_INET6, ip_server, &scomm->serv_addr.sin6_addr.s6_addr);
  if (return_code == 0) {
    printf("%s\n", ip_server);
		DieWithUserMessage("inet_pton() failed", "invalid address string");
  }	else if (return_code < 0)
		DieWithSystemMessage("inet_pton() failed");
}

// conecta com o servidor e envia a mensagem especificada via parametro
void send_message(srv_comm *scomm, char *message) {
  // estabelecendo conexao
	if (connect(scomm->sock, (struct sockaddr*) &scomm->serv_addr, sizeof(struct sockaddr_in6)) < 0)
		DieWithSystemMessage("connect() failed");
	else
		printf("connect() - Estabelecendo conexão...\n");
	
  
  // enviando mensagem para o servidor 
	size_t messageLen = strlen(message); // Determine input length
	ssize_t nbytes_sent = send(scomm->sock, message, messageLen, 0);
	
	if (nbytes_sent < 0)
		DieWithSystemMessage("send() failed");
	else if (nbytes_sent != messageLen)
		DieWithUserMessage("send()", "sent unexpected number of bytes");
	else
		printf("send() - Enviando comando %s para o servidor...\n", message);
}

// escuta resposta do servidor
char* get_response(srv_comm *scomm) {
	unsigned int totalBytesRcvd = 0; // Count of total bytes received
	int numBytes = 0;
	char *response = (char*) malloc(BUFSIZE * sizeof(char));
	
	fputs("Received: ", stdout); // Setup to print the echoed string

	while (1) {
		char buffer[BUFSIZE]; // I/O buffer
		/* Receive up to the buffer size (minus 1 to leave space for
		a null terminator) bytes from the sender */
	
		numBytes = recv(scomm->sock, buffer, BUFSIZE - 1, 0);
		if (numBytes <= 0) break;

		if (totalBytesRcvd) response = (char*) realloc(response, strlen(response) + BUFSIZE);
		strcat(response, buffer);
		
				
		totalBytesRcvd += numBytes; // Keep tally of total bytes
		//buffer[numBytes] = '\0'; // Terminate the string!
		//fputs(buffer, stdout); // Print the echo buffer
		//printf("%s\n", buffer);
	}

	fputc('\n', stdout); // Print a final linefeed
	
	return response;
}

//////////////////////////////////
// SEÇÃO REFERENTE AOS COMANDOS //
//////////////////////////////////

char* prepare_message_get (char *filename) {
  int msgLen = 5 + strlen(filename);
  char* message = (char*) malloc(msgLen * sizeof(char));
  strcpy(message, "get ");
  strcat(message, filename);
  return message;
}

char* prepare_message_list () {
  char* message = (char*) malloc(5 * sizeof(char));
  strcpy(message, "list");
  return message;
}

void read_response_get(char *raw_response) {
  printf("%s\n", raw_response);
}

void read_response_list(char *raw_response) {
  int i = 0;
  printf("\nLista de arquivos disponiveis para baixar: \n");
  while(1){  	
  	if(raw_response[i] == '\\' && raw_response[i+1] == '0') break;
  	if(raw_response[i] == '\\' && raw_response[i+1] == 'n'){
  		printf("\n"); 
  		i++;
  	}
  	else
  		printf("%c", raw_response[i]);  	
  	i++;
  }  
}


/////////////////////////////////////


int main(int argc, char *argv[]) {
	if (argc == 1) 
	  DieWithUserMessage("Commands available: ",
	        "get, list");
	
	char *command = argv[1];
	char *filename;
	char *ip_server;
	in_port_t port_server;
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
  srv_comm *scomm = create_servcomm();
  setup_addr(scomm, ip_server, port_server);

  // preparing message
  char *message;
  if (!strcmp(command, "get")) 
    message = prepare_message_get(filename);
  else if (!strcmp(command, "list"))
    message = prepare_message_list();
  
	// send message
	send_message(scomm, message);

  // receive response
  char *raw_response = get_response(scomm);

  // interpret response
  if (!strcmp(command, "get")) 
    read_response_get(raw_response);
  else if (!strcmp(command, "list"))
    read_response_list(raw_response);

  // close communications
  close_servcomm(scomm);
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

