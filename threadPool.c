#include "threadPool.h"
#include <assert.h>
#include <netdb.h>
#include <poll.h>
#include <fcntl.h>

// Define a timeout value in seconds and microseconds
#define ERROR -1

static bool sClientTermination = false;
static bool sRemoteTermination = false;

//===================================================================================
// Prototypes
//===================================================================================
static void* keyboardRoutine(void* args);
static void* udpSendRoutine(void* args);
static void* udpReceiveRoutine(void* args);
static void* screenOutputRoutine(void* args);

static int timeoutUntilAvailable(int descriptor, int milliseconds);

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
    assert(pUdp != NULL);

    pUdp->socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (pUdp->socket == ERROR) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints;
    struct addrinfo* info;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;      // IPv4
    hints.ai_socktype = SOCK_DGRAM; // UDP socket
    hints.ai_flags = 0;
    hints.ai_protocol = 0;

    int status = getaddrinfo(pUdp->remoteMachineName, NULL, &hints, &info);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo failed for host '%s': %s\n", pUdp->remoteMachineName, gai_strerror(status));
        exit(EXIT_FAILURE);
    }

    // Retrieve the first address from info
    struct sockaddr_in* remote_addr = (struct sockaddr_in*)info->ai_addr;

    // Setup remote address
    memset(&pUdp->remoteAddress, 0, sizeof(struct sockaddr_in));
    pUdp->remoteAddress.sin_family = AF_INET;
    pUdp->remoteAddress.sin_port = htons(pUdp->remotePort);
    pUdp->remoteAddress.sin_addr = remote_addr->sin_addr;

    // Free the address info structure
    freeaddrinfo(info);

    // Setup client address
    memset(&pUdp->clientAddress, 0, sizeof(struct sockaddr_in));
    pUdp->clientAddress.sin_family = AF_INET;
    pUdp->clientAddress.sin_port = htons(pUdp->clientPort);
    pUdp->clientAddress.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address and port
    status = bind(pUdp->socket, (struct sockaddr*)&pUdp->clientAddress, sizeof(pUdp->clientAddress));
    if (status == ERROR) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Client listening on port %d...\n", pUdp->clientPort);

    pUdp->pClientList = List_create();
    assert(pUdp->pClientList != NULL);
    
    pUdp->pRemoteList = List_create();
    assert(pUdp->pRemoteList != NULL);
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
    for (int i = 0; i < MAX_THREADS; i++) {
        if (pthread_join(pThreadPool->threadPool[i], NULL) != 0) {
            fprintf(stderr, "Error joining thread\n");
            exit(EXIT_FAILURE);
        }
    }

    printf("\nThreads Joined\n");
}

//===================================================================================
// Routines Helpers
//===================================================================================

static int timeoutUntilAvailable(int descriptor, int milliseconds) {
    
    struct pollfd polltime;
    polltime.fd = descriptor;
    polltime.events = POLLIN;   // Check for input readiness

    // Wait for events on the file descriptors
    return poll(&polltime, 1, milliseconds);
}

//===================================================================================
// Routines
//===================================================================================

static void* keyboardRoutine(void* args) {
    ThreadArg arg = *(ThreadArg*)args;
    char clientBuffer[MAX_CHAR_COUNT];

    // Set stdin to non-blocking mode
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

    // We have to do this first time or else you see it's blank
    printf("\nMe: ");
    fflush(stdout); // Flush the output buffer to ensure prompt output

    while (1) {
        // Check for termination condition
        if (sClientTermination || sRemoteTermination) {
            break;
        }

        int ret = timeoutUntilAvailable(STDIN_FILENO + 1, 100);
        if (ret == ERROR) {
            perror("poll failed");
            exit(EXIT_FAILURE);

        } else if (ret == 0) {
            // Timeout occurred, skip
            continue;
        }

        // Input is available, read it
        printf("Me: ");
        fflush(stdout);
        fgets(clientBuffer, MAX_CHAR_COUNT, stdin);

        // Check if input was too long, consume remaining characters from stdin
        if (strchr(clientBuffer, '\n') == NULL) {
            while (getchar() != '\n');
        }

        // Entering Critical Section
        pthread_mutex_lock(&arg.pThreadPool->listMutex);

        // First message sent will be at the end
        int status = List_prepend(arg.pUdp->pClientList, strdup(clientBuffer)); // Make a copy of the input buffer
        if (status == EXIT_FAILURE) {
            perror("list_prepend error");
        }

        // Signal the udpSendRoutine thread that a new message is available
        pthread_cond_signal(&arg.pThreadPool->clientListCondition);

        // Terminate connection
        if (strncmp(clientBuffer, "!", 1) == 0) {
            printf("Connection Ended\n");
            pthread_cond_signal(&arg.pThreadPool->remoteListCondition);
            sClientTermination = true;
        }

        // Exiting Critical Section
        pthread_mutex_unlock(&arg.pThreadPool->listMutex);
    }

    pthread_exit(NULL);
}


static void* udpSendRoutine(void* args) {
    ThreadArg arg = *(ThreadArg*)args;
    char* message = NULL;

    while (1) {
        pthread_mutex_lock(&arg.pThreadPool->listMutex);
        // Entering Critical Section

        // Check if the list is empty
        while (List_count(arg.pUdp->pClientList) == 0) {
            // Wait for a signal from the keyboardRoutine thread indicating a new message
            pthread_cond_wait(&arg.pThreadPool->clientListCondition, &arg.pThreadPool->listMutex);
            if (sClientTermination || sRemoteTermination) {
                break;
            }
        }

        // Remote Terminated, nothing to send
        if (sRemoteTermination) {
            pthread_mutex_unlock(&arg.pThreadPool->listMutex);
            break;
        }

        // Get a message from the list
        message = (char*)List_trim(arg.pUdp->pClientList);

        // Exiting Critical Section
        pthread_mutex_unlock(&arg.pThreadPool->listMutex);

        // Send the message over the network
        sendto(arg.pUdp->socket, message, strlen(message), 0, (struct sockaddr*)&arg.pUdp->remoteAddress, sizeof(arg.pUdp->remoteAddress));
        free(message);

        // Send Termination to remote and end client
        if (sClientTermination) {
            break;
        }
    }

    pthread_exit(NULL);
}

static void* udpReceiveRoutine(void* args) {
    ThreadArg arg = *(ThreadArg*)args;
    char recieveBuffer[MAX_CHAR_COUNT];

    while (1) {
        if (sClientTermination || sRemoteTermination) {
            break;
        }

        int ret = timeoutUntilAvailable(arg.pUdp->socket, 1000);
        if (ret == ERROR) {
            perror("poll failed");
            exit(EXIT_FAILURE);

        } else if (ret == 0) {
            // Timeout occurred, skip
            continue;
        }

        memset(recieveBuffer, 0, MAX_CHAR_COUNT);
        socklen_t addressLength = sizeof(arg.pUdp->remoteAddress);
        
        ssize_t result = recvfrom(arg.pUdp->socket, recieveBuffer, MAX_CHAR_COUNT, 0, (struct sockaddr*)&arg.pUdp->remoteAddress, &addressLength);

        if (result == ERROR) {
            perror("Recvfrom failed");
            exit(EXIT_FAILURE);
        }

        pthread_mutex_lock(&arg.pThreadPool->listMutex);
        // Entering Critical Section

        int status = List_prepend(arg.pUdp->pRemoteList, strdup(recieveBuffer));
        if (status == ERROR) {
            perror("list_prepend error");
        }

        // Signal the screenOutputRoutine thread that a new message is available
        pthread_cond_signal(&arg.pThreadPool->remoteListCondition);

        // Terminate connection
        if (strncmp(recieveBuffer, "!", 1) == 0) {
            printf("Remote Connection Ended\n");

            // Release wait threads
            pthread_cond_signal(&arg.pThreadPool->clientListCondition);
            sRemoteTermination = true;
            pthread_cond_signal(&arg.pThreadPool->remoteListCondition);
        }

        // Exiting Critical Section
        pthread_mutex_unlock(&arg.pThreadPool->listMutex);
    }

    pthread_exit(NULL);
}

static void* screenOutputRoutine(void* args) {
    ThreadArg arg = *(ThreadArg*)args;
    char* message = NULL;

    while (1) {
        pthread_mutex_lock(&arg.pThreadPool->listMutex);
        // Entering Critical Section
        
        // Check if the list is empty
        while (List_count(arg.pUdp->pRemoteList) == 0) {
            // Wait for a signal from the keyboardRoutine thread indicating a new message
            pthread_cond_wait(&arg.pThreadPool->remoteListCondition, &arg.pThreadPool->listMutex);
            if (sClientTermination || sRemoteTermination) {
                break;
            }
        }

        // Terminate here because there is nothing in the buffer
        if (sClientTermination) {
            pthread_mutex_unlock(&arg.pThreadPool->listMutex);
            break;
        }

        // Get a message from the list
        message = (char*)List_trim(arg.pUdp->pRemoteList);

        // Exiting Critical Section
        pthread_mutex_unlock(&arg.pThreadPool->listMutex);
        
        if (sRemoteTermination) {
            break;
        } else {
            // Clear the current line and move the cursor to the beginning
            printf("\r\033[K");

            // Print the received message with the "Remote:" prefix
            printf("Remote: %s", message);
            free(message);

            // Print the "Me:" prompt again on the same line
            printf("Me: ");
            fflush(stdout);
        }
    }

    pthread_exit(NULL);
}