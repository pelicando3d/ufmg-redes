#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>

const int8_t false = 0;
const int8_t true = 1;

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

unsigned carry_around_add(unsigned a, unsigned b) {
  unsigned c = a + b;
  return (c &0xffff)+(c >>16);
}

unsigned checksum(uint8_t *headers, uint8_t *data, uint16_t len){
  unsigned s = 0;
  unsigned i = 0;
  for (; i < 14; i+=2) { // 14 bytes header
    unsigned w = headers[i] + (headers[i+1])<<8;
    s = carry_around_add(s, w);
  }

  for (i = 0; i < len; i+=2) {
    unsigned w = data[i] + (data[i+1])<<8;
    s = carry_around_add(s, w);
  }
  return~s &0xffff;
}

uint16_t
cksum(uint8_t *headers, uint8_t *data, uint16_t len) {
  register u_long sum = 0;
  u_short count = 14;
  while (count--) {
    sum += *headers++;
    if (sum & 0xFFFF0000) {
      sum &= 0xFFFF;
      sum++;  
    }
  }

  count = len;
  while (count--) {
    sum += *data++;
    if (sum & 0xFFFF0000) {
      sum &= 0xFFFF;
      sum++;  
    }
  }

  return ~(sum & 0xFFFF);
}


void receive_file(char* filename, int client) {
  uint32_t total_bytesSent = 0, total_msgSent = 0, recvpackets = 0;
  uint32_t inSYNC1, inSYNC2;
  uint16_t inCHKSUM, inLENGTH, CHKSUM, lastCHKSUM;
  uint8_t inID, inFLAGS, inDATA[bufsize], lastID = 1;
  uint8_t buffer[bufsize + 1], *packets, byteArray[14];
  ssize_t nElem = 0, nbytes;
  FILE *out_file;

  out_file = fopen(filename, "wb+"); // open in binary mode  
  if(out_file == NULL)
    die(8); //file doesnt exist   
  memset (buffer, '\0', bufsize);
  
  while(true) {
    //Receiveing Package header [SYNC|SYNC|CHKSUM|LENGTH|ID|FLAGS]
    nbytes = recv (client, &inSYNC1, 4, 0);
    nbytes = recv (client, &inSYNC2, 4, 0);
    nbytes = recv (client, &inCHKSUM, 2, 0);
    nbytes = recv (client, &inLENGTH, 2, 0);
    nbytes = recv (client, &inID, 1, 0);
    nbytes = recv (client, &inFLAGS, 1, 0);

    // printf("Received: [%x][%x][%x][%x][%x][%x]\n", inSYNC1, inSYNC2, inCHKSUM, inLENGTH, inID, inFLAGS);

    // Converting byte order
    inSYNC1 = ntohl(inSYNC1);
    inSYNC2 = ntohl(inSYNC2);
    inCHKSUM = ntohs(inCHKSUM);
    inLENGTH = ntohs(inLENGTH);

    if(inSYNC1 != inSYNC2 && inSYNC1 != 0xDCC023C2) continue; // recebi um pacote cagado, mal identificado - Abandonar

    //Receiving Data with length determined by inLENGTH
    nbytes = recv (client, inDATA, inLENGTH, 0);
    //printf(" + %d bytes de dados #%d\n", nbytes, recvpackets);

    //Calculating 'revert' Checksum
    memcpy(&byteArray[0], &inSYNC1, sizeof(inSYNC1)); // 00 - 31  (4 bytes) : SYNC
    memcpy(&byteArray[4], &inSYNC2, sizeof(inSYNC2)); // 32 - 63  (4 bytes) : SYNC
    memcpy(&byteArray[8], &inCHKSUM, sizeof(inCHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&byteArray[10], &inLENGTH, sizeof(inLENGTH));// 80 - 95 (2 bytes) : LENGTH
    memcpy(&byteArray[12], &inID, sizeof(inID));// 96 - 103 (1 byte) : ID
    memcpy(&byteArray[13], &inFLAGS, sizeof(inFLAGS));// 104 - 111 (1 bytes) : FLAGS

    printf("RECEBIDO = [%x][%x][%x][%x][%x][%x]\n\n", inSYNC1, inSYNC2, inCHKSUM, inLENGTH, inID, inFLAGS);
    
    // #################### Enviando ACK ############################
    // ##############################################################
    // TODO CHECAGEM CORRETA DO CHKSUM

    if(lastID != inID /*&& valid checksum*/ ) {
      uint32_t SYNC = 0xDCC023C2;
      uint16_t CHKSUM = 0, LENGTH = 0;
      uint8_t  ID, FLAGS = 0x80;

      ID = inID; 
      lastID = ID;
      lastCHKSUM = inCHKSUM;

      send(client, &SYNC, sizeof(SYNC), 0);
      send(client, &SYNC, sizeof(SYNC), 0);
      send(client, &CHKSUM , sizeof(CHKSUM), 0);
      send(client, &LENGTH, sizeof(LENGTH), 0);
      send(client, &ID, sizeof(ID), 0);
      send(client, &FLAGS, sizeof(FLAGS), 0);

      //Writing data received in the output file
      fwrite(inDATA, 1, nbytes, out_file); 
     
    } else if (lastCHKSUM == CHKSUM && lastID == inID) { // recebi msm pacote, mandar so ack
      uint32_t SYNC = 0xDCC023C2;
      uint16_t CHKSUM = 0, LENGTH = 0;
      uint8_t  ID = inID, FLAGS = 0x80;

      send(client, &SYNC, sizeof(SYNC), 0);
      send(client, &SYNC, sizeof(SYNC), 0);
      send(client, &CHKSUM , sizeof(CHKSUM), 0);
      send(client, &LENGTH, sizeof(LENGTH), 0);
      send(client, &ID, sizeof(ID), 0);
      send(client, &FLAGS, sizeof(FLAGS), 0);

    }

    if(inFLAGS == 0x40) break; // Checking if FLAG == END --> END OF TRANSMISSION

    // ##############################################################
    // ##############################################################
    recvpackets++;
    
  }
  fclose(out_file); // Closing file
  printf("File %s received from %i. Packets: %u\n\n", filename, client, recvpackets);
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

  
  return 0;
}
