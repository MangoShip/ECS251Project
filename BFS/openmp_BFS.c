#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <omp.h>

#define MAX_VERTICES 100000000  // maximum allowed nodes

// Structure for an adjacency list node
typedef struct {
    int *neighbors; // dynamic array of neighbor indices
    int count;      // current number of neighbors
    int capacity;   // allocated capacity of neighbors array
} AdjList;

// Initialize an adjacency list
void initAdjList(AdjList *list) {
    list->neighbors = NULL;
    list->count = 0;
    list->capacity = 0;
}

// Add a neighbor to the adjacency list; reallocates as needed
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

// Add an undirected edge between u and v
void addEdge(AdjList *graph, int u, int v) {
    addNeighbor(&graph[u], v);
    addNeighbor(&graph[v], u);
}

// Generate a random graph with n nodes and an average degree of about 10.
// We target m = n * 5 undirected edges (each edge is stored twice).
void generate_random_graph(AdjList *graph, int n) {
    // Initialize each node's adjacency list
    for (int i = 0; i < n; i++) {
        initAdjList(&graph[i]);
    }
    long long m = (long long)n * 5;  // target number of undirected edges
    for (long long i = 0; i < m; i++) {
        int u = rand() % n;
        int v = rand() % n;
        while (u == v) { // avoid self-loop
            v = rand() % n;
        }
        // Duplicate edges are very unlikely in a sparse graph; ignore duplicates.
        addEdge(graph, u, v);
    }
}

// Compute difference between two timespecs in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before) {
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Parallel BFS using OpenMP (level-synchronous) on an adjacency list graph.
// The BFS always starts from node 0. Instead of printing every vertex visited,
// it only outputs the total number of vertices reached.
void parallel_bfs_omp(AdjList *graph, int n, int startVertex, int num_threads) {
    bool *visited = calloc(n, sizeof(bool));
    if (visited == NULL) {
        fprintf(stderr, "Memory allocation failed for visited\n");
        exit(1);
    }
    int *current_frontier = malloc(n * sizeof(int));
    int *next_frontier = malloc(n * sizeof(int));
    if (current_frontier == NULL || next_frontier == NULL) {
        fprintf(stderr, "Memory allocation failed for frontiers\n");
        exit(1);
    }
    int current_size = 0, next_size = 0;

    // Initialize BFS with the start vertex
    current_frontier[0] = startVertex;
    current_size = 1;
    visited[startVertex] = true;

    omp_set_num_threads(num_threads);

    while (current_size > 0) {
        next_size = 0;

        #pragma omp parallel for schedule(dynamic)
        for (int i = 0; i < current_size; i++) {
            int u = current_frontier[i];
            for (int j = 0; j < graph[u].count; j++) {
                int v = graph[u].neighbors[j];
                #pragma omp critical
                {
                    if (!visited[v]) {
                        visited[v] = true;
                        next_frontier[next_size] = v;
                        next_size++;
                    }
                }
            }
        }

        // Prepare for next level: update current_frontier
        for (int i = 0; i < next_size; i++) {
            current_frontier[i] = next_frontier[i];
        }
        current_size = next_size;
    }

    // Count the number of visited nodes
    int visited_count = 0;
    for (int i = 0; i < n; i++) {
        if (visited[i]) {
            visited_count++;
        }
    }

    // Output only the size (i.e. the total number of vertices reached by BFS)
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

    if (n <= 0 || n > MAX_VERTICES) {
        fprintf(stderr, "Number of nodes must be positive and no more than %d\n", MAX_VERTICES);
        exit(1);
    }
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        exit(1);
    }

    // Seed the random number generator
    srand(time(NULL));

    // Dynamically allocate the graph (an array of adjacency lists)
    AdjList *graph = malloc(n * sizeof(AdjList));
    if (graph == NULL) {
        fprintf(stderr, "Memory allocation failed for graph\n");
        exit(1);
    }

    // Generate a random graph with n nodes
    generate_random_graph(graph, n);

    printf("Test case 1, size %d\n", n);

    struct timespec start_time, end_time;
    double elapsed_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);
    parallel_bfs_omp(graph, n, 0, num_threads);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    elapsed_time = difftimespec_ns(end_time, start_time);
    printf("Parallel (OpenMP) BFS traversal time: %f ns\n", elapsed_time);

    // Free the graph memory
    for (int i = 0; i < n; i++) {
        free(graph[i].neighbors);
    }
    free(graph);

    return 0;
}
