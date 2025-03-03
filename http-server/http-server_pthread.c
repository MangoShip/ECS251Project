#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#include "string.h"
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6969
#define BUFFER_SIZE 4096
#define NUM_THREADS 16

struct sockaddr_in server_addr, client_addr;
int server_fd;

int req_number = 0;

void close_server_fd()
{ 
    printf("\nClosing server...\n");
    printf("Responded to %d requests", req_number);
    if (server_fd > 0)
        close(server_fd);
    server_fd = -1;
    exit(0);
}

int main() {
    int client_fd;
    socklen_t client_addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Signal handler for interrupt
    signal(SIGINT, close_server_fd);

    // Listen for connections
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Listening on http://localhost:%d/\n", PORT);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Read request
        ssize_t bytes_received = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_received < 0) {
            perror("read");
            close(client_fd);
            continue;
        }
        buffer[bytes_received] = '\0'; // Null-terminate request

        // Send HTTP response
        char response[BUFFER_SIZE];
        snprintf(response, sizeof(response), 
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/plain\r\n"
                 "Content-Length: %zd\r\n"
                 "Connection: close\r\n\r\n%s", bytes_received, buffer);
        /*printf("\rResponded to request %d", req_number++);*/
        write(client_fd, response, strlen(response));

        req_number++;

        close(client_fd);
    }
    printf("\n");

    close(server_fd);
    return 0;
}
