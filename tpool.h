//
// Created by Giuseppe Muschetta on 07/07/21
// written on M1 (ARM) iMac
//

#ifndef THREADPOOL_CLIENT_SERVER_TPOOL_H
#define THREADPOOL_CLIENT_SERVER_TPOOL_H


#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


//la funzione che usera' il threadpool
typedef void (*tpool_function)(void *arg);

//tipo dati rappresentante una linked list
//sara' usata come una coda fifo in cui elimino in testa e aggiungo in coda
typedef struct task{
    //ogni task avra' la funzione del pool thread
    tpool_function foo;
    //l'argomento della funzione
    void *arg;
    //il puntatore al successivo elemento
    struct task *next;
}task_t;


typedef struct tpool{
    task_t *head;
    task_t *tail;
    bool stop;
    pthread_mutex_t mutex;
    pthread_cond_t startWorking;
    pthread_cond_t finishWorking;
    int total_threads;
    int working_threads;
}tpool_t;


//protoipi di funzione
tpool_t *createThreadPool(int max_threads);
void threadPoolDestroy(tpool_t *tp);
task_t *createTask(tpool_function foo, void *arg);
void destroyTask(task_t *task);
bool threadPoolAddTask(tpool_t *tp, tpool_function foo, void *arg); //faro' signal su startWorking
task_t *getTask(tpool_t *tp);
void tpoolWait(tpool_t *tp);


#endif //THREADPOOL_CLIENT_SERVER_TPOOL_H
