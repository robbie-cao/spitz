#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include <sys/select.h>
#include <sys/queue.h>

#include "log.h"


#define BUFFER_SIZE         256

static char buf[BUFFER_SIZE];

pthread_t thread_stdin, thread_download, thread_upload;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_download = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_upload = PTHREAD_COND_INITIALIZER;

struct tailhead *headp;                 /*  Tail queue head. */
struct entry {
    TAILQ_ENTRY(entry) entries;         /*  Tail queue. */
    char data[BUFFER_SIZE];
};

TAILQ_HEAD(tailhead, entry) head;

void thread_stdin_handler(int *arg)
{
    const char *LOG_TAG = "IN";
    struct entry *item;

    fd_set readfds;
    FD_ZERO(&readfds);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    fprintf(stdout, "-> T - Input\n");
    while (1) {
        FD_SET(STDIN_FILENO, &readfds);

        if (select(1, &readfds, NULL, NULL, NULL)) {
            scanf("%s", buf);
            LOGD(LOG_TAG, "%s\n", buf);

            switch (buf[0]) {
                case '1':
                    if (buf[1] == '1') {
                        LOGD(LOG_TAG, "Wakeup T1\n");
                        item = malloc(sizeof(*item));
                        strncpy(item->data, buf, BUFFER_SIZE);
                        LOGD(LOG_TAG, "Add to queue - item: %s\n", item->data);
                        pthread_mutex_lock(&mutex);
                        TAILQ_INSERT_TAIL(&head, item, entries);
                        pthread_cond_signal(&cv_download);
                        pthread_mutex_unlock(&mutex);
                    }
                    break;
                case '2':
                    LOGD(LOG_TAG, "Wakeup T2\n");
                    pthread_mutex_lock(&mutex);
                    pthread_cond_signal(&cv_upload);
                    pthread_mutex_unlock(&mutex);
                    break;
                default:
                    break;
            }
        }

        printf("...\n");
    }
    fprintf(stdout, "<- T - Input\n");
}

void thread_download_handler(int *arg)
{
    const char *LOG_TAG = "DL";
    struct entry *item;

    fprintf(stdout, "-> T - Download\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1) {
        pthread_mutex_lock(&mutex);
        if (TAILQ_EMPTY(&head)) {
            LOGD(LOG_TAG, "Sleeping\n");
            pthread_cond_wait(&cv_download, &mutex);
        }
        LOGD(LOG_TAG, "Wakeup\n");
        item = TAILQ_FIRST(&head);
        LOGD(LOG_TAG, "Retrieve from queue - item: %s\n", item->data);
        TAILQ_REMOVE(&head, head.tqh_first, entries);
        free(item);
        pthread_mutex_unlock(&mutex);

        // TODO
        sleep(5);
    }
    fprintf(stdout, "<- T - Download\n");
}

void thread_upload_handler(int *arg)
{
    const char *LOG_TAG = "UL";
    fprintf(stdout, "-> T - Upload\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1) {
        pthread_mutex_lock(&mutex);
        LOGD(LOG_TAG, "Sleeping\n");
        pthread_cond_wait(&cv_upload, &mutex);
        LOGD(LOG_TAG, "Wakeup\n");
        memset(buf, 0, sizeof(buf));
        pthread_mutex_unlock(&mutex);

        // TODO
    }
    fprintf(stdout, "<- T - Upload\n");
}

int main (void)
{
    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    TAILQ_INIT(&head);                      /*  Initialize the queue. */

    pthread_create(&thread_stdin,
            NULL,
            (void *) thread_stdin_handler,
            (void *) NULL);
    pthread_create(&thread_download,
            NULL,
            (void *) thread_download_handler,
            (void *) NULL);
    pthread_create(&thread_upload,
            NULL,
            (void *) thread_upload_handler,
            (void *) NULL);

    pthread_join(thread_stdin, NULL);
    pthread_join(thread_download, NULL);
    pthread_join(thread_upload, NULL);

    pthread_exit(NULL);

    return 0;
}
