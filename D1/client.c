  #include <stdio.h>
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <string.h>
  #include <stdlib.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <stdint.h>

  // #ToDo Inserir zeros para calculo do CHECKSUM para viabilizar msg de tamanho impar

  const int8_t false = 0;
  const int8_t true = 1;

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



  void send_file(char* filename , int ssocket) {
    FILE *input_file;
    
    uint32_t total_bytesSent = 0, total_msgSent = 0;
    uint32_t SYNC = 0xDCC023C2;
    uint16_t CHKSUM = 0, LENGTH = 0, nElem = 0;
    uint8_t ID = 0, FLAGS = 0, byteArray[14], buffer[bufsize]; // 8 bits only  

    // Packet MIN SIZE is 14 bytes due to headers

    input_file = fopen(filename, "rb"); // open in binary mode
    if(input_file == NULL)
      die(8); //file doesnt exist      

    printf("Enviando arquivo %s para o cliente %u \n", filename, ssocket);

    memcpy(&byteArray[0], &SYNC, sizeof(SYNC)); // 00 - 31  (4 bytes) : SYNC
    memcpy(&byteArray[4], &SYNC, sizeof(SYNC)); // 32 - 63  (4 bytes) : SYNC
    memcpy(&byteArray[8], &CHKSUM, sizeof(CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&byteArray[12], &ID, sizeof(ID));// 96 - 103 (1 byte) : ID
    memcpy(&byteArray[13], &FLAGS, sizeof(FLAGS));// 104 - 111 (1 bytes) : FLAGS
    SYNC = htonl(SYNC);
    uint32_t sentpackets = 0;

    while(true) {          
      LENGTH = fread(&buffer, sizeof(char), bufsize - 14, input_file); //Reading files in chunks of bufsize at most (-16 frp, header)
      nElem = LENGTH;
      if (nElem <= 0 && !feof(input_file)) { //If I couldnt read anything but I havent reached end of file, there is an error
        die(9); // reading error;
      }
      //printf("PACKET = [%x][%x][%x][%x][%x][%x]\n\n", SYNC, SYNC, CHKSUM, LENGTH, ID, FLAGS);
            
      CHKSUM = cksum(byteArray, buffer, nElem);  
      CHKSUM = htons(CHKSUM);    
      LENGTH = htons(LENGTH); 


	// TODO: COlOCAR TIMEOUT DE 1 SEGUNDO PARA RETRANSMISSAO EM CASO DE NAO 
	// RECEBER ACK
      if(feof(input_file)){ //end of file, end of transmission
        printf("Fim do arquivo !\n\n");  
        fclose(input_file);
        FLAGS = 0x40;
      }

      send(ssocket, &SYNC, sizeof(SYNC), 0);
      send(ssocket, &SYNC, sizeof(SYNC), 0);
      send(ssocket, &CHKSUM, sizeof(CHKSUM), 0);
      send(ssocket, &LENGTH, sizeof(LENGTH), 0);
      send(ssocket, &ID, sizeof(ID), 0);
      send(ssocket, &FLAGS, sizeof(FLAGS), 0); 
      send(ssocket, buffer, nElem, 0);
      printf("ENVIADO = [%x][%x][%x][%x][%x][%x]\n\n", SYNC, SYNC, CHKSUM, LENGTH, ID, FLAGS);
      sentpackets++;

      //printf("+ %d bytes de dados #%d\n", nElem, sentpackets);

      printf("\tEsperando ACK %d\n", ID);

      // #######################################################################
      // ######################### ESPERA POR ACK ##############################
      // #######################################################################
      uint32_t inSYNC1, inSYNC2;
      uint16_t inCHKSUM, inLENGTH;
      uint8_t inID, inFLAGS;

      recv (ssocket, &inSYNC1, 4, 0);
      recv (ssocket, &inSYNC2, 4, 0);
      recv (ssocket, &inCHKSUM, 2, 0);
      recv (ssocket, &inLENGTH, 2, 0);
      recv (ssocket, &inID, 1, 0);
      recv (ssocket, &inFLAGS, 1, 0);

      // toDo: Checagem se pacote esta correto (CHECKSUM)

      if(inID == ID && inFLAGS == 0x80) {
        ID = !ID;
      }else {
	printf("Problema com o ACK");
      }

     if(FLAGS == 0x40) break; // end of transmission

      // #######################################################################
      // #######################################################################
      // #######################################################################

    }  

    printf("File %s sent to %i. Packets: %u\n\n", filename, ssocket, sentpackets);
  }

  int main(int argc, char** argv){
    int ssocket, port;  
    char buffer[bufsize];
    char *c_port, *hostaddr, *in_name, *out_name;
    struct sockaddr_in serverAddr;  
   
    // Spliting first argument into host addr and port 
    hostaddr = strtok (argv[1],":");
    c_port = strtok (NULL, ":");
    in_name = argv[2];
    out_name = argv[3];
    
    
    //initiate_passiveConnection(&clientSocket, &serverAddr, c_port, hostaddr);  
    initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);
    send_file(in_name, ssocket);

    close(ssocket);

    return 0;
  }
