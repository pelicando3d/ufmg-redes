#include <iostream>
#include <string>
#include <cstring>
#include <sstream>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <cassert>
#include <fstream>
#include <dirent.h>
#include <bitset>


using namespace std;

void split(const string &s, char delim, vector<string> &elems) {
	stringstream ss;
  ss.str(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
}

class server {
 
 private:
  struct sockaddr_in client_socket;
  int client_fd;
  socklen_t len;
 
  int port;
  int bufferSize;
  char* dir;
  char cport[128];
  
  void create_server_socket (){
 
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int s;
 
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET6;    /* IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
     
    s = getaddrinfo(NULL, cport, &hints, &result);
    if (s != 0) {
       fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
       exit(EXIT_FAILURE);
    }
 
    /* getaddrinfo() returns a list of address structures.
      Try each address until we successfully bind(2).
      If socket(2) (or bind(2)) fails, we (close the socket
      and) try the next address. */
    socket_fd = -1;
    bool valid_connection = false;
    for (rp = result; rp != NULL; rp = rp->ai_next) {
      socket_fd = socket(rp->ai_family, rp->ai_socktype,
               rp->ai_protocol);
      if (socket_fd == -1)
        continue;
  
      if (bind(socket_fd, rp->ai_addr, rp->ai_addrlen) == 0){
        cout << "Servidor iniciado. Porta:" << port << endl;
        valid_connection = true;
        // success
        break;
      }
 
      close(socket_fd);
    }
 
    if (not valid_connection) {
      perror ("Erro ao criar servidor");
      exit (1);
    }
  }
 
  void send_message (std::string message){
    send (client_fd, message.c_str(), strlen (message.c_str()), 0);
  }
 
  std::string receive_message () const{
    char buffer[bufferSize];
    memset (buffer, '\0', bufferSize);
    int nbytes = recv (client_fd, buffer, bufferSize, 0);
 
    if (nbytes < 0){
      cout << "recv() failed\n";
      return std::string();
    }
    else{
      std::string s (buffer);
      return s;
    }
 
  }
 
 public:
  int socket_fd;
  server (char* port, char* buffer, char* dir){
    this->port = atoi(port);
    this->bufferSize = atoi(buffer);
    this->dir = dir;
    strcpy (this->cport, port);
    create_server_socket();
  }
 
  ~server (){
    //cout << " chamou o destrutor\n ";
    //close (socket_fd);
    // std::cout << "Fechou servidor\n";
  }
 
  void listen_port (){
    if (listen (socket_fd, 1) < 0){
      cout << "Erro ao iniciar listener\n";
    }else{
      cout << "Listen() - Permitindo que socket ouça requisições de conexões!\nAguardando conexão...\n";
    }    
  }
 
  void accept_connection (){
    cout << " fd do server: " << socket_fd << endl;
    client_fd = accept (socket_fd, (struct sockaddr*)&(client_socket), &(len));
    if (client_fd < 0){
      cout << "accept() failed\n";
    }else{
      cout << "Accept() - Conexão recebida e aceita!\n";
    }
    cout << "cliente fd: " << client_fd << endl;
  }
 
  bool send_filelist(){
    DIR *pd = 0;
    struct dirent *pdirent = 0;
    char tmpBuf[bufferSize];
    pd = opendir(dir);
    if(pd == NULL) {
      // ERROR: wrong path
      return false; // define an erro CODE
    }  
    while (pdirent=readdir(pd)){
      //int n = sprintf(tmpBuf, "$%lu$%s", strlen(pdirent->d_name), pdirent->d_name);
      int n = sprintf(tmpBuf, "%s\\n", pdirent->d_name);
      ssize_t numBytesSent = send(client_fd, tmpBuf, strlen(tmpBuf), 0);
      if (numBytesSent < 0){
        cout << "send() failed";
      }else{
        cout << "send() - Enviando mensagem para o cliente (" << pdirent->d_name << ")!\n";
      }
    }
    ssize_t numBytesSent = send(client_fd, "\\0", 2, 0);
  }

  void send_file(string filename){
    char buffer[bufferSize];
    FILE *file = fopen(filename.c_str(), "rb"); // open in binary mode
    if(file == NULL) cout << "Erro ao abrir o arquivo " << filename << endl;
    unsigned int bytesRead = 0;
    unsigned int totalBytesSent = 0;
    
    ssize_t nElem = 0;
    while(1) {
      nElem = fread(&buffer, sizeof(char), bufferSize, file);        
      if (nElem <= 0 && !feof(file)) { 
        cout << "Reading error. Bytes read: " << bytesRead << endl; 
        return;
      }

      ssize_t numBytesSent = send(client_fd, buffer, nElem, 0);
      if (numBytesSent < 0){
        cout << "send() failed"; break;
      }else{
        totalBytesSent += numBytesSent;
        cout << "send() - Bytes ja enviados: " << totalBytesSent << endl;
      }

      if(feof(file)){
        cout << "End of file\n";
        break;
      }
    }
    fclose(file);
  }

  bool receive_from_client (){
    std::string str;
    while (true){
      str = receive_message();
      if(str == "list"){
        send_filelist();        
        return true;
      //if string starts with get
      }else if(str.compare(0, 3, "get") == 0){
     		//gets filename
				string filename = str.substr(4, str.length() - 4);
        send_file(filename);
        return true;
      }
    }
    return false;
  }
 
  void close_connection_with_client (){
    //cout << "close connection, socket value: " << socket_fd << endl;
    close(client_fd);
  }
   
};
 
 
int main (int argc, char* argv[]){
 
  server s (argv[1], argv[2], argv[3]);
  // server s;
  s.listen_port();
  while (true){
    s.accept_connection();
    while (true){
      bool status = s.receive_from_client();
      if (status)
        break;
    } 
    s.close_connection_with_client();
  }
 
 
  return 0;
}
