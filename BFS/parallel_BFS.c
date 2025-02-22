#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define MAX_VERTICES 100

// Calculate the difference between two timespecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before) {
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Graph structure using an adjacency matrix
struct Graph {
    int vertices;
    int adjMatrix[MAX_VERTICES][MAX_VERTICES];
};

// Structure for thread arguments
struct thread_args {
    int start;               // Start index in the current frontier array
    int end;                 // End index (exclusive) in the current frontier array
    int *current_frontier;   // Pointer to the current frontier array
    struct Graph *graph;     // Pointer to the graph
    int *next_frontier;      // Pointer to the global next frontier array
    int *next_size;          // Pointer to the global next frontier size
    pthread_mutex_t *mutex;  // Pointer to the mutex for updating shared data
    bool *visited;           // Pointer to the global visited array
};

// Thread function to process a portion of the current frontier
void* bfs_thread(void* arg) {
    struct thread_args *args = (struct thread_args*)arg;
    for (int i = args->start; i < args->end; i++) {
        int u = args->current_frontier[i];
        for (int v = 0; v < args->graph->vertices; v++) {
            if (args->graph->adjMatrix[u][v] == 1) {
                pthread_mutex_lock(args->mutex);
                if (!args->visited[v]) {
                    args->visited[v] = true;
                    int pos = *(args->next_size);
                    args->next_frontier[pos] = v;
                    (*(args->next_size))++;
                }
                pthread_mutex_unlock(args->mutex);
            }
        }
    }
    return NULL;
}

// Parallel BFS using a level-synchronous approach
void parallel_bfs(struct Graph *graph, int startVertex, int num_threads) {
    bool visited[MAX_VERTICES] = {false};
    int current_frontier[MAX_VERTICES];
    int next_frontier[MAX_VERTICES];
    int current_size = 0;
    int next_size = 0;

    // Initialize BFS with the start vertex
    current_frontier[0] = startVertex;
    current_size = 1;
    visited[startVertex] = true;

    // Print the start vertex
    printf("%d ", startVertex);

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    // Level-synchronous BFS loop
    while (current_size > 0) {
        next_size = 0;
        // Dynamically allocate arrays for threads and their arguments
        pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
        struct thread_args *args = malloc(num_threads * sizeof(struct thread_args));

        // Divide the current frontier among threads
        int chunk_size = current_size / num_threads;
        int remainder = current_size % num_threads;
        int index = 0;
        for (int i = 0; i < num_threads; i++) {
            args[i].start = index;
            int extra = (i < remainder) ? 1 : 0;
            args[i].end = index + chunk_size + extra;
            args[i].current_frontier = current_frontier;
            args[i].graph = graph;
            args[i].next_frontier = next_frontier;
            args[i].next_size = &next_size;
            args[i].mutex = &mutex;
            args[i].visited = visited;
            index = args[i].end;
        }

        // Create threads to process the current frontier
        for (int i = 0; i < num_threads; i++) {
            if (args[i].start < args[i].end) {
                pthread_create(&threads[i], NULL, bfs_thread, (void*)&args[i]);
            }
        }
        // Wait for all threads to finish processing the current level
        for (int i = 0; i < num_threads; i++) {
            if (args[i].start < args[i].end) {
                pthread_join(threads[i], NULL);
            }
        }

        // Print the vertices in the next frontier (next level)
        for (int i = 0; i < next_size; i++) {
            printf("%d ", next_frontier[i]);
        }
        // Prepare for the next level: update current frontier
        for (int i = 0; i < next_size; i++) {
            current_frontier[i] = next_frontier[i];
        }
        current_size = next_size;

        free(threads);
        free(args);
    }

    pthread_mutex_destroy(&mutex);
    printf("\n");
}

// Initialize the graph: set number of vertices and clear the adjacency matrix
void initGraph(struct Graph *graph, int vertices) {
    graph->vertices = vertices;
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            graph->adjMatrix[i][j] = 0;
        }
    }
}

// Add an undirected edge to the graph
void addEdge(struct Graph *graph, int startVertex, int endVertex) {
    graph->adjMatrix[startVertex][endVertex] = 1;
    graph->adjMatrix[endVertex][startVertex] = 1;
}

int main(int argc, char *argv[]) {
    // Command line arguments:
    // program name + vertices + edges + (2*edges for edge pairs) + startVertex + num_threads
    if (argc < 5) {
        fprintf(stderr, "Usage: %s <vertices> <edges> <edge pairs...> <startVertex> <num_threads>\n", argv[0]);
        exit(1);
    }

    int vertices = atoi(argv[1]);
    int edges = atoi(argv[2]);

    if (vertices > MAX_VERTICES) {
        fprintf(stderr, "Vertices exceed maximum limit (%d).\n", MAX_VERTICES);
        exit(1);
    }

    if (argc != (2 * edges + 5)) {
        fprintf(stderr, "Invalid number of arguments.\n");
        fprintf(stderr, "Usage: %s <vertices> <edges> <edge pairs...> <startVertex> <num_threads>\n", argv[0]);
        exit(1);
    }

    struct Graph graph;
    initGraph(&graph, vertices);

    // Read each edge's two vertices from the command line
    for (int i = 0; i < edges; i++) {
        int u = atoi(argv[3 + 2 * i]);
        int v = atoi(argv[3 + 2 * i + 1]);
        if (u < 0 || u >= vertices || v < 0 || v >= vertices) {
            fprintf(stderr, "Vertex indices must be in range 0 to %d.\n", vertices - 1);
            exit(1);
        }
        addEdge(&graph, u, v);
    }

    int startVertex = atoi(argv[3 + 2 * edges]);
    if (startVertex < 0 || startVertex >= vertices) {
        fprintf(stderr, "Start vertex must be in range 0 to %d.\n", vertices - 1);
        exit(1);
    }

    int num_threads = atoi(argv[3 + 2 * edges + 1]);
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive.\n");
        exit(1);
    }

    // Print test case information
    printf("Test case 1, size %d\n", vertices);

    struct timespec start_time, end_time;
    double elapsed_time;

    // Measure the time for parallel BFS
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    parallel_bfs(&graph, startVertex, num_threads);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    elapsed_time = difftimespec_ns(end_time, start_time);
    printf("Parallel BFS traversal time: %f ns\n", elapsed_time);

    return 0;
}
