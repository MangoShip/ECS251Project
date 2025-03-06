#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../tholder/tholder.h"

#define BUFFER_SIZE 4096

struct sockaddr_in server_addr, client_addr;
int server_fd;

atomic_int req_number = ATOMIC_VAR_INIT(0);

void close_server_fd()
{ 
    printf("\n");
    printf("Responded to %d requests", atomic_load(&req_number));
    if (server_fd > 0){
        printf("\nClosing server...\n");
        close(server_fd);
    }
    server_fd = -1;
    exit(0);
}

void *handle_request(void *args)
{
    int client_fd = (int)args;
    char buffer[BUFFER_SIZE];

    // Read request
    ssize_t bytes_received = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_received < 0) {
        perror("read");
        close(client_fd);
        return NULL;
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

    atomic_fetch_add(&req_number, 1);

    close(client_fd);
    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [PORT]\n", argv[0]);
        exit(1);
    }
    int port;
    sscanf(argv[1], "%d", &port);

    int client_fd;
    socklen_t client_addr_len = sizeof(client_addr);

    // Create socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind address and port
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
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

    printf("Listening on http://localhost:%d/\n", port);

    while (1) {
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        tholder_t *tid = (tholder_t *)malloc(sizeof(tholder_t));
        tholder_create(tid, NULL, handle_request, (void *)client_fd);
    }
    printf("\n");

    close(server_fd);
    return 0;
}
