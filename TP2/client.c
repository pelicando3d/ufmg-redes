#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tp_socket.h"
#include "timer.c"


// ToDo: Implementar no servidor o caso de o ACK  final falhar

int espera = 1000;

void error(char *msg) {
    perror(msg);
    exit(0);
}

uint16_t tcp_checksum(const char *data_p, size_t len, so_addr *src_addr, so_addr *dest_addr){
    int i;
    char *ad6 = malloc(1000 * sizeof(char));
    char *ad6_2 = malloc(1000 * sizeof(char));
    inet_ntop(AF_INET6, &src_addr->sin6_addr, ad6, 1000);
    inet_ntop(AF_INET6, &dest_addr->sin6_addr, ad6_2, 1000);
    
    unsigned char x;
    unsigned short crc = 0xFFFF;

    while (len--){
        x = crc >> 8 ^ *data_p++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }

    for (i = 0; ad6[i] != '\0'; i++){
        x = crc >> 8 ^ *ad6++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);
    }

    for (i = 0; ad6_2[i] != '\0'; i++){
        x = crc >> 8 ^ *ad6_2++;
        x ^= x>>4;
        crc = (crc << 8) ^ ((unsigned short)(x << 12)) ^ ((unsigned short)(x <<5)) ^ ((unsigned short)x);        
    }

    return crc;

}


int send_datagram(int so, char* buff, int buff_len, so_addr* to_addr, so_addr* from_addr){
    int ret, crc;    

    crc = tcp_checksum(buff, buff_len, from_addr, to_addr);
    printf("\tCRC client send: %d\n", crc);
    //printf("\t%s, %d, %s, %s\n",buff, buff_len, inet_ntop(AF_INET6, &from_addr->sin6_addr, ad6, 1000), inet_ntop(AF_INET6, &to_addr->sin6_addr, ad6_2, 1000));

    ret = tp_sendto(so, buff,  buff_len,  (struct sockaddr *)to_addr);
    return ret;
}

int receive_datagram(int so, char* buff, int buff_len, so_addr* from_addr, so_addr* to_addr){
    int ret, crc;

    ret = tp_recvfrom(so, buff, buff_len,  (struct sockaddr *)from_addr);

    crc = tcp_checksum(buff, ret, to_addr,  from_addr);
    
    printf("CRC Server recv: %d\n", crc);

    //printf("\t%s, %d, %s, %s\n",buff, ret, inet_ntop(AF_INET6, &from_addr->sin6_addr, ad6, 100), inet_ntop(AF_INET6, &to_addr->sin6_addr, ad6, 100));


    return ret;
}

int handler_sock;
char *handler_buf;
int handler_chunk_size;
struct sockaddr *to, *from;
int reenviarSW;


void stopWait_handler(int signalid){
    if(reenviarSW){
        printf("\n\tReenviando Pacote\n");
        int numBytesSent = send_datagram(handler_sock, handler_buf,  handler_chunk_size, to, from);
        mysettimer(espera);
    }
}

int main(int argc, char* argv[]) {
    int sock;
    int status;
    struct addrinfo sainfo, *psinfo, *localinfo;
    struct sockaddr_in6 sin6;
    int sin6len;
      
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char *buf;
    int buffer_size;
    int window_size;
    int filename_size;
    char *filename, msg_toSend;  
    int bytesLeft;
    int sendPosition = 0;
    int chunck_size;
    int numBytesSent;
    char *copyName;

    /* check command line arguments */
    if (argc != 6) {
        fprintf(stderr,"Uso: %s <nome ou IPv6 do servidor> <porto> <nome_arquivo ><tam_buffer> <tam_janela>\n", argv[0]);
        exit(0);
    }


    hostname = argv[1];
    portno = atoi(argv[2]);
    filename = argv[3];
    buffer_size = atoi(argv[4]);
    window_size = atoi(argv[5]);
    filename_size = strlen(filename);    
    buf = (char*) malloc(buffer_size * sizeof(char));
    copyName = (char*) malloc(filename_size + 5 * sizeof(char));
    strcpy(copyName, "copy_");
    strcat(copyName, filename);
  

    tp_init();
    sock = tp_socket(portno, hostname); //ToDo Voltar essa funcao pro original

   /*sock = socket(AF_INET6, SOCK_DGRAM,0);
   status = bind(sock, (struct sockaddr *)&sin6, sin6len);*/
  
    if(sock < 0){     
      printf("Error %d\n", sock); // Tem que corrigir o erro -1 do bind
    }
  
    memset(&sainfo, 0, sizeof(struct addrinfo));
     
    sainfo.ai_flags = 0;
    sainfo.ai_family = PF_INET6;
    sainfo.ai_socktype = SOCK_DGRAM;
    sainfo.ai_protocol = IPPROTO_UDP;
    //status = getaddrinfo("ip6-localhost", argv[2], &sainfo, &localinfo); //get local info
    status = getaddrinfo(hostname, argv[2], &sainfo, &psinfo); //get server info
    
     
    bzero(buf, buffer_size);
    printf("Requesting file: %s (%d)\n", filename, filename_size);
    msg_toSend = malloc((filename_size+5) * sizeof(char)); // Alocando variavel msg_toSend que tera o filename + id de fim de string
    strcat(filename, "\\0");
    bytesLeft = strlen(filename);

    int wait_ack = 0;

    size_t elen;
    int    done; /* Controle do loop */


    char entrada[1000];
      

    while(bytesLeft > 0){      
        chunck_size = bytesLeft > buffer_size ? buffer_size : bytesLeft;
        memcpy(buf, filename + sendPosition, chunck_size);      
        

        handler_sock = sock;
        handler_buf = buf;
        handler_chunk_size = chunck_size;
        to = (struct sockaddr *)psinfo->ai_addr;
        from = (struct sockaddr *)psinfo->ai_addr;

        signal(SIGALRM, stopWait_handler);
        mysettimer(espera);


        done = 0;
        while (!done) {
            errno = 0; /* Só por garantia */
            //fprintf(stderr,"Enviando PACOTE 1 NOME\n");
            numBytesSent = send_datagram(sock, buf,  chunck_size, (struct sockaddr *)psinfo->ai_addr, (struct sockaddr *)psinfo->ai_addr);
            reenviarSW = 1;
            if ( ( n = receive_datagram(sock, buf, buffer_size,  (struct sockaddr *)psinfo->ai_addr, (struct sockaddr *)localinfo->ai_addr) ) < 0 ) {                
                /* chamadas de socket só retornam < 0 se deu erro */                
                if (errno==EINTR) {
                    printf("\ninterrompida\n");
                    /* uma chamada interrompida seria tratada aqui */
                    errno = 0;
                } else if (errno) {
                    perror("fgets");
                    exit(1);
                } else if (feof(stdin)) {
                    fprintf(stderr,"Entrada vazia.\n");
                    exit(0);
                }
            } else {
                printf("RETORNOU COM SUCESSO\n");
                reenviarSW = 0;
                done = 1;
            }
        }

        

        wait_ack = 1;
        
        printf("Sent:            %s \t ", buf);
        if (numBytesSent < 0) 
          error("ERROR in sendto");      
        sendPosition += numBytesSent;
        bytesLeft -= numBytesSent;        
        printf("Bytes Sent: %d  -  Bytes Left: %d\n",numBytesSent , bytesLeft);
        memset(buf, '\0', strlen(buf)*sizeof(buf));
    }    

    printf("File requested\n");

    /* Abrindo arquivo para gravacao */
    FILE *file = fopen(copyName, "w+");  
    if (file == NULL) // Invalid file or file not found
        printf("erro abrir");


    int times = 0;
    char ack[30];
    int totalBytesRcvd = 0;
    memset(buf, '\0', sizeof(buf));
    while (1) {
        n = receive_datagram(sock, buf, buffer_size,  (struct sockaddr *)psinfo->ai_addr, (struct sockaddr *)localinfo->ai_addr);
        times++;
        if (n == 0)
          break;
        if (n < 0)
          error("ERROR in recvfrom");
        if (n > 1 && buf[n-2] == '\\' && buf[n-1] == '0')
          break;
  
        fwrite(buf, 1, n, file); // writes file
        totalBytesRcvd += n;
        sprintf(ack, "ACK - %d", times);
        numBytesSent = send_datagram(sock, ack,  30,  (struct sockaddr *)psinfo->ai_addr, (struct sockaddr *)localinfo->ai_addr);

    }
    fclose(file);


  // free memory
  freeaddrinfo(psinfo);
  psinfo = NULL;

  shutdown(sock, 2);
  close(sock);
  return 0;
}

