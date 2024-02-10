#include "threadPool.h"
#include <assert.h>

//===================================================================================
// Prototypes
//===================================================================================
static void* keyboardRoutine(void* args);
static void* udpSendRoutine(void* args);
static void* udpReceiveRoutine(void* args);
static void* screenOutputRoutine(void* args);

//===================================================================================
// Internal Structs/Enums
//===================================================================================
typedef struct ThreadArg {
    ThreadPool*  pThreadPool;
    UDP*         pUdp;
} ThreadArg;

// Easy thread/mutex tracking
typedef enum PoolType {
    POOL_TYPE_KEYBOARD_ROUTINE,
    POOL_TYPE_UDP_SEND_ROUTINE,
    POOL_TYPE_UDP_RECEIVE_ROUTINE,
    POOL_TYPE_SCREEN_OUTPUT_ROUTINE
} PoolType;

//===================================================================================
// Functions
//===================================================================================

void udpInitialize(UDP* pUdp) {

}

static void freeItem(void* pItem) {
    if (pItem != NULL) {
        free(pItem);
    }
}

void destroyUdp(UDP* pUdp) {
    assert(pUdp != NULL);

    if (pUdp->pClientList != NULL) {
        List_free(pUdp->pClientList, &freeItem);
    }

    if (pUdp->pRemoteList != NULL) {
        List_free(pUdp->pRemoteList, &freeItem);
    }

    // Close the socket
    close(pUdp->socket);
}

void threadPoolInitialize(ThreadPool* pThreadPool) {
    assert(pThreadPool != NULL);
    pthread_mutex_init(&pThreadPool->listMutex, NULL);
    pthread_cond_init(&pThreadPool->clientListCondition, NULL);
    pthread_cond_init(&pThreadPool->remoteListCondition, NULL);
}

void destroyThreadPool(ThreadPool* pThreadPool) {
    assert(pThreadPool != NULL);
    pthread_mutex_destroy(&pThreadPool->listMutex);
    pthread_cond_destroy(&pThreadPool->clientListCondition);
    pthread_cond_destroy(&pThreadPool->remoteListCondition);
}

void createThreadRoutine(ThreadPool* pThreadPool, UDP* pUdp) {
    assert(pThreadPool != NULL);
    assert(pUdp != NULL);
    
    int result = 0;
    
    ThreadArg threadArg;
    threadArg.pThreadPool = pThreadPool;
    threadArg.pUdp = pUdp;

    result = pthread_create(&pThreadPool->threadPool[POOL_TYPE_UDP_SEND_ROUTINE], NULL, udpSendRoutine, &threadArg);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %d\n", result);
        exit(EXIT_FAILURE);
    }

    result = pthread_create(&pThreadPool->threadPool[POOL_TYPE_UDP_RECEIVE_ROUTINE], NULL, udpReceiveRoutine, &threadArg);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %d\n", result);
        exit(EXIT_FAILURE);
    }

    result = pthread_create(&pThreadPool->threadPool[POOL_TYPE_KEYBOARD_ROUTINE], NULL, keyboardRoutine, &threadArg);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %d\n", result);
        exit(EXIT_FAILURE);
    }

    result = pthread_create(&pThreadPool->threadPool[POOL_TYPE_SCREEN_OUTPUT_ROUTINE], NULL, screenOutputRoutine, &threadArg);
    if (result != 0) {
        fprintf(stderr, "Error creating thread: %d\n", result);
        exit(EXIT_FAILURE);
    }

    // Join the four threads
    for (int i = 0; i < 4; i++) {
        if (pthread_join(pThreadPool->threadPool[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("\nThreads Joined\n");
}


//===================================================================================
// Routines
//===================================================================================

static void* keyboardRoutine(void* args) {
    return args;
}

static void* udpSendRoutine(void* args) {
    return args;
}

static void* udpReceiveRoutine(void* args) {
    return args;
}

static void* screenOutputRoutine(void* args) {
    return args;
}
