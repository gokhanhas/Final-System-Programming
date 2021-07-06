/*
 * Gokhan Has - 161044067
 * CSE 344 - System Programming 
 * Final Project
 * CLIENT SIDE
 */

#include <stdio.h>
#include <stdlib.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <unistd.h> 
#include <string.h> 
#include <errno.h>
#include <time.h> 
#include <ctype.h>

#define TRUE 1
#define FALSE 0
#define MAX_SIZE 800000

int portNumber, source, destination;

// Same arguments must be digit, fx portnumber, control these arguments.
int controlIfArgumentDigit(char* argumentValue);

// Print error message and exit gracefully.
void errorExit(char* error); 

// Print program usage if the user entered wrong input.
void printUsage();


int main(int argc, char **argv) {
    
    clock_t first, second;
    first = clock();

    char *aValue = NULL;
    char *pValue = NULL;
    char *sValue = NULL;
    char *dValue = NULL; 

    int aIndex = 0;
    int pIndex = 0;
    int sIndex = 0;
    int dIndex = 0;

    int another = 0;
    int c = 0;

    while ((c = getopt(argc, argv, "a:p:s:d:")) != -1)
    switch (c) {
        case 'a':
            aIndex += 1;
            aValue = optarg;
            break;
        
        case 'p':
            pIndex += 1;
            pValue = optarg;
            break;
        
        case 's':
            sIndex += 1;
            sValue = optarg;
            break;

        case 'd':
            dIndex += 1;
            dValue = optarg;
            break;

        case '?':
            another += 1;
            break;

        default:
            abort();
    } 
    
    if(aIndex == 1 && pIndex == 1 && sIndex == 1 && dIndex == 1 && another == 0) {
        // There is no problem with command line arguments ...
        
        if(!controlIfArgumentDigit(pValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -p argument must be positive number ");            
        }
        if(!controlIfArgumentDigit(sValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -s argument must be positive number ");            
        }
        if(!controlIfArgumentDigit(dValue)) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -d argument must be positive number ");            
        }

        portNumber = atoi(pValue);
        source = atoi(sValue);
        destination = atoi(dValue);

       
        if(source < 0) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -s : parameter value must be grater than zero ");
        }

        if(portNumber < 0) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -p : PortNumber problem ");
        }

        if(destination < 0) {
            errno = EIO;
            printUsage();
            errorExit("ERROR : -d : parameter value must be grater than zero ");
        }
    } 
    else {
        errno = EIO;
        printUsage();
        errorExit("ERROR ");
    }


    pid_t processID = getpid();
    
    time_t tcurrent = time(NULL);
    char* time_str = strtok(ctime(&tcurrent), "\n");
    printf("%s :\tClient (%d) connecting to %s:%d\n",time_str, processID, aValue, portNumber);
    int socketFD = 0; 
    struct sockaddr_in serverAddress; 

    // ###### SOCKET OPERATIONS  ######
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
        errorExit("ERROR ! Socket problem in client ");
    
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_port = htons(portNumber); 

    if(inet_pton(AF_INET, aValue, &serverAddress.sin_addr) <= 0)  
        errorExit("ERROR ! inet_pton problem in client ");
     
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)  
        errorExit("ERROR ! connection problem in client ");

    tcurrent = time(NULL);
    time_str = strtok(ctime(&tcurrent), "\n");
    printf("%s :\tClient (%d) connected and requesting a path from node %d to %d\n", time_str, processID, source, destination);
    int arr[2];
    arr[0] = source;
    arr[1] = destination;

    if(send(socketFD, &arr, 8 ,0) == -1) {
        errorExit("ERROR ! send problem in client ");
    }        
    
    int arrx[MAX_SIZE];
    int readedByte = recv(socketFD, &arrx, MAX_SIZE, 0);
    if(readedByte == -1) 
        errorExit("ERROR ! recv problem ");
    
    
    second = clock();
    double diff = ((double)(second-first)) / CLOCKS_PER_SEC;
    if(arrx[0] == -1) {
        // RETURN NO PATH ...
        tcurrent = time(NULL);
        time_str = strtok(ctime(&tcurrent), "\n");
        printf("%s :\tServer's response (%d): NO PATH, arrived in %.2f seconds, shutting down.\n",time_str, processID, diff);
    } 
    else {
        // RETURN PATH ...
        tcurrent = time(NULL);
        time_str = strtok(ctime(&tcurrent), "\n");
        printf("%s :\tServer's response to (%d): ",time_str, processID);
        int i = 0;
        while(i < MAX_SIZE) {
            if(arrx[i+1] == -1) {
                printf("%d,",arrx[i]);
                break;
            }
            printf("%d->",arrx[i]);
            i++;
        } 
        printf(" arrived in %.2lf seconds\n", diff);
    }
    return 0;
}

void printUsage() {
    printf("\n");
    printf("############################ USAGE ############################\n");
    printf("#       ./client -a 127.0.0.1 -p PORT -s 768 -d 979           #\n");
    printf("# -a :     IP address of the machine running the server       #\n");
    printf("# -p : port number at which the server waits for connections  #\n");
    printf("# -s :          source node of the requested path             #\n");
    printf("# -d :      destination node of the requested path            #\n");
    printf("###############################################################\n\n");
}

int controlIfArgumentDigit(char* argumentValue) {
    int i = 0;
    for(i = 0; i < strlen(argumentValue); i++) {
        if(isdigit(argumentValue[i]) == 0)
            return FALSE;
    }
    return TRUE;
}

void errorExit(char* error) {
    perror(error);
    exit(EXIT_FAILURE);
}