#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

const uint32_t bufsize = 1024;

void die(uint32_t id) {
  printf("Im dead LoL WoWOOWOW (%u)\n", id);
}

//toDo: get server ip by hostname
void initiate_activeConnection(int *ssocket, struct sockaddr_in* server, char* port, char* server_addr) {
  int server_port = atoi(port);
  *ssocket = socket(AF_INET, SOCK_STREAM, 0);
  server->sin_family = AF_INET;
  server->sin_port = htons(server_port); /* Set port number, using htons function to use proper byte order */  
  server->sin_addr.s_addr = inet_addr(server_addr); /* Set IP address to localhost */  
  memset(server->sin_zero, '\0', sizeof server->sin_zero); /* Set all bits of the padding field to 0 */

  socklen_t addr_size = sizeof server;
  connect(*ssocket, (struct sockaddr *) &server, addr_size);
}


void send_file(char* filename , int ssocket) {
  FILE *input_file;
  char buffer[bufsize]; 
  uint32_t total_bytesSent = 0, total_msgSent = 0;    
  ssize_t nElem = 0;

  input_file = fopen(filename, "rb"); // open in binary mode
  
  if(input_file == NULL)
    die(8); //file doesnt exist      

  printf("Enviando arquivo %s para o cliente %u \n", filename, ssocket);

  while(1) {    
    nElem = fread(&buffer, sizeof(char), bufsize, input_file); //Reading files in chunks of bufsize at most
    
    if (nElem <= 0 && !feof(input_file)) { //If I couldnt read anything but I havent reached end of file, there is an error
      die(9); // reading error;
    }

    //toDo: Construct Packet Here, before sending    
    ssize_t num_bytesSent = send(ssocket, buffer, nElem, 0);
    
    if (num_bytesSent < 0){ //If i couldnt send anything, there is an error -- Is it RIGHT?
      die(7); //error send
    }else{ //Otherwise, we increment our counter of total sent bytes and total sent packages      
      total_bytesSent += num_bytesSent;
      total_msgSent += 1;
      //cout << "send() - Bytes ja enviados: " << totalBytesSent << endl;
    }

    if(feof(input_file)){ //end of file, end of transmission
      printf("Fim do arquivo !\n\n");      
      break;
    }
  }
  printf("File %s sent to %i. Bytes: %u, Packets: %u\n\n", filename, total_bytesSent, total_msgSent);
}

int main(int argc, char** argv){
  int ssocket, port;  
  char buffer[bufsize];
  char *c_port, *hostaddr, *in_name, *out_name;
  struct sockaddr_in serverAddr;  
  //FILE *input_file, *output_file;
 
  // Spliting first argument into host addr and port 
  hostaddr = strtok (argv[1],":");
  c_port = strtok (NULL, ":");
  in_name = argv[2];
  out_name = argv[3];
  
  
  //initiate_passiveConnection(&clientSocket, &serverAddr, c_port, hostaddr);  
  initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);    
  send_file(in_name, ssocket);


  /*---- Connect the socket to the server using the address struct ----*/
  

  /*---- Read the message from the server into the buffer ----*/
  //recv(clientSocket, buffer, 1024, 0);

  /*---- Print the received message ----*/
  printf("Data received: %s",buffer);   

  return 0;
}
