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
  std::string message;
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
        cout << "connect() - Estabelecendo conexão...\n";
        sock = server_fd;
        break;
      }else{
        cout << "connect() failed";
      }

       close(server_fd);
    }

    if (rp == NULL) {               /* No address succeeded */
       fprintf(stderr, "Could not connect\n");
       exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);           /* No longer needed */


  }



// escuta resposta do servidor
std::string response_list() {
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  unsigned int numBytes = 0;
  std::string response;
  char buffer[buffer_size];  
  while (true) {
    numBytes = recv(sock, buffer, buffer_size - 1, 0);
    if (numBytes <= 0) break;
    buffer[numBytes] = '\0';
    if (totalBytesRcvd) response += buffer;
    totalBytesRcvd += numBytes;    
  }

  return response;
}

unsigned int response_get() {
  unsigned int totalBytesRcvd = 0; // Count of total bytes received
  unsigned int numBytes = 0;  

  FILE *file = fopen(filename, "w+"); // file to be downloaded will be written here
  char buffer[buffer_size+1]; // I/O buffer
  // FILE *file = fopen("teste_escrita", "ab"); // file to be downloaded will be written here

  while (true) {
    numBytes = recv(sock, buffer, buffer_size, 0);
    if (numBytes <= 0) break;
    fwrite(&buffer, 1, numBytes, file); // writes file
    totalBytesRcvd += numBytes;
  }
  fclose(file);
  return totalBytesRcvd;
}

//////////////////////////////////
// SEÇÃO REFERENTE AOS COMANDOS //
//////////////////////////////////

void list_files(std::string raw_response) {
  int i = 0;
  //printf("raw:\n%s\n",raw_response);
  cout << "\nLista de arquivos disponiveis para baixar: \n";
  while(1){   
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
    // std::cout << "Fechou cliente\n";
  }



  void init_transmission (){
    if (is_get){    
      gettimeofday (&t0, NULL);
      message = "get " + to_string(filename);
    }
    else
      message = "list\0" ;
  }

  // conecta com o servidor e envia a mensagem especificada via parametro
void send_message() {
  // enviando mensagem para o servidor 
  size_t messageLen = message.size();  // Determine input length
  ssize_t nbytes_sent = send(sock, message.c_str(), messageLen, 0);
  
  if (nbytes_sent < 0)
    cerr << "send() failed";
  else if (nbytes_sent != messageLen)
    cerr << "send()", "sent unexpected number of bytes";
  else
    cout << "send() - Enviando comando " << message << "(" << nbytes_sent << ") para o servidor...\n";  
}

  void receive_response(){
    cout << "AKI\n";
    if (is_get) {    
      unsigned int numBytes = response_get();
      gettimeofday (&t1, NULL);    
      float sec = t1.tv_sec-t0.tv_sec;
      float micro = t1.tv_usec-t0.tv_usec;
      float elapsed = ((float)(sec)*1000 + (float)(micro)/1000) / 1000;
      float kbps = numBytes/elapsed;
      //printf("Arquivo %s\tBuffer %5u byte, %10.2f kbps (%u bytes em %3u.%06u s) %3.6f(s)\n", filename, buffer_size, kbps, numBytes, sec, micro);
      printf("Arquivo %s\tBuffer %5u byte, %10.2f kbps (%u bytes em %3.6f s)\n", filename, buffer_size, kbps, numBytes, elapsed);
      //Utilizando o segundo printf, porque o primeiro retorna alguns valores errados em alguns casos.

    } else {
      std::string message = response_list();
      list_files(message);
    }
  }  

  void close_connection_with_server(){
    close(sock);
  }
};



void catch_errors(int argc, char** argv){
  if(argc < 5){
    cerr << "Number of arguments is incorrect!";
  }
  if (!strcmp(argv[1], "get") && argc != 6) {
    cerr << "get <Filename> <Server Address> <Server Port> <Buffer Size>";
  }
  if (!strcmp(argv[1], "list") && argc != 5) {
    cerr << "list <Server Address> <Server Port> <Buffer Size>";
  }
}

int main (int argc, char* argv[]){

  catch_errors(argc, argv);
  

  
  client c (argc, argv);

  c.init_transmission();
  cout << "AKI\n";
  c.send_message ();    
  c.receive_response();
  c.close_connection_with_server();

  return 0;
}
