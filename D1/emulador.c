#include "emulador.h"

int main(int argc, char** argv){
    int ssocket, insocket, newSocket;  
    char *c_port, *hostaddr, *in_name, *out_name;
    struct sockaddr_in serverAddr;  
    socklen_t addr_size;
    struct sockaddr_storage serverStorage;  

        /* checando se numero de argumentos esta dentro do aceitavel 5 + 1 opcional de debug */
    if(argc < 5 || argc > 6) error_msg(arguments);

    /* ativando saidas de debug */
    if(argc == 6 && ! strcmp(argv[5], "-debug") )
        debug = true;


    printf("\n\n############################################################################# \
            \nPara a transferencia de arquivos muito grandes, o programa pode demorar \
            \npara finalizar as transferencias. Caso queira se certificar que o programa \
            \nnao travou, use a flag -debug para verificar os passos em execucao. \
            \n#############################################################################\n\n");


    /* inicializando o emulador com o lado passivo da conexao */
    if(argv[1][0] == '-' && argv[1][1] == 's') {
        c_port = argv[2];
        in_name = argv[3];
        out_name = argv[4];

        /* abrindo socket, no IP = ip do hostname */
        initiate_passiveConnection(&insocket, &serverAddr, c_port);
        if(listen(insocket,5)==0)
            printf("#### Esperando conexoes ####\n");
        else
           error_msg(listening);

        /* aceitando conexao em um novo socket*/
        addr_size = sizeof serverStorage;
        newSocket = accept(insocket, (struct sockaddr *) &serverStorage, &addr_size);

        printf("### Conexao Aceita ###\n");

        /* transferencia de arquivos */
        receive_send_file(in_name, out_name, newSocket);

        /* fechando sockets abertos */
        close(insocket);    
        close(newSocket);

    /* inicializando o emulador com o lado ativo da conexao */
    } else if (argv[1][0] == '-' && argv[1][1] == 'c') {
        // Dividindo argumento 1 em porta e endereco ip
        hostaddr = strtok (argv[2],":");
        c_port = strtok (NULL, ":");
        in_name = argv[3];
        out_name = argv[4];

        initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);

        /* transferencia de arquivos*/
        send_receive_file(in_name, out_name, ssocket);

        /* fechando socket*/
        close(ssocket);

    /* problema na identificacao dos argumentos*/        
    } else {
        error_msg(arguments);
    } 
    return 0;
}


void mysettimer(int milisegundos) {
    struct itimerval newvalue, oldvalue;
    newvalue.it_value.tv_sec  = milisegundos / 1000;
    newvalue.it_value.tv_usec = milisegundos % 1000 * 1000;
    newvalue.it_interval.tv_sec  = 0;
    newvalue.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &newvalue, &oldvalue);
}

void unlocker_handler(int signum) {
    if(debug) printf(" ######################### Desbloquenado reenvio #################################\n");
    reenviar = true;
    mysettimer(1000);
}

void stop_timer(int signum) {
    if(debug) printf("Timeout para o ultimo pacote foi desativado\n");
    return ;
}

void error_msg(errorID ID) {
    switch(ID) {
        case arguments:
            fprintf(stderr, "Erro, linha de comando digitada incorretamente.\n");
            fprintf(stderr, "Uso 1: ./dcc023c2 -s <PORT> <INPUT> <OUTPUT>\n");
            fprintf(stderr, "Uso 2: ./dcc023c2 -c <IPPAS>:<PORT> <INPUT> <OUTPUT>\n");
            fprintf(stderr, "\nVoce tambem pode utilizar a flag extra -debug para ver as saidas das operacoes\n");      
            break;
        case listening:
            fprintf(stderr, "Erro ocorreu durante processo de listening\n");
            break;
        default:
            break;
    }
    exit(1);
}

void pack(Package *p) {
    memcpy(&(p->byte_array[0]), &(p->SYNC1), sizeof(p->SYNC1)); // 00 - 31  (4 bytes) : SYNC
    memcpy(&(p->byte_array[4]), &(p->SYNC2), sizeof(p->SYNC2)); // 32 - 63  (4 bytes) : SYNC
    memcpy(&(p->byte_array[8]), &(p->CHKSUM), sizeof(p->CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&(p->byte_array[10]), &(p->LENGTH), sizeof(p->LENGTH)); // 80 - 95  (2 bytes) : LENGTH
    memcpy(&(p->byte_array[12]), &(p->ID), sizeof(p->ID));// 96 - 103 (1 byte) : ID
    memcpy(&(p->byte_array[13]), &(p->FLAGS), sizeof(p->FLAGS));// 104 - 111 (1 bytes) : FLAGS
    p-> h2ns = false;
}

void repack(Package *p) { 
    memcpy(&(p->byte_array[8]), &(p->CHKSUM), sizeof(p->CHKSUM)); // 64 - 79  (2 bytes) : CHKSUM
    memcpy(&(p->byte_array[10]), &(p->LENGTH), sizeof(p->LENGTH)); // 80 - 95  (2 bytes) : LENGTH
    memcpy(&(p->byte_array[12]), &(p->ID), sizeof(p->ID));// 96 - 103 (1 byte) : ID
    memcpy(&(p->byte_array[13]), &(p->FLAGS), sizeof(p->FLAGS));// 104 - 111 (1 bytes) : FLAGS
}

void hton(Package *p) {
    p->SYNC1 = htonl(p->SYNC1);
    p->SYNC2 = htonl(p->SYNC2);
    p->CHKSUM = htons(p->CHKSUM);
    p->LENGTH = htons(p->LENGTH);
}

void ntoh(Package *p) {
    p->SYNC1 = ntohl(p->SYNC1);
    p->SYNC2 = ntohl(p->SYNC2);
    p->CHKSUM = ntohs(p->CHKSUM);
    p->LENGTH = ntohs(p->LENGTH);
}

void initiate_passiveConnection(int *csocket, struct sockaddr_in* server, char* cport) {
    int port = atoi(cport);
    *csocket = socket(PF_INET, SOCK_STREAM, 0);

    char hostname[1024];
    hostname[1023] = '\0';
    gethostname(hostname, 1023);

    struct addrinfo hints,*res;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC; 
    hints.ai_socktype = SOCK_DGRAM;
    uint32_t err = getaddrinfo(hostname, cport, &hints,&res);

    if(err != 0){
        perror("getaddrinfo");
        printf("getaddrinfo %s\n",strerror(errno));
        printf("getaddrinfo : %s \n",gai_strerror(err));
        return ;
    }

    struct sockaddr_in *addr;
    addr = (struct sockaddr_in *)res->ai_addr; 

    printf("Iniciando conexao passivamente [%s|%s].\n", inet_ntoa((struct in_addr)addr->sin_addr), cport);

    /*---- Configuracoes do lado do servidor ----*/  
    server->sin_family = AF_INET; 
    server->sin_port = htons(port);
    server->sin_addr = (struct in_addr)addr->sin_addr; 
    //server->sin_addr.s_addr = inet_addr("192.168.1.8");

    memset(server->sin_zero, '\0', sizeof server->sin_zero);  /* Set all bits of the padding field to 0 */
    bind(*csocket, (struct sockaddr *) server, sizeof(*server)); /*---- Bind the address struct to the socket ----*/
    printf("Socket aberto, id = %d\n", *csocket);
}

void die(uint32_t id) {
    printf("Erro ao lidar com arquivo (%u)\n", id);
}

void initiate_activeConnection(int *ssocket, struct sockaddr_in* server, char* port, char* server_addr) {
    printf("Iniciando conexao ativamente [%s|%s].\n", server_addr, port);
    int server_port = atoi(port);
    *ssocket = socket(PF_INET, SOCK_STREAM, 0);
    server->sin_family = AF_INET;
    server->sin_port = htons(server_port);
    server->sin_addr.s_addr = inet_addr(server_addr); 
    memset(server->sin_zero, '\0', sizeof server->sin_zero); 

    socklen_t addr_size = sizeof *server;
    connect(*ssocket, (struct sockaddr *) server, addr_size);
}

unsigned carry_around_add(unsigned a, unsigned b) {
    unsigned c = a + b;
    return (c &0xffff)+(c >>16);
}

uint16_t checksum(uint8_t *headers, uint8_t *data, uint16_t len){
    if(debug) printf("Calculando checksum sobre: \n");
    unsigned s = 0;
    unsigned i = 0;

    if(debug) printf("[%x][%x][%x][%x][%x][%x]", *((uint32_t*) headers), *((uint32_t*)&headers[4]), *((uint16_t*) &headers[8]), *((uint16_t*)&headers[10]), headers[12], headers[13] );

    /*if(debug)
    for (i = 0; i < len; i+=2) {
         printf("%x", *(data+i));
    }
    printf("]\n");
*/
    for (i = 0; i < 14; i+=2) { // 14 bytes header
      unsigned w = headers[i] + (headers[i+1]<<8);
      s = carry_around_add(s, w);
    }

    for (i = 0; i < len; i+=2) {
      unsigned w = data[i] + (data[i+1]<<8);
      s = carry_around_add(s, w);
    }

    return~s &0xffff;
}


uint16_t
cksum(uint8_t *headers, uint8_t *data, uint16_t len) {
    if(debug) printf("Calculando checksum sobre: \n");
    if(debug) printf("[%x][%x][%x][%x][%x][%x]", *((uint32_t*) headers), *((uint32_t*)&headers[4]), *((uint16_t*) &headers[8]), *((uint16_t*)&headers[10]), headers[12], headers[13] );

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


void send_package(Package *p, int socket) {
    uint32_t nbytes = 0;    
    hton(p);
    send(socket, &(p->SYNC1),  sizeof(p->SYNC1),  0);
    send(socket, &(p->SYNC2),  sizeof(p->SYNC2),  0);
    send(socket, &(p->CHKSUM), sizeof(p->CHKSUM), 0);
    send(socket, &(p->LENGTH), sizeof(p->LENGTH), 0);
    send(socket, &(p->ID),     sizeof(p->ID),     0);
    send(socket, &(p->FLAGS),  sizeof(p->FLAGS),  0);        
    ntoh(p);
    if(p->LENGTH > 0) nbytes = send(socket, p->DATA,  p->LENGTH,  0);
    //if(p->LENGTH > 0) printf("\t\t\t[socket] - Pacote enviado: [%x][%x][%x]  [%x(%u)] [%x][%x]DATA[%s] -- sent: %u\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->LENGTH, p->ID, p->FLAGS, p->DATA, nbytes);        
    if(debug){ 
        if(p->LENGTH > 0) printf("\t\t\t[socket] - Pacote enviado: [%x][%x][%x]  [%x(%u)] [%x][%x]DATA[%c##%c] -- sent: %u\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->LENGTH, p->ID, p->FLAGS, p->DATA[0], p->DATA[p->LENGTH-2], nbytes);        
        else printf("\t\t\t[socket] - Pacote enviado: [%x][%x][%x]  [%x(%u)] [%x][%x] -- sent: %u\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->LENGTH, p->ID, p->FLAGS, nbytes);        
    }
}

void recv_package(Package *p, int socket) {
    p->h2ns = false;
    recv(socket, &(p->SYNC1),  4, 0);
    recv(socket, &(p->SYNC2),  4, 0);
    recv(socket, &(p->CHKSUM), 2, 0);
    recv(socket, &(p->LENGTH), 2, 0);
    recv(socket, &(p->ID),     1, 0);
    recv(socket, &(p->FLAGS),  1, 0);    
    if(debug) printf("[socket] - Pacote recebido: [%x][%x][%x][%x][%x][%x]\n", p->SYNC1, p->SYNC2, p->CHKSUM, p->LENGTH, p->ID, p->FLAGS);    
    ntoh(p);    
}

void send_receive_file(char* in_name, char * out_name , int ssocket) {
    FILE *input_file, *output_file;
    uint32_t sentpackets = 0, recvpackets = 0;
    
    Package data, ack, indata;   

    uint8_t lastID = 1;
    uint16_t lastCHKSUM = 0;
    data.SYNC1 = 0xDCC023C2;
    data.SYNC2 = 0xDCC023C2;
    data.CHKSUM = 0;
    data.LENGTH = 0;
    data.ID = 0;
    data.FLAGS = 0;

    indata.SYNC1 = 0;
    indata.SYNC2 = 0;
    indata.CHKSUM = 0;
    indata.LENGTH = 0;
    indata.ID = 0;
    indata.FLAGS = 0;
    

    uint16_t nElem = 0;
    uint8_t buffer[bufsize]; // 8 bits only  

    // Packet MIN SIZE is 14 bytes due to headers
    input_file = fopen(in_name, "rb"); // open in binary mode
    if(input_file == NULL)
        die(8); //file doesnt exist      

      // Packet MIN SIZE is 14 bytes due to headers
    output_file = fopen(out_name, "wb+"); // open in binary mode
    if(output_file == NULL)
        die(8); //file doesnt exist      


    printf("Enviando arquivo %s para o cliente %u \n", in_name, ssocket);

        
    pack(&data);

    while(true) {          
      if(debug) printf("######## loop ##########\n");
      if(!reenviar && !blocked && more_to_send) {
        data.LENGTH = fread(&buffer, sizeof(char), bufsize - 14, input_file); //Reading files in chunks of bufsize at most (-16 frp, header)
        nElem = data.LENGTH;        
        if (nElem <= 0 && !feof(input_file)) { //If I couldnt read anything but I havent reached end of file, there is an error
            die(9); // reading error;
        }else {          
          if(debug)  printf("#__buffer lido - %u bytes__#\n", data.LENGTH);
        }

        if(feof(input_file)){ //end of file, end of transmission
            printf("Fim do arquivo !\n\n");  
            fclose(input_file);
            data.FLAGS = 0x40;
        } else {
            more_to_send = true;
        }
        
        data.DATA = buffer;
        data.DATA[nElem-1] = '\0';        
        pack(&data);
        data.CHKSUM = cksum(data.byte_array, data.DATA, nElem);
        
        
        if(debug) printf("#__envio liberado\n");
        send_package(&data, ssocket);                        
        signal(SIGALRM, unlocker_handler);    
        mysettimer(timeout);
        blocked = true;
        sentpackets++;

      }
      else if(reenviar) {
        send_package(&data, ssocket);                        
        blocked = true;
        reenviar = false; // so ira acionar denovo depois de um segundo
      } else {
        if(debug) printf("#__envio bloqueado - more_to_send = %u, reenviar = %u, blocked (falta ack)= %u\n", more_to_send, reenviar, blocked);
      }

      if(!more_to_send && !more_to_recv) break;      
        
        if(debug) printf("\tEsperando ACK (%u) ou Dados\n", data.ID);
        recv_package(&indata, ssocket);     
      
        if(indata.SYNC1 != indata.SYNC2 || indata.SYNC1 != 0xDCC023C2) {
            if(debug) printf("\t\tDescartando pacote, inicio nao estao correto\n");
            continue; // recebi um pacote cagado, mal identificado - Abandonar
        }

        //Receiving Data with length determined by data.LENGTH
        if(indata.LENGTH > 0 && indata.FLAGS != 0x80) { // Se for pacote de dados
            indata.DATA = malloc(indata.LENGTH * sizeof(uint8_t));
            uint32_t nbytes = recv (ssocket, indata.DATA, indata.LENGTH, 0);
            indata.CHKSUM = htons(indata.CHKSUM);
            pack(&indata); //copiando headers para um bytearray para calcular chksum;
            uint16_t in_checksum = cksum(indata.byte_array, indata.DATA, indata.LENGTH);
            if(debug) printf("in checksum calculado $: %x\n", in_checksum);


            if(lastID != indata.ID /*&& Checksum valido */ ) {
                ack.ID = indata.ID;
                ack.ID = indata.ID;
                ack.SYNC1 = 0xDCC023C2;
                ack.SYNC2 = 0xDCC023C2;
                ack.CHKSUM = 0;
                ack.LENGTH = 0;
                ack.FLAGS = 0x80;


                 //ToDo: check for errors after sending data
                 if(debug) printf("\t\t - ## Pacote Valido, escrevendo dados em disco e mandando ACK\n");

                 send_package(&ack, ssocket);
                 if(debug) printf("ACK enviado (%u)\n", ack.ID);
                 lastID = indata.ID;
                 lastCHKSUM = indata.CHKSUM;
                 recvpackets++;
                
                 //Writing data received in the output file
                 fwrite(indata.DATA, 1, nbytes, output_file); 
                 if(debug) printf("Dados escritos no disco\n");
             
             } else if (lastCHKSUM == indata.CHKSUM && lastID == indata.ID) { // recebi msm pacote, mandar so ack
                 if(debug) printf("\t\t - ## Pacote Repetido - enviando mesmo ACK\n");
                 send_package(&ack, ssocket);
             }

             if(indata.FLAGS == 0x40) more_to_recv = false;

        } else if (indata.LENGTH == 0 && indata.FLAGS == 0x80) { // Se for pacote ACK
            if(indata.ID == data.ID) { // verifica se o ACK que mandei tem msm ID do pacote de dados que enviei
              signal(SIGALRM, stop_timer); // desativa o timeout para aquele pacote ser reenviado
              if(debug)  printf("ACK recebido: %u\n", indata.ID);
              data.ID = !data.ID; // inverte o ID do proximo pacote a enviar
              reenviar = false;
              blocked = false;
            }else {
              if(debug) printf("Problema com o ACK - reenviar (ackID = %x, ackFLAG = %x)\n", indata.ID, indata.FLAGS);
              reenviar = true;
            }
        } else if(indata.FLAGS == 0x40){          
          if(debug) printf("No more to receive\n");
          more_to_recv = false;
        }


        if(data.FLAGS == 0x40){ // end of transmission          
          more_to_send = false;
        }
      // #######################################################################
      // #######################################################################
      // #######################################################################

    }  
    fclose(output_file);
    printf("Arquivo %s lido de     %i. Pacotes enviados : %u\n\n", in_name, ssocket,  sentpackets);
    printf("Arquivo %s escrito de  %i. Pacotes recebidos: %u\n\n", out_name, ssocket, recvpackets);
  }



void receive_send_file(char* in_name, char* out_name , int ssocket) {
    FILE *input_file, *output_file;
    uint32_t sentpackets = 0, recvpackets = 0;
    
    Package data, ack, indata;    

    uint8_t lastID = 1;
    uint16_t lastCHKSUM = 0;
    data.SYNC1 = 0xDCC023C2;
    data.SYNC2 = 0xDCC023C2;
    data.CHKSUM = 0;
    data.LENGTH = 0;
    data.ID = 0;
    data.FLAGS = 0;
    uint16_t nElem = 0;
    uint8_t buffer[bufsize]; // 8 bits only  

    // Packet MIN SIZE is 14 bytes due to headers
    input_file = fopen(in_name, "rb"); // open in binary mode
    if(input_file == NULL)
        die(8); //file doesnt exist      

      // Packet MIN SIZE is 14 bytes due to headers
    output_file = fopen(out_name, "wb+"); // open in binary mode
    if(output_file == NULL)
        die(8); //file doesnt exist      


    printf("Enviarei o arquivo %s para o cliente %u \n", in_name, ssocket);

    signal(SIGALRM, unlocker_handler);    
    pack(&data);

    ///bool blocked = false;
    
    while(true) {
       if(debug) printf("############# loop ###############\n");
       if(debug) printf("\tEsperando ACK (%u) ou Dados\n", data.ID);   
      recv_package(&indata, ssocket);
        
        if(indata.SYNC1 != indata.SYNC2 || indata.SYNC1 != 0xDCC023C2) {
             if(debug) printf("\t\tPacote abandonado\n");
            continue; // recebi um pacote cagado, mal identificado - Abandonar
        }

        //Receiving Data with length determined by data.LENGTH
        if(indata.LENGTH > 0 && indata.FLAGS != 0x80) { // Se for pacote de dados
           if(debug) printf("Pacote de dados - receberei %u bytes\n", indata.LENGTH);
          indata.DATA = malloc(indata.LENGTH * sizeof(uint8_t));
          uint32_t nbytes = recv(ssocket, indata.DATA, indata.LENGTH, 0);
           if(debug) printf("Recebi %u bytes [%c##%c]\n", nbytes, indata.DATA[0], indata.DATA[indata.LENGTH-2]);

           if(debug) printf("--> Pacote recebido: [%x][%x][%x][%x][%x][%x]\n", indata.SYNC1, indata.SYNC2, indata.CHKSUM, indata.LENGTH, indata.ID, indata.FLAGS);    
    
          indata.CHKSUM = htons(indata.CHKSUM);
          pack(&indata); //copiando headers para um bytearray para calcular chksum;

          uint16_t in_checksum = cksum(indata.byte_array, indata.DATA, indata.LENGTH);
          //AKI
           if(debug) printf("in checksum calculado: %x\n", in_checksum);

          

          if(lastID != indata.ID /*&& valid checksum*/ ) {
            ack.ID = indata.ID;
            ack.SYNC1 = 0xDCC023C2;
            ack.SYNC2 = 0xDCC023C2;
            ack.CHKSUM = 0;
            ack.LENGTH = 0;
            ack.FLAGS = 0x80;

             //ToDo: check for errors after sending data
              if(debug) printf("\t\t - ## Pacote Valido, escrevendo dados em disco e mandando ACK\n");
             
             //Writing received data in the output file
             fwrite(indata.DATA, 1, nbytes, output_file); 
              if(debug) printf("\t\t # Dados escritos no diisco\n");

             send_package(&ack, ssocket);
             lastID = indata.ID;
             lastCHKSUM = data.CHKSUM;
             recvpackets++;
            
              if(debug) printf("\t\tACK (%u) enviado\n", lastID);
         
          } else if (lastCHKSUM == indata.CHKSUM && lastID == indata.ID) { // recebi msm pacote, mandar so ack
              if(debug) printf("\t\t - ## Pacote Repetido - enviando mesmo ACK\n");
             send_package(&ack, ssocket);
          }
        } else if (indata.LENGTH == 0 && indata.FLAGS == 0x80) { // Se for pacote ACK
            if(indata.ID == data.ID) {
              signal(SIGALRM, stop_timer);
               if(debug) printf("ACK recebido: %u\n", indata.ID);
              data.ID = !data.ID;
              reenviar = false;
              blocked = false;
            }/*else {
              int check;
               if(debug) printf("Problema com o ACK - reenviar (ackID = %x, ackFLAG = %x)\n", indata.ID, indata.FLAGS);
              //reenviar = true;
            }*/
        }

        if(!more_to_send && !more_to_recv) break;      

        if(indata.FLAGS == 0x40){
           if(debug) printf("No more to receive\n");
          more_to_recv = false;
        }

        if(!reenviar && !blocked && more_to_send) {
            data.LENGTH = fread(&buffer, sizeof(char), bufsize - 14, input_file); //Reading files in chunks of bufsize at most (-14 frp, header)
            nElem = data.LENGTH;
            if (nElem <= 0 && !feof(input_file)) { 
                die(9); // reading error;
            }else {          
               if(debug) printf("#__buffer lido - %u bytes__#\n", data.LENGTH);
            }

            if(feof(input_file)){ //end of file, end of transmission
                printf("Fim do arquivo !\n\n");  
                fclose(input_file);
                data.FLAGS = 0x40;
            } else {
                more_to_send = true;
            }

            data.DATA = buffer;
            data.DATA[nElem-1] = '\0';
            data.CHKSUM = cksum(data.byte_array, data.DATA, nElem);        
            repack(&data);

             if(debug) printf("#__envio liberado\n");
            send_package(&data, ssocket);                        
            signal(SIGALRM, unlocker_handler);    
            mysettimer(timeout);
            blocked = true;
            sentpackets++;

      } else if(reenviar) {
            send_package(&data, ssocket);                        
            blocked = true;
            reenviar = false; // so ira acionar denovo depois de um segundo
      } else {
             if(debug) printf("#__envio bloqueado - more_to_send = %u, reenviarSW = %u, blocked = %u\n", more_to_send, reenviar, blocked);
      }
      


      if(data.FLAGS == 0x40){ // end of transmission
          more_to_send = false;
      }

    }
    fclose(output_file);
    printf("Arquivo %s lido de     %i. Pacotes enviados : %u\n\n", in_name, ssocket,  sentpackets);
    printf("Arquivo %s escrito de  %i. Pacotes recebidos: %u\n\n", out_name, ssocket, recvpackets);
}