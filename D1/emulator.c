#include "base.c"
  // #ToDo Inserir zeros para calculo do CHECKSUM para viabilizar msg de tamanho impar


int main(int argc, char** argv){
    int ssocket, insocket, newSocket, port;  
    char buffer[bufsize];
    char *c_port, *hostaddr, *in_name, *out_name;
    struct sockaddr_in serverAddr;  
    socklen_t addr_size;
    bool server = false;
    struct sockaddr_storage serverStorage;  
    Threads_data *data;
    data = malloc(sizeof(Threads_data));

    //toDo check num arguments

    if(argv[1][0] == '-' && argv[1][1] == 's') {
        c_port = argv[2];
        in_name = argv[3];
        out_name = argv[4];

        initiate_passiveConnection(&insocket, &serverAddr, c_port);
        if(listen(insocket,5)==0)
            printf("Listening\n");
        else
           printf("Error\n");

        /*---- Accept call creates a new socket for the incoming connection ----*/
        addr_size = sizeof serverStorage;
        newSocket = accept(insocket, (struct sockaddr *) &serverStorage, &addr_size);

        printf("Connection Accepted\n");

        data->socket = newSocket;
        data->filename = in_name;
        int err = pthread_create(&(tid[0]), NULL, &send_file_thread, (void *) data);


        printf("############ SERVER SIDE RECEIVING FILE FIRST ################\n");
        receive_file(out_name, newSocket);
        printf("############ SERVER RECEIVED FILE ################\n");
        // ### INICIAR A PARTE DE ENVIO AQUI


        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");


//        send_file(in_name, newSocket);

        //

        close(insocket);    
        close(newSocket);
    } else if (argv[1][0] == '-' && argv[1][1] == 'c') {
        // Spliting first argument into host addr and port 
        hostaddr = strtok (argv[2],":");
        c_port = strtok (NULL, ":");
        in_name = argv[3];
        out_name = argv[4];

        //initiate_passiveConnection(&clientSocket, &serverAddr, c_port, hostaddr);  
        initiate_activeConnection(&ssocket, &serverAddr, c_port, hostaddr);

        data->socket = ssocket;
        data->filename = out_name;        
        int err = pthread_create(&(tid[0]), NULL, &receive_file_thread, (void *) data);


        printf("############ CLIENT SIDE SENDING FILE FIRST ################\n");
        send_file(in_name, ssocket);
        printf("############ CLIENT SENT FILE ################\n");


        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        else
            printf("\n Thread created successfully\n");


  //      receive_file(out_name, ssocket);

        close(ssocket);
        
    } else {
        int a;
        //todo error in arguments
        return -1;
    } 

    return 0;
}


