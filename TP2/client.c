/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <unistd.h>

#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char *buf;
    int buffer_size;
    int window_size;
    char *filename, msg_toSend;
    int filename_size;
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



    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
    (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    bzero(buf, buffer_size);
    printf("Requesting file: %s (%d)\n", filename, filename_size);
    msg_toSend = malloc((filename_size+5) * sizeof(char)); // Alocando variavel msg_toSend que tera o filename + id de fim de string
    //strcpy(msg_toSend, filename);
    strcat(filename, "\\0");


    int bytesLeft = strlen(filename);
    int sendPosition = 0;
    int chunck_size;
    int numBytesSent;

    /* send the message to the server */
    serverlen = sizeof(serveraddr);
    
    while(bytesLeft > 0){      
      chunck_size = bytesLeft > buffer_size ? buffer_size : bytesLeft;
      memcpy(buf, filename + sendPosition, chunck_size);      
      numBytesSent = sendto(sockfd, buf, chunck_size, 0, &serveraddr, serverlen);
      printf("Sent:            %s \t ", buf);
      if (numBytesSent < 0) 
        error("ERROR in sendto");      
      sendPosition += numBytesSent;
      bytesLeft -= numBytesSent;        
      printf("Bytes Sent: %d  -  Bytes Left: %d\n",numBytesSent , bytesLeft);
      memset(buf, '\0', strlen(buf)*sizeof(buf));
    }    
    printf("File requested\n");
    


    /* print the server's reply */
    /*
    n = recvfrom(sockfd, buf, buffer_size, 0, &serveraddr, &serverlen);
    if (n < 0) 
      error("ERROR in recvfrom");
    printf("Echo from server: %s (%d/%d)", buf, n, strlen(buf));
*/
    
    int totalBytesRcvd = 0;

    memset(buf, '\0', sizeof(buf));

    

    FILE *file = fopen(copyName, "w+"); // file to be downloaded will be written here
  
    if (file == NULL) // Invalid file or file not found
      printf("erro abrir");
    int times = 0;
    char ack[30];
    while (1) {
      n = recvfrom(sockfd, buf, buffer_size, 0, &serveraddr, &serverlen);
      times++;
//      printf("%d (%d)", n, times);
      if (n == 0)
        break;
      if (n < 0)
        error("ERROR in recvfrom");
      if (n > 1 && buf[n-2] == '\\' && buf[n-1] == '0')
        break;

      
      /*if (times >= 2056){
        printf("\n\n%s", buf);
      }*/

      fwrite(buf, 1, n, file); // writes file
      totalBytesRcvd += n;
      sprintf(ack, "ACK - %d", times);
      numBytesSent = sendto(sockfd, ack, 30, 0, &serveraddr, serverlen);

    }
    fclose(file);
  


    return 0;
}