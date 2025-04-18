#include "stdio.h"
#include "time.h"
#include "sys/time.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "pthread.h"
#include <unistd.h>
#include <omp.h>

//Input information, including thread count, graph count, and threshold
int num_threads;
int num_graphs;
double threshold;

//Current number of nodes in graph
int num_nodes;

//The initial eigenvector, and the matrix associated with the graph's connections
double *eigen;
double **matrix;
double *new_eigen;

//File that is read for the current graph
FILE *graph;

//Time data
double total_time;
struct timespec iter_start_time, iter_end_time;

//Get difference in nanoseconds (taken from radixsort)
double difftimespec_ns(const struct timespec after, const struct timespec before)
{
  return ((double)after.tv_sec - (double)before.tv_sec) * (double)1000000000
        + ((double)after.tv_nsec - (double)before.tv_nsec);
}

void *pagerank_parallel(){
    #pragma omp parallel for
    for (int i = 0; i < num_nodes; i++){
            for (int j = 0; j < num_nodes; j++){
                new_eigen[i] += matrix[i][j] * eigen[j];
            }
    }
    return 0;
}

void pagerank(){
  double error = 100000;
  new_eigen = calloc(num_nodes, sizeof(double));
  while(error > threshold){
    pagerank_parallel();
    //Find the norm in order to normalize the new eigenvector
    double norm = 0;
    for(int i = 0; i < num_nodes; i++){
      norm += new_eigen[i]*new_eigen[i];
    }
    norm = sqrt(norm);
    //Normalize the new eigenvector while calculating the error
    double new_error = 0;
    for(int i = 0; i < num_nodes; i++){
      new_eigen[i]/=norm;
      new_error += (new_eigen[i] - eigen[i])*(new_eigen[i] - eigen[i]);
      eigen[i] = new_eigen[i];
    }
    error = sqrt(new_error);
  }
  double total = 0;
  for(int i = 0; i < num_nodes; i++){
    total += eigen[i];
  }
  for(int i = 0; i < num_nodes; i++){
    eigen[i]/=total;
  }
}

void create_matrix(){
    char input_line[256];
    num_nodes = atoi(fgets(input_line, 255, graph));

    /*Creates the matrix based on the graph's connections*/
    matrix = (double **)malloc(num_nodes * sizeof(double*));
    for (int i = 0; i < num_nodes; i++)
        matrix[i] = (double*)calloc(num_nodes, sizeof(double));
    eigen = (double *)calloc(num_nodes, sizeof(double));
    for(int i = 0; i < num_nodes; i++){
        eigen[i] = 1;
    }
    //Take all edges, add a 1 to the matrix at coordinates [from_edge, to_edge]
    char* current_edge = malloc(255);
    current_edge = fgets(input_line, 255, graph);
    while(strcmp(current_edge, "END") != 0){
        int from_edge = atoi(strtok(current_edge, " "));
        int to_edge = atoi(strtok(NULL, " "));
        matrix[from_edge][to_edge] = 1;
        current_edge = fgets(input_line, 255, graph);
    }
    //Look at each column, find the number of 1 nodes, and divide all of the 1 nodes by the number of 1 nodes
    int node_count = 0;
    for(int j = 0; j < num_nodes; j++){
        for(int i = 0; i < num_nodes; i++){
            if(matrix[i][j] > 0.5){
                node_count++;
            }
        }
        for(int i = 0; i < num_nodes; i++){
            if(matrix[i][j] > 0.5){
                matrix[i][j] = 1.0/node_count;
            }
        }
        node_count = 0;
    }
}

void free_all_data(){
    //Frees all data that is not going to be used for the second iteration (everything except threads)
    free(eigen);
    free(new_eigen);
    for(int i = 0; i < num_nodes; i++){
        free(matrix[i]);
    }
    free(matrix);

}

int main(int argc, char** argv){
    if(argc < 5){
        printf("Required 4 arguments: folder name containing graph data, number of graphs,threshold, number of threads");
        return -1;
    }


    total_time = 0;
    char filename[256];
    num_graphs = atoi(argv[2]);
    threshold = atof(argv[3]);
    num_threads = atoi(argv[4]);
    omp_set_num_threads(num_threads);

    //Timing for all iterations together
    for(int i = 0; i < num_graphs; i++){
        strcpy(filename, argv[1]);
        char temp[11];
        sprintf(temp, "%d", i);
        strcat(filename, "/");
        strcat(filename, temp);
        strcat(filename, ".txt");
        printf("---------------------------\n");
        printf("%s", filename);

        graph = fopen(filename, "r");

        create_matrix();
        clock_gettime(CLOCK_MONOTONIC, &iter_start_time);
        pagerank();
        clock_gettime(CLOCK_MONOTONIC, &iter_end_time);

        fclose(graph);
        printf("Probabilities from PageRank: ");
        for(int i = 0; i < num_nodes; i++){
            printf("%f, ", eigen[i]);

        }
        printf("\n");
        printf("Time for file %d: %f", i, difftimespec_ns(iter_end_time, iter_start_time)/ 1e9);
        printf("\n");
        total_time += difftimespec_ns(iter_end_time, iter_start_time)/ 1e9;

        free_all_data();

    }
    printf("---------------------------\n");
    printf("Total pagerank execution times across all files: %f", total_time);






}