#include "List.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

/*

------INFO FROM MAN PAGES------

Pthreads:
    int pthread_create(pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg);

Sockets:
    int socket(int domain, int type, int protocol);

Bind: 
    int bind(int sockfd, const struct sockaddr *addr, socklen_t arrlen);
    Assigns address specified by addr to the socket referred to by the file descriptor sockfd.  addrlen
        specifies the size, in bytes, of the address structure pointed to by addr.
    It is normally necessary to assign a local address using bind() before a SOCK_STREAM socket may 
        receive connections

Send:
    ssize_t send(int sockfd, const void *buf, size_t len, int flags);
    ssize_t sendto(int sockfd, const void *buf, size_t len, int flags,
                      const struct sockaddr *dest_addr, socklen_t addrlen);
    ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);

Receive:
    ssize_t recv(int sockfd, void *buf, size_t len, int flags);
    ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
                        struct sockaddr *src_addr, socklen_t *addrlen);
    ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);

Get Address:
    int getaddrinfo(const char *node, const char *service,
                       const struct addrinfo *hints,
                       struct addrinfo **res);
    void freeaddrinfo(struct addrinfo *res);
    const char *gai_strerror(int errcode);


------OTHER------
Finding host name: type "hostname" into the command line

*/


#define NUM_THREADS 4


// TESTING
const char LOCAL_HOST[] = "127.0.0.1";


// GLOBALS
List *listRx;
List *listTx;
int localPort;
int remotePort;
char *outputIP;
pthread_t tids[NUM_THREADS];


enum thread_type {
    KEYBOARD,
    UDP_OUTPUT,
    UDP_INPUT,
    SCREEN_OUTPUT
};



// On receipt of input, adds the input to the list of messages
//  to be sent to the remote s-talk client
void * keyboard_thread () {

    while(1) {
        
        printf("creating reply...\n");
        char messageTx[LIST_MAX_NUM_NODES]; // Buffer for input from keyboard
        fgets(messageTx, LIST_MAX_NUM_NODES, stdin);
        char *newChar = (char *)malloc(strlen(messageTx) + 1);
        strcpy(newChar, messageTx);
        for (int i = 0; i < strlen(newChar); i++) {
            List_append(listTx, &newChar[i]);
        }

    }

    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// Take each message off this list and send it over the network
//  to the remote client
void * UDP_output_thread() {

    printf("here\n");

    // CREATE REPLY
    char messageTx[LIST_MAX_NUM_NODES]; // Buffer for input from keyboard
    fgets(messageTx, LIST_MAX_NUM_NODES, stdin);
    char *newChar = (char *)malloc(strlen(messageTx) + 1);
    strcpy(newChar, messageTx);
    for (int i = 0; i < strlen(newChar); i++) {
        List_append(listTx, &newChar[i]);
    }

    // INITIALIZE SOCKETS
    struct sockaddr_in sock_out;
    struct in_addr addr_out;
    memset(&sock_out, 0, sizeof(sock_out));
    sock_out.sin_family = AF_INET;
    inet_aton(outputIP, &addr_out);
    sock_out.sin_addr.s_addr = (in_addr_t)addr_out.s_addr;    // htonl = host to network long
    sock_out.sin_port = htons(remotePort); // htons = host to network short

    // CREATE AND BIND SOCKET
    int socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); // Create the socket locally
    if (socketDescriptor < 0) {
        perror("Failed to create remote socket\n");
        exit(-1);
    }

    struct sockaddr_in sinRemote;   // Output parameter
    unsigned int sin_len = sizeof(sinRemote);   // In/out parameter

    // SEND REPLY
    // List_first(listTx);
    // if (List_count(listTx) > 0) {
        
    //     char *output = List_remove(listTx);
    //     unsigned int sin_len = sizeof(sinRemote);
    //     sendto(socketDescriptor, output, sizeof(output), 0, 
    //             (struct sockaddr *)&sinRemote, sin_len);

    // }

    sin_len = sizeof(sinRemote);
    sendto(socketDescriptor, messageTx, strlen(messageTx), 0, 
                (struct sockaddr *)&sinRemote, sin_len);  

    // CLOSE SOCKET
    close(socketDescriptor);

    // pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// On receipt of input from the remote s-talk client, puts the 
//  message on the list of messages that need to be printed to
//  the local screen
void * UDP_input_thread() {
    
    printf("threading...\n");

    // INITIALIZE SOCKETS
    struct sockaddr_in sock_in;
    memset(&sock_in, 0, sizeof(sock_in));
    sock_in.sin_family = AF_INET;
    sock_in.sin_addr.s_addr = htonl(INADDR_ANY);    // htonl = host to network long
    sock_in.sin_port = htons(localPort); // htons = host to network short

    // CREATE AND BIND SOCKET
    int socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); // Create the socket locally
    if (socketDescriptor < 0) {
        perror("Failed to create local socket\n");
        exit(-1);
    }
    bind(socketDescriptor, (struct sockaddr*)&sock_in, sizeof(sock_in));    // Open socket
    
    while (1) {
        
        // RECEIVE
        struct sockaddr_in sinRemote;   // Output parameter
        unsigned int sin_len = sizeof(sinRemote);   // In/out parameter
        char messageRx[LIST_MAX_NUM_NODES];    // Client data written into here
                                                // This is effectively a buffer for receive
        int bytesRx = recvfrom(socketDescriptor, messageRx, LIST_MAX_NUM_NODES, 0,
                                    (struct sockaddr *)&sinRemote, &sin_len);


        // PROCESS MESSAGE
        for (int i = 0; i < bytesRx; i++) {
            List_append(listRx, &messageRx[i]);
        }

    }
    

    // CLOSE SOCKET
    close(socketDescriptor);
    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// Take each message off of the list and output to the screen
void * screen_output_thread() {


    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}


int main (int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(-1);
    }

    // CHECK PORT AVAILABILITY
    localPort = atoi(argv[1]);
    remotePort = atoi(argv[3]);

    // Find the IP address of the remote terminal
    if (strcmp(argv[2], "localhost") == 0) {
        outputIP = (char *)malloc(strlen(LOCAL_HOST) + 1);
        strcpy(outputIP, LOCAL_HOST);
    } else {
        outputIP = (char *)malloc(strlen(argv[2]) + 1);
        strcpy(outputIP, argv[2]);
    }
    struct in_addr addr_out;
    inet_aton(outputIP, &addr_out);
    

    // CREATE LISTS
    listRx = List_create();
    listTx = List_create();

    pthread_attr_t attr[NUM_THREADS];

    pthread_attr_init(&attr[UDP_INPUT]);
    pthread_create(&tids[UDP_INPUT], &attr[UDP_INPUT], UDP_input_thread, NULL);

    pthread_create(&tids[KEYBOARD], NULL, keyboard_thread, NULL);


    while (1) {

        
        // INITIALIZE SOCKETS

        // Output socket
        struct sockaddr_in sock_out;
        memset(&sock_out, 0, sizeof(sock_out));
        sock_out.sin_family = AF_INET;
        sock_out.sin_addr.s_addr = (in_addr_t)addr_out.s_addr;    // htonl = host to network long
        sock_out.sin_port = htons(remotePort); // htons = host to network short


        // CREATE AND BIND SOCKETS

        // Output socket
        int socketDescriptor_out = socket(AF_INET, SOCK_DGRAM, 0); // Create the socket remotely
        if (socketDescriptor_out < 0) {
            perror("Failed to create remote socket\n");
            exit(-1);
        }
        
        // OUTPUT TO MONITOR
        printf(">>");
        while (List_count(listRx) != 0) {
            printf("%c", *(char *)List_remove(listRx));
        }
        printf("\n");


        // // CREATE REPLY
        // printf("creating reply...\n");
        // char messageTx[LIST_MAX_NUM_NODES]; // Buffer for input from keyboard
        // fgets(messageTx, LIST_MAX_NUM_NODES, stdin);
        // char *newChar = (char *)malloc(strlen(messageTx) + 1);
        // strcpy(newChar, messageTx);
        // for (int i = 0; i < strlen(newChar); i++) {
        //     List_append(listTx, &newChar[i]);
        // }

        // UDP_output_thread();

        printf("outputting...\n");
        if (List_count(listTx) > 0) {
            struct sockaddr_in sinRemote;
            int sin_len = sizeof(sinRemote);
            char *output = List_remove(listTx);
            sendto(socketDescriptor_out, output, sizeof(output), 0, 
                    (struct sockaddr *)&sinRemote, sin_len);    // We will have the client's IP address and port
        }


    }

    pthread_join(tids[UDP_INPUT], NULL);

    
    return 0;
}