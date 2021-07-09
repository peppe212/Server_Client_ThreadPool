//
// Created by Giuseppe Muschetta on 07/07/21
// written on M1 (ARM)
//

#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#define SOCKET_NAME "./mysocket"
#define UNIX_PATH_MAX 104
#define BUFFER_SIZE 64
#define OFFSET 96;
#define MYNANOSLEEP 10000000

#define syscall(a,b)                    \
            if((a) == -1){              \
                perror(b);              \
                exit(EXIT_FAILURE);     \
            }


void mynanosleep(long value){
    struct timespec ts;
    ts.tv_sec=0;
    ts.tv_nsec=value;
    nanosleep(&ts,NULL);
}

char *generateRandom(){
    char *string = (char*)malloc((BUFFER_SIZE)*(sizeof(*string)));
    int i=0;
    while(i < (BUFFER_SIZE/4)) {
        string[i] = (1 + rand() % 25) + OFFSET;
        mynanosleep(MYNANOSLEEP);
        assert(string[i] < 123);
        i++;
    }
    assert(i == 16);
    string[i] = '\0';
    return string;
}


int main(void){
    srand(time(NULL));
    int fd_skt;
    char message[BUFFER_SIZE];

    struct sockaddr_un sa;
    strncpy(sa.sun_path,SOCKET_NAME,UNIX_PATH_MAX);
    sa.sun_family=AF_UNIX;

    //ottengo il socket di conversazione:
    fd_skt = socket(AF_UNIX,SOCK_STREAM,0);

    //connect:
    while( (connect(fd_skt,(struct sockaddr *)&sa,sizeof(sa))) == -1){
        if(errno == ENOENT){
            sleep(1);
            continue;
        }
        else{
            perror(NULL);
            exit(EXIT_FAILURE);
        }
    }

    //"protocollo"di conversazione col server:
    //il client saluta il server appena connesso
    write(fd_skt,"ciao!!!",8);
    //il client invia al server una stringa minuscola da uppare
    char *sendMeToServer = generateRandom();
    write(fd_skt,sendMeToServer,BUFFER_SIZE);
    //il server invia al client la stringa uppata
    read(fd_skt,sendMeToServer,BUFFER_SIZE);
    fprintf(stdout,"Il server scrive: %s\n",sendMeToServer);
    //il client ringrazia il server per l'arduo lavoro
    write(fd_skt,"grazie Server!",15);
    //il server dice al client arrivederci
    read(fd_skt,message,BUFFER_SIZE);
    fprintf(stdout,"Il server scrive: %s\n",message);
    //il client chiude la connessione
    mynanosleep(MYNANOSLEEP);
    close(fd_skt);
}

