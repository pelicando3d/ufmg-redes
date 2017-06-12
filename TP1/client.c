#include "base.c"
  // #ToDo Inserir zeros para calculo do CHECKSUM para viabilizar msg de tamanho impar


int main(int argc, char** argv){
    int ssocket, insocket, newSocket, port;  
    char buffer[bufsize];
    char *c_port, *hostaddr, *in_name, *out_name;
    struct sockaddr_in serverAddr;  
    bool server = false;
    //toDo check num arguments

    if(argv[1][0] == '-' && argv[1][1] == 's') {
        c_port = argv[1];
        in_name = argv[2];
        out_name = argv[3];
        server = true;
    } else if (argv[1][0] == '-' && argv[1][1] == 'c') {
        // Spliting first argument into host addr and port 
        hostaddr = strtok (argv[1],":");
        c_port = strtok (NULL, ":");
        in_name = argv[2];
        out_name = argv[3];
    } else {
        int a;
        //todo error in arguments
        return -1;
    }
   
    if(server) {
        initiate_passiveConnection(&insocket, &serverAddr, port);
        if(listen(insocket,5)==0)
            printf("Listening\n");
        else
           printf("Error\n");

        /*---- Accept call creates a new socket for the incoming connection ----*/
        addr_size = sizeof serverStorage;
        newSocket = accept(insocket, (struct sockaddr *) &serverStorage, &addr_size);

        printf("Connection Accepted\n");
        receive_file(out_name, newSocket);

        close(inocket);    
        close(newSocket);
    } else {
        //initiate_passiveConnection(&clientSocket, &serverAddr, c_port, hostaddr);  
        initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);
        send_file(in_name, ssocket);
        close(ssocket);
    } 

    return 0;
}


