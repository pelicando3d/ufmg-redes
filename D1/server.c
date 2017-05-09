#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>


const uint32_t bufsize = 1024;

void die(uint32_t id) {
  printf("Im dead LoL WoWOOWOW (%u)\n", id);
}

void initiate_passiveConnection(int *csocket, struct sockaddr_in* server, char* cport) {  
  printf("Iniciando conexao passivamente [127.0.0.1|%s].\n", cport);
  int port = atoi(cport);
  *csocket = socket(PF_INET, SOCK_STREAM, 0);
  /*---- Configure settings of the server address struct ----*/  
  server->sin_family = AF_INET; /* Address family = Internet */  
  server->sin_port = htons(7891); /* Set port number, using htons function to use proper byte order */  
  server->sin_addr.s_addr = inet_addr("127.0.0.1"); /* Set IP address to localhost */  
  memset(server->sin_zero, '\0', sizeof server->sin_zero);  /* Set all bits of the padding field to 0 */
  bind(*csocket, (struct sockaddr *) server, sizeof(*server)); /*---- Bind the address struct to the socket ----*/
  printf("Socket aberto, id = %d\n", *csocket);
}

void receive_file(char* filename, int client) {
  char buffer[bufsize + 1], *packets;
  FILE *out_file;
  uint32_t total_bytesSent = 0, total_msgSent = 0;    
  ssize_t nElem = 0;
  int nbytes;
  //  char *response = (char*) malloc(buffer_size);
  // response[0] = '\0';

  out_file = fopen(filename, "wb+"); // open in binary mode
  
  if(out_file == NULL)
    die(8); //file doesnt exist   

  memset (buffer, '\0', bufsize);

  while( (nbytes = recv (client, buffer, bufsize, 0)) ){      
    if (nbytes < 0){
      die(5); // Error accept
    }
    buffer[nbytes] = '\0';
    printf("Recebeu: %s\n", buffer);
    fwrite(&buffer, 1, nbytes, out_file); // writes file

    /* Montar Pacotes 
    if (totalBytesRcvd)
      response = (char*) realloc(response, strlen(response) + strlen(buffer) + 1);
    strcat(response, buffer);
    */
  }
  fclose(out_file);
}

int main(int argc, char **argv){
  int insocket, newSocket;
  char buffer[bufsize];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  char* port, *in_name, *out_name;

  
  port = argv[1];
  in_name = argv[2];
  out_name = argv[3];
  

  initiate_passiveConnection(&insocket, &serverAddr, port);

  /*---- Listen on the socket, with 5 max connection requests queued ----*/
  if(listen(insocket,5)==0)
    printf("Listening\n");
  else
    printf("Error\n");

  /*---- Accept call creates a new socket for the incoming connection ----*/
  addr_size = sizeof serverStorage;
  newSocket = accept(insocket, (struct sockaddr *) &serverStorage, &addr_size);

  printf("Connection Accepted\n");

  receive_file(out_name, newSocket);

  /*---- Send message to the socket of the incoming connection ----*/
  strcpy(buffer,"Hello World\n");
  send(newSocket,buffer,13,0);

  return 0;
}
