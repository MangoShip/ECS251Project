#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"

FILE *current_file;

int min_graph, max_graph;


void generate_graph(){
    char filedata[1000000];
    char edge_data[1000000];
    int num_edges = 0;
    int size = rand() % (max_graph - min_graph) + min_graph;
    char temp[11];
    sprintf(temp, "%d", size);
    strcpy(filedata, temp);
    strcat(filedata, "\n");
    strcpy(edge_data, "");
    for(int i = 0; i < size; i++){
        for(int j = 0; j < size; j++){
            if(rand() % 4 == 0){
                strcat(edge_data, "\n");
                num_edges++;
                sprintf(temp, "%d", size);
                strcat(edge_data, temp);
                strcat(edge_data, " ");
                sprintf(temp, "%d", size);
                strcat(edge_data, temp);
            }
        }
    }
    if(num_edges == 0){
        num_edges = 1;
        strcat(edge_data, "\n0 1");
    }

    sprintf(temp, "%d", num_edges);
    strcat(filedata, temp);
    strcat(filedata, edge_data);

    fprintf(current_file, "%s", filedata);
}


int main(int argc, char** argv){
    if(argc < 3){
        printf("Required 4 arguments: folder name containing graph data, number of files to create, smallest num nodes, largest num nodes");
        return -1;
    }

    int num_files = atoi(argv[2]);
    char filename[256];
    min_graph = atoi(argv[3]);
    max_graph = atoi(argv[4]);

    for(int i = 0; i < num_files; i++){
        strcpy(filename, argv[1]);
        char temp[3];
        sprintf(temp, "%d", i);
        strcat(filename, "/");
        strcat(filename, temp);
        strcat(filename, ".txt");
        current_file = fopen(filename, "w");

        generate_graph();

        fclose(current_file);
    }
}