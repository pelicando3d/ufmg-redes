#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

typedef enum {false, true} bool;

typedef enum {exibidor, emissor} client_type;


/*Pacote - byte-array*/
typedef struct package {
  uint32_t SYNC1, SYNC2; //0xDCC023A1 e 0xDCC023B2
  uint16_t TYPE, ID_SRC, ID_DST, SEQ_N, D_LENGTH;
  uint8_t  ID, *DATA;
  bool h2ns;
} Package;

/*Associacao de IDs, IPs, FDs*/
struct assoc;

typedef struct assoc {
  uint16_t id;
  int fd, port;
  char ip[20];
  bool alive;
} Assoc;

void erase_assoc(Assoc *a) {
  printf("\nAssociacao Desfeita: ID = %u --> %s:%d - fd = %d\n", a->id, a->ip, a->port, a->fd);
  free(a);
}

void associate(Assoc *a, uint16_t id, int fd, int port, char* ip){
  a->id   = id;
  a->fd   = fd;
  a->port = port;
  memcpy(a->ip, ip, strlen(ip));
  a->alive = true;
  printf("\nNova Associacao: ID = %u --> %s:%d - fd = %d\n", a->id, a->ip, a->port, a->fd);
}

uint16_t get_free_id(uint16_t *ids_in_use) {
  uint16_t i = 0;
  for (; i < 65534; i++) {
    if(ids_in_use[i] == 0) {
      ids_in_use[i] = 1;
      return i;
    }
  }
  return -1; // unsigned, nao da certo, ToDo: Fix it!
}

void release_id(uint16_t *ids_in_use, uint16_t id) {
  if(id < 65534)
    ids_in_use[id] = 0;
}

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
}

void construct_package(Package *p, char *buffer) {
  p->h2ns = false;
  p->SYNC1 = 0; p->SYNC2 = 0; p->TYPE = 0; p->ID_SRC = 0;
  p->ID_DST = 0; p->SEQ_N = 0; p->D_LENGTH = 0;
  memcpy(&(p->SYNC1), &(buffer[0]), sizeof(p->SYNC1));
  memcpy(&(p->SYNC2), &(buffer[4]), sizeof(p->SYNC2));
  memcpy(&(p->TYPE), &(buffer[8]), sizeof(p->TYPE));
  memcpy(&(p->ID_SRC), &(buffer[10]), sizeof(p->ID_SRC));
  memcpy(&(p->ID_DST), &(buffer[12]), sizeof(p->ID_DST));
  memcpy(&(p->SEQ_N), &(buffer[14]), sizeof(p->SEQ_N));
  memcpy(&(p->D_LENGTH), &(buffer[16]), sizeof(p->D_LENGTH));
  ntoh(p);
  printf("[socket] - Pacote recebido: [%x][%x]-[%d][%d][%d][%d][%d]\n", p->SYNC1, p->SYNC2, p->TYPE, p->ID_SRC, p->ID_DST, p->SEQ_N, p->D_LENGTH);
}

int main(int argc , char *argv[]) {
  int opt = true;
  int master_socket , addrlen , new_socket , client_socket[1000] , total_clients = 0 , activity, i , valread , sd;
  int max_sd;
  struct sockaddr_in address;
  Assoc **assoc_array; // Um array com enderecos de associacoes
  int16_t port;
  uint8_t buffer[1025];
  fd_set readfds; //set de descritores de socket
  char *message = "TESTE SERVIDOR\n";
  uint32_t myID = 65535; // determinado na especificacao
  uint32_t total_received_bytes = 0, active_connections = 0, assoc_size = 30;
  uint8_t full_received_header[18];
  uint16_t current_index = 0, ids_in_use[65534]; // nao e uma boa pratica --> asuahushaus

  memset(ids_in_use, 0, sizeof(ids_in_use));

  // Todo: Checar validade de parametros passados ao programa
  port = atoi(argv[1]);

  // Inicializando os nossos clientes, que serao alocados dinamicamente
  // Em um teste inicial e estatico, verifiquei que podemos ter ate 60
  // conexoes simultaneas no servidor, entao estou utilizando este valor
  // como um teto para a alocacao estatica
  for (i = 0; i < 1000; i++) {
    client_socket[i] = 0;
  }

  // criando socket principal responsavel por lidar com a funcao ACCEPT
  if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // habilitando o nosso socket para receber diferentes conexoes
  if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  // Tipo da rede
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // bind do socket para ip local e porta definida por parametro
  if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  printf("Servidor iniciado e escutando na porta %d \n", port);

  // especificando do backlog, maximo de conexoes pendentes
  if (listen(master_socket, 4) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // aceitando conexao de entrada
  addrlen = sizeof(address);
  puts("Experando conexao");

  // inicializando nosso array de associacoes com 30 possiveis clientes, caso
  // tenhamos mais clientes do que este valor, devemos realocar memoria
  assoc_array = malloc(30 * sizeof(*assoc_array));
  memset(assoc_array, 0, 30 * sizeof(*assoc_array));

  while(true) {
    // limpar nosso set de SOCKETS para clientes
    FD_ZERO(&readfds);

    // adicionando nosso socket principal (para accept) no set
    FD_SET(master_socket, &readfds);
    max_sd = master_socket;
    total_clients++;


    // adicionando nossa lista estatica maxima para o set de sockets
    for ( i = 0 ; i < total_clients ; i++) {
      // descritor do socket
      sd = client_socket[i];

      // se o descritor for valido, entao adiciona a lista de observacao
      if(sd > 0)
        FD_SET( sd , &readfds);

      // maior valor para descritores, a ser utilizado pela funcao select
        if(sd > max_sd)
          max_sd = sd;
        }

      // Esperando por atividades em algum dos sockets, sem timeout
      activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);

      if ((activity < 0) && (errno!=EINTR)) {
        printf("select error");
      }

      // Qualquer situacao associada ao nosso socket principal deve ser um conexao de entrada
      if (FD_ISSET(master_socket, &readfds)) {
        if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
          perror("accept");
          exit(EXIT_FAILURE);
        }
        total_clients++;

        //Dados do cliente que solicitou conexao, como o ID, IP e PORTA
        // ID = new_socket
        // IP = inet_ntoa(address.sin_addr)
        // PORTA = ntohs(address.sin_´port)
        printf("Novo exibidor conectado, socket fd %d , ip : %s , porta : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

          // send new connection greeting message
          //if( send(new_socket, message, strlen(message), 0) != strlen(message) ) {
          //    perror("send");
          //}

          // adicionando novo cliente a nossa lista de clientes
          for (i = 0; i < total_clients; i++) {
            // se a posicao estiver livre
            if( client_socket[i] == 0 ) {
              client_socket[i] = new_socket;
              //printf("adicionado a lista de sockets com posicao %d\n" , i);
              // O ID pode ser o proprio id do socket ou o I nesse array, definir isso
              break;
            }
          }
        }

        // Caso a mudanca seja em algum cliente, entao temos de analisar o pacote
        for (i = 0; i < total_clients; i++)  {
          sd = client_socket[i];

          // ToDo> Todo as operacoes abaixo precisao ser individualizadas para CADA CLIENTE
          if (FD_ISSET( sd , &readfds)) {
            // checar se a conexao foi interrompida, se sim, fechar socket
            // 18 bytes e o tamanho do pacote sem DATA

            if ((valread = read( sd , buffer, 18)) == 0) {
              getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
              printf("Host desconectado %d , ip %s , porta %d \n" , sd, inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

              // fechar socket e liberar posicao no array
              total_clients--;
              close( sd );
              client_socket[i] = 0;

              int i = 0;
              for(; i < assoc_size; i++) {
                if(assoc_array[i] != 0 && assoc_array[i]->fd == sd) {
                  release_id(ids_in_use, assoc_array[i]->id);
                  erase_assoc(assoc_array[i]);
                  assoc_array[i] = 0;
                  break;
                }
              }
            }

            // caso a conexao nao tenha sido finalizada, vamos processar o pacote de entrada
            else {
              // verificando se recebemos um header inteiro (18 bytes) ou so parte dele

              if(total_received_bytes == 0) { // estamos esperando um pacote completo (header)
                total_received_bytes = valread;
                uint16_t i = 0;
                for(; i < valread; i++) {
                  full_received_header[i+current_index] =
                            buffer[i+current_index];
                }
                current_index += i;
              } else {                        // estamos esperando parte do header que esta incompleto
                total_received_bytes += valread;
                uint16_t i = 0;
                for(; i < valread; i++) {
                  full_received_header[i+current_index] =
                            buffer[i+current_index];
                }
                current_index += i;
              }

              if(total_received_bytes == 18) {
                Package IN;
                construct_package(&IN, full_received_header);

                if(IN.SYNC1 == 0xDCC023A1 && IN.SYNC2 == 0xDCC023B2) {
                  // tratar pacote
                  if(IN.TYPE ==  3) { // Tipo OI
                    if(IN.ID_DST == 65535) { // Se o destino esta corretamente setado como o servidor
                      if(IN.ID_SRC == 0) { // Exibidor se conectando
                        // Alocar ID para este cliente, mapealo e enviar OK para ele
                        int i = 0;
                        for(; i < assoc_size; i++) {
                          // se nao encontrar uma associacao ativa, criar uma
                          if(assoc_array[i] == 0) {
                            Assoc *new_assoc = malloc(sizeof(*new_assoc));
                            uint16_t client_id = get_free_id(ids_in_use);
                            associate(new_assoc, client_id, new_socket, ntohs(address.sin_port), inet_ntoa(address.sin_addr));
                            assoc_array[i] = new_assoc;
                            // Agora que associou, vamos enviar a msg OK de volta ao cliente
                            Package OK;
                            OK.SYNC1    = 0xDCC023A1;
                            OK.SYNC2    = 0xDCC023B2;
                            OK.TYPE     = 1;
                            OK.ID_SRC   = 65535;
                            OK.ID_DST   = client_id;
                            OK.SEQ_N    = IN.SEQ_N;
                            OK.D_LENGTH = 0;
                            
                            send_package(&OK, assoc_array[i]->fd);

                            break;
                          } else {
                            // checar se praquele socket ja existe alguma associacao
                            // esse caso nao deve ocorrer
                          }
                        }
                      }
                    }
                  }
                }
                total_received_bytes = 0;
                current_index = 0;
                memset(&full_received_header, 0, sizeof(full_received_header));
              }

            }
          }
        }
    }
    return 0;
}
