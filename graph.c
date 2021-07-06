/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * GRAPH.C
 */

#include "graph.h"

Graph* initializeGraph(int max) {
    
    Graph* initGrap = (Graph*) malloc(sizeof(Graph));
    initGrap->graphSize = max;
    initGrap->graphArr = (struct VertexNode*) malloc(sizeof(struct VertexNode) * initGrap->graphSize);
    int i;
    for(i = 0; i < max; i++) {
        initGrap->graphArr[i].vertexNumber = -1;
        initGrap->graphArr[i].edgeElement = NULL;
    }   
    return initGrap;
}

void addEdge(Graph* graph, int source, int destination) {
    if(source == destination)
        return;
    
    int size = graph->graphSize;
    if(source > size) {
        graph = reinitializeGraph(graph, source);
    }
    
    graph->graphArr[source].vertexNumber = source;
    
    struct VertexNode* Node = &(graph->graphArr[source]);
    while(Node->edgeElement != NULL) {
        Node = Node->edgeElement;
    };
    Node->edgeElement = (struct VertexNode*) malloc(sizeof(struct VertexNode) * 1);
    Node = Node->edgeElement;
    Node->vertexNumber = destination;
    Node->edgeElement = NULL;       
}

Graph* reinitializeGraph(Graph* oldGraph, int maxNumberNode) {
    Graph* newGraph = (Graph*) malloc(sizeof(Graph) * maxNumberNode);
    int i;
    for(i = 0; i < maxNumberNode; i++) {
        if(oldGraph->graphArr[i].vertexNumber > 0) {
            newGraph->graphArr[oldGraph->graphArr[i].vertexNumber].vertexNumber = oldGraph->graphArr[i].vertexNumber;
            newGraph->graphArr[oldGraph->graphArr[i].vertexNumber].edgeElement = oldGraph->graphArr[i].edgeElement;
        }
    }
    newGraph->graphSize = maxNumberNode;
    freeGraph(oldGraph);
    return newGraph;
}

void freeGraph(Graph* _graph) {
    int size = _graph->graphSize;
    int i;
    for(i = 0; i < size; i++) {
        struct VertexNode* Node_x = NULL;
        while(_graph->graphArr[i].edgeElement != NULL) {
            Node_x = _graph->graphArr[i].edgeElement;
            _graph->graphArr[i].edgeElement = _graph->graphArr[i].edgeElement->edgeElement;

            if(Node_x != NULL) 
                free(Node_x);
        }
        
    }
    if(_graph->graphArr != NULL)
        free(_graph->graphArr);
    if(_graph != NULL)
        free(_graph);
}

void printGraph(Graph* graph) {
    int i, max = graph->graphSize;
    for(i = 0; i < max; i++) {
        if(graph->graphArr[i].vertexNumber != -1) {
            printf("%8d  ---->  ",graph->graphArr[i].vertexNumber);
            struct VertexNode* printNode = graph->graphArr[i].edgeElement;
            while(printNode != NULL) {
                printf("%d  ",printNode->vertexNumber);
                printNode = printNode->edgeElement;
            }
            printf("\n");
        }
    }
    printf("\n");
}