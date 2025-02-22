#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

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

// Queue structure for BFS
struct Queue {
    int items[MAX_VERTICES];
    int front;
    int rear;
};

// Initialize the queue
void initQueue(struct Queue* q) {
    q->front = -1;
    q->rear = -1;
}

// Check if the queue is empty
bool isEmpty(struct Queue* q) {
    return q->front == -1;
}

// Enqueue an element into the queue
void enqueue(struct Queue* q, int value) {
    if (q->rear == MAX_VERTICES - 1) {
        fprintf(stderr, "Queue is full\n");
        return;
    }
    if (q->front == -1)
        q->front = 0;
    q->items[++q->rear] = value;
}

// Dequeue an element from the queue
int dequeue(struct Queue* q) {
    if (isEmpty(q)) {
        fprintf(stderr, "Queue is empty\n");
        return -1;
    }
    int value = q->items[q->front];
    q->front++;
    if (q->front > q->rear)
        q->front = q->rear = -1;
    return value;
}

// BFS traversal function
void bfs(struct Graph* graph, int startVertex) {
    bool visited[MAX_VERTICES] = {false};
    struct Queue q;
    initQueue(&q);

    visited[startVertex] = true;
    enqueue(&q, startVertex);

    while (!isEmpty(&q)) {
        int currentVertex = dequeue(&q);
        printf("%d ", currentVertex);

        // Traverse all adjacent vertices of the current vertex
        for (int i = 0; i < graph->vertices; i++) {
            if (graph->adjMatrix[currentVertex][i] == 1 && !visited[i]) {
                enqueue(&q, i);
                visited[i] = true;
            }
        }
    }
}

// Initialize the graph: set the number of vertices and initialize the adjacency matrix to 0
void initGraph(struct Graph* graph, int vertices) {
    graph->vertices = vertices;
    for (int i = 0; i < vertices; i++) {
        for (int j = 0; j < vertices; j++) {
            graph->adjMatrix[i][j] = 0;
        }
    }
}

// Add an undirected edge to the graph
void addEdge(struct Graph* graph, int startVertex, int endVertex) {
    graph->adjMatrix[startVertex][endVertex] = 1;
    graph->adjMatrix[endVertex][startVertex] = 1;
}

int main(int argc, char *argv[]) {
    // Command line arguments: program name + vertices + edges + 2*edges for edge pairs + startVertex
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <vertices> <edges> <edge pairs...> <startVertex>\n", argv[0]);
        exit(1);
    }

    int vertices = atoi(argv[1]);
    int edges = atoi(argv[2]);

    if (vertices > MAX_VERTICES) {
        fprintf(stderr, "Vertices exceed maximum limit (%d).\n", MAX_VERTICES);
        exit(1);
    }

    if (argc != (2 * edges + 4)) {
        fprintf(stderr, "Invalid number of arguments.\n");
        fprintf(stderr, "Usage: %s <vertices> <edges> <edge pairs...> <startVertex>\n", argv[0]);
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

    int startVertex = atoi(argv[argc - 1]);
    if (startVertex < 0 || startVertex >= vertices) {
        fprintf(stderr, "Start vertex must be in range 0 to %d.\n", vertices - 1);
        exit(1);
    }

    // Print test case information (test case number and input size)
    printf("Test case 1, size %d\n", vertices);

    struct timespec start_time, end_time;
    double elapsed_time;

    // Get start time, perform BFS, and get end time
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    bfs(&graph, startVertex);
    clock_gettime(CLOCK_MONOTONIC, &end_time);

    elapsed_time = difftimespec_ns(end_time, start_time);
    printf("\nBFS traversal time: %f ns\n", elapsed_time);

    return 0;
}
