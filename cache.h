/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * CACHE.H
 */

#ifndef _CACHE_H
#define _CACHE_H

#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

typedef struct _cacheBlock {
    int last;
    int arraySize;
    int* path;
    struct _cacheBlock* next;
} CacheBlock;

typedef struct _cache {
    int cacheSize;
    int first;
    CacheBlock* nextBlock;
} CacheEntry;


// Assigns initial values to all elements of the cache array.
void initializeCache(CacheEntry* cacheEntryArr, int size);

// This function works when an element is added to the cache. Necessary information is taken from Queue * as a parameter.
void addLast(CacheEntry* cacheEntryArr, Queue* pathx);

// All resources used by cache are free.
void freeCache(CacheEntry* cacheEntryArr);

//  It is used in the case of searching for a path in cache. Returns the data structure of type CacheBlock *. 
// Path is printed and send to from this structure.
CacheBlock* searchCache(CacheEntry* cacheEntryArr, int source, int destination);

// In the cache, it is written for control purposes to understand whether the elements are added properly or not.
void printCacheBlock(CacheBlock* block);


#endif // _CACHE_H 