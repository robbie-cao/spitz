#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/queue.h>

#include <curl/curl.h>
#include <sys/stat.h>

#include "log.h"


#define BUFFER_SIZE         256

static char buf[BUFFER_SIZE];

pthread_t thread_stdin, thread_download, thread_upload;
pthread_mutex_t mutex_download = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_upload = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv_download = PTHREAD_COND_INITIALIZER;
pthread_cond_t cv_upload = PTHREAD_COND_INITIALIZER;

struct tailhead *headp;                 /*  Tail queue head. */
struct entry {
    TAILQ_ENTRY(entry) entries;         /*  Tail queue. */
    char data[BUFFER_SIZE];
};

TAILQ_HEAD(tailhead, entry) head_dq, head_uq;

struct progress {
    double lastruntime;
    CURL *curl;
};

#define MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL     3

/* this is how the CURLOPT_XFERINFOFUNCTION callback works */
static int xferinfo(void *p,
        curl_off_t dltotal, curl_off_t dlnow,
        curl_off_t ultotal, curl_off_t ulnow)
{
    struct progress *myp = (struct progress *)p;
    CURL *curl = myp->curl;
    double curtime = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

    /* under certain circumstances it may be desirable for certain functionality
       to only run every N seconds, in order to do this the transaction time can
       be used */
    if((curtime - myp->lastruntime) >= MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL) {
        myp->lastruntime = curtime;
        fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
    }

    fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
            "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
            "\r\n",
            ulnow, ultotal, dlnow, dltotal);

    return 0;
}

struct MemoryStruct {
    char *memory;
    size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    printf("\r\nGET RESPONSE:%s\r\n",(char *)contents);

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    free(mem->memory);

    return realsize;
}

int file_upload(char *file_path)
{
    CURL *curl;
    CURLcode res;
    struct stat file_info;
    double speed_upload, total_time;
    FILE *fd;

    struct MemoryStruct chunk;
    struct progress prog;

    chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
    chunk.size = 0;    /* no data at this point */

    fd = fopen(file_path, "rb"); /* open file to upload */
    if (!fd) {
        return 1; /* can't continue */
    }

    /* to get the file size */
    if (fstat(fileno(fd), &file_info) != 0) {
        return 1; /* can't continue */
    }

    curl = curl_easy_init();

    if (!curl) {
        fclose(fd);
        return 1;
    }

    /*   */
    struct curl_httppost* post = NULL;
    struct curl_httppost* last = NULL;

    curl_formadd(&post, &last, CURLFORM_COPYNAME, "filename", CURLFORM_FILE, file_path, CURLFORM_END);

    /* upload to this place */
    curl_easy_setopt(curl, CURLOPT_URL,"http://test.muabaobao.com/record/upload");

    /* send all data to this function  */
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

    /* we pass our 'chunk' struct to the callback function */
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

    /* tell it to "upload" to the URL */
    curl_easy_setopt(curl, CURLOPT_HTTPPOST, post);

    /* set where to read from (on Windows you need to use READFUNCTION too) */
    //curl_easy_setopt(curl, CURLOPT_READDATA, fd);

    /* and give the size of the upload (optional) */
    curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_info.st_size);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)file_info.st_size);

    /* enable verbose for easier tracing */
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, xferinfo);
    /* pass the struct pointer into the xferinfo function, note that this is
       an alias to CURLOPT_PROGRESSDATA */
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    res = curl_easy_perform(curl);

    /* Check for errors */
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n",
                curl_easy_strerror(res));

    } else {
        /* now extract transfer info */
        curl_easy_getinfo(curl, CURLINFO_SPEED_UPLOAD, &speed_upload);
        curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &total_time);

        fprintf(stderr, "Speed: %.3f bytes/sec during %.3f seconds\n",
                speed_upload, total_time);
    }

    /* always cleanup */
    curl_easy_cleanup(curl);
    curl_formfree(post);
    fclose(fd);

    return 0;
}


void thread_stdin_handler(int *arg)
{
    const char *LOG_TAG = "IN";
    struct entry *item;

    fd_set readfds;

    (void) arg;     // Make compiler happy

    fprintf(stdout, "-> T - Input\n");

    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);

    while (1) {
        if (select(1, &readfds, NULL, NULL, NULL)) {
            int ret = scanf("%s", buf);

            if (ret) {
                LOGD(LOG_TAG, "%s\n", buf);
            }

            switch (buf[0]) {
                case '1':
                    if (strlen(buf + 1)) {
                        LOGD(LOG_TAG, "Wakeup T1\n");
                        item = malloc(sizeof(*item));
                        strncpy(item->data, buf + 1, BUFFER_SIZE);
                        LOGD(LOG_TAG, "Add to dl queue - item: %s\n", item->data);
                        pthread_mutex_lock(&mutex_download);
                        TAILQ_INSERT_TAIL(&head_dq, item, entries);
                        pthread_cond_signal(&cv_download);
                        pthread_mutex_unlock(&mutex_download);
                    }
                    break;
                case '2':
                    LOGD(LOG_TAG, "Wakeup T2\n");
                    if (strlen(buf + 1)) {
                        LOGD(LOG_TAG, "Wakeup T2\n");
                        item = malloc(sizeof(*item));
                        strncpy(item->data, buf + 1, BUFFER_SIZE);
                        LOGD(LOG_TAG, "Add to ul queue - item: %s\n", item->data);
                        pthread_mutex_lock(&mutex_upload);
                        TAILQ_INSERT_TAIL(&head_uq, item, entries);
                        pthread_cond_signal(&cv_upload);
                        pthread_mutex_unlock(&mutex_upload);
                    }
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

    (void) arg;     // Make compiler happy

    fprintf(stdout, "-> T - Download\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1) {
        pthread_mutex_lock(&mutex_download);
        if (TAILQ_EMPTY(&head_dq)) {
            LOGD(LOG_TAG, "Sleeping\n");
            pthread_cond_wait(&cv_download, &mutex_download);
        }
        LOGD(LOG_TAG, "Wakeup\n");
        item = TAILQ_FIRST(&head_dq);
        LOGD(LOG_TAG, "Retrieve from dl queue - item: %s\n", item->data);
        TAILQ_REMOVE(&head_dq, head_dq.tqh_first, entries);
        pthread_mutex_unlock(&mutex_download);

        // TODO
        {
            // Test
            // URL input from stdin
            // eg: 1http://chrishumboldt.com/
            char cmd[512];
            snprintf(cmd, sizeof(cmd), "wget %s", item->data);
            system(cmd);
        }
        free(item);
    }
    fprintf(stdout, "<- T - Download\n");
}

void thread_upload_handler(int *arg)
{
    const char *LOG_TAG = "UL";
    struct entry *item;

    (void) arg;     // Make compiler happy

    fprintf(stdout, "-> T - Upload\n");
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    while (1) {
        pthread_mutex_lock(&mutex_upload);
        if (TAILQ_EMPTY(&head_uq)) {
            LOGD(LOG_TAG, "Sleeping\n");
            pthread_cond_wait(&cv_upload, &mutex_upload);
        }
        LOGD(LOG_TAG, "Wakeup\n");
        item = TAILQ_FIRST(&head_uq);
        LOGD(LOG_TAG, "Retrieve from ul queue - item: %s\n", item->data);
        TAILQ_REMOVE(&head_uq, head_uq.tqh_first, entries);
        pthread_mutex_unlock(&mutex_upload);

        // TODO
        {
            // Test
            file_upload(item->data);
        }
        free(item);
    }
    fprintf(stdout, "<- T - Upload\n");
}

int main (void)
{
    fprintf(stdout, "Welcome - %s %s\n", __DATE__, __TIME__);

    TAILQ_INIT(&head_dq);                   /*  Initialize the queue. */
    TAILQ_INIT(&head_uq);                   /*  Initialize the queue. */

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
