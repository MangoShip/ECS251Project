#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>

#define MAX_NODES 100000000  // Maximum allowed nodes

// Structure for an adjacency list node
typedef struct {
    int *neighbors; // Dynamic array of neighbor indices
    int count;      // Current number of neighbors
    int capacity;   // Allocated capacity for the neighbors array
} AdjList;

// Initialize an adjacency list
void initAdjList(AdjList *list) {
    list->neighbors = NULL;
    list->count = 0;
    list->capacity = 0;
}

// Add a neighbor to the adjacency list; reallocates memory as needed
void addNeighbor(AdjList *list, int v) {
    if (list->count == list->capacity) {
        int new_capacity = (list->capacity == 0) ? 4 : list->capacity * 2;
        int *new_neighbors = realloc(list->neighbors, new_capacity * sizeof(int));
        if (new_neighbors == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
        list->neighbors = new_neighbors;
        list->capacity = new_capacity;
    }
    list->neighbors[list->count++] = v;
}

// Add an undirected edge between nodes u and v
void addEdge(AdjList *graph, int u, int v) {
    addNeighbor(&graph[u], v);
    addNeighbor(&graph[v], u);
}

// Generate a complex random graph with n nodes.
// For each node, assign a random degree between 50 and 100,
// and add that many undirected edges to randomly chosen nodes (avoiding self-loops).
void generate_complex_graph(AdjList *graph, int n) {
    int min_degree = 50;
    int max_degree = 100;
    // Initialize each node's adjacency list
    for (int i = 0; i < n; i++) {
        initAdjList(&graph[i]);
    }
    // For each node, generate a random degree and add that many edges.
    for (int i = 0; i < n; i++) {
        int degree = min_degree + rand() % (max_degree - min_degree + 1);
        for (int j = 0; j < degree; j++) {
            int v = rand() % n;
            while (v == i) { // avoid self-loop
                v = rand() % n;
            }
            // Add an undirected edge (duplicates are acceptable)
            addEdge(graph, i, v);
        }
    }
}

// Compute the difference between two timespec structures in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before) {
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Structure for thread arguments used in parallel BFS
struct thread_args {
    int start;             // Start index in the current frontier array
    int end;               // End index (exclusive) in the current frontier array
    int *current_frontier; // Pointer to the current frontier array
    int *next_frontier;    // Pointer to the shared next frontier array
    int *next_size;        // Pointer to the shared next frontier size
    AdjList *graph;        // Pointer to the graph (array of AdjList)
    bool *visited;         // Pointer to the global visited array
    pthread_mutex_t *mutex; // Pointer to the mutex for synchronizing access
};

// Thread function for processing a portion of the current frontier
void *bfs_thread(void *arg) {
    struct thread_args *args = (struct thread_args *)arg;
    for (int i = args->start; i < args->end; i++) {
        int u = args->current_frontier[i];
        for (int j = 0; j < args->graph[u].count; j++) {
            int v = args->graph[u].neighbors[j];
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
    return NULL;
}

// Parallel BFS using pthreads (level-synchronous) on an adjacency list graph.
// BFS always starts from node 0 and outputs only the total number of vertices reached.
void parallel_bfs_pthread(AdjList *graph, int n, int startVertex, int num_threads) {
    bool *visited = calloc(n, sizeof(bool));
    if (visited == NULL) {
        fprintf(stderr, "Memory allocation failed for visited\n");
        exit(1);
    }
    int *current_frontier = malloc(n * sizeof(int));
    int *next_frontier = malloc(n * sizeof(int));
    if (current_frontier == NULL || next_frontier == NULL) {
        fprintf(stderr, "Memory allocation failed for frontier arrays\n");
        exit(1);
    }
    int current_size = 0, next_size = 0;

    // Initialize BFS with the start vertex
    current_frontier[0] = startVertex;
    current_size = 1;
    visited[startVertex] = true;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    while (current_size > 0) {
        next_size = 0;
        pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
        struct thread_args *args = malloc(num_threads * sizeof(struct thread_args));
        if (threads == NULL || args == NULL) {
            fprintf(stderr, "Memory allocation failed for threads\n");
            exit(1);
        }

        // Divide the current frontier among threads
        int chunk_size = current_size / num_threads;
        int remainder = current_size % num_threads;
        int index = 0;
        for (int i = 0; i < num_threads; i++) {
            args[i].start = index;
            int extra = (i < remainder) ? 1 : 0;
            args[i].end = index + chunk_size + extra;
            args[i].current_frontier = current_frontier;
            args[i].next_frontier = next_frontier;
            args[i].next_size = &next_size;
            args[i].graph = graph;
            args[i].visited = visited;
            args[i].mutex = &mutex;
            index = args[i].end;
        }

        // Create threads to process the current frontier
        for (int i = 0; i < num_threads; i++) {
            if (args[i].start < args[i].end)
                pthread_create(&threads[i], NULL, bfs_thread, (void *)&args[i]);
        }
        // Wait for all threads to finish
        for (int i = 0; i < num_threads; i++) {
            if (args[i].start < args[i].end)
                pthread_join(threads[i], NULL);
        }

        free(threads);
        free(args);

        // Update current frontier with next frontier
        for (int i = 0; i < next_size; i++) {
            current_frontier[i] = next_frontier[i];
        }
        current_size = next_size;
    }

    pthread_mutex_destroy(&mutex);

    // Count the number of visited nodes
    int visited_count = 0;
    for (int i = 0; i < n; i++) {
        if (visited[i])
            visited_count++;
    }
    // Output only the visited count
    printf("BFS visited count: %d\n", visited_count);

    free(visited);
    free(current_frontier);
    free(next_frontier);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_nodes> <num_threads>\n", argv[0]);
        exit(1);
    }

    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    if (n <= 0 || n > MAX_NODES) {
        fprintf(stderr, "Number of nodes must be positive and no more than %d\n", MAX_NODES);
        exit(1);
    }
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        exit(1);
    }

    // Set a fixed random seed to ensure consistent graph generation
    srand(42);

    // Dynamically allocate the graph (an array of adjacency lists)
    AdjList *graph = malloc(n * sizeof(AdjList));
    if (graph == NULL) {
        fprintf(stderr, "Memory allocation failed for graph\n");
        exit(1);
    }

    // Generate a complex random graph with n nodes
    generate_complex_graph(graph, n);

    printf("Test case 1, size %d\n", n);

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    parallel_bfs_pthread(graph, n, 0, num_threads);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    double elapsed_time = difftimespec_ns(end_time, start_time);
    printf("Parallel (pthread) BFS traversal time: %f ns\n", elapsed_time);

    // Free the graph memory
    for (int i = 0; i < n; i++) {
        free(graph[i].neighbors);
    }
    free(graph);

    return 0;
}
