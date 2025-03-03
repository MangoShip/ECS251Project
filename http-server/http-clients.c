#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6969
#define REQUEST "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: StressTest/1.0\r\nConnection: close\r\n\r\n"
#define BUFFER_SIZE 4096

#define NUM_THREADS 32

int send_request() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 0;
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 0;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return 0;
    }

    // Send HTTP request
    if (write(sock, REQUEST, strlen(REQUEST)) < 0) {
        perror("write");
        close(sock);
        return 0;
    }

    // Read response
    ssize_t bytes_received = read(sock, buffer, BUFFER_SIZE - 1);
    int ret = 0;
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        ret = 1;
        /*printf("%s\n", buffer);*/
    } else {
        perror("read");
    }

    close(sock);
    return ret;
}

void *send_request_pthread(void *args)
{
    int local_successes = 0;
    for (int i = 0; i < 100; i++)
        local_successes += send_request();
    return (void *)local_successes;
}

int main() {

    pthread_t tid[NUM_THREADS];
    int global_successes = 0;

    printf("Starting threads\n");
    for (size_t i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, send_request_pthread, NULL);
    }

    for (size_t i = 0; i < NUM_THREADS; i++) {
        int thread_successes;
        pthread_join(tid[i], (void *)&thread_successes);
        printf("Thread %ld returned %d\n", i, thread_successes);
        global_successes += thread_successes;
    }

    printf("Number of responses: %d\n", global_successes);
    return 0;
}
