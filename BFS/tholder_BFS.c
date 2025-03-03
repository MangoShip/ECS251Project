#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "../tholder/tholder.h"

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

// Add a neighbor to the adjacency list; reallocates memory if needed
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

// Generate a random graph with n nodes and about n*5 undirected edges (each edge stored twice)
void generate_random_graph(AdjList *graph, int n) {
    // Initialize each node's adjacency list
    for (int i = 0; i < n; i++) {
        initAdjList(&graph[i]);
    }
    long long m = (long long)n * 5;
    for (long long i = 0; i < m; i++) {
        int u = rand() % n;
        int v = rand() % n;
        while (u == v) { // Avoid self-loop
            v = rand() % n;
        }
        // Add the edge to the graph
        addEdge(graph, u, v);
    }
}

// Calculate the difference between two timespec values in nanoseconds
double difftimespec_ns(const struct timespec after, const struct timespec before) {
    return ((double)after.tv_sec - (double)before.tv_sec) * 1e9 +
           ((double)after.tv_nsec - (double)before.tv_nsec);
}

// Structure for BFS task arguments for each thread
struct bfs_task_arg {
    int start_index;          // Start index in the current frontier array
    int end_index;            // End index in the current frontier array
    int *current_frontier;    // Pointer to the current frontier array
    AdjList *graph;           // Pointer to the graph (array of adjacency lists)
    bool *visited;            // Global visited array
    int n;                    // Total number of nodes
    int *local_next;          // Local next frontier array (allocated per thread)
    int local_count;          // Count of nodes in the local next frontier
};

// Thread function for BFS: process a subset of the current frontier
void *bfs_task(void *arg) {
    struct bfs_task_arg *task = (struct bfs_task_arg*) arg;
    // Allocate memory for the local next frontier; size set to n (worst-case scenario)
    task->local_next = malloc(task->n * sizeof(int));
    if (task->local_next == NULL) {
        fprintf(stderr, "Memory allocation failed in bfs_task\n");
        exit(1);
    }
    task->local_count = 0;
    // Iterate over the assigned portion of the current frontier
    for (int i = task->start_index; i < task->end_index; i++) {
        int u = task->current_frontier[i];
        // Traverse all neighbors of node u
        for (int j = 0; j < task->graph[u].count; j++) {
            int v = task->graph[u].neighbors[j];
            // Atomically check and set visited[v]; if it was false, mark it as true and add to local_next
            if (__sync_bool_compare_and_swap(&task->visited[v], false, true)) {
                task->local_next[task->local_count++] = v;
            }
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <number_of_nodes> <num_threads>\n", argv[0]);
        exit(1);
    }
    int n = atoi(argv[1]);
    int num_threads = atoi(argv[2]);
    if (n <= 0) {
        fprintf(stderr, "Number of nodes must be positive\n");
        exit(1);
    }
    if (num_threads <= 0) {
        fprintf(stderr, "Number of threads must be positive\n");
        exit(1);
    }

    srand(time(NULL));

    // Allocate memory for the graph (array of adjacency lists)
    AdjList *graph = malloc(n * sizeof(AdjList));
    if (graph == NULL) {
        fprintf(stderr, "Memory allocation failed for graph\n");
        exit(1);
    }
    generate_random_graph(graph, n);

    // Allocate and initialize the visited array (all false initially)
    bool *visited = calloc(n, sizeof(bool));
    if (visited == NULL) {
        fprintf(stderr, "Memory allocation failed for visited\n");
        exit(1);
    }

    // Allocate memory for current and next frontier arrays (each of size n)
    int *current_frontier = malloc(n * sizeof(int));
    int *next_frontier = malloc(n * sizeof(int));
    if (current_frontier == NULL || next_frontier == NULL) {
        fprintf(stderr, "Memory allocation failed for frontier arrays\n");
        exit(1);
    }

    // Initialize BFS: start with node 0
    current_frontier[0] = 0;
    int current_size = 1;
    visited[0] = true;

    // Initialize the tholder thread library (allowing up to 50 tasks in the queue)
    tholder_init(50);

    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    // Level-synchronous BFS loop: expand each level to form the next frontier
    while (current_size > 0) {
        // Create task arguments for each thread
        struct bfs_task_arg task_args[num_threads];
        tholder_t thread_array[num_threads];

        // Divide the current frontier evenly among the threads
        int chunk_size = (current_size + num_threads - 1) / num_threads;
        for (int t = 0; t < num_threads; t++) {
            int start = t * chunk_size;
            int end = (start + chunk_size) < current_size ? (start + chunk_size) : current_size;
            task_args[t].start_index = start;
            task_args[t].end_index = end;
            task_args[t].current_frontier = current_frontier;
            task_args[t].graph = graph;
            task_args[t].visited = visited;
            task_args[t].n = n;
            task_args[t].local_next = NULL;
            task_args[t].local_count = 0;

            tholder_create(&thread_array[t], NULL, bfs_task, (void *)&task_args[t]);
        }

        // Wait for all threads to complete
        for (int t = 0; t < num_threads; t++) {
            tholder_join(thread_array[t], NULL);
        }

        // Merge local next frontier arrays from all threads into the global next_frontier array
        int next_size = 0;
        for (int t = 0; t < num_threads; t++) {
            for (int i = 0; i < task_args[t].local_count; i++) {
                next_frontier[next_size++] = task_args[t].local_next[i];
            }
            free(task_args[t].local_next);
        }

        // Prepare for the next level: swap current_frontier and next_frontier, and update current_size
        int *temp = current_frontier;
        current_frontier = next_frontier;
        next_frontier = temp;
        current_size = next_size;
    }

    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = difftimespec_ns(end_time, start_time);

    // Count the total number of visited nodes
    int visited_count = 0;
    for (int i = 0; i < n; i++) {
        if (visited[i]) {
            visited_count++;
        }
    }
    printf("BFS visited count: %d\n", visited_count);
    printf("Parallel (tholder) BFS traversal time: %f ns\n", elapsed);

    // Free the memory allocated for each node's neighbor array
    for (int i = 0; i < n; i++) {
        free(graph[i].neighbors);
    }
    free(graph);
    free(visited);
    free(current_frontier);
    free(next_frontier);

    tholder_destroy();

    return 0;
}
