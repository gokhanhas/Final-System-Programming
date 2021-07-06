/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * SERVER SIDE
 */

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/mman.h>
#include <time.h> 
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <fcntl.h>


#include "graph.h"
#include "cache.h"
#include "queue.h"

#define FALSE 0
#define TRUE 1
#define MAX_SIZE 800000


typedef struct _oneThread {
    int id;
    int isWorking;
    int socket_fd;
    int source;
    int destination;
    int pipe[2];
    pthread_t thread;
    struct _oneThread* nextThread;
} OneThread;


typedef struct _threadPool {
    int size;
    int maxCapacity;
    int* ids;
    void (*function) (void*);
    pthread_cond_t* conditionArr;
    pthread_mutex_t* mutexes;
    OneThread* firstThread;
    OneThread* lastThread;
} ThreadPool_t;


typedef struct which_ {
    int _id;
    char which;
} threadClasification;



/* GLOBAL VARIABLES */
// FOR COMMAND LINE ARGUMENTS 
int portNumber = 0, rVal = 0;
int minThreadCount = 0, maxThreadCount = 0;
int ifThreadPoolCreated = FALSE;
int maxNodeNumber = 0;
Graph* graph; 
CacheEntry* cache;

// FOR DYNAMIC POOL THREAD OPERATIONS
pthread_t dynamicThread;
int dynamicPipe[2];

// FOR THREAD POOL
ThreadPool_t* threadPool = NULL;
pthread_t dynamicPoolOperation_thread;
pthread_mutex_t threadPoolMutex;
pthread_cond_t threadPoolConddition;

// FOR INPUT AND LOG FILE
FILE* inputFile = NULL;
FILE* logFile   = NULL;
pthread_mutex_t logFileMutex;

// FOR READERS, WRITERS PRIORIZATION 
static int activeReaders = 0, activeWriters = 0;
static int waitingReaders = 0, waitingWriters = 0;
pthread_mutex_t okToReadMutex, okToWriteMutex;
pthread_cond_t okToRead_Cond, okToWrite_Cond;

int* returnMessages = NULL;
static int SIGINT_FLAG = 0;

static int controlPercent = 0;
double oldPercent;
int final = 2; 
int finalArr[3] = {0,0,0};
int duplicateControl = 0;

// FOR SOCKET OPERATION ...
int newFD, serverFD;
int op = 1;
struct sockaddr_in socketAddress;
int socketAdressSize = sizeof(socketAddress);
/* END GLOBAL VARIABLES */


/* FUNCTION DEFINE */

// Print error message and exit gracefully.
void errorExit(char* error);

// Print program usage if the user entered wrong input.
void printUsage();

// Same arguments must be digit, fx portnumber, control these arguments.
int controlIfArgumentDigit(char* argumentValue);

// The server should not run twice at the same time.
int controlDuplicateServer();

// Returns the maximum number of nodes in the graph.
size_t getMaksimumNodeNumber(char* fileName, int* edgeNumber);

// The graph is initialized by reading the file line by line.
int readFileAndInitializeGraph(char *fileName, Graph* graph, int nodeNumber, int edgeNumber) ; 

//  When the server runs for the first time in the log file, the messages are printed. 
// Only timestamp is placed in the last of these messages.
void printStartMessageToLogFile(char* iVal, char* oVal);

//  It is the function that calculates the classical BFS algorithm. With the help of the queue, bfs is calculated. At the level level, 
// it is not always possible to go to the next level before the upper level ends. If there is an edge between nodes, 
// the shortest is always found.
int BFSalgorithm(Graph* graph, int source, int destination, int* visitedArr, int* path, int* pathIndex, int* count, int maxEdge) ;

// According to the BFS algorithm, the path between the nodes must be found and saved in the cache or sent to the clients.
void getPath(Queue * queue, int* path, int destination, int maxNode, int kIndex, int count);

// It is the thread function that changes the reputation of the thread pool. Adding to threadpool is done in this function.
void* reinitializeThreadPool(void* argument);

// It is the main function where threads will work. Incoming requests are calculated here, if any, search or add operation in cache.
void* response_threads(void* argument);

// This is the function that initializes threadpool initially.
void initializeThreadPool();

// Resources used for threadpool are free.
void freeThreadPool();

// Returns the thread structure not working.
OneThread* returnEmptyThread();

// Returns the number of threads running.
int getRunningThreadCount();

// Prepares the variables required for BFS.
void initializeBFSarrays(int* path, int* visitedArr);

// If path cache is found, this function is used to write to the log file.
void printPathFromDataBase(CacheBlock* block);

// It is the signal handler function.
void signal_handler(int sigNo);

// Creating a socket in C programming language is done in this function.
void create_server();

// When CTRLC (SIGINT) arrives, threads running at that time must be finished.
void waitAllThreads();

// PrintMessages ...
void printConnectionMessage(int id);
void printSearchDatabase(int id, int src, int dest);
void printNoDatabase(int id, int src, int dest);
void printPathCalculated(int id, Queue* path_queue);
void printRespondingMsg(int id);
void printPathNoPossible(int id, int src, int dest);
void printPathFoundDatabase(int id, CacheBlock* searched);

/* END FUNCTION DEFINE */


int main(int argc, char **argv) {
    
    // Assignments and functions required for the signal catcher.
    struct sigaction sact;
    sact.sa_handler = &signal_handler;
    sact.sa_flags = 0;
    sigfillset(&sact.sa_mask);
    sigemptyset( &sact.sa_mask );
    if(sigaction(SIGINT,&sact, NULL) != 0) {
        errorExit("ERROR ! sigaction ");
    }
    controlDuplicateServer();
    int parentId = 0, sID = 0;
    char *iValue = NULL;
    char *pValue = NULL;
    char *oValue = NULL;
    char *sValue = NULL; 
    char *xValue = NULL;
    char *rValue = NULL;

    int iIndex = 0;
    int pIndex = 0;
    int oIndex = 0;
    int sIndex = 0;
    int xIndex = 0;
    int rIndex = 0;

    int another = 0;
    int c = 0;

    while ((c = getopt(argc, argv, "i:p:o:s:x:r:")) != -1)
    switch (c) {
        case 'i':
            iIndex += 1;
            iValue = optarg;
            break;
        
        case 'p':
            pIndex += 1;
            pValue = optarg;
            break;
        
        case 'o':
            oIndex += 1;
            oValue = optarg;
            break;

        case 's':
            sIndex += 1;
            sValue = optarg;
            break;

        case 'x':
            xIndex += 1;
            xValue = optarg;
            break;
        
        case 'r':
            rIndex += 1;
            rValue = optarg;
            break;

        case '?':
            another += 1;
            break;

        default:
            abort();
    } 

    if(iIndex == 1 && pIndex == 1 && oIndex == 1 && sIndex == 1 && xIndex == 1 && rIndex == 1 && another == 0) {
        // There is no problem with command line arguments ...
        
        if(!controlIfArgumentDigit(pValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -p argument must be number ");
        }
        if(!controlIfArgumentDigit(sValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -s argument must be number ");            
        }
        if(!controlIfArgumentDigit(xValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -x argument must be number ");            
        }
        if(!controlIfArgumentDigit(rValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -r argument must be number ");            
        }


        portNumber = atoi(pValue);
        minThreadCount = atoi(sValue);
        maxThreadCount = atoi(xValue);
        rVal = atoi(rValue);
       
        
        if(minThreadCount < 2) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -s parameter value must be at least 2 ");
        }

        if(portNumber < 0) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -p : PortNumber problem ");
        }

        if(maxThreadCount < 0 || maxThreadCount < minThreadCount) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -x : Maximum allowed number of threads problem ");
        }
        
        if(!(rVal == 0 || rVal == 1 || rVal == 2)){
            errno = EIO;
            printUsage();
            errorExit("ERROR : -r : r must be 0, 1 or 2 ");
        }
        
    } else {
        errno = EIO;
        printUsage();
        errorExit("ERROR ");
    }
    
    if(strcmp(iValue, oValue) == 0) {
        errno = EIO;
        errorExit("ERROR ! input file and log file are same ");
    }

    if(SIGINT_FLAG == 1) 
        exit(EXIT_SUCCESS);
    
    // START THE DEAMON PROCESS ...
    parentId = fork();
    if (parentId < 0)
        errorExit("ERROR : Fork problem ");
    if (parentId > 0) { 
        // Exit gracefully the parent process ...
        exit(EXIT_SUCCESS);
    }
    umask(0);
    sID = setsid();
    if(sID < 0)
        errorExit("ERROR : sID problem ");
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    // END DEAMON OPERATIONS ...
    
    // CREATE LOG FILE ...
    returnMessages = (int*) malloc(sizeof(int) * maxThreadCount);
    logFile = fopen(oValue, "a+");
    if(logFile == NULL) {
        free(returnMessages);
        errorExit("ERROR : Logfile does not open ");
    }
    printStartMessageToLogFile(iValue, oValue);
    
    // CREATE DYNAMIC THREAD PIPE ...
    if(pipe(dynamicPipe) == -1) {
        errorExit("ERROR : pipe problem ");
    }
    
    // INITIALIZE GRAPH ...
    int edgeCounter = 0;
    maxNodeNumber = getMaksimumNodeNumber(iValue, &edgeCounter);
    graph = initializeGraph(maxNodeNumber);
    readFileAndInitializeGraph(iValue, graph, maxNodeNumber, edgeCounter);
    
    // CREATE CACHE ...
    cache = (CacheEntry*) malloc(sizeof(CacheEntry) * maxNodeNumber);
    initializeCache(cache, maxNodeNumber);

    // THREADPOOL MALLOC AND INITIALIZE ...
    threadPool = (ThreadPool_t*) malloc(sizeof(ThreadPool_t) * 1);
    ifThreadPoolCreated = TRUE;
    initializeThreadPool();

    // CREATE SERVER ...
    create_server();

    // ENDLESS LOOP CONTINUING TO SIGNAL INCOMING.
    while(SIGINT_FLAG == 0) {

        if((newFD = accept(serverFD, (struct sockaddr*)&socketAddress,(socklen_t*)&socketAdressSize)) < 0) {
            if(SIGINT_FLAG == 1)
                break;
            else 
                errorExit("ERROR ! accept problem "); 
        }
        
        OneThread* temp = returnEmptyThread();
        if(temp != NULL){
            // The process of reading the numbers from the client.
            temp->socket_fd = newFD;
            int* readedNumbers = (int *)malloc(sizeof(int) * 2);
            if(recv(newFD, readedNumbers, 8, 0) == -1) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! recv (1) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
            }
                
            int* goToThread = (int*) malloc(sizeof(int) * 4);
            goToThread[0] = 1;
            goToThread[1] = newFD;
            goToThread[2] = readedNumbers[0];
            goToThread[3] = readedNumbers[1];
            temp->isWorking = 1;
            temp->socket_fd = newFD;
            
            // Calculation of the percentage value.
            double percent = (((double)(100.0/threadPool->size))*(double)getRunningThreadCount());

            // ######################### WRITE LOG FILE ############################
            time_t tcurrent = time(NULL);
            char* time_str = strtok(ctime(&tcurrent), "\n");
            pthread_mutex_lock(&logFileMutex);
            fprintf(logFile,"%s :\tA connection has been delegated to thread id #%d, system load %.2lf%%\n",time_str,temp->id, percent);
            fflush(logFile);
            pthread_mutex_unlock(&logFileMutex);
            // #####################################################################
            
            // Sending information to the thread.
            if(write(temp->pipe[1], goToThread, sizeof(int) * 4) == -1)
                errorExit("ERROR ! write 1 problem ");

            if(percent >= 75.00 && ((threadPool->size) < maxThreadCount)) {
                // If this condition is working, resize will be done. Therefore, the necessary information is sent to the thread to be resized.
                if(controlPercent == 0) {
                    oldPercent = percent;
                    controlPercent = 1;
                }
                int whichOperation = 0; 
                if(write(dynamicPipe[1], &whichOperation, sizeof(int)) == -1) {
                    time_t tcurrentERR = time(NULL);
                    char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                    fprintf(logFile, "%s :\tERROR ! write (1) problem %s\n", time_strERR, strerror(errno));
                    fflush(logFile);
                }
            }
            free(readedNumbers);
            free(goToThread);
        }
        else {
            /* The maximum number of threads entered by the user has been created and should be waited 
                if there is no thread at the moment. The thread that has finished its work will wake up here. */
            pthread_mutex_lock(&threadPoolMutex);
            pthread_cond_wait(&threadPoolConddition, &threadPoolMutex);
            pthread_mutex_unlock(&threadPoolMutex);
        }
    }
    
    // CLOSING THE SOCKET ...
    close(serverFD);
    // WAITING FOR ALL THREADS ...
    waitAllThreads();
    freeThreadPool();
    freeGraph(graph);
    freeCache(cache);
    if(returnMessages != NULL)
        free(returnMessages);

    time_t tcurrent = time(NULL);
    char* timeSTR = strtok(ctime(&tcurrent), "\n");
    fprintf(logFile,"%s :\tAll threads have terminated, server shutting down.\n",timeSTR);
    fflush(logFile);
    unlink("controlXXX");
    if(errno = fclose(logFile) != 0) 
        errorExit("ERROR ! LogFile does not close ");
    return 0;
}

void errorExit(char* error) {
    perror(error);
    if(duplicateControl == 0) {
        unlink("controlXXX");
        if(logFile != NULL)
            fclose(logFile); 
    }
    exit(EXIT_FAILURE);
}

void printUsage() {
    printf("\n");
    printf("################################# USAGE ##################################\n");
    printf("# ./server -i pathToFile -p PORT -o pathToLogFile -s 4 -x 24 -r 0        #\n");
    printf("# -i :      denotes the relative or absolute path to an input file       #\n");
    printf("#               containing a directed unweightedgraph                    #\n");
    printf("# -p :    this is the port number the server will use for incoming       #\n");
    printf("#                          connections                                   #\n");
    printf("# -o :    is the relative or absolute path of the log file to which      #\n");
    printf("# the server daemon will write all of its output (normal output&errors). #\n");
    printf("# -s : this is the number of threads in the pool at startup (at least 2) #\n");
    printf("# -x :  this is the maximum allowed number of threads, the pool must not #\n");
    printf("#                       grow beyond this number                          #\n");
    printf("# -r :                 0 -> reader prioritization                        #\n");
    printf("#                      1 -> writer prioritization                        #\n");
    printf("#             2 -> equal priorities to readers and writers               #\n");
    printf("##########################################################################\n\n");
}

int controlIfArgumentDigit(char* argumentValue) {
    int i = 0;
    for(i = 0; i < strlen(argumentValue); i++) {
        if(isdigit(argumentValue[i]) == 0)
            return FALSE;
    }
    return TRUE;
}

int controlDuplicateServer() {
    
    if(access("controlXXX", R_OK) != -1) {
        duplicateControl = 1;
        errno = EIO;
        errorExit("ERROR ! SERVER HAS ALREADY RUNNING ");
    }

    FILE* controlFile = NULL;
    controlFile = fopen("controlXXX", "w+");
    if(controlFile == NULL) {
        errno = EIO;
        errorExit("ERROR ! CONTROL FILE IS NOT OPEN ");
    }

    fclose(controlFile);
    return TRUE;
}

size_t getMaksimumNodeNumber(char* fileName, int* edgeNumber) {
    inputFile = fopen(fileName, "r");
    if(inputFile == NULL) {
        free(returnMessages);
        errorExit("ERROR ! InputFile does not open ");
    }
    int maxNodeNumber = 0;
    char line[255];
    while(fgets(line,255,inputFile)) {
        if(line[0] != 35) { // control '#' character in graph input file
            (*edgeNumber)++;
            int source = 0, destination = 0, maxValLine = 0 ; 
            sscanf(line, "%d\t%d\n",&source, &destination);
            if(source > destination)
                maxValLine = source;
            else
                maxValLine = destination;
            
            if(maxValLine > maxNodeNumber)
                maxNodeNumber = maxValLine;   
        }
    }
    
    if(errno = fclose(inputFile) != 0) {
        free(returnMessages);
        errorExit("ERROR : file close porblem ");
    }
    return maxNodeNumber+1;
}

int readFileAndInitializeGraph(char *fileName, Graph* graph, int nodeNumber, int edgeNumber) {
    clock_t first, second;
    inputFile = fopen(fileName, "r");
    if(inputFile == NULL) {
        free(returnMessages);
        errorExit("ERROR ! InputFile does not open ");
    }
    first = clock();
    int count = 0;
    char line[255];
    while(fgets(line,255,inputFile)) {
        if(line[0] != 35) {
            int source = 0, destination = 0; 
            sscanf(line, "%d\t%d\n",&source, &destination);
            count++;
            if(source < 0 || destination < 0) {
                time_t tcurrent = time(NULL);
                char* time_str = strtok(ctime(&tcurrent), "\n");
                fprintf(logFile,"%s :\tThere is a negative value in line %d of the file. This line is not added to the graph.\n",time_str,count);
                fflush(logFile);
            }
            else
                addEdge(graph, source, destination);       
        }
        if(SIGINT_FLAG == 1){
            if(errno = fclose(inputFile) != 0) {
                free(returnMessages);
                errorExit("ERROR : input file close problem ");
            }
            return FALSE;
        }
    }
    if(errno = fclose(inputFile) != 0) {
        free(returnMessages);
        errorExit("ERROR : input file close problem ");
    }
    second = clock();
    double diff = ((double)(second-first)) / CLOCKS_PER_SEC;
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    fprintf(logFile,"%s :\tGraph loaded in %.2f seconds with %d nodes and %d edges.\n",time_str,diff, nodeNumber, edgeNumber);
    fflush(logFile);
    return TRUE;
}

void printStartMessageToLogFile(char* iVal, char* oVal) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    fprintf(logFile, "Executing with parameters:\n-i %s\n-p %d\n-o %s\n-s %d\n-x %d\n%s :\tLoading graph...\n",iVal, portNumber, oVal, minThreadCount, maxThreadCount, time_str);
    fflush(logFile);
}

int BFSalgorithm(Graph* graph, int source, int destination, int* visitedArr, int* path, int* pathIndex, int* count, int maxEdge) {
    int cur;
    visitedArr[source] = 1;
    Queue* myqueue = (Queue*) malloc(sizeof(Queue) * 1);
    initializeQueue(myqueue);
    push(myqueue, source);
    // Since the vertexes at the same level will be navigated in order, they are thrown into the queue.
    while(myqueue->size != 0) {
        cur = pop(myqueue);
        struct VertexNode* Node = &(graph->graphArr[cur]);
        while(Node->edgeElement != NULL) { // All nodes adjacent to that vertex should be checked.
            Node = Node->edgeElement;
            if(visitedArr[Node->vertexNumber] == 0) {
                // Visited array is kept. If I have visited that vertex, 
                // I should not visit it again. Otherwise infinite loop occurs.
                visitedArr[Node->vertexNumber] = 1;
                //  1 is placed on the vertex array. Thus, it is understood that the vertex is 
                // looked at while the cycle continues.
                path[Node->vertexNumber] = cur;
                //  Vertexs I visit are thrown back to queue. Thus, the problem of looking at the nodes 
                // at the same level is fixed by keeping the level levels the same.
                push(myqueue, Node->vertexNumber);
                if(Node->vertexNumber == destination){
                    // path founded ...
                    freeQueue(myqueue);
                    return TRUE;
                }
            } else if(Node->vertexNumber == destination && source == destination) {
                // Means that source and destination is same ...
                path[destination] = cur;
                freeQueue(myqueue);
                return TRUE;
            }

        }
        if(Node->edgeElement != NULL) { // All nodes adjacent to that vertex should be checked.
            Node = Node->edgeElement;
            if(visitedArr[Node->vertexNumber] == 0) {
                // Visited array is kept. If I have visited that vertex, 
                // I should not visit it again. Otherwise infinite loop occurs.
                visitedArr[Node->vertexNumber] = 1;
                //  1 is placed on the vertex array. Thus, it is understood that the vertex is 
                // looked at while the cycle continues.
                path[Node->vertexNumber] = cur;
                //  Vertexs I visit are thrown back to queue. Thus, the problem of looking at the nodes 
                // at the same level is fixed by keeping the level levels the same.
                push(myqueue, Node->vertexNumber);
                if(Node->vertexNumber == destination){
                    // path founded ...
                    freeQueue(myqueue);
                    return TRUE;
                }
            } else if(Node->vertexNumber == destination && source == destination) {
                // Means that source and destination is same ...
                path[destination] = cur;
                freeQueue(myqueue);
                return TRUE;
            }
        } 
    }
    // There is no path between source and destination ...
    freeQueue(myqueue);
    return FALSE;
}

void waitAllThreads() {
    write(dynamicPipe[1], &final, sizeof(int));
    if(pthread_join(dynamicThread, NULL) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_join dynamic thread problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR ! pthread_join ");
    }
    OneThread* temp = threadPool->firstThread;
    for(int i = 0; i < threadPool->size; i++) {
        write(temp->pipe[1], &finalArr, sizeof(int)*3);
        temp = temp->nextThread;
    }  
}

void getPath(Queue * queue, int* path, int destination, int maxNode, int kIndex, int count) {
    int* arr = (int*) malloc(sizeof(int) * maxNode);
    arr[kIndex] = destination;
    kIndex++;
    int dst = destination;
    while(path[destination] != -15) {
        arr[kIndex] =  path[destination];
        if(path[destination] == dst) {
            kIndex++;
            break;
        }    
        else 
            destination = path[destination];
        kIndex++;
    }
    kIndex--;
    for(int k = kIndex; k >= 0; k--) {
        int value = arr[k];
        push(queue, value);
    }
    free(arr);
}

void* reinitializeThreadPool(void* argument) {
    while(TRUE) {
        int whichOper;
        read(dynamicPipe[0], &whichOper, sizeof(int));
        if(whichOper == 2) {
            return NULL;
        }
        else {
            // THREADPOOL GROWS ...
            int oldSize = threadPool->size;
            int addedThread = oldSize / 4;
            if(addedThread == 0) {
                addedThread = oldSize * 2;
            }
            if(addedThread + oldSize >= maxThreadCount) {
                addedThread = maxThreadCount - oldSize;
            }
            if(addedThread == 0) {
                break;
            }
            // ######################### WRITE LOG FILE ############################
            time_t tcurrent = time(NULL);
            char* time_str = strtok(ctime(&tcurrent), "\n");
            pthread_mutex_lock(&logFileMutex);
            fprintf(logFile,"%s :\tSystem load %.2f%%, pool extended to %d threads.\n",time_str, oldPercent, (threadPool->size + addedThread));
            fflush(logFile);
            pthread_mutex_unlock(&logFileMutex);
            controlPercent = 0;
            // #####################################################################
            int i = 0;
            while(i < addedThread) {
                threadPool->lastThread->nextThread = (OneThread*) malloc(sizeof(OneThread) * 1);
                threadPool->lastThread->nextThread->id = oldSize + i;
                threadPool->lastThread->nextThread->isWorking = 0;
                threadPool->lastThread->nextThread->socket_fd = -1;
                threadPool->lastThread->nextThread->source = -1;
                threadPool->lastThread->nextThread->destination = -1;
                if(pipe((threadPool->lastThread->nextThread)->pipe) == -1) {
                    errorExit("ERROR pipe problem ");
                }
                
                if(pthread_create(&(threadPool->lastThread->nextThread->thread), NULL, response_threads, (threadPool->lastThread->nextThread)) != 0) {
                    errorExit("ERROR pthread_create ");
                }
                // ADDED IS O(1). BECAUSE LAST NODE IS KEEPING ...
                threadPool->lastThread = threadPool->lastThread->nextThread;
                i++;
            }
            threadPool->size = oldSize + addedThread;
        }
    }
    return NULL;
}


void printConnectionMessage(int id) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: waiting for connection\n",time_str, id);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printSearchDatabase(int id, int src, int dest) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: searching database for a path from node %d to node %d\n",time_str,
        id, src, dest);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printNoDatabase(int id, int src, int dest) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: no path in database, calculating %d->%d\n",time_str,
            id, src, dest);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printPathCalculated(int id, Queue* path_queue) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: path calculated: ",time_str, id);
    printPathQueue(path_queue, logFile);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printRespondingMsg(int id) {
    time_t tcurrent = time(NULL);
    char *time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: responding to client and adding path to database\n",time_str, id);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printPathNoPossible(int id, int src, int dest) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: path not possible from node %d to %d\n",time_str, id, src, dest);
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void printPathFoundDatabase(int id, CacheBlock* searched) {
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    pthread_mutex_lock(&logFileMutex);
    fprintf(logFile,"%s :\tThread #%d: path found in database: ",time_str, id);
    printPathFromDataBase(searched);   
    fflush(logFile);
    pthread_mutex_unlock(&logFileMutex);
}

void* response_threads(void* argument) {

    OneThread* thread_one = (OneThread *) (argument);
    thread_one->isWorking = 0;
    int id = thread_one->id;
    printConnectionMessage(id);
    int arr[MAX_SIZE];
    
    while(TRUE) {
        int *temp = (int *)malloc(sizeof(int) * 4 * sizeof(int));
        read(thread_one->pipe[0],temp, 4 * sizeof(int));
        thread_one->isWorking = 1;
        if(temp[0] == 0){
            free(temp);
            break;
        }
        int fd = temp[1], source = temp[2], destination = temp[3];
        
        // READERS, WRITERS PRIORIZATION
        if(rVal == 2) 
            pthread_mutex_lock(&okToReadMutex);
        
        if(rVal == 0) {
            // READERS ...
            pthread_mutex_lock(&okToReadMutex);
            while(activeReaders + activeWriters > 0) {
                waitingReaders++;
                pthread_cond_wait(&okToRead_Cond, &okToReadMutex);
                waitingReaders--;
            }
            activeReaders++;
            pthread_mutex_unlock(&okToReadMutex);
        }
        else if(rVal == 1) {
            // WRITERS ...
            pthread_mutex_lock(&okToWriteMutex);
            while(activeWriters + waitingWriters > 0) {
                waitingReaders++;
                pthread_cond_wait(&okToRead_Cond, &okToWriteMutex);
                waitingReaders--;
            }
            activeReaders++;
            pthread_mutex_unlock(&okToWriteMutex);   
        } 
        printSearchDatabase(id, source, destination);
        // SEARCHING IN CACHE ...
        CacheBlock* searchingOrWritingPath = NULL;
        searchingOrWritingPath = searchCache(cache, source, destination);
        
        if(rVal == 0) {
            // READERS ...
            pthread_mutex_lock(&okToReadMutex);
            activeReaders--;
            if(waitingReaders > 0)
                pthread_cond_signal(&okToRead_Cond);
            else if(waitingWriters > 0)
                pthread_cond_broadcast(&okToWrite_Cond);
            pthread_mutex_unlock(&okToReadMutex);
        } else if(rVal == 1) {
            // WRITERS ...
            pthread_mutex_lock(&okToWriteMutex);
            activeReaders--;
            if(activeReaders == 0 && waitingWriters > 0) {
                pthread_cond_signal(&okToWrite_Cond);
            }
            pthread_mutex_unlock(&okToWriteMutex);
        }
        
        if(searchingOrWritingPath == NULL){
            // PATH IS NOT IN CACHE ...
            printNoDatabase(id, source, destination);
            int* visitedArr = (int*) malloc(sizeof(int) * maxNodeNumber);
            int* path = (int*) malloc(sizeof(int) * maxNodeNumber);
            initializeBFSarrays(path, visitedArr);
            int pathIndex = 0, count = -15;
            int resultBFS = BFSalgorithm(graph, source, destination, visitedArr, path, &pathIndex, &count, maxNodeNumber);
            Queue* PathQueue; 

            if(resultBFS == TRUE) {
                // PATH CALCULATED ...
                PathQueue = (Queue*) malloc (sizeof(Queue) * 1);
                initializeQueue(PathQueue);
                getPath(PathQueue, path, destination, maxNodeNumber, 0, count);
                printPathCalculated(id, PathQueue);
                if(rVal == 0) {
                    // READERS ...
                    pthread_mutex_lock(&okToReadMutex);
                    while(activeReaders + waitingReaders > 0) {
                        waitingWriters++;
                        pthread_cond_wait(&okToWrite_Cond, &okToReadMutex);
                        waitingWriters--;
                    }
                    activeWriters++;
                    pthread_mutex_unlock(&okToReadMutex);
                } else if (rVal == 1) {
                    // WRITERS ...
                    pthread_mutex_lock(&okToWriteMutex);
                    while(activeWriters + activeReaders > 0) {
                        waitingWriters++;
                        pthread_cond_wait(&okToWrite_Cond,&okToWriteMutex);
                        waitingWriters--;
                    }
                    activeWriters++;
                    pthread_mutex_unlock(&okToWriteMutex);
                }
                printRespondingMsg(id); 
                // ADD PATH TO CACHE ...
                addLast(cache, PathQueue);
                
                // SEND RESULT TO CLIENT ...
                Node* tempx = PathQueue->first;
                int g = 0, size = PathQueue->size;
                while(g < size) {
                    arr[g] = tempx->value;
                    g++;
                    tempx = tempx->next;
                }
                arr[g] = -1;
                if(send(fd, &arr, (size+1)*4,0) == -1) {
                    time_t tcurrentERR = time(NULL);
                    char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                    fprintf(logFile, "%s :\tERROR ! send (1) problem %s\n", time_strERR, strerror(errno));
                    fflush(logFile);
                }

                if(rVal == 0) {
                    // READERS ...
                    pthread_mutex_lock(&okToReadMutex);
                    activeWriters--;
                    if(activeWriters == 0 && waitingReaders > 0) {
                        pthread_cond_signal(&okToRead_Cond);
                    }
                    pthread_mutex_unlock(&okToReadMutex);
                } else if(rVal == 1) {
                    // WRITERS ...
                    pthread_mutex_lock(&okToWriteMutex);
                    activeWriters--;
                    if(waitingWriters > 0) {
                        pthread_cond_signal(&okToWrite_Cond);
                    } else if(waitingReaders > 0) {
                        pthread_cond_broadcast(&okToRead_Cond);
                    }
                    pthread_mutex_unlock(&okToWriteMutex);
                }

                freeQueue(PathQueue);            
            } 
            else {
                // THERE IS NO PATH SOURCE TO DESTINATION IN CACHE ...
                printPathNoPossible(id, source, destination);
                // SEND RESULT TO CLIENT ...
                int g = 0;
                while(g < 20) {
                    arr[g] = -1; 
                    g++;
                }
                if(send(fd, &arr, 80, 0) == -1) {
                    time_t tcurrentERR = time(NULL);
                    char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                    fprintf(logFile, "%s :\tERROR ! send (2) problem %s\n", time_strERR, strerror(errno));
                    fflush(logFile);
                }
            }
            free(visitedArr);
            free(path);
        } 
        else {
            // PATH FOUNDED IN CACHE ...
            printPathFoundDatabase(id,searchingOrWritingPath);
            // SEND RESULT TO CLIENT ...
            int size = searchingOrWritingPath->arraySize, g = 0;
            while(g < size) {
                arr[g] = searchingOrWritingPath->path[g]; 
                g++;
            }
            arr[g] = -1;
            if(send(fd, &arr, (size+1)*4,0) == -1) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! send (3) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
            }
        }
        free(temp);
        if(rVal == 2)
            pthread_mutex_unlock(&okToReadMutex);
        thread_one->isWorking = 0;
        pthread_cond_signal(&threadPoolConddition);
        bzero(arr, MAX_SIZE);
    }
    return NULL;
}

void initializeThreadPool() {
    
    threadPool->size = minThreadCount;
    threadPool->maxCapacity = maxThreadCount;
    threadPool->conditionArr = (pthread_cond_t*) malloc(sizeof(pthread_cond_t) * minThreadCount);
    threadPool->mutexes = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t) * minThreadCount);
    threadPool->firstThread = NULL;
    threadPool->lastThread = NULL;
    threadPool->ids = (int*) malloc(sizeof(int) * minThreadCount);
    int i;
    for(i = 0; i < minThreadCount; i++) {
        if(pthread_cond_init(&(threadPool->conditionArr[i]), NULL) != 0) {
            errorExit("ERROR pthread_cond_init ");
        }
        if(pthread_mutex_init(&(threadPool->mutexes[i]), NULL) != 0) {
            errorExit("ERROR pthread_mutex_init ");
        }
        threadPool->ids[i] = i;
    }
    
    // FOR READERS, WRITERS PRIORIZATION
    if(pthread_mutex_init(&okToReadMutex, NULL) != 0) 
            errorExit("ERROR pthread_mutex_init ");
    if(pthread_mutex_init(&okToWriteMutex, NULL) != 0) 
            errorExit("ERROR pthread_mutex_init ");
    if(pthread_cond_init(&okToRead_Cond, NULL) != 0) 
        errorExit("ERROR pthread_cond_init ");
    if(pthread_cond_init(&okToWrite_Cond, NULL) != 0) 
        errorExit("ERROR pthread_cond_init ");
    if(pthread_cond_init(&threadPoolConddition, NULL) != 0)
        errorExit("ERROR pthread_cond_init ");

    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    fprintf(logFile,"%s :\tA pool of %d threads has been created\n", time_str, threadPool->size);
    fflush(logFile);

    for(i = 0; i < threadPool->size; i++) {
        OneThread* tempx = (OneThread*) malloc(sizeof(OneThread) * 1);
        tempx->id = i;
        tempx->isWorking = 0;
        tempx->nextThread = NULL;
        tempx->socket_fd = -1;
        tempx->source = -1;
        tempx->destination = -1;
        if(pipe(tempx->pipe) == -1) {
            errorExit("ERROR ! pipe problem ");
        }
        if(i == 0) {
            threadPool->firstThread = threadPool->lastThread = tempx;
        }
        else {
            threadPool->lastThread->nextThread = tempx;
            threadPool->lastThread = tempx;
        }
    }

    if(pthread_mutex_init(&threadPoolMutex, NULL) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_mutex_init problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR : mutex init");
    }

    if(pthread_create(&(dynamicThread), NULL, reinitializeThreadPool, NULL) != 0) {
        errorExit("ERROR pthread_create ");
    }

    OneThread* temp = threadPool->firstThread;
    for(i = 0; i < threadPool->size; i++) {
        if(pthread_create(&(temp->thread), NULL, response_threads, temp) != 0) {
            errorExit("ERROR pthread_create ");
        }
        temp = temp->nextThread;
    }
}

void freeThreadPool() {
    if(ifThreadPoolCreated) {        
        int i;
        OneThread* temp = threadPool->firstThread;
        for(i = 0; i < threadPool->size; i++) {
            if(pthread_join(temp->thread, NULL) != 0) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! pthread_join (0) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
                errorExit("ERROR pthread_join ");
            }
            temp = temp->nextThread;
        }
        
        temp = threadPool->firstThread;
        for(i = 0; i < threadPool->size; i++) {
            if(errno = close(temp->pipe[0]) != 0) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! pipe close (1) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
                errorExit("ERROR pipe close ");
            }
            if(errno = close(temp->pipe[1]) != 0) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! pipe close (2) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
                errorExit("ERROR pipe close ");
            }
            temp = temp->nextThread;
        }

        int k = 0;
        while(k < threadPool->size) {
            OneThread* temp = threadPool->firstThread;
            threadPool->firstThread = threadPool->firstThread->nextThread;
            free(temp);
            k++;
        }
        
        for(i = 0; i < minThreadCount; i++) {
            if(errno = pthread_cond_destroy(&(threadPool->conditionArr[i])) != 0) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! pthread_cond_destroy (1) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
                errorExit("ERROR pthread_cond_destroy ");
            }
            // MUTEX DESTROY ...
            if(errno = pthread_mutex_destroy(&(threadPool->mutexes[i])) != 0) {
                time_t tcurrentERR = time(NULL);
                char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
                fprintf(logFile, "%s :\tERROR ! pthread_mutex_destroy (1) problem %s\n", time_strERR, strerror(errno));
                fflush(logFile);
                errorExit("ERROR pthread_mutex_destroy ");
            }
        }
        free(threadPool->conditionArr);
        free(threadPool->mutexes);
        free(threadPool->ids);

    }

    if(close(dynamicPipe[0]) == -1) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! close (1) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR : close problem ");
    }

    if(close(dynamicPipe[1]) == -1) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! close (2) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR : close problem ");
    }

    if(errno = pthread_cond_destroy(&threadPoolConddition) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_cond_destroy (2) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR ! pthread_cond_destroy ");
    }

    if(errno = pthread_cond_destroy(&okToRead_Cond) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_cond_destroy (3) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR ! pthread_cond_destroy ");
    }

    if(errno = pthread_cond_destroy(&okToWrite_Cond) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_cond_destroy (4) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR ! pthread_cond_destroy ");
    }

    if(errno = pthread_mutex_destroy(&(okToReadMutex)) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_mutex_destroy (2) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR pthread_mutex_destroy ");
    }

    if(errno = pthread_mutex_destroy(&okToWriteMutex) != 0) {
        time_t tcurrentERR = time(NULL);
        char* time_strERR = strtok(ctime(&tcurrentERR), "\n");
        fprintf(logFile, "%s :\tERROR ! pthread_mutex_destroy (3) problem %s\n", time_strERR, strerror(errno));
        fflush(logFile);
        errorExit("ERROR pthread_mutex_destroy ");
    }

    free(threadPool);
} 

OneThread* returnEmptyThread() {
    while(1){
    OneThread* freeThread = threadPool->firstThread;
    int i = rand() % (threadPool->size), k = 0;
    while(k < i) {
        freeThread = freeThread->nextThread;
        k++;
    }
    if(freeThread->isWorking == 0) {
        return freeThread;
    }
    }
    return NULL;
}

void signal_handler(int sigNo) {
    if(sigNo == 2) {
        time_t tcurrent = time(NULL);
        char* time_str = strtok(ctime(&tcurrent), "\n");
        pthread_mutex_lock(&logFileMutex);
        fprintf(logFile,"%s :\tTermination signal (SIGINT) received, waiting for ongoing threads to complete.\n",time_str);
        fflush(logFile);
        pthread_mutex_unlock(&logFileMutex);
        SIGINT_FLAG = 1;
    }
}

int getRunningThreadCount() {
    int count = 0, i = 0 , size = threadPool->size;
    OneThread* temp = threadPool->firstThread;
    while(i < size) {
        if(temp->isWorking == 1)
            count++;
        temp = temp->nextThread;
        i++;
    }
    if(count == threadPool->size) {
        time_t tcurrent = time(NULL);
        char* time_str = strtok(ctime(&tcurrent), "\n");
        pthread_mutex_lock(&logFileMutex);
        fprintf(logFile,"%s :\tNo thread is available ! Waiting for one.\n",time_str);
        fflush(logFile);
        pthread_mutex_unlock(&logFileMutex);
    }
    return count;
}

void initializeBFSarrays(int* path, int* visitedArr) {
    int i;
    for(i = 0; i < maxNodeNumber; i++){
        visitedArr[i] = 0;
        path[i] = -15;
    }                    
}

void printPathFromDataBase(CacheBlock* block) {
    size_t size = block->arraySize;
    if(block->path == NULL) {
        return;
    }
    int i;
    for(i = 0; i < size; i++) {
        if(i == size-1) 
            fprintf(logFile,"%d \n",block->path[i]);
        else 
            fprintf(logFile,"%d->",block->path[i]);
    }
}

void create_server() {
    // ########### SOCKET OPERATIONS #################
   
    if( (serverFD = socket(AF_INET, SOCK_STREAM,0)) == 0) {
        errorExit("ERROR : socket failed ");
    }

    if(setsockopt(serverFD, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &op, sizeof(op)) == -1) {
        errorExit("ERROR : setsockopt problem ");
    }

    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = INADDR_ANY;
    socketAddress.sin_port = htons(portNumber);

    if(bind(serverFD, (struct sockaddr *)&socketAddress, sizeof(socketAddress)) < 0) {
        errorExit("ERROR : bind problem ");
    }

    if(listen(serverFD, 4000) < 0) {
        errorExit("ERROR : listen problem ");
    }
}