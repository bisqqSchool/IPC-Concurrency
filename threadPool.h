#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#define _GNU_SOURCE // For pthread_barrier
#include <pthread.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "list.h"

#define MAX_THREADS 4
#define MAX_CHAR_COUNT 1024

typedef struct ThreadPool {
    pthread_t         threadPool[MAX_THREADS];   // Four threads per routine
    pthread_mutex_t   listMutex;                 // Only one shared resource (Nodes)
    pthread_cond_t    clientListCondition;       // Condition variable for signaling
    pthread_cond_t    remoteListCondition;       // Condition variable for signaling
} ThreadPool;

typedef struct UDP {
    struct sockaddr_in  remoteAddress;
    struct sockaddr_in  clientAddress;
    List*               pRemoteList;
    List*               pClientList;
    uint16_t            remotePort;
    uint16_t            clientPort;
    int                 socket;
    const char*         remoteMachineName;
} UDP;

// Setup Udp and destroy it
void udpInitialize(UDP* pUdp);
void destroyUdp(UDP* pUdp);

// Setup thread pool and destroy it
void threadPoolInitialize(ThreadPool* pThreadPool);
void destroyThreadPool(ThreadPool* pThreadPool);
void createThreadRoutine(ThreadPool* pThreadPool, UDP* pUdp);

#endif
