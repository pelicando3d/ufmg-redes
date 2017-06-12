#include "base.c"



int main(int argc, char **argv){
    int insocket, newSocket;
    char buffer[bufsize];
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    char* port, *in_name, *out_name;

    
    port = argv[1];
    in_name = argv[2];
    out_name = argv[3];
    

    initiate_passiveConnection(&insocket, &serverAddr, port);

    /*---- Listen on the socket, with 5 max connection requests queued ----*/
    if(listen(insocket,5)==0)
        printf("Listening\n");
    else
        printf("Error\n");

    /*---- Accept call creates a new socket for the incoming connection ----*/
    addr_size = sizeof serverStorage;
    newSocket = accept(insocket, (struct sockaddr *) &serverStorage, &addr_size);

    printf("Connection Accepted\n");

    receive_file(out_name, newSocket);

    
    return 0;
}
