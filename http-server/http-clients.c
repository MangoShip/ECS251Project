#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>

#include <unistd.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define REQUEST "GET / HTTP/1.1\r\nHost: localhost\r\nUser-Agent: StressTest/1.0\r\nConnection: close\r\n\r\n"
#define BUFFER_SIZE 4096

int server_port = 0;

atomic_int num_failed_requests = ATOMIC_VAR_INIT(0);

double gtod(void)
{
  static struct timeval tv;
  static struct timezone tz;
  gettimeofday(&tv,&tz);
  return tv.tv_sec*1e6 + tv.tv_usec;
}

double send_request() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1.0;
    }

    // Set up server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return -1.0;
    }

    double start_time = gtod();

    // Connect to server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(sock);
        return -1.0;
    }

    // Send HTTP request
    if (write(sock, REQUEST, strlen(REQUEST)) < 0) {
        perror("write");
        close(sock);
        return -1.0;
    }

    // Read response
    ssize_t bytes_received = read(sock, buffer, BUFFER_SIZE - 1);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
    } else {
        perror("read");
        close(sock);
        return -1.0;
    }

    double elapsed = gtod() - start_time;

    close(sock);
    return elapsed;
}

void *send_request_pthread(void *args)
{
    size_t num_requests = (size_t)args;
    double local_total_latency = 0.0;
    for (size_t i = 0; i < num_requests; i++)
    {
        double latency = send_request();
        /*printf("Latency: %f\n", latency);*/
        if (latency < 0.0) {
            printf("Request failed\n");
            atomic_fetch_add(&num_failed_requests, 1);
        } else {
            local_total_latency += latency;
        }
    }
    double *result = (double *)malloc(sizeof(double));
    /*printf("Total thread latency: %f\n", local_total_latency);*/
    *result = local_total_latency;
    return (void *)result;
}

int main(int argc, char *argv[]) {
    if (argc < 3)
    {
        printf("Usage: %s [PORT] [REQ_PER_THREAD] [NUM_THREADS]\n", argv[0]);
        exit(1);
    }
    sscanf(argv[1], "%d", &server_port);

    size_t num_requests_per_thread;
    sscanf(argv[2], "%zu", &num_requests_per_thread);

    size_t num_threads;
    sscanf(argv[3], "%zu", &num_threads);

    pthread_t tid[num_threads];

    printf("Starting threads\n");
    for (size_t i = 0; i < num_threads; i++) {
        pthread_create(&tid[i], NULL, send_request_pthread, (void *)num_requests_per_thread);
    }

    double global_total_latency = 0.0;
    void *result;
    for (size_t i = 0; i < num_threads; i++) {
        pthread_join(tid[i], &result);
        double local_total_latency = *(double *)result;
        global_total_latency += local_total_latency;
    }

    size_t successful_requests = (num_requests_per_thread * num_threads) - atomic_load(&num_failed_requests);
    double average_latency = global_total_latency / (double)successful_requests;

    printf("Average latency: %f us\n", average_latency);
    printf("Number of failed requests: %d\n", atomic_load(&num_failed_requests));
    return 0;
}
