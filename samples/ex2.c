/**
 * cc ex-2.c -lev -lpthread -o ex-2
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <ev.h>
#include <pthread.h>

#define BUFSIZE         1024

static char buf[BUFSIZE];

ev_io stdin_watcher;

pthread_mutex_t r3_mutex = PTHREAD_MUTEX_INITIALIZER;

// all watcher callbacks have a similar signature
// this callback is called when data is readable on stdin
static void stdin_cb (EV_P_ ev_io *w, int revents)
{
    pthread_mutex_lock(&r3_mutex);
    if (fgets(buf, BUFSIZE, stdin)) {
        if (strncmp("quit\n", buf, 4) == 0) {
            ev_io_stop(EV_A_ w);
            ev_break(EV_A_ EVBREAK_ALL);
        }
        fprintf(stdout, "Input: ");
        fputs(buf, stdout);
    }
    pthread_mutex_unlock(&r3_mutex);
}

void thread1_func(int *arg)
{
    while (1) {
        fprintf(stdout, "Thread-1: ");
        pthread_mutex_lock(&r3_mutex);
        fputs(buf, stdout);
        pthread_mutex_unlock(&r3_mutex);
        sleep(2);
    }
}

void thread2_func(int *arg)
{
    while (1) {
        time_t t;

        srand((unsigned) time(&t));
        pthread_mutex_lock(&r3_mutex);
        sprintf(buf, "%d\n", rand());
        fprintf(stdout, "Thread-2: ");
        fputs(buf, stdout);
        pthread_mutex_unlock(&r3_mutex);
        sleep(5);
    }
}

int main (void)
{
    struct ev_loop *loop = EV_DEFAULT;
    pthread_t thread1, thread2;

    ev_io_init (&stdin_watcher, stdin_cb, STDIN_FILENO, EV_READ);
    ev_io_start (loop, &stdin_watcher);

    pthread_create(&thread1,
            NULL,
            (void *) thread1_func,
            (void *) NULL);
    pthread_create(&thread2,
            NULL,
            (void *) thread2_func,
            (void *) NULL);

    // now wait for events to arrive
    ev_run (loop, 0);

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);

    // break was called, so exit
    return 0;
}
