/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * GRAPH.H
 */

#ifndef _GRAPH_H
#define _GRAPH_H 

#include <stdio.h>
#include <stdlib.h>


struct VertexNode {
    int vertexNumber;
    struct VertexNode* edgeElement;
};

typedef struct _Graph {
    int graphSize;
    struct VertexNode* graphArr;
} Graph;

// Initializes graph by node number.
Graph* initializeGraph(int max);

// This function works when you want to add an edge to the graph.
void addEdge(Graph* graph, int source, int destination); 

// It is written to re-initialize the graph when necessary.
Graph* reinitializeGraph(Graph* oldGraph, int maxNumberNode);

// The blocks separated by malloc for the graph structure are returned.
void freeGraph(Graph* _graph);

// It is written to understand whether the graph is properly read or not. It is not used in main.
void printGraph(Graph* graph);

#endif