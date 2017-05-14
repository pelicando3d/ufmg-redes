#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include "timer.c"


pthread_t tid[2];

typedef enum {false, true} bool;
const uint32_t bufsize = 1024; // min 14 because packet headers
const int timeout = 1000; // millisec

typedef struct Threads_data {
    int socket;
    char *filename;
} Threads_data;

typedef struct package {
  uint32_t SYNC, SYNC1, SYNC2;
  uint16_t LENGTH, CHKSUM;
  uint8_t  ID, FLAGS, *DATA, byte_array[14];
  bool h2ns;
} Package;

void pack(Package *p) {
    memcpy(&(p->byte_array[0]), &(p->SYNC1), sizeof(p->SYNC1)); // 00 - 31  (4 bytes) : SYNC
    memcpy(&(p->byte_array[4]), &(p->SYNC2), sizeof(p->SYNC2)); // 32 - 63  (4 bytes) : SYNC
    memcpy(&(p->byte_array[8]), &(p->CHKSUM), sizeof(p->CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&(p->byte_array[8]), &(p->LENGTH), sizeof(p->LENGTH)); // 80 - 95  (2 bytes) : LENGTH
    memcpy(&(p->byte_array[12]), &(p->ID), sizeof(p->ID));// 96 - 103 (1 byte) : ID
    memcpy(&(p->byte_array[13]), &(p->FLAGS), sizeof(p->FLAGS));// 104 - 111 (1 bytes) : FLAGS
    p-> h2ns = false;
}

void repack(Package *p) { 
    memcpy(&(p->byte_array[8]), &(p->CHKSUM), sizeof(p->CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&(p->byte_array[8]), &(p->LENGTH), sizeof(p->LENGTH)); // 80 - 95  (2 bytes) : LENGTH
    memcpy(&(p->byte_array[12]), &(p->ID), sizeof(p->ID));// 96 - 103 (1 byte) : ID
    memcpy(&(p->byte_array[13]), &(p->FLAGS), sizeof(p->FLAGS));// 104 - 111 (1 bytes) : FLAGS
}

void hton(Package *p) {
    if(!p->h2ns) {
        p->SYNC1 = htonl(p->SYNC1);
        p->SYNC2 = htonl(p->SYNC2);
        p->h2ns = true;    
    }
    p->CHKSUM = htons(p->CHKSUM);
    p->LENGTH = htons(p->LENGTH);
}

void ntoh(Package *p) {
    if(!p->h2ns) {
        p->SYNC1 = ntohl(p->SYNC1);
        p->SYNC2 = ntohl(p->SYNC2);
        p->h2ns = true;    
    }
    p->CHKSUM = ntohs(p->CHKSUM);
    p->LENGTH = ntohs(p->LENGTH);
}

void initiate_passiveConnection(int *csocket, struct sockaddr_in* server, char* cport) {  
  printf("Iniciando conexao passivamente [127.0.0.1|%s].\n", cport);
  int port = atoi(cport);
  *csocket = socket(PF_INET, SOCK_STREAM, 0);
  /*---- Configure settings of the server address struct ----*/  
  server->sin_family = AF_INET; /* Address family = Internet */  
  server->sin_port = htons(port); /* Set port number, using htons function to use proper byte order */  
  server->sin_addr.s_addr = inet_addr("127.0.0.1"); /* Set IP address to localhost */  
  memset(server->sin_zero, '\0', sizeof server->sin_zero);  /* Set all bits of the padding field to 0 */
  bind(*csocket, (struct sockaddr *) server, sizeof(*server)); /*---- Bind the address struct to the socket ----*/
  printf("Socket aberto, id = %d\n", *csocket);
}


  void die(uint32_t id) {
    printf("Im dead LoL WoWOOWOW (%u)\n", id);
  }

  //toDo: get server ip by hostname
  void initiate_activeConnection(int *ssocket, struct sockaddr_in* server, char* port, char* server_addr) {
    printf("Iniciando conexao ativamente [%s|%s].\n", server_addr, port);
    int server_port = atoi(port);
    *ssocket = socket(PF_INET, SOCK_STREAM, 0);
    server->sin_family = AF_INET;
    server->sin_port = htons(server_port); /* Set port number, using htons function to use proper byte order */  
    server->sin_addr.s_addr = inet_addr(server_addr); /* Set IP address to localhost */  
    memset(server->sin_zero, '\0', sizeof server->sin_zero); /* Set all bits of the padding field to 0 */

    socklen_t addr_size = sizeof *server;
    connect(*ssocket, (struct sockaddr *) server, addr_size);
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

int resocket, reenviarSW = 0;
Package repackage;

void stopWait_handler(int signalid){
  uint16_t reSIZE = ntohs(repackage.LENGTH);
    if(reenviarSW){
      printf("REENVIANDO = [%x][%x][%x][%x][%x][%x]\n\n", repackage.SYNC1, repackage.SYNC2, repackage.CHKSUM, repackage.LENGTH, repackage.ID, repackage. FLAGS);
      send(resocket, &repackage.SYNC1, sizeof(repackage.SYNC1), 0);
      send(resocket, &repackage.SYNC2, sizeof(repackage.SYNC2), 0);
      send(resocket, &repackage.CHKSUM, sizeof(repackage.CHKSUM), 0);
      send(resocket, &repackage.LENGTH, sizeof(repackage.LENGTH), 0);
      send(resocket, &repackage.ID, sizeof(repackage.ID), 0);
      send(resocket, &repackage.FLAGS, sizeof(repackage.FLAGS), 0); 
      send(resocket, repackage.DATA, reSIZE, 0);      
      mysettimer(timeout);
    }
}


  uint16_t
  cksum(uint8_t *headers, uint8_t *data, uint16_t len) {
    register u_long sum = 0;
    u_short count = 14;
    while (count--) {
      sum += *headers++;
      if (sum & 0xFFFF0000) {
        /* carry occurred,
        so wrap around */
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


send_package(Package *p, int socket) {
    hton(p);
    send(socket, &(p->SYNC1),  sizeof(p->SYNC1),  0);
    send(socket, &(p->SYNC2),  sizeof(p->SYNC2),  0);
    send(socket, &(p->CHKSUM), sizeof(p->CHKSUM), 0);
    send(socket, &(p->LENGTH), sizeof(p->LENGTH), 0);
    send(socket, &(p->ID),     sizeof(p->ID),     0);
    send(socket, &(p->FLAGS),  sizeof(p->FLAGS),  0);
    printf("\t\t\t[socket] - Pacote enviado: [%x][%x][%x][%x][%x][%x]\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->ID, p->FLAGS);        
}

recv_package(Package *p, int socket) {
    p->h2ns = false;
    recv(socket, &(p->SYNC1),  4, 0);
    recv(socket, &(p->SYNC2),  4, 0);
    recv(socket, &(p->CHKSUM), 2, 0);
    recv(socket, &(p->LENGTH), 2, 0);
    recv(socket, &(p->ID),     1, 0);
    recv(socket, &(p->FLAGS),  1, 0);    
    printf("[socket] - Pacote recebido: [%x][%x][%x][%x][%x][%x]\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->ID, p->FLAGS);    
    ntoh(p);    
}


void send_file(char* filename , int ssocket) {
    FILE *input_file;
    uint32_t total_bytesSent = 0, total_msgSent = 0;
    uint32_t sentpackets = 0;
    
    Package data, ack;    

    data.SYNC1 = 0xDCC023C2;
    data.SYNC2 = 0xDCC023C2;
    data.CHKSUM = 0;
    data.LENGTH = 0;
    data.ID = 0;
    data.FLAGS = 0;
    uint16_t nElem = 0;
    uint8_t buffer[bufsize]; // 8 bits only  

    // Packet MIN SIZE is 14 bytes due to headers
    input_file = fopen(filename, "rb"); // open in binary mode
    if(input_file == NULL)
        die(8); //file doesnt exist      

    printf("Enviando arquivo %s para o cliente %u \n", filename, ssocket);

    signal(SIGALRM, stopWait_handler);    
    pack(&data);
    
    while(true) {          
        data.LENGTH = fread(&buffer, sizeof(char), bufsize - 14, input_file); //Reading files in chunks of bufsize at most (-16 frp, header)
        nElem = data.LENGTH;
        if (nElem <= 0 && !feof(input_file)) { //If I couldnt read anything but I havent reached end of file, there is an error
            die(9); // reading error;
        }
        //printf("PACKET = [%x][%x][%x][%x][%x][%x]\n\n", SYNC, SYNC, CHKSUM, LENGTH, ID, FLAGS);
            
        data.CHKSUM = cksum(data.byte_array, data.DATA, nElem);
        data.DATA = buffer;
        repack(&data);


        if(feof(input_file)){ //end of file, end of transmission
            printf("Fim do arquivo !\n\n");  
            fclose(input_file);
            data.FLAGS = 0x40;
        }


        hton(&data);


        repackage.SYNC1 = data.SYNC1;
        repackage.SYNC2 = data.SYNC2;
        repackage.CHKSUM = data.CHKSUM; 
        repackage.LENGTH = data.LENGTH; 
        repackage.ID = data.ID; 
        repackage.FLAGS = data.FLAGS; 
        repackage.DATA = data.DATA;
        resocket = ssocket;

        send(ssocket, &(data.SYNC1), sizeof(data.SYNC1), 0);
        send(ssocket, &(data.SYNC2), sizeof(data.SYNC2), 0);
        send(ssocket, &(data.CHKSUM), sizeof(data.CHKSUM), 0);
        send(ssocket, &(data.LENGTH), sizeof(data.LENGTH), 0);
        send(ssocket, &(data.ID), sizeof(data.ID), 0);
        send(ssocket, &(data.FLAGS), sizeof(data.FLAGS), 0); 
        send(ssocket, buffer, nElem, 0);
        printf("ENVIADO = [%x][%x][%x][%x][%x][%x]\n\n", data.SYNC1, data.SYNC2, data.CHKSUM, data.LENGTH, data.ID, data.FLAGS);
        sentpackets++;

      //printf("+ %d bytes de dados #%d\n", nElem, sentpackets);

        printf("\tEsperando ACK %d\n", data.ID);
        mysettimer(timeout); // 1 segundo
        reenviarSW = 1;

      // #######################################################################
      // ######################### ESPERA POR ACK ##############################
      // #######################################################################

        recv (ssocket, &(ack.SYNC1), 4, 0);
        recv (ssocket, &(ack.SYNC2), 4, 0);
        recv (ssocket, &(ack.CHKSUM), 2, 0);
        recv (ssocket, &(ack.LENGTH), 2, 0);
        recv (ssocket, &(ack.ID), 1, 0);
        recv (ssocket, &(ack.FLAGS), 1, 0);

      // toDo: Checagem se pacote esta correto (CHECKSUM)

        if(ack.ID == data.ID && ack.FLAGS == 0x80) {
            data.ID = !data.ID;
            reenviarSW = 0;
        }else {
           printf("Problema com o ACK - reenviar (ackID = %x, ackFLAG = %x)\n", ack.ID, ack.FLAGS);
            //reenviarSW = 1;
        }

        if(data.FLAGS == 0x40) break; // end of transmission

      // #######################################################################
      // #######################################################################
      // #######################################################################
    }  
    printf("File %s sent to %i. Packets: %u\n\n", filename, ssocket, sentpackets);
  }


void* send_file_thread(void *requirements) {
    printf("############ THREAD SENDING FILE SECOND ################\n");
    struct Threads_data *data = (struct Threads_data*) requirements;
    send_file(data->filename, data->socket);
    printf("############ THREAD FILE SENT ################\n");    
}


void receive_file(char* filename, int client) {
    uint32_t total_bytesSent = 0, total_msgSent = 0, recvpackets = 0;
    //uint32_t inSYNC1, inSYNC2;
    //uint16_t inCHKSUM, inLENGTH, CHKSUM, lastCHKSUM;
    uint16_t lastCHKSUM;
    Package data, ack;
    //uint8_t inID, inFLAGS, inDATA[bufsize], lastID = 1;
    uint8_t lastID = 1;
    uint8_t buffer[bufsize + 1];
    ssize_t nElem = 0, nbytes;
    FILE *out_file;


    ack.SYNC1 = 0xDCC023C2;
    ack.SYNC2 = 0xDCC023C2;
    ack.CHKSUM = 0;
    ack.LENGTH = 0;
    ack.FLAGS = 0x80;               
    

    out_file = fopen(filename, "wb+"); // open in binary mode    
    if(out_file == NULL)
        die(8); //file doesnt exist     
    memset (buffer, '\0', bufsize);
    
    while(true) {
        //Receiveing Package header [SYNC|SYNC|CHKSUM|LENGTH|ID|FLAGS]
        recv_package(&data, client);
        
        if(data.SYNC1 != data.SYNC2 || data.SYNC1 != 0xDCC023C2) {
            printf("\t\tPACOTE CAGADO ABANDONADO\n");
            continue; // recebi um pacote cagado, mal identificado - Abandonar
        }

        //Receiving Data with length determined by data.LENGTH
        data.DATA = malloc(data.LENGTH * sizeof(uint8_t));
        nbytes = recv (client, data.DATA, data.LENGTH, 0);
        //printf(" + %d bytes de dados #%d\n", nbytes, recvpackets);

        //Calculating 'revert' Checksum
        memcpy(&data.byte_array[0],  &data.SYNC1,  sizeof(data.SYNC1)); // 00 - 31    (4 bytes) : SYNC
        memcpy(&data.byte_array[4],  &data.SYNC2,  sizeof(data.SYNC2)); // 32 - 63    (4 bytes) : SYNC
        memcpy(&data.byte_array[8],  &data.CHKSUM, sizeof(data.CHKSUM)); // 64 - 79    (2 bytes) : CHKSUM
        memcpy(&data.byte_array[10], &data.LENGTH, sizeof(data.LENGTH));// 80 - 95 (2 bytes) : LENGTH
        memcpy(&data.byte_array[12], &data.ID,     sizeof(data.ID));// 96 - 103 (1 byte) : ID
        memcpy(&data.byte_array[13], &data.FLAGS,  sizeof(data.FLAGS));// 104 - 111 (1 bytes) : FLAGS

        
        // #################### Enviando ACK ############################
        // ##############################################################
        // TODO CHECAGEM CORRETA DO CHKSUM

        /*int livre;
                printf("Esperando entrada: ");
        scanf("%d", &livre);
        printf("Aceitou: %d \n", livre);
        */
        if(lastID != data.ID /*&& valid checksum*/ ) {
            ack.ID = data.ID;

            //ToDo: check for errors after sending data
            printf("\t\t - ## Pacote Valido, escrevendo dados em disco e mandando ACK\n");

            send_package(&ack, client);
            lastID = ack.ID;
            lastCHKSUM = data.CHKSUM;
            recvpackets++;

            
            //Writing data received in the output file
            fwrite(data.DATA, 1, nbytes, out_file); 
         
        } else if (lastCHKSUM == data.CHKSUM && lastID == data.ID) { // recebi msm pacote, mandar so ack
            printf("\t\t - ## Pacote Repetido - enviando mesmo ACK\n");
            send_package(&ack, client);            
        }

        printf("\n");

        if(data.FLAGS == 0x40) break; // Checking if FLAG == END --> END OF TRANSMISSION

        // ##############################################################
        // ##############################################################
        
    }
    fclose(out_file); // Closing file
    printf("File %s received from %i. Packets: %u\n\n", filename, client, recvpackets);
}


void *receive_file_thread(void *requirements) {
    printf("############ THREAD RECEIVING FILE SECOND ################\n");
    Threads_data *data = (Threads_data*) requirements;
    receive_file(data->filename, data->socket);
    printf("############ THREAD FILE RECEIVED ################\n");
    
}