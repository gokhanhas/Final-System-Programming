/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * CACHE.C
 */


#include "cache.h"

void initializeCache(CacheEntry* cacheEntryArr, int size) {
    int i;
    for(i = 0;i < size; i++) {
        cacheEntryArr[i].cacheSize = size;
        cacheEntryArr[i].first = i;
        cacheEntryArr[i].nextBlock = NULL;
    }
}

void addLast(CacheEntry* cacheEntryArr, Queue* pathx) {
    
    int first = pathx->first->value;
    if(cacheEntryArr[first].nextBlock == NULL) {
        cacheEntryArr[first].nextBlock = (CacheBlock*) malloc(sizeof(CacheBlock) * 1);
        cacheEntryArr[first].nextBlock->last = pathx->last->value;
        cacheEntryArr[first].nextBlock->arraySize = (pathx->size);
        cacheEntryArr[first].nextBlock->path = (int*) malloc(sizeof(int) * (pathx->size)); 
        Node* temp = pathx->first;
        int i;
        for(i=0; i < pathx->size; i++) {
            cacheEntryArr[first].nextBlock->path[i] = temp->value;
            temp = temp->next;
        }
        cacheEntryArr[first].nextBlock->next = NULL;
        return;
    }
    
    CacheBlock* oneBlock = cacheEntryArr[first].nextBlock;
    while(oneBlock->next != NULL) {
        oneBlock = oneBlock->next;
    }
    
    oneBlock->next = (CacheBlock*) malloc(sizeof(CacheBlock) * 1);
    oneBlock->next->last = pathx->last->value;
    oneBlock->next->arraySize = pathx->size;
    oneBlock->next->path = (int*) malloc(sizeof(int) * (pathx->size)); 
    Node* temp = pathx->first;
    int i;
    for(i=0; i < pathx->size; i++) {
        oneBlock->next->path[i] = temp->value;
        temp = temp->next;
    }
    oneBlock->next->next = NULL;
}

void freeCache(CacheEntry* cacheEntryArr) {
    int size = cacheEntryArr[0].cacheSize;
    int i;
    for(i = 0; i < size; i++) {
        CacheBlock* temp;
        while(cacheEntryArr[i].nextBlock != NULL) {
            temp = cacheEntryArr[i].nextBlock;
            cacheEntryArr[i].nextBlock = cacheEntryArr[i].nextBlock->next;
            if(temp != NULL) {
                if(temp->path != NULL)
                    free(temp->path);
                free(temp);
            }
        }
    }
    free(cacheEntryArr);
}

CacheBlock* searchCache(CacheEntry* cacheEntryArr, int source, int destination) {
    CacheBlock*  sourceBlock = cacheEntryArr[source].nextBlock;
    while(sourceBlock != NULL) {
        if(sourceBlock->last == destination) {
            return sourceBlock;
        }
        sourceBlock = sourceBlock->next;
    }
    return NULL;
}

void printCacheBlock(CacheBlock* block) {
    if(block == NULL) {
        printf("NULL GELDI");
    }
    printf("LAST : %d, ARRAYSIZE : %d\n",block->last, block->arraySize);
    int s = block->arraySize;
    if(block->path == NULL) {
        printf("PATH NULL GELDI\n");
        return;
    }
    for(int i = 0; i < s; i++) {
        printf("%d - ",block->path[i]);
    }
    printf("\n");
}