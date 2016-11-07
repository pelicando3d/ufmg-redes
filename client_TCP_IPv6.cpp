#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include <cassert>
#include <dirent.h>
#include <netdb.h>
#include <fstream>
#include <sstream>

using namespace std;

void die(int code);

class client {

 private:
  struct sockaddr_in server_socket; 
  struct sockaddr_in client_socket;
  int server_fd;
  socklen_t len;
  //srv_comm *scomm 

  int buffer_size;
  bool is_get;

  char filename[100]; /* filename - get command */
  char address[128]; /* Server address */
  char cport[128];
  int port;
  int sock;
  char *message;
  struct timeval t0, t1;

  template <typename T>
  std::string to_string(T value){
    std::ostringstream os ;
    os << value ;
    return os.str() ;
  }



  void connect_to_server (){
               /* Obtain address(es) matching host/port */

    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;

    //scomm = (srv_comm*) malloc(sizeof(srv_comm));
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;    /* IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = 0;

    s = getaddrinfo(address, cport, &hints, &result);
    if (s != 0) {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
       exit(EXIT_FAILURE);
    }

    for (rp = result; rp != NULL; rp = rp->ai_next) {
      server_fd = socket(rp->ai_family, rp->ai_socktype,
                    rp->ai_protocol);
      if (server_fd == -1)
           continue;

      if (connect(server_fd, rp->ai_addr, rp->ai_addrlen) != -1){
        // cout << "connect() - Estabelecendo conexão...\n";
        sock = server_fd;
        break;
      }else{
        // cout << "connect() failed";
        die(6);
      }

       close(server_fd);
    }

    if (rp == NULL) { // No address succeeded 
       die(6);
    }

    freeaddrinfo(result); 


  }



// escuta resposta do servidor
char* response_list() {
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  unsigned int numBytes = 0;
  char *response = (char*) malloc(buffer_size);
  response[0] = '\0';
  char buffer[buffer_size];

  while (true) {
    numBytes = recv(sock, buffer, buffer_size - 1, 0);    
    if (numBytes == 0)
      break;
    if(numBytes < 0)
      die(7);
    buffer[numBytes] = '\0';
    if (totalBytesRcvd)
      response = (char*) realloc(response, strlen(response) + strlen(buffer) + 1);
    strcat(response, buffer);
    totalBytesRcvd += numBytes;
  }

  return response;
}

unsigned int response_get() {
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  unsigned int numBytes = 0;  

  FILE *file = fopen(filename, "w+"); // file to be downloaded will be written here
  
  if (file == NULL) // Invalid file or file not found
    die(8);

  char buffer[buffer_size+1]; // I/O buffer

  while (true) {
    numBytes = recv(sock, buffer, buffer_size, 0);
    if (numBytes == 0)
      break;
    if (numBytes < 0)
      die(7);

    fwrite(&buffer, 1, numBytes, file); // writes file
    totalBytesRcvd += numBytes;
  }
  fclose(file);
  return totalBytesRcvd;
}

//////////////////////////////////
// SECTION TO HANDLE COMMANDS   //
//////////////////////////////////

char* prepare_message_get () {
  int msgLen = 5 + strlen(filename);
  char* message = (char*) malloc(msgLen * sizeof(char));

  if (message == NULL)
    die(999); //Error while allocating memory


  strcpy(message, "get ");
  strcat(message, filename);
  return message;
}

char* prepare_message_list () {
  char* message = (char*) malloc(5 * sizeof(char));

  if (message == NULL)
    die(999); //Error while allocating memory

  strcpy(message, "list");
  return message;
}

void list_files(char *raw_response) {
  int i = 0;
  //cout << "\nLista de arquivos disponiveis para baixar: \n";
  while(true){   
    if(raw_response[i] == '\\' && raw_response[i+1] == '0') break;
    if(raw_response[i] == '\\' && raw_response[i+1] == 'n'){
      cout << endl;
      i++;
    }
    else
      cout << raw_response[i];    
    i++;
  }  
}

 public:
  client (int argc, char** argv){
    int argv_offset = !strcmp(argv[1], "get") ? 1 : 0;
    if(argv_offset)
      strcpy (this->filename, argv[2]);
    strcpy (this->address, argv[argv_offset + 2]);      
    this->port = atoi(argv[argv_offset + 3]);
    this->buffer_size = atoi(argv[argv_offset + 4]);
    strcpy (this->cport, argv[argv_offset + 3]);
    this->is_get = argv_offset;
    connect_to_server();

    char hostname[100];
    size_t len = 100;
    gethostname (hostname, len);
  }


  ~client (){
    //close (server_fd);
  }



  void init_transmission (){
    if (is_get){    
      gettimeofday (&t0, NULL);
      message = prepare_message_get();
    }
    else
      message = prepare_message_list();
  }

  // conecta com o servidor e envia a mensagem especificada via parametro
void send_message() {
  // enviando mensagem para o servidor 
  std::string s = message + to_string("\\0");
  unsigned int bytesLeft = s.length();
  unsigned int pos, len;
  pos = len = 0;

  while(bytesLeft) {
    len = bytesLeft > (unsigned)buffer_size ? buffer_size : bytesLeft;
    ssize_t nbytes_sent = send(sock, s.c_str() + pos, len, 0); 
    
    if (nbytes_sent < 0)
      //cerr << "send() failed";
      die(7);
    else if (nbytes_sent != len)
      //cerr << "sent unexpected number of bytes";
      die(7);
    else {
      bytesLeft -= nbytes_sent;
      pos += nbytes_sent;
    }
  }
}

  void receive_response(){
    if (is_get) {    
      unsigned int numBytes = response_get();
      gettimeofday (&t1, NULL);    
      float sec = t1.tv_sec-t0.tv_sec;
      float micro = t1.tv_usec-t0.tv_usec;
      float elapsed = ((float)(sec)*1000 + (float)(micro)/1000) / 1000;
      float kbps = numBytes/elapsed;
      //printf("Arquivo %s\tBuffer %5u byte, %10.2f kbps (%u bytes em %3u.%06u s) %3.6f(s)\n", filename, buffer_size, kbps, numBytes, sec, micro);
      printf("Arquivo %s\tBuffer %u bytes, %10.2f kbps (%u bytes em %3.6f s)\n", filename, buffer_size, kbps, numBytes, elapsed);
      // printf("%5u %10.2f %u %3.6f\n", buffer_size, kbps, numBytes, elapsed);
      //Utilizando o segundo printf, porque o primeiro retorna alguns valores errados em alguns casos.

    } else {
      char *message = response_list();
      list_files(message);
    }
  }  

  void close_connection_with_server(){
    close(sock);
  }
};


void catch_errors(int argc, char** argv){
  if(argc < 5){
    die(1);
  }
  if (!strcmp(argv[1], "get") && argc != 6) {
    die(11);
  }
  if (!strcmp(argv[1], "list") && argc != 5) {
    die(12);    
  }
}

int main (int argc, char* argv[]){

  catch_errors(argc, argv);
  
  client c (argc, argv);

  c.init_transmission();
  c.send_message ();
  c.receive_response();
  c.close_connection_with_server();
  
  return 0;
}


void die(int code){
  switch(code){
    case 1:
      cerr << "Erro: -1 - Descrição: Erros nos argumentos de entrada\n";
      exit(-1);        
    case 2:
      cerr << "Erro: -2 - Descrição: Erro de criação de socket\n" ;
      exit(-2);
    case 3:
      cerr << "Erro: -3 - Descrição: Erro de bind\n" ;
      exit(-3);
    case 4:
      cerr << "Erro: -4 - Descrição: Erro de listen\n" ;
      exit(-4);
    case 5:
      cerr << "Erro: -5 - Descrição: Erro de accept\n" ;
      exit(-5);
    case 6:
      cerr << "Erro: -6 - Descrição: Erro de connect\n" ;
      exit(-6);
    case 7:
      cerr << "Erro: -7 - Descrição: Erro de comunicação com servidor/cliente\n" ;
      exit(-7);
    case 8:
      cerr << "Erro: -8 - Descrição: Arquivo solicitado não encontrado\n" ;
      exit(-8);
    case 9:
      cerr << "Erro: -9 - Descrição: Erro em ponteiro\n" ;
      exit(-9);
    case 10:
      cerr << "Erro: -10 - Descrição: Comando de clienteFTP não existente\n" ;
      exit(-10);
    case 11:
      cerr << "Erro: -11 - Descrição: Commando correto -> get <Filename> <Server Address> <Server Port> <Buffer Size>\n";
      exit(-11);
    case 12:
      cerr << "Erro: -12 - Descrição: Commando correto -> list <Server Address> <Server Port> <Buffer Size>\n";
      exit(-12);
    default:
      cerr << "Erro: -999 - Descrição: Error nao listado\n" ;
      exit(-999);
  }
}
