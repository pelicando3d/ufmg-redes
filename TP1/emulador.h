#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdint.h>
#include <netdb.h>
#include <sys/errno.h>  /* para errno, perror */
#include <sys/time.h>   /* para setitimer */
#include <signal.h>   /* para signal */

const uint32_t bufsize = 1024; // min 14 por causa dos headers
const int timeout = 1000; // em millisegundos (1 segundo total)
typedef enum {arguments, listening, arquivo_null} errorID;
typedef enum {false, true} bool;
bool reenviar = false;
bool blocked = false;
bool debug = false;
bool more_to_send = true;
bool more_to_recv = true;

/*Pacote - byte-array e utilizado no calculo do checksum*/
typedef struct package {
    uint32_t SYNC, SYNC1, SYNC2;
    uint16_t LENGTH, CHKSUM;
    uint8_t  ID, FLAGS, *DATA, byte_array[14];
    bool h2ns;
} Package;


/* Funcao para ativar alarme de acordo com o timeout (milisegundos)*/    
void mysettimer(int milisegundos);
/* Funcao chamada ao se atigir o timeout. Ativa o reenvio de pacotes via flag reenviar*/
void unlocker_handler(int signum);
/* Funcao que substitui a chamada de unlocker_handler no caso em que o ACK chegou*/
void stop_timer(int signum);
/* Mensagens de erro */
void error_msg(errorID ID);
/* Copiar dados do pacote para um byte array para o calculo posterior do checksum*/
void pack(Package *p);
/* Copia parcial dos dados do pacote para um byte array*/
void repack(Package *p);
/* Converte a ordem dos bytes de todos os elementos do pacote host -> network*/
void hton(Package *p);
/* Converte a ordem dos bytes de todos os elementos do pacote network -> host*/
void ntoh(Package *p);
/* Inicializar socket no lado passivo da conexao */
void initiate_passiveConnection(int *csocket, struct sockaddr_in* server, char* cport);
/* Messagem de erro*/
void die(uint32_t id);
/* Inicializar socket no lado ativo da conexao*/
void initiate_activeConnection(int *ssocket, struct sockaddr_in* server, char* port, char* server_addr);
/* checksum carry out*/
unsigned carry_around_add(unsigned a, unsigned b);
/* Calculo de checksum, adaptacao da versao em Python (moodle)*/
uint16_t checksum(uint8_t *headers, uint8_t *data, uint16_t len);
/* Calculo de checksum, versao do livro texto*/
uint16_t cksum(uint8_t *headers, uint8_t *data, uint16_t len);
/* Enviar todos os dados do pacote para o destino */
void send_package(Package *p, int socket);
/* Receber um pacote da rede*/
void recv_package(Package *p, int socket);
/* Processo de envio e recebimento de pacotes - inicializacao ativa*/
void send_receive_file(char* in_name, char * out_name , int ssocket) ;
/* Processo de envio e recebimento de pacotes - inicializacao passiva*/
void receive_send_file(char* in_name, char* out_name , int ssocket);













