#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

const uint32_t bufsize = 1024;

void initiate_passiveConnection(int *socket, struct sockaddr_in* server, char* port) {  
  socket = socket(AF_INET, SOCK_STREAM, 0);
  /*---- Configure settings of the server address struct ----*/  
  server->sin_family = AF_INET; /* Address family = Internet */  
  server->sin_port = htons(port); /* Set port number, using htons function to use proper byte order */  
  server->sin_addr.s_addr = inet_addr("127.0.0.1"); /* Set IP address to localhost */  
  memset(server->sin_zero, '\0', sizeof server->sin_zero);  /* Set all bits of the padding field to 0 */
  bind(socket, (struct sockaddr *) &server, sizeof(server)); /*---- Bind the address struct to the socket ----*/
}

void receive_file() {
  char buffer[bufferSize + 1];
  //memset (buffer, '\0', bufferSize);
  int nbytes;
  bool signal = false;
  std::string msg, end("\\0");

  while( (nbytes = recv (client_fd, buffer, bufferSize, 0)) ){      
    //cout << "recebido: " << buffer << "," << nbytes << endl;
    if (nbytes < 0){
      die(5); // Error accept
      //return std::string();
    }
    buffer[nbytes] = '\0';
    msg += buffer;
    if(signal and buffer[nbytes-1] == '0') 
      return msg;
    if (nbytes > 1 and buffer[nbytes-2] == '\\' and buffer[nbytes-1] == '0')
      return msg;      
    if(buffer[nbytes-1] == '\\')
      signal = true;
  }
  return std::string();
}

int main(int argc, char **argv){
  int insocket, newSocket;
  char buffer[bufsize];
  struct sockaddr_in serverAddr;
  struct sockaddr_storage serverStorage;
  socklen_t addr_size;
  char* port, in_name, out_name;

  
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

  receive_file();

  /*---- Send message to the socket of the incoming connection ----*/
  strcpy(buffer,"Hello World\n");
  send(newSocket,buffer,13,0);

  return 0;
}
