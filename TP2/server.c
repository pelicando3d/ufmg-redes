#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "tp_socket.h"
#include <pthread.h>
#include "timer.c"

typedef enum { false, true } bool;

int LAR = 0;

void error(char *msg) {
  perror(msg);
  exit(1);
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


int send_datagram(int so, char* buff, int buff_len, so_addr* to_addr){
    int ret, crc;    

//    crc = tcp_checksum(buff, buff_len, from_addr, to_addr);

    ret = tp_sendto(so, buff,  buff_len,  to_addr);
    return ret;
}

int receive_datagram(int so, char* buff, int buff_len, so_addr* from_addr){ // ultimo eh end local
    int ret, crc;

    ret = tp_recvfrom(so, buff, buff_len,  from_addr);

  //  crc = tcp_checksum(buff, ret, to_addr,  from_addr);
    
    return ret;
}

int *revs, *block;

struct arg_struct {    
    int so;
    char* buff;
    int buff_len;
    so_addr* from_addr;
    int seq;
};

void *_send(void *arguments)
{
    struct arg_struct *args = (struct arg_struct *)arguments;
        

    while(!revs[args->seq]){
        if(block[args->seq]) printf("\nReenviando pacote %d\n", args->seq);
        else  printf("\nEnviando pacote %d\n", args->seq);
        int ret = send_datagram(args->so, args->buff, args->buff_len, args->from_addr);
        block[args->seq] = 1;
        sleep(2);
    }
    revs[args->seq] = 0;
    block[args->seq] = 0;

    pthread_exit(NULL);
    return NULL;
}

int window_space(int sws){
    int i = 0;
    int sum = 0;
    for(i; i<sws; i++){
        sum += block[i];
    }
    if(sum)
        return 0;
    return 1;
}


int sw_sock;
char *sw_buf;
int sw_buf_size;
so_addr *to;
so_addr *from;


void* receiveACKs(void *arg){
    char *ack = malloc(10 * sizeof(char));
    int i;
    int n;

    while(1){
        n = receive_datagram(sw_sock, sw_buf, sw_buf_size,  from);
        if(n < 0){
            printf("\n\tERRO AO RECEBER ACK\n");
            break;
        }
        memcpy(ack, sw_buf, 10);
        LAR = atoi(ack);
        revs[LAR] = 1;
        printf("\t\tACK Recebido - %d (%s)\n", LAR, sw_buf);
    }

}



int main(int argc, char **argv){
    int sock;
    int status;
    struct sockaddr_in6 sin6;
    struct addrinfo sainfo, *localinfo;
    int sin6len;   
    int portno; // id da porta
    int buffer_size; // tamanho do buffer
    int SWS; // tamanho da janela
    char *dir; // diretorio a ser utilizado
    char *filepath; // path pro arquivo a ser enviado
    char *buf; // buffer p/ mensagem
    char *hostaddrp;     
    int n; // bytesEnviados

    /* Checar argumentos da linha de comando */
    if (argc != 5) {
      fprintf(stderr, "Uso corrento: %s <porto do servidor> <tam_buffer> <tam_janela> <diretório a ser utilizado>\n", argv[0]);
      exit(1);
    }

    /* Transferindo argumentos */
    portno = atoi(argv[1]);
    buffer_size = atoi(argv[2]);
    SWS = atoi(argv[3]);    
    filepath = argv[4];


    revs = malloc(SWS*sizeof(int));
    memset(revs, 0, SWS);

    block = malloc(SWS*sizeof(int));
    memset(block, 0, SWS);

    
  
    
    /* Iniciando TP */
    
    tp_init();
    sock = tp_socket(portno, NULL);    
    buf = (char*) malloc(buffer_size * sizeof(char));
       
    /* Inicializando estrutura de enderecos e protocolos*/
    memset(&sin6, 0, sizeof(struct sockaddr_in6));
    sin6len = sizeof(struct sockaddr_in6);
    sin6.sin6_port = htons(portno);
    sin6.sin6_family = AF_INET6;
    sin6.sin6_addr = in6addr_any;    
    status = getsockname(sock, (struct sockaddr *)&sin6, &sin6len);


    /*Recebendo Nome do arquivo para enviar */
    char *msg;    
    int totalBytes = 0;
    int totalBytesRec = 0;
    bool signal = false;

    msg = malloc(buffer_size * sizeof(char));    
    strcpy(msg, "");

    /* Recebe nome do arquivo em chuncks, pois nome do arquivo pode
    *  ser maior do que o buffer disponivel para envio no cliente
    */    
      
    bzero(buf, buffer_size);
    totalBytesRec = 0;
    totalBytes = 0;
    
    memset(msg, '\0', sizeof(msg));
    memset(buf, '\0', sizeof(buf));    
    memset(filepath, '\0', sizeof(filepath));    
    
    printf("\t\tState 1\n");
    while( (n = receive_datagram(sock, buf, buffer_size, (so_addr *) &sin6 )) ) {    
        totalBytes += n;
        totalBytesRec += strlen(buf);
        if (n < 0)
            printf("ERROR in recvfrom");
           
        buf[n] = '\0';
        strcat(msg, buf); //msg += buffer;
      
        if(signal && buf[n-1] == '0') 
            break;
        if (n > 1 && buf[n-2] == '\\' && buf[n-1] == '0')
            break;
        if(buf[n-1] == '\\')
            signal = true;    
        msg = realloc(msg, (strlen(msg)+n) * sizeof(char*));
    }         
    
    // Teste para falhas ACK nao chega e tem q retransmitir
    // int t = 0;
    // printf("Aperte enter para mandar ACK");
    // scanf("%d", &t);

    int _numBytesSent = send_datagram(sock, "ACK",  strlen("ACK"), (so_addr *) &sin6);
  
    msg[strlen(msg)-2] = '\0'; // remove o sinal de fim de mensagem \0 incluido no nome do arquivo
    printf("server received %d/%d bytes: %s\tsize of string: %lu\n", totalBytesRec, totalBytes, msg, strlen(msg));



    // ================ Ate aqui ta legal ================= //

















     /* 
     * Servidor recebeu nome do arquivo, agora vai enviar para o cliente
     */

    /* cruando Path para arquivo baseado no diretorio e filename*/    
    strcat(filepath, msg);


    FILE *file = fopen(filepath, "r"); 

    if(file == NULL){
        printf("ERRO ABRI FILE %s", filepath);
        exit(2); //Todo: passar pra funcao de erro
    }
    else
        printf("Arquivo %s aberto com sucesso\n", filepath);



    // ================ Ate aqui ta legal ================= //









    unsigned int totalBytesSent = 0;
    unsigned int totalMsgSent = 0;
    int  numBytesSent;    
    ssize_t nElem = 0;
    int chunck_size;    
    int times = 0;    
    bool initiated = false;
    char ack[30]; // "ok: num_seq"


    int err;
    sw_sock = sock;
    char *rec_buf = malloc(buffer_size * sizeof(char));
    sw_buf = rec_buf;
    sw_buf_size = buffer_size;
    from =  &sin6 ;    


    // Thread que ficara ouvindo os ACKs
    pthread_t tid[1];
    err = pthread_create(&(tid[0]), NULL, &receiveACKs, NULL);

    // Thread pra reenvio em janela deslizante
    pthread_t janelas[9999];

    /* enviando arquivo em chunks */
    int seqId = 0;

    int LFS = 0;
    
    int SeqNum = -1;
    char *datagrama = malloc(buffer_size * sizeof(char));
    int mayRead = 1;







    // Laco para enviar os blocos do arquivo para o cliente
    int count = 0;
    int once = 0;
    pthread_t window[SWS];
    struct arg_struct args;

    while(1) {

        if(mayRead){
            nElem = fread(buf, sizeof(char), buffer_size-20, file);       
            chunck_size =  nElem < buffer_size ? nElem : buffer_size;  
            printf("chuck size: %d, strlen buf: %lu, buf size: %d, nElen: %lu\n", chunck_size, strlen(buf), buffer_size, nElem);
      
            if (nElem <= 0 && !feof(file)) { 
              printf("Erro Leitura\n");
              break; // error leitura Todo: mandar pra funcao
            }

            

            SeqNum++;

            SeqNum = SeqNum % SWS;
            int crc = tcp_checksum(buf, chunck_size, (so_addr *) &sin6, (so_addr *) &sin6);
            sprintf(datagrama, "%010d%010d%s", SeqNum, crc, buf);
            //printf("\t\tDatagram(%s), Lenght(%lu)\n", datagrama, strlen(datagrama));
          

            LFS = SeqNum;

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
        
        
        if(window_space(SWS)){
            mayRead = 1;

            numBytesSent = send_datagram(sock, datagrama,  chunck_size, (so_addr *) &sin6);

            args.so = sock;
            args.buff = datagrama ;
            args.buff_len = chunck_size ;
            args.from_addr = (so_addr*) &sin6 ;
            args.seq = SeqNum ;

            if (pthread_create(&window[SeqNum], NULL, &_send, (void *)&args) != 0) {
                printf("Erro ao criar Thread!\n");
                return -1;
            }
            
        } else {
            mayRead = 0;
        }
    }
  
       
          printf("Arquivo: %s enviado para o cliente em %d bytes\n", msg, totalBytesSent);

          


          numBytesSent = send_datagram(sock, "\\0",  strlen("\\0"), (so_addr *) &sin6);









    fclose(file);
    shutdown(sock, 2);
    close(sock);
    pthread_exit(NULL);
    return 0;

}  