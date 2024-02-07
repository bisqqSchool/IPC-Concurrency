#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

int main() {
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[MAX_BUFFER_SIZE];

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setup server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    while (1) {
        // Send message to server
        printf("Client: ");
        fgets(buffer, MAX_BUFFER_SIZE, stdin);
        sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));

        // Terminate connection
        if (buffer[0] == '!') {
            printf("Connection ended\n");
            break;
        }

        // Receive message from server
        memset(buffer, 0, MAX_BUFFER_SIZE);
        recvfrom(client_socket, buffer, MAX_BUFFER_SIZE, 0, NULL, NULL);

        // Print received message
        printf("Server: %s", buffer);
    }

    // Close the socket
    close(client_socket);

    return 0;
}
