/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define BUFSIZE 1024
typedef enum { false, true } bool;

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(int argc, char **argv) {
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


  // Alocando o buffer da mensagem de acordo com o bufsize

  buf = (char*) malloc(buffer_size * sizeof(char));

  /* 
   * socket: create the parent socket 
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) 
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets 
   * us rerun the server immediately after we kill it; 
   * otherwise we have to wait about 20 secs. 
   * Eliminates "ERROR on binding: Address already in use" error. 
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, 
       (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /* 
   * bind: associate the parent socket with a port 
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr, 
     sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  // Onde a magica acontece: Espera sinal do cliente e envia arquivo

  clientlen = sizeof(clientaddr);
  char msg[9064];
  strcpy(msg, "");
  bool signal = false;
  int totalBytes = 0;
  int totalBytesRec = 0;
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */

    bzero(buf, buffer_size);


    totalBytesRec = 0;
    totalBytes = 0;

    memset(msg, '\0', sizeof(msg));
    memset(buf, '\0', sizeof(buf));    
    memset(filepath, '\0', sizeof(filepath));    

    printf("%s - %s \n\n", msg, buf);
    while( (n = recvfrom(sockfd, buf, buffer_size, 0, (struct sockaddr *) &clientaddr, &clientlen)) ){      
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


     hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr, 
        sizeof(clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s (%s)\n", 
     hostp->h_name, hostaddrp);
    printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);
    

    //cout << "Enviando arquivo: " << filename << " para o cliente.\n ";
    int times = 0;
    while(1) {
      printf("Lendo\n");
      nElem = fread(buf, sizeof(char), buffer_size, file); 
    
      chunck_size =  nElem > buffer_size ? buffer_size : nElem;

      printf("chuck size: %d, strlen buf: %d, buf size: %d, nElen: %d\n", chunck_size, strlen(buf), buffer_size, nElem);

      if (nElem <= 0 && !feof(file)) { 
        printf("Erro Leitura\n");
        break; // error leitura
      }

      numBytesSent = sendto(sockfd, buf, chunck_size, 0, (struct sockaddr *) &clientaddr, clientlen);
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
    
    numBytesSent = sendto(sockfd, "\\0", sizeof("\\0"), 0, (struct sockaddr *) &clientaddr, clientlen);
    printf("Arquivo: %s enviado para o cliente em %d blocos\n", msg, times);

    fclose(file);    

  }
}
