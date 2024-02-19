#include "threadPool.h"
#include <ctype.h>

int isNumeric(const char *str) {
    while (*str) {
        if (!isdigit(*str))
            return 0;
        str++;
    }
    return 1;
}

int main(int argc, char const* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <myPort> <remoteMachineName> <remotePortNumber>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Make sure ports are numerical
    if (!isNumeric(argv[1]) || !isNumeric(argv[3])) {
        printf("Port numbers must be numeric.\n");
        return EXIT_FAILURE;
    }

    // Create a thread pool object and initialize the thread pool
    ThreadPool pool;
    threadPoolInitialize(&pool);

    // Collect arguments for udp setup
    UDP udp;
    udp.clientPort = (uint16_t)atoi(argv[1]);
    udp.remotePort = (uint16_t)atoi(argv[3]);
    udp.remoteMachineName = argv[2];

    // Initialize UDP
    udpInitialize(&udp);

    // Create Processes
    createThreadRoutine(&pool, &udp);

    // Destroy Objects in Reverse Order
    destroyUdp(&udp);
    destroyThreadPool(&pool);

    return EXIT_SUCCESS;
}
