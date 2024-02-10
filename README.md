# IPC-Concurrency

### REPORT

- **2024-02-05**
    - Setup Makefile to create executables
    - UDP client and server testing. We made two different main functions that compiled into two different executables and used this to test that our basic communication was working with a hard coded port

- **2024-02-06**
    - Added connection termination:  `!`

- **2024-02-07**
    - Removed `start.c` and created `s-talk.c` (main file)
        - Calls the thread functions and udp functions, while gathering main args
    - Created `Threadpool.h`
        - Structure for threadpool containing necessary variables for producer-consumer and critical section
        - Structure for UDP containing the necessary variables for creating a UDP connection
    - Created `Threadpool.c`
        - Contains internal enum and struct for threading and organization
        - Threadpool/UDP init and destroy functions
        - ThreadRoutine function setup to create and use threads
    - Included `list.h` and `list.o`
    - Restructured the Makefile to include `pthreads` and `list.o` for build into a `s-talk` executable
        - Removed `udp_client.c` and `udp_server.c` as target
    - Planning to incorporate `udp_client.c` and `udp_server.c` knowledge as well as removing them as it was used for understanding the communciation