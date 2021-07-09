//
// Created by Giuseppe Muschetta on 07/07/21
// written on M1 (ARM)
//

#include "tpool.h"


//il cuore di tutto il funzionamento del threadpool
void *tpWorker(void *arg){
    tpool_t *tp = (tpool_t*)arg;
    task_t *task = NULL;

    while(true){
        //ricordo che la risorsa condivisa e' il threadpool stesso
        pthread_mutex_lock(&(tp->mutex));     //LOCK

        while( (tp->head == NULL && !tp->stop) ){
            //aspetto finche' il tpool resta attivo e non ho neanche un task in coda
            pthread_cond_wait(&(tp->startWorking),&(tp->mutex));
        }
        //a questo punto almeno un task sara' presente nella coda implementata FIFO
        //e almeno un thread se ne sta occupando

        //la funzione threadPoolDestroy settera' il booleano a true per terminare il threadpool
        if(tp->stop)
            break;

        task = getTask(tp);
        tp->working_threads++;
        pthread_mutex_unlock(&(tp->mutex));   //UNLOCK


        //i threads del tpool iniziano a svolgere il lavoro della funzione che gli passeremo!
        if(task != NULL){
            task->foo(task->arg);
            destroyTask(task);
        }


        pthread_mutex_lock(&(tp->mutex));     //LOCK
        tp->working_threads--;
        //signal on wait function
        if(!tp->stop && tp->working_threads == 0 && tp->head == NULL){
            pthread_cond_signal(&(tp->finishWorking));
        }
        pthread_mutex_unlock(&(tp->mutex));   //UNLOCK

    }//end_while

    tp->total_threads--;
    pthread_cond_signal(&(tp->finishWorking));
    pthread_mutex_unlock(&(tp->mutex));      //UNLOCK  for while breaker threads
    return NULL;
}


tpool_t *createThreadPool(int max_threads){

    tpool_t *tp = (tpool_t*)malloc(sizeof(*tp));
    if(tp == NULL){
        errno = ENOMEM;
        perror("Error");
        exit(EXIT_FAILURE);
    }

    //inizializzo tutta la struct dopo averla allocata in memoria
    tp->stop=false;
    tp->head=NULL;
    tp->tail=NULL;
    tp->total_threads = max_threads;
    tp->working_threads=0;

    pthread_mutex_init(&(tp->mutex),NULL);
    pthread_cond_init(&(tp->startWorking),NULL);
    pthread_cond_init(&(tp->finishWorking),NULL);

    pthread_t thread;
    int i;
    for(i=0;i<max_threads;i++){
        pthread_create(&thread,NULL,tpWorker,tp);
        pthread_detach(thread);
    }

    return tp;
}

void threadPoolDestroy(tpool_t *tp){
    if(tp == NULL)
        return;

    pthread_mutex_lock(&(tp->mutex));
    task_t *corr = tp->head;
    task_t *prec = NULL;

    //dealloco ogni elemento della coda
    while(corr!=NULL){
        prec = corr->next;
        destroyTask(corr);
        corr = prec;
    }
    tp->stop = true;
    pthread_cond_broadcast(&(tp->startWorking));
    pthread_mutex_unlock(&(tp->mutex));

    tpoolWait(tp);

    //distruggo mutex e variabili di condizione
    pthread_mutex_destroy(&(tp->mutex));
    pthread_cond_destroy(&(tp->finishWorking));
    pthread_cond_destroy(&(tp->startWorking));

    //a questo punto dealloco il threadpool stesso
    free(tp);
}


task_t *createTask(tpool_function foo, void *arg){

    task_t *task = (task_t*)malloc(sizeof(*task));
    if(task == NULL){
        fprintf(stdout,"malloc failed at %d\n",__LINE__);
        exit(EXIT_FAILURE);
    }

    task->next=NULL;
    task->arg=arg;
    task->foo=foo;

    return task;
}


void destroyTask(task_t *task){
    if(task == NULL)
        return;
    free(task);
}


//si aggiunge in coda
bool threadPoolAddTask(tpool_t *tp, tpool_function foo, void *arg){
    if(tp == NULL)
        return false;

    //creiamo un task e poi lo aggiungiamo in coda
    task_t *task = createTask(foo,arg);
    assert(task != NULL);

    pthread_mutex_lock(&(tp->mutex));    //MUTEX LOCK
    if(tp->tail != NULL){
        tp->tail->next = task;
        tp->tail = task;
    }
    else{
        tp->head = task;
        tp->tail = tp->head;
    }

    pthread_cond_broadcast(&(tp->startWorking));
    pthread_mutex_unlock(&(tp->mutex));  //MUTEX UNLOCK
    return true;
}

//si prende dalla testa
task_t *getTask(tpool_t *tp){
    if(tp == NULL)
        return NULL;

    task_t *toReturn = tp->head;
    if(toReturn == NULL)
        return NULL;

    if(toReturn->next == NULL){
        tp->head = NULL;
        tp->tail = NULL;
    }
    else{
        tp->head = tp->head->next;
    }
    return toReturn;
}

void tpoolWait(tpool_t *tp){
    if(tp == NULL)
        return;
    pthread_mutex_lock(&(tp->mutex));
    while( (!tp->stop && tp->working_threads != 0) || (tp->stop && tp->total_threads != 0) ){
        pthread_cond_wait(&(tp->finishWorking),&(tp->mutex));
    }
    pthread_mutex_unlock(&(tp->mutex));
}
