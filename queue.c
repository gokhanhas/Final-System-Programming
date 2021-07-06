/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * QUEUE.C
 */

#include "queue.h"

void initializeQueue(Queue* queue) {
    queue->size = 0;
    queue->first = NULL;
    queue->last = NULL;
}

void push(Queue* queue, int element) {
    
    Node* temp = (Node*)malloc(sizeof(Node) * 1);
    temp->value = element;
    temp->next = NULL;

    if(queue->size == 0) {
        queue->first = queue->last = temp;
    }
    else {
        queue->last->next = temp;
        queue->last = temp;
    }
    queue->size = queue->size + 1;
}

int pop(Queue* queue){
    if(queue->size == 0)
        return -1;
    int returnVal = queue->first->value;
    Node * temp = queue->first;
    queue->first = queue->first->next;
    queue->size = queue->size - 1;
    free(temp);
    return returnVal; 
}


void freeQueue(Queue* queue) {
    while(queue->first != NULL) {
        Node* temp = queue->first;
        queue->first = queue->first->next;
        free(temp);
    }
    if(queue->first != NULL)
        free(queue->first);
    free(queue);
}


void printPathQueue(Queue* queue, FILE* fileptr) {
    if(queue == NULL)
        return;
    Node* temp = queue->first;
    while(temp->next != NULL) {
        fprintf(fileptr,"%d->",temp->value);
        temp = temp->next;
    }
    fprintf(fileptr,"%d ",temp->value);
    fprintf(fileptr,"\n");
}

