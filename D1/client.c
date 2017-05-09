#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>


const uint32_t psize = 1024;
const uint32_t bufsize = 1024; // min 14 because packet headers


void die(uint32_t id) {
  printf("Im dead LoL WoWOOWOW (%u)\n", id);
}

//toDo: get server ip by hostname
void initiate_activeConnection(int *ssocket, struct sockaddr_in* server, char* port, char* server_addr) {
  printf("Iniciando conexao ativamente [%s|%s].\n", server_addr, port);
  int server_port = atoi(port);
  *ssocket = socket(PF_INET, SOCK_STREAM, 0);
  server->sin_family = AF_INET;
  server->sin_port = htons(7891); /* Set port number, using htons function to use proper byte order */  
  server->sin_addr.s_addr = inet_addr("127.0.0.1"); /* Set IP address to localhost */  
  memset(server->sin_zero, '\0', sizeof server->sin_zero); /* Set all bits of the padding field to 0 */

  socklen_t addr_size = sizeof *server;
  connect(*ssocket, (struct sockaddr *) server, addr_size);
}


void send_file(char* filename , int ssocket) {
  FILE *input_file;
  char buffer[bufsize]; 
  uint32_t total_bytesSent = 0, total_msgSent = 0;    
  ssize_t nElem = 0;
  uint32_t SYNC = 0xDCC023C2;
  uint16_t CHKSUM = 0;
  uint16_t LENGTH = 0xAAB;
  uint8_t ID = 1;
  uint8_t FLAGS = 0; // 8 bits only
  uint8_t byteArray[bufsize];
  uint32_t DATA;

  // Packet MIN SIZE is 14 bytes due to headers

  input_file = fopen(filename, "rb"); // open in binary mode
  if(input_file == NULL)
    die(8); //file doesnt exist      

  printf("Enviando arquivo %s para o cliente %u \n", filename, ssocket);

  while(1) {    
    LENGTH = fread(&buffer, sizeof(char), bufsize - 16, input_file); //Reading files in chunks of bufsize at most (-16 frp, header)
    if (nElem <= 0 && !feof(input_file)) { //If I couldnt read anything but I havent reached end of file, there is an error
      die(9); // reading error;
    }
    printf("READ %d ELEMENTS\n",LENGTH);
    //buffer[LENGTH] = '\0';

    // Packet Headers
    SYNC = htonl(SYNC);
    CHKSUM = htonl(CHKSUM);
    LENGTH = htons(LENGTH);
    //DATA = htonl(buffer);

    printf("PACKET = [%x][%x][%x][%x][%x][%x]\n\n", SYNC, SYNC, CHKSUM, LENGTH, ID, FLAGS);

    memcpy(&byteArray[0], &SYNC, sizeof(SYNC)); // 00 - 31  (4 bytes) : SYNC
    memcpy(&byteArray[4], &SYNC, sizeof(SYNC)); // 32 - 63  (4 bytes) : SYNC
    memcpy(&byteArray[8], &CHKSUM, sizeof(CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&byteArray[10], &LENGTH, sizeof(LENGTH));// 80 - 95 (2 bytes) : LENGTH
    memcpy(&byteArray[12], &ID, sizeof(ID));// 96 - 103 (1 byte) : ID
    memcpy(&byteArray[13], &FLAGS, sizeof(FLAGS));// 104 - 111 (1 bytes) : FLAGS

    //memcpy(&byteArray[14], &DATA, sizeof(DATA));// 112 - limit (X bytes) : DATA

    
    int i = 0;
    for(; i < 50; i++) {
      printf("byte %d (%u) (%x)\n",i, byteArray[i], byteArray[i]);
    }

    return ;
    
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
    fclose(input_file);
  }
  printf("File %s sent to %i. Bytes: %u, Packets: %u\n\n", filename, ssocket, total_bytesSent, total_msgSent);
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
  //initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);    
  send_file(in_name, ssocket);
  close(ssocket);


  /*---- Connect the socket to the server using the address struct ----*/
  

  /*---- Read the message from the server into the buffer ----*/
  //recv(clientSocket, buffer, 1024, 0);

  /*---- Print the received message ----*/
  printf("Data received: %s",buffer);   

  return 0;
}
