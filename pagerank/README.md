# PageRank

The programs created here are algorithms used to compute PageRank values for a graph.

## Usage
For data-generate, you call the program with the following arguments:
* Folder name containing graph data
* Number of graphs needed to be created
* Smallest number of nodes in a graph
* Largest number of nodes in a graph

Example of creating 10 graphs that contain between 100 and 200 nodes in the folder ../data:
```python
make
./data-generate ../data 10 100 200
```

For pagerank-parallel, pagerank-tholder, and pagerank-serial, you call the program with the following arguments:
* Folder name containing graph data
* Number of graphs that are in the data folder
* Threshold needed to reach before stopping PageRank
* Number of threads to be created (for parallel and tholder versions)

Example of running 10 graphs in the folder ../data on all pageranks:
```python
make
./pagerank-parallel ../data 10 0.000001 8
./pagerank-tholder ../data 10 0.000001 8
./pagerank-serial ../data 10 0.000001
```
