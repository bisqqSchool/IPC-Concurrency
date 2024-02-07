#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int main() {
    int server_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    char buffer[MAX_BUFFER_SIZE];

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket to address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    socklen_t addr_len = sizeof(client_addr);

    while (1) {
        // Receive message from client
        memset(buffer, 0, MAX_BUFFER_SIZE);
        recvfrom(server_socket, buffer, MAX_BUFFER_SIZE, 0, (struct sockaddr*)&client_addr, &addr_len);

        // Terminate connection
        if (buffer[0] == '!') {
            printf("Client ended connection\n");
            break;
        }

        // Print received message
        printf("Client: %s", buffer);

        // Send message to client
        printf("Server: ");
        fgets(buffer, MAX_BUFFER_SIZE, stdin);
        sendto(server_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&client_addr, addr_len);
    }

    // Close the socket
    close(server_socket);

    return 0;
}
