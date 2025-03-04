#include "stdio.h"
#include "math.h"
#include "stdlib.h"
#include "string.h"
#include "stdbool.h"

FILE *current_file;

int min_graph, max_graph;


void generate_graph(){
    char filedata[10000];
    char edge_data[10000];
    int num_edges = 0;
    int size = 0;
    if(max_graph == min_graph){
        size = max_graph;

    }
    else{
        size = rand() % (max_graph - min_graph) + min_graph;
    }
    char temp[11];
    sprintf(temp, "%d", size);
    strcpy(filedata, temp);
    fprintf(current_file, "%s", filedata);
    strcpy(edge_data, "");
    int num_connections = 3;
    int connections[] = {-1,-1,-1};
    bool found = true;
    for(int i = 0; i < size; i++){
      for(int j = 0; j < num_connections; j++){
        int space = rand() % size;
        while(found){
	    found = false;
          space = rand() % size;
          for(int k = 0; k < num_connections; k++){
            if(connections[k] == space){
	      found = true;
              space = rand() % size;
	      break;
            }
          }
        }
	    connections[j] = space;
      }
    for(int j = 0; j < num_connections; j++){
	    strcpy(edge_data, "\n");
        num_edges++;
        sprintf(temp, "%d", i);
        strcat(edge_data, temp);
        strcat(edge_data, " ");
        sprintf(temp, "%d", connections[j]);
        strcat(edge_data, temp);
        fprintf(current_file, "%s", edge_data);

       }

    }

    if(num_edges == 0){
        num_edges = 1;
        fprintf(current_file, "\n 0 1");
    }

    fprintf(current_file, "\nEND");
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
        char temp[11];
        sprintf(temp, "%d", i);
        strcat(filename, "/");
        strcat(filename, temp);
        strcat(filename, ".txt");
        current_file = fopen(filename, "w");

        generate_graph();

        fclose(current_file);
    }
}