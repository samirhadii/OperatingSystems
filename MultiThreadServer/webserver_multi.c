#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/socket.h>
#include <semaphore.h> // include support for semaphors
#include "webserver.h"

#define MAX_REQUEST 100

int port, numThread;

//initialize semaphores
sem_t full;
sem_t empty;


pthread_t *threads;
pthread_mutex_t mutex;
pthread_cond_t cond;
int job_queue[MAX_REQUEST];
int job_count = 0;
int job_front = 0;
int job_rear = 0;

void add_job(int fd) {
    //put semaphore wait empty here to make sure there is an available slot
    sem_wait(&empty);
    pthread_mutex_lock(&mutex);
    job_queue[job_rear] = fd;
    job_rear = (job_rear + 1) % MAX_REQUEST;
    job_count++;
    pthread_cond_signal(&cond);  // Signal worker threads
    pthread_mutex_unlock(&mutex);
    //add a semaphore_post here to show that an item has been added
    sem_post(&full);
}

void* worker(void* arg) {
    while (1) {
        //put semaphore wait full here to check if there is an item to process
        sem_wait(&full);
        pthread_mutex_lock(&mutex);
        while (job_count == 0) {
            pthread_cond_wait(&cond, &mutex);  // Wait for jobs
        }
        int fd = job_queue[job_front]; // grab a process from the job queue 
        job_front = (job_front + 1) % MAX_REQUEST;
        job_count--;
        pthread_mutex_unlock(&mutex);
        //add a semaphore post empty here to denote that a slot is available
        sem_post(&empty);
        process(fd);  // Process the connection
        close(fd);     // Close the connection when done
    }
    return NULL;
}

void listener(int port) {
    int sock;
    struct sockaddr_in sin;
    struct sockaddr_in peer;
    int peer_len = sizeof(peer);
    int r;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(port);
    r = bind(sock, (struct sockaddr*)&sin, sizeof(sin));
    if (r < 0) {
        perror("Error binding socket:");
        return;
    }

    r = listen(sock, 5);
    if (r < 0) {
        perror("Error listening socket:");
        return;
    }

    printf("HTTP server listening on port %d\n", port);
    while (1) {
        int fd;
        fd = accept(sock, NULL, NULL);
        if (fd < 0) {
            printf("Accept failed.\n");
            break;
        }
        add_job(fd);  // Add the connection to the job queue
    }
    close(sock);
}


int main(int argc, char *argv[])
{
	if (argc != 3 || atoi(argv[1]) < 2000 || atoi(argv[1]) > 50000)
	{
		fprintf(stderr, "./webserver_multi PORT(2001 ~ 49999) #_of_threads\n");
		return 0;
	}

	int i;
	port = atoi(argv[1]);
	numThread = atoi(argv[2]);

    // Allocate memory for the threads array
    threads = (pthread_t*)malloc(numThread * sizeof(pthread_t));

	// Initialize the mutex, semaphore, and condition variable
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    sem_init(&full, 0, 0);     // Initialize full semaphore to 0
    sem_init(&empty, 0, MAX_REQUEST);  // Initialize empty semaphore to the maximum number of slots

    // Create worker threads
    for (int i = 0; i < numThread; i++) {
        if (pthread_create(&threads[i], NULL, worker, NULL) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    listener(port);

    // Clean up resources
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);
    sem_destroy(&full);
    sem_destroy(&empty);

    // Free the memory for threads when done
    free(threads);

	return 0;
}
