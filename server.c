//
// Created by Giuseppe Muschetta on 07/07/21
// written on M1 (ARM)
//

#include "tpool.h"
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#define UNIX_PATH_MAX 104
#define SOCKET_NAME "./mysocket"
#define BUFFER_SIZE 64

#define syscall(a,b)                    \
            if((a) == -1){              \
                perror(b);              \
                exit(EXIT_FAILURE);     \
            }


//struct che serve per passare gli argomenti alla funzione del threadpool
typedef struct data{
    int connectionCounter;
    int fd_client;
}data_t;


//variabili volatile sig_atomic_t da utilizzare nel signalHandlder
volatile sig_atomic_t closed = 0; //booleano


//la funzione che opera il server:
void toup(char *string) {
    assert(string);
    int i = 0;
    while (string[i] != '\0') {
        if(islower(string[i])) {
            string[i] = (char) toupper(string[i]);
        }
        i++;
    }
}


//la funzione usata dal threadpool per gestire tutta la comunicazione col client
void tpoolFunction(void *arg){
    //gestisco la consversazione con i client usando un pool di threads
    data_t info = *(data_t*)arg;
    char message[BUFFER_SIZE];
    char *toBeUpped = calloc(BUFFER_SIZE,sizeof(*toBeUpped));

    fprintf(stdout,"Il thread al lavoro sul client %d e' %ld\n",
            info.connectionCounter,(long)pthread_self());

    //PROTOCOLLO DI CONVERSAZIONE CON IL CLIENT:
    //il client i-esimo saluta il server a connessione instaurata
    syscall( (read(info.fd_client,message,BUFFER_SIZE)),"read")
    fprintf(stdout,"Il client %d scrive: %s\n",info.connectionCounter,message);
    //il client invia al server una stringa in minuscolo da uppare
    syscall( (read(info.fd_client,toBeUpped,BUFFER_SIZE)),"read")
    fprintf(stdout,"Il client scrive: la stringa da uppare e' %s\n",toBeUpped);
    toup(toBeUpped);
    //il server invia al client la stringa uppata
    syscall( (write(info.fd_client,toBeUpped,BUFFER_SIZE)),"write")
    //i client ringrazia il server e chiude la conversazione
    syscall( (read(info.fd_client,message,BUFFER_SIZE)),"read")
    fprintf(stdout,"Il client %d scrive: %s\n",info.connectionCounter,message);
    //il server saluta il client
    syscall ( (write(info.fd_client,"arrivederci!!!",15)),"write")
    //il server chiude la conversazione col client
    syscall( (close(info.fd_client)),"close fd_client")
    //possiamo deallocare la memoria della struct argomenti
}

//only safe atomic functions and volatile sig_atomic_t variables
//quando arriva SIGINT o SIGQUIT il server iniziera' la fase di terminazione
void signalHandler(int signum){
    switch(signum){
        case SIGINT:
            write(STDOUT_FILENO,"SIGINT arrived... ",19);
            closed = 1;
            break;
        case SIGQUIT:
            write(STDOUT_FILENO,"SIGQUIT arrived... ",20);
            closed = 1;
            break;
        default:
            break;
    }
    printf("\n");
    //I segnali potevano anche essere gestiti con un thread handler e la SC sigwait
    //in tal caso si potevano evitare le variabili atomiche e le funzioni safe
}


int main(int argc, char **argv) {
    (void)unlink(SOCKET_NAME);
    //E'necessario passare un argomento che stabilisca il num di threads da usare
    if(argc < 2){
        fprintf(stderr,"Expecting one argument ./appname <arg> \n");
        exit(EXIT_FAILURE);
    }

    //GESTIONE MINIMALE DEI SEGNALI:
    //metodologia con sigaction e variabili volatile sig_atomic_t
    struct sigaction sig;
    //resetto la struct
    memset(&sig,0,sizeof(sig));
    sig.sa_handler = SIG_IGN;
    //ignoro SIGPIPE
    sigaction(SIGPIPE,&sig,NULL);

    //gestisco SIGINT (Ctrl + C) E SIGQUIT (Ctrl + \)
    sig.sa_handler = signalHandler;
    syscall((sigaction(SIGINT,&sig,NULL)),"sigaction to handle SIGINT");
    syscall((sigaction(SIGQUIT,&sig,NULL)),"sigaction to handle SIGQUIT");
    //---------------------------------------- end signal handling


    //parsing dell'argomento:
    long threads = strtol(argv[1],NULL,10);
    if(threads < 1){
        threads = 2;
    }

    //CREAZIONE DEL THREADPOOL:
    tpool_t *tp = createThreadPool((int)threads);

    //CREAZIONE DEL SERVER che utilizzera' il threadpool:
    int fd_skt;
    struct sockaddr_un sa;
    //memset(&sa,0,sizeof(sa));
    strncpy(sa.sun_path,SOCKET_NAME,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;

    //ottengo il listen socket
    syscall( (fd_skt = socket(AF_UNIX,SOCK_STREAM,0)),"listen socket creation")
    //bind, listen, accept
    syscall( (bind(fd_skt,(struct sockaddr *)&sa,sizeof(sa))),"bind")
    syscall( (listen(fd_skt,SOMAXCONN)),"listen")

    int connectionCounter=0;
    while(!closed) {
        data_t info;
        //ACCEPT con controllo su EINTR
        if( (info.fd_client = accept(fd_skt,NULL,0)) == -1 ){
            if(errno == EINTR){
                sleep(1);
                continue;
            }
            else{
                perror("accept");
                exit(EXIT_FAILURE);
            }
        }
        connectionCounter++;
        printf("Accettata connessione con il client %d\n", connectionCounter);
        info.connectionCounter = connectionCounter;
        //QUI SI AGGIUNGE LAVORO DA AFFIDARE AL THREADPOOL
        threadPoolAddTask(tp, tpoolFunction, &info);
    }
    //se arriviamo qui vuol dire che abbiamo terminato il server:
    printf("SERVER IS SHUTTING DOWN\n");
    close(fd_skt);
    printf("Ho chiuso il listen socket...\n");
    //possiamo eliminare anche il threadpool
    //qui dentro c'e' la free del tp
    threadPoolDestroy(tp);
    printf("Ho deallocato completamente il threadpool...\n");
    printf("Terminazione del server avvenuta correttamente!\n");
    //--------------------------------------  end of Server managed by the Threadpool
    return EXIT_SUCCESS;
}
