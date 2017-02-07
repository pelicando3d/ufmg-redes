#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tp_socket.h"

#define MAXBUF 65536

void error(char *msg) {
    perror(msg);
    exit(0);
}


int send_datagram(int so, char* buff, int buff_len, so_addr* to_addr){
    int ret;
    ret = tp_sendto(so, buff,  buff_len,  (struct sockaddr *)to_addr);
    return ret;
}

int receive_datagram(int so, char* buff, int buff_len, so_addr* from_addr){
    int ret;
    ret = tp_recvfrom(so, buff, buff_len,  (struct sockaddr *)from_addr);
    return ret;
}

int main(int argc, char* argv[]) {
    int sock;
    int status;
    struct addrinfo sainfo, *psinfo;
    struct sockaddr_in6 sin6;
    int sin6len;
    char buffer[MAXBUF];
  
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
  
    sock = tp_socket(portno);
  
    if(sock < 0){     
      printf("Error %d\n", sock); // Tem que corrigir o erro -1 do bind
    }
  
    memset(&sainfo, 0, sizeof(struct addrinfo));
     
    sainfo.ai_flags = 0;
    sainfo.ai_family = PF_INET6;
    sainfo.ai_socktype = SOCK_DGRAM;
    sainfo.ai_protocol = IPPROTO_UDP;
    status = getaddrinfo(hostname, argv[2], &sainfo, &psinfo);
     
    bzero(buf, buffer_size);
    printf("Requesting file: %s (%d)\n", filename, filename_size);
    msg_toSend = malloc((filename_size+5) * sizeof(char)); // Alocando variavel msg_toSend que tera o filename + id de fim de string
    strcat(filename, "\\0");
    bytesLeft = strlen(filename);
      
    while(bytesLeft > 0){      
        chunck_size = bytesLeft > buffer_size ? buffer_size : bytesLeft;
        memcpy(buf, filename + sendPosition, chunck_size);      
        numBytesSent = send_datagram(sock, buf,  chunck_size, (struct sockaddr *)psinfo->ai_addr);
        
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
        n = receive_datagram(sock, buf, buffer_size,  (struct sockaddr *)psinfo->ai_addr);
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
        numBytesSent = send_datagram(sock, ack,  30,  (struct sockaddr *)psinfo->ai_addr);

    }
    fclose(file);


  // free memory
  freeaddrinfo(psinfo);
  psinfo = NULL;

  shutdown(sock, 2);
  close(sock);
  return 0;
}

