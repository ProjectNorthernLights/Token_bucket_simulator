#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ctype.h>
#include <unistd.h>
#include <signal.h>

#include "token_bucket.h"
#include "MemoryManager.h"

void* server(void*);
void* packet_arriver(void*);
void* token_arriver(void*);
void* monitor(void*);
void* cleanup_queues(void*);

// global variables
pthread_mutex_t mutex, server_mutex;
pthread_cond_t cv;
long base_time;
sigset_t set;


pthread_t t1, t2, t3, monitor_t;
struct MemoryManager m;
struct packet_attributes pa;


int main(int argc, char* argv[])
{
    struct token_bucket_system s;
    mem_init(&m);
    pa.lambda = 0.5; pa.mu = 0.35; pa.P = 3;
    init(&s,20,10,1.5);

    parseInput(&pa, &s, argc, argv);
    pthread_mutex_init(&mutex, 0);
    pthread_mutex_init(&server_mutex, 0);
    pthread_cond_init(&cv, 0);

    // get the base time
    base_time = gettime();

    // mask the SIGINT signal
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGTERM);

    pthread_sigmask(SIG_BLOCK, &set, 0);

    // start the system
    printf("%09.3fms: emulation begins\n", 0.0f);
    // start the token arriver
    pthread_create(&t1, 0, token_arriver, (void*)&s);
    // start the packet arriver
    pthread_create(&t2, 0, packet_arriver, (void*)&s);
    // start the server
    pthread_create(&t3, 0, server, (void*)&s);
    // start the interrupt handler
    pthread_create(&monitor_t, 0, monitor, (void*)&s);

    // wait for the threads to finish
    pthread_join(t2, 0);
    pthread_join(t3, 0);

    mem_free(&m);

    return 0;
}

void *token_arriver(void *a)
{
    struct token_bucket_system *tbs = (struct token_bucket_system*)a;
    long curr_time;

    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);


    for(;;)
    {
        usleep(1000000.0f/tbs->r);
        pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthread_mutex_unlock, (void*)&mutex);
            tbs->tokens++;
            tbs->token_index++;
            curr_time = gettime();
            curr_time -= base_time;
            if(tbs->tokens > tbs->B)
            {
                tbs->tokens = tbs->B;
                pthread_testcancel();
                printf("%09.3fms: token%d arrives, dropped\n", (float)curr_time/1000.0f, tbs->token_index);
            }
            else
            {
                pthread_testcancel();
                printf("%09.3fms: token t%d arrives, token bucket now has %d tokens\n", (float)curr_time/1000.0f, \
                       tbs->token_index, tbs->tokens);
            }
            pthread_testcancel();

            if(!My402ListEmpty(&tbs->Q1))
            {
                My402ListElem *item = My402ListFirst(&tbs->Q1);
                void *data = item->obj;
                struct packet *pack = (struct packet*)data;
                if(tbs->tokens >= pack->P)
                {
                    My402ListUnlink(&tbs->Q1, item);
                    tbs->tokens = tbs->tokens - pack->P;
                    curr_time = gettime() - base_time;
                    pthread_testcancel();
                    printf("%09.3fms: p%d leaves Q1, time in Q1 = %.3f, token bucket now has %d tokens\n", (float)curr_time/1000.0f, \
                          ((struct packet*)data)->index, (float)(curr_time - ((struct packet*)data)->start_time)/1000.0f, \
                            tbs->tokens);
                    My402ListAppend(&tbs->Q2, data);
                    curr_time = gettime() - base_time;
                    pthread_testcancel();
                    printf("%09.3fms: p%d enters Q2\n", (float)curr_time/1000.0f, ((struct packet*)data)->index);
                    ((struct packet*)data)->start_time = curr_time;
                    pthread_testcancel();
                    pthread_cond_signal(&cv);
                }
             }

        pthread_mutex_unlock(&mutex);
        pthread_cleanup_pop(0);
    }// end for

    return 0;
}

void *packet_arriver(void *a)
{
    struct token_bucket_system *tbs = (struct token_bucket_system*)a;
    int i;
    long inter_time = 0, curr_time;
    char line[35];

    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);

    for(i = 0;i < tbs->number_of_packets;i++)
    {
        if(tbs->file_specified)
        {
            if(fgets(line, 35, tbs->fp))
            {
                tokenizeLine(line, &pa.lambda, &pa.P, &pa.mu);
            }// end if
        }// end if
        float t = 1000000.0f/pa.lambda;
        usleep(t);
        curr_time = gettime() - base_time;
        tbs->packet_index++;

        pthread_mutex_lock(&mutex);
            pthread_cleanup_push(pthread_mutex_unlock, &mutex);
            pthread_testcancel();
            printf("%09.3fms: p%d arrives, needs %d tokens, inter-arrival time = %.3fms\n", (float)curr_time/1000.0f, \
               tbs->packet_index, pa.P, (float)(curr_time - inter_time)/1000.0f);
            struct packet *p = (struct packet*)mem_alloc(&m, sizeof(struct packet));
            p->mu = pa.mu;
            p->P = pa.P;
            p->arrival_time = curr_time;
            inter_time = curr_time;
            p->index = tbs->packet_index;
            curr_time = gettime() - base_time;
            p->start_time = curr_time;
            pthread_testcancel();
            printf("%09.3fms: p%d enters Q", (float)curr_time/1000.0f, p->index);
            if(tbs->tokens >= pa.P)
                // directly enter the job into Q2
            {
                tbs->tokens -= pa.P;
                My402ListAppend(&tbs->Q2, (void*)p);
                printf("2\n");
                pthread_testcancel();
                pthread_cond_signal(&cv);
            }// end if
            else
            {
                My402ListAppend(&tbs->Q1, (void*)p);
                printf("1\n");
            }
        pthread_mutex_unlock(&mutex);
        pthread_cleanup_pop(0);
    }// end for

    return 0;
}

void *server(void *a)
{
    struct token_bucket_system *tbs = (struct token_bucket_system*)a;
    int i;
    long curr_time;

    pthread_sigmask(SIG_BLOCK, &set, 0);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, 0);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, 0);


    for(i = 0;i < tbs->number_of_packets; i++)
    {
        pthread_cleanup_push(cleanup_queues, a);
        pthread_testcancel();

        pthread_mutex_lock(&mutex);
            while(My402ListEmpty(&tbs->Q2))
                pthread_cond_wait(&cv, &mutex);
            // take the head of Q2 as the current job
        pthread_mutex_lock(&server_mutex);
            My402ListElem *e = My402ListFirst(&tbs->Q2);
            struct packet *p = (struct packet*)e->obj;
            My402ListUnlink(&tbs->Q2, e);
            curr_time = gettime() - base_time;
            printf("%09.3fms: p%d begins service as S, time in Q2 = %.3fms\n", (float)curr_time/1000.0f \
                   , p->index, (float)(curr_time - p->start_time)/1000.0f);
            p->start_time = curr_time;
        pthread_mutex_unlock(&mutex);
        usleep(1000000.0f/p->mu);

        pthread_mutex_lock(&mutex);
            curr_time = gettime() - base_time;
            printf("%09.3fms: p%d departs from S, service time = %.3fms time in system = %.3fms\n", (float)curr_time / 1000.0f \
                   , p->index, (float)(curr_time - p->start_time)/1000.0f, (float)(curr_time - p->arrival_time)/1000.0f);
        pthread_mutex_unlock(&mutex);
        pthread_mutex_unlock(&server_mutex);
        pthread_cleanup_pop(0);
    }// end for

    return 0;
}

void* monitor(void *a)
{
    int sig;

    sigwait(&set, &sig);

    pthread_cancel(t1);
    pthread_cancel(t2);

    pthread_mutex_lock(&server_mutex);
        pthread_cancel(t3);
    pthread_mutex_unlock(&server_mutex);

}

void* cleanup_queues(void *ptr)
{
    struct token_bucket_system *tbs = (struct token_bucket_system*)ptr;

    My402ListUnlinkAll(&tbs->Q1);
    My402ListUnlinkAll(&tbs->Q2);

    return 0;
}
