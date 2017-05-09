#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

const uint32_t bufsize = 1024;

void die(uint32_t id) {
	printf("Im dead LoL WoWOOWOW (%u)\n", id);
}

//toDo: get server ip by hostname
void initiate_connection(int *, struct sockaddr_in* server, char* port, char* server_addr) {
  int server_port = atoi(port);
  csocket = socket(AF_INET, SOCK_STREAM, 0);
  server->sin_family = AF_INET;
  /* Set port number, using htons function to use proper byte order */
  server->sin_port = htons(server_port);
  /* Set IP address to localhost */
  server->sin_addr.s_addr = inet_addr(server_addr); 
  /* Set all bits of the padding field to 0 */
  memset(server->sin_zero, '\0', sizeof server->sin_zero);

  socklen_t addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
}


void send_file(char* filename , int hostid, ...) {
	FILE *input_file;
	char buffer[bufsize];	
    uint32_t total_bytesSent = 0, total_msgSent = 0;    
    ssize_t nElem = 0;

    input_file = fopen(filename, "rb"); // open in binary mode
    
    if(input_file == NULL)
      die(8); //file doesnt exist      

    printf("Enviando arquivo %s para o cliente %u \n" << filename, hostid)

    while(1) {
      nElem = fread(&buffer, sizeof(char), bufsize, input_file); 
      if (nElem <= 0 && !feof(file)) { 
        die(9); // reading error;
      }

      ssize_t num_bytesSent = send(hostid, buffer, nElem, 0);
      if (num_bytesSent < 0){
        die(7); //error send
      }else{
        total_bytesSent += num_bytesSent;
        total_msgSent += 1;
        //cout << "send() - Bytes ja enviados: " << totalBytesSent << endl;
      }

      if(feof(file)){
      	printf("Fim do arquivo !\n\n");
        /*cout << "End of file\n";        
        cout << totalMsgSent << endl;*/
        break;
      }
    }
}

int main(int argc, char** argv){
  int clientSocket, port;  
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
  initiate_activeConnection(clientSocket, &serverAddr, c_port, hostaddr);    
  send_file();


  /*---- Connect the socket to the server using the address struct ----*/
  

  /*---- Read the message from the server into the buffer ----*/
  //recv(clientSocket, buffer, 1024, 0);

  /*---- Print the received message ----*/
  printf("Data received: %s",buffer);   

  return 0;
}
