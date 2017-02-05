/* tp_socket.c - funções auxiliares para uso de sockets UDP
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h> // memset

#include "tp_socket.h"

#define MTU 1024 

int tp_mtu(void)
{
    fprintf(stderr,"tp_mtu called\n");
    /*******************************************************
     * A alteração do MTU oferecido para o PJD pode ser    *
     * feita alterando-se o valor da macro usada a seguir. *
     *******************************************************/
    return MTU;
}

int tp_sendto(int so, char* buff, int buff_len, so_addr* to_addr)
{
    int count;
    fprintf(stderr,"tp_sendto called (%d bytes)\n", buff_len);
    /*******************************************************
     * Aqui seria um bom lugar para injetar erros a fim de *
     * exercitar a funcionalidade do protocolo da camada   *
     * acima (o PJD).                                      *
     *******************************************************/
    count = sendto(so, (void*)buff, buff_len, 0,
            (struct sockaddr*) to_addr, sizeof(so_addr));
    fprintf(stderr,"tp_sendto returning (sent %d bytes)\n", count);
    return count;
    
}

int tp_recvfrom(int so, char* buff, int buff_len, so_addr * from_addr) {
    int count;
    unsigned int sockaddr_len = sizeof(so_addr);
    fprintf(stderr,"tp_recvfrom called (%d bytes)\n",buff_len);
    /*******************************************************
     * Aqui seria um bom lugar para injetar erros a fim de *
     * exercitar a funcionalidade do protocolo da camada   *
     * acima (o PJD).                                      *
     *******************************************************/
    count = recvfrom(so,(void*)buff,(size_t)buff_len,0,
            (struct sockaddr*) from_addr, &sockaddr_len);
    fprintf(stderr,"tp_recvfrom returning (received %d bytes)\n",count);
    return count;
}

int tp_init(void) {
    fprintf(stderr,"tp_init called\n");
    /*********************************************************
     * Exceto para fins de automatizar os testes com versões *
     * alteradas deste código (para injetar erros), não deve *
     * haver motivo para se alterar esta função.             *
     *********************************************************/
    return 0;
}

/*****************************************************
 * A princípio não deve haver motivo para se alterar *
 * as funções a seguir.                              *
 *****************************************************/

int tp_socket(unsigned short port) {
    int     so;
    so_addr local_addr;
    int     addr_len =sizeof(local_addr);

    fprintf(stderr,"tp_socket called\n");
    if ((so=socket(AF_INET6, SOCK_DGRAM, 0))<0) {
        return -1;
    }
    if (tp_build_addr(&local_addr, NULL, port)<0) {
        return -2;
    }
    if (bind(so, (struct sockaddr *) &local_addr, sizeof(local_addr))<0) {
        return -3;
    }
    return so;
}

int tp_build_addr(so_addr * addr, char* hostname, int port) {

    fprintf(stderr,"tp_build_addr called\n");
    memset(addr, 0, sizeof(so_addr)); // Zero out structure
    addr->sin6_family = AF_INET6; // IPv6 address family
    addr->sin6_port   = htons(port);
    if (hostname==NULL) {
        addr->sin6_addr = in6addr_any;
    } else {
        /*** falta verificar se está funcionando corretamente ***/
        int status;
        struct addrinfo hints;
        struct addrinfo * result;
        memset(&hints, 0, sizeof hints); // make sure the struct is empty
        hints.ai_family   = AF_INET6;    // IPv6
        hints.ai_socktype = SOCK_DGRAM;  // UCP sockets
        hints.ai_flags    = AI_PASSIVE;  // fill in my IP for me

        char portc[6];
        sprintf(portc,"%d",port);

        if ((status = getaddrinfo(hostname, portc, &hints, &result)) != 0) {
            freeaddrinfo(result);
            fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
            return -1;
        } else {
            struct sockaddr_in6 * ipv6 = (struct sockaddr_in6 *)result->ai_addr;
            memcpy(&(addr->sin6_addr), &ipv6->sin6_addr,sizeof(struct in6_addr));
            freeaddrinfo(result);
        }        
    }
    return 0;
}
