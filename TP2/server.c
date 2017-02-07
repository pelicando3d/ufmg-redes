#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include "tp_socket.h"

#define MAXBUF 65536

#define BUFSIZE 1024
typedef enum { false, true } bool;

void error(char *msg) {
  perror(msg);
  exit(1);
}



int main(int argc, char **argv)
{
   int sock;
   int status;
   struct sockaddr_in6 sin6;
   int sin6len;
   char buffer[MAXBUF];

   int sockfd; // socket
  int portno; // id da porta
  int clientlen; /* byte size of client's address */
  int buffer_size; // tamanho do buffer
  int window_size; // tamanho da janela
  char *dir; // diretorio a ser utilizado
  char *filepath;
  struct sockaddr_in serveraddr; // endereco do servidor
  struct sockaddr_in clientaddr; // endereco do cliente
  struct hostent *hostp; // informacoes do cliente
  char *buf; // buffer p/ mensagem
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n; /* message byte size */
  so_addr  addr;
  char *hostname;

  // Checar argumentos da linha de comando
  if (argc != 5) {
    fprintf(stderr, "Uso corrento: %s <porto do servidor> <tam_buffer> <tam_janela> <diretório a ser utilizado>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);
  buffer_size = atoi(argv[2]);
  window_size = atoi(argv[3]);
  dir = argv[4];
  filepath = argv[4];

  

  tp_init();
  sock = tp_socket(portno);

  
  buf = (char*) malloc(buffer_size * sizeof(char));

   /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));




  memset(&sin6, 0, sin6len);
  sin6len = sizeof(struct sockaddr_in6);
  sin6.sin6_port = htons(portno);
  sin6.sin6_family = AF_INET6;
  sin6.sin6_addr = in6addr_any;
  status = getsockname(sock, (struct sockaddr *)&sin6, &sin6len);

  char msg[9064];
  strcpy(msg, "");
  bool signal = false;
  int totalBytes = 0;
  int totalBytesRec = 0;

  while (1) {
    bzero(buf, buffer_size);
    totalBytesRec = 0;
    totalBytes = 0;

    memset(msg, '\0', sizeof(msg));
    memset(buf, '\0', sizeof(buf));    
    memset(filepath, '\0', sizeof(filepath));    

    printf("%s - %s \n\n", msg, buf);
    printf("\t\tState 1\n");
    //while( (n = recvfrom(sockfd, buf, buffer_size, 0, (struct sockaddr *) &clientaddr, &clientlen)) ){    
    while( (n = tp_recvfrom(sock, buf, buffer_size, (struct sockaddr *) &sin6)) ){    

      totalBytes += n;
      totalBytesRec += strlen(buf);
      if (n < 0)
        error("ERROR in recvfrom");
      
      buf[n] = '\0';
      strcat(msg, buf); //msg += buffer;

      if(signal && buf[n-1] == '0') 
        break;
      if (n > 1 && buf[n-2] == '\\' && buf[n-1] == '0')
        break;
      if(buf[n-1] == '\\')
        signal = true;


    }

    msg[strlen(msg)-2] = '\0';
    printf("server received %d/%d bytes: %s\tsize of string: %d\n", totalBytesRec, totalBytes, msg, strlen(msg));
  

    /* 
     * Servidor recebeu nome do arquivo, agora vai enviar para o cliente
     */

    strcat(filepath, dir);
    strcat(filepath, msg);
    FILE *file = fopen(filepath, "rb"); // open in binary mode

    char line[12];
    char *result;

    
    if(file == NULL){
      printf("ERRO ABRI FILE %s", dir);
      exit(2);
    }
    else
      printf("Arquivo %s aberto com sucesso\n", dir);


    //unsigned int bytesRead = 0;
    unsigned int totalBytesSent = 0;
    unsigned int totalMsgSent = 0;
    int  numBytesSent;
    
    ssize_t nElem = 0;

    int chunck_size;

    //cout << "Enviando arquivo: " << filename << " para o cliente.\n ";
    int times = 0;
    printf("\t\tState 2\n");
    int initiated = 0;
    char ack[30];
    while(1) {
      printf("Lendo\n");
      nElem = fread(buf, sizeof(char), buffer_size, file); 
    
      chunck_size =  nElem > buffer_size ? buffer_size : nElem;

      printf("chuck size: %d, strlen buf: %d, buf size: %d, nElen: %d\n", chunck_size, strlen(buf), buffer_size, nElem);

      if (nElem <= 0 && !feof(file)) { 
        printf("Erro Leitura\n");
        break; // error leitura
      }

 /*     if(initiated){
         //n = recvfrom(sockfd, ack, 30, 0, (struct sockaddr *) &clientaddr, &clientlen);
         n = tp_recvfrom(sockfd, ack, 30, (struct sockaddr *) &clientaddr);
         printf("%s\n", ack);
      }*/
      //numBytesSent = sendto(sockfd, buf, chunck_size, 0, (struct sockaddr *) &clientaddr, clientlen);
      numBytesSent = tp_sendto(sock, buf,  chunck_size, (struct sockaddr *) &sin6);


      initiated = 1;
      
      times++;

      if (numBytesSent < 0){
        printf("Erro Envio\n");
        break; // error sento
      }else{
        totalBytesSent += numBytesSent;
        totalMsgSent += 1;
      }

      if(feof(file)){
        break;
      }
    }


    char *end = "\\0";
    printf("\t\tState 3\n");
    //numBytesSent = sendto(sockfd, end, sizeof(end), 0, (struct sockaddr *) &clientaddr, clientlen);
    numBytesSent = tp_sendto(sock, end,  sizeof(end), (struct sockaddr *) &sin6);
    printf("Arquivo: %s enviado para o cliente em %d blocos\n", msg, times);

    fclose(file);    
 



   shutdown(sock, 2);
   close(sock);
   return 0;
}

}