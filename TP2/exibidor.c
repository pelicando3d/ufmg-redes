#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

typedef enum {false, true} bool;

/*Pacote - byte-array*/
typedef struct package {
    uint32_t SYNC1, SYNC2; //0xDCC023A1 e 0xDCC023B2
    uint16_t TYPE, ID_SRC, ID_DST, SEQ_N, D_LENGTH;
    uint8_t  ID, *DATA;
    bool h2ns;
} Package;


void hton(Package *p) {
    p->SYNC1 = htonl(p->SYNC1);
    p->SYNC2 = htonl(p->SYNC2);
    p->TYPE  = htons(p->TYPE);
    p->ID_SRC  = htons(p->ID_SRC);
    p->ID_DST  = htons(p->ID_DST);
    p->SEQ_N  = htons(p->SEQ_N);
    p->D_LENGTH  = htons(p->D_LENGTH);
}

void ntoh(Package *p) {
	p->SYNC1 = ntohl(p->SYNC1);
    p->SYNC2 = ntohl(p->SYNC2);
    p->TYPE  = ntohs(p->TYPE);
    p->ID_SRC  = ntohs(p->ID_SRC);
    p->ID_DST  = ntohs(p->ID_DST);
    p->SEQ_N  = ntohs(p->SEQ_N);
    p->D_LENGTH  = ntohs(p->D_LENGTH);
}

void send_package(Package *p, int socket) {
    uint32_t nbytes = 0;
    hton(p); // convertendo pacote para formado de rede, networkByteOrder
    send(socket, &(p->SYNC1),  sizeof(p->SYNC1),  0);
    send(socket, &(p->SYNC2),  sizeof(p->SYNC2),  0);
    send(socket, &(p->TYPE), sizeof(p->TYPE), 0);
    send(socket, &(p->ID_SRC), sizeof(p->ID_SRC), 0);
    send(socket, &(p->ID_DST),     sizeof(p->ID_DST),     0);
    send(socket, &(p->SEQ_N),  sizeof(p->SEQ_N),  0);
    send(socket, &(p->D_LENGTH),  sizeof(p->D_LENGTH),  0);
    ntoh(p); // convertendo de volta
    if(p->D_LENGTH > 0) nbytes = send(socket, p->DATA,  p->D_LENGTH,  0);
    printf("[socket] - Pacote Enviado: [%x][%x]-[%x][%x][%x][%x][%x]\n", p->SYNC1, p->SYNC2, p->TYPE, p->ID_SRC, p->ID_DST, p->SEQ_N, p->D_LENGTH);
    printf("[socket] - Pacote Enviado: %x%x%x%x%x%x%x\n", p->SYNC1, p->SYNC2, p->TYPE, p->ID_SRC, p->ID_DST, p->SEQ_N, p->D_LENGTH);
}


void recv_package(Package *p, int socket) {
    p->h2ns = false;
    recv(socket, &(p->SYNC1),   4, 0);
    recv(socket, &(p->SYNC2),   4, 0);
    recv(socket, &(p->TYPE),    2, 0);
    recv(socket, &(p->ID_SRC),  2, 0);
    recv(socket, &(p->ID_DST),  2, 0);
    recv(socket, &(p->SEQ_N),   2, 0);
    recv(socket, &(p->D_LENGTH),2, 0);
    ntoh(p);
    printf("[socket] - Pacote recebido: [%x][%x]-[%x][%x][%x][%x][%x]\n", p->SYNC1, p->SYNC2, p->TYPE, p->ID_SRC, p->ID_DST, p->SEQ_N, p->D_LENGTH);
}


int main(int argc, char** argv){;
  int clientSocket;
  char buffer[1024];
  struct sockaddr_in serverAddr;
  socklen_t addr_size;
  char *ip_server, *port;
  uint16_t my_id;

  if(argc < 2) {
  	// ToDo: tratar erro
  	return 1;
  }

  // Dividindo argumento 1 em porta e endereco ip
  ip_server = strtok (argv[1],":");
  port = strtok (NULL, ":");



  // Criando socket para em modo TCP
  clientSocket = socket(PF_INET, SOCK_STREAM, 0);
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(port));
  serverAddr.sin_addr.s_addr = inet_addr(ip_server);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  addr_size = sizeof serverAddr;
  connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);


  // Criando pacote OI
  Package OI;
  OI.SYNC1    = 0xDCC023A1;
  OI.SYNC2    = 0xDCC023B2;
  OI.TYPE     = 3;
  OI.ID_SRC   = 0;
  OI.ID_DST   = 65535; // id fixo do servidor
  OI.SEQ_N    = 0;
  OI.D_LENGTH = 0;

  // Enviando OI para o servidor
  send_package(&OI, clientSocket);

  // Recebendo OK ou ERRO do servidor
  Package IN;
  recv_package(&IN, clientSocket);
  if (IN.TYPE == 1) {
      my_id = IN.ID_DST;
      printf("Conectado ao servidor com ID = %u\n", my_id);
  } else if (IN.TYPE == 2) {
    // error, tratar de alguma forma
  }

  bool release = false;
  while (!release) {
    printf("\n\t=== Aguardando nova mensagem ===\n");
    recv_package(&IN, clientSocket);
    switch (IN.TYPE) {
      case 1: // OK
        // do something
      break;
      case 2: // ERRO
        // do something
      break;
      case 3: //OI
        // do something
      break;
      case 4: // FLW
        // do something
        printf("Recebendo pedido de desconexao de : %u\n", IN.ID_SRC);
        release = true;
      break;
      case 5: // MSG
        // do something
        printf("Nova Mensagem de: %u\n", IN.ID_SRC);
        printf("  %s\n", IN.DATA);
      break;
      case 6: // CREQ
        // do something
      break;
      case 7: // CLIST
        // do something
      break;
      default:
        // do something
      break;

    }
  }


  return 0;
}
