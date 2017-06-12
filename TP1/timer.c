#include <stdio.h>      /* para fprintf, fgetln */
#include <stdlib.h>     /* para atoi, exit */
#include <sys/errno.h>  /* para errno, perror */
#include <sys/time.h>   /* para setitimer */
#include <signal.h>   /* para signal */
#include <string.h>   /* para strlen */

typedef enum {false, true} bool;
bool reenviar = false;
bool blocked = false;
    
void mysettimer(int milisegundos) {
    struct itimerval newvalue, oldvalue;

    newvalue.it_value.tv_sec  = milisegundos / 1000;
    newvalue.it_value.tv_usec = milisegundos % 1000 * 1000;
    newvalue.it_interval.tv_sec  = 0;
    newvalue.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &newvalue, &oldvalue);
}

void unlocker_handler(int signum) {
    printf(" ######################### Desbloquenado reenvio #################################\n");
    reenviar = true;
    mysettimer(1000);
}

void stop_timer(int signum) {
    printf("Timeout para o ultimo pacote foi desativado\n");
    return ;
}

