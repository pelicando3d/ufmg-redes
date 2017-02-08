/* impaciente.c - exemplo de uso de sinais para temporização
 * Este programa não tem relação direta com os demais disponibilizados,
 * apenas ilustra como funciona o sistema de temporização por sinais e
 * apresenta algumas funções para facilitar o seu uso.
 *
 * Tente entender como ele funciona antes de executá-lo e experimente
 * alterá-lo como quiser.
 */
#include <stdio.h>      /* para fprintf, fgetln */
#include <stdlib.h>     /* para atoi, exit */
#include <sys/errno.h>  /* para errno, perror */
#include <sys/time.h>   /* para setitimer */
#include <signal.h>   /* para signal */
#include <string.h>   /* para strlen */

int    espera;

void
mysettimer(int milisegundos)
{
    struct itimerval newvalue, oldvalue;

    newvalue.it_value.tv_sec  = milisegundos / 1000;
    newvalue.it_value.tv_usec = milisegundos % 1000 * 1000;
    newvalue.it_interval.tv_sec  = 0;
    newvalue.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &newvalue, &oldvalue);
}

void
timer_handler(int signum)
{
    fprintf(stderr,"Anda logo %d!\n", signum);
    /* Uma outra opção seria setar uma variável global
     * e testá-la no loop principal do programa, mas isso
     * só funcionaria nos casos onde a chamada bloqueada 
     * fosse interrompida pelo sinal
     * (o que não acontece no caso do teclado, pelo menos).
     */
    mysettimer(espera);  /* Melhor lugar para reiniciar o timer */
}

void
mysethandler(void)
{
    signal(SIGALRM,timer_handler);
}

int retransmit = 0;
int sw_ack = 0;

void stopwait(int signum){    
    if(!sw_ack){
      retransmit = 1;
      mysettimer(espera);
    }
}

void
stopwaitHandler(void)
{
    signal(SIGALRM,stopwait);
}


void
Janela_Deslizante_Handler(void){
    if(!sw_ack){
      retransmit = 1;
      mysettimer(espera);
    }    
}

int main2(int argc, char* argv[]) {
    char   entrada[512];
    size_t elen;
    int    done; /* Controle do loop */

    espera = (argc == 2)? atoi(argv[1]): 1000;

    mysethandler(); /* Só precisa ser feito uma vez */
    mysettimer(espera);

    done = 0;
    while (!done) {
        errno = 0; /* Só por garantia */
        fprintf(stderr,"Escreva algo:\n");
        if (fgets(entrada,sizeof(entrada),stdin) == NULL ) {
            /* fgets retorna erro se a entrada estava vazia ou se deu erro */
            /* chamadas de socket só retornam < 0 se deu erro */
            if (errno==EINTR) {
                printf("\ninterrompida\n");
                /* uma chamada interrompida seria tratada aqui */
                errno = 0;
            } else if (errno) {
                perror("fgets");
                exit(1);
            } else if (feof(stdin)) {
                fprintf(stderr,"Entrada vazia.\n");
                exit(0);
            }
        } else {
            done = 1;
        }
    }

   /*
    * Há dois comportamentos possíveis para funções que bloqueiam na 
    * ocorrência de um sinal: elas podem ser reiniciadas pelo kernel 
    * depois que o tratador for chamado, ou elas podem retornar com um 
    * valor de errno igual a EINTR. No caso atual, como a leitura de dados
    * é do teclado, o S.O. reinicia a chamada internamente. Isso tem o
    * efeito de apenas escrever "Anda logo!" a cada intervalo de tempo.
    * Se a chamada a fgets fosse interrompida, o resultado seria que
    * a cada sinalização do temporizador seriam também re-escrita
    * a mensagem "Escreva algo:" depois de cada "Anda logo!". 
    * Esse comportamento deve ser observado em chamadas a recvfrom,
    * por exemplo.
    */

    entrada[strlen(entrada)-1] = 0; /* retira '\n' do fim da linha */
    fprintf(stderr,"Entrada: \"%s\"\n",entrada);
}
