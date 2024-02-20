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
#define BUFFER_SIZE 1024



// CONSTANTS
const char CODE_EXIT[] = "!";             // Used to determine when we should exit the program
const char LOCAL_HOST[] = "127.0.0.1";  // Used to communicate with two terminals on the same host


// GLOBALS
List *listRx;
List *listTx;
int localPort;
int remotePort;
char *outputIP;
pthread_t tids[NUM_THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condTx = PTHREAD_COND_INITIALIZER;
pthread_cond_t condRx = PTHREAD_COND_INITIALIZER;
bool status_exit;


enum thread_type {
    KEYBOARD,
    UDP_OUTPUT,
    UDP_INPUT,
    SCREEN_OUTPUT
};



// On receipt of input, adds the input to the list of messages
//  to be sent to the remote s-talk client
void * keyboard_thread () {

    printf("Keyboard thread...\n");
    

    // LOOP
    while(1) {

        char messageTx[BUFFER_SIZE]; // Buffer for input from keyboard
        fgets(messageTx, BUFFER_SIZE, stdin);
        char *newMessage = (char *)malloc(strlen(messageTx));
        strcpy(newMessage, messageTx);
        fflush(stdin);

        // LOCK THREAD
        pthread_mutex_lock(&mutex);     
        List_append(listTx, newMessage);
        char *appendedItem = List_first(listTx);

        // Exit status recognized, initiate exit procedure
        if (strncmp(newMessage, CODE_EXIT, 1) == 0) {
            printf("Exiting keyboard thread...\n");
            status_exit = true;
            
            // UNLOCK THREAD
            pthread_cond_signal(&condTx);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }

        // UNLOCK THREAD
        pthread_cond_signal(&condTx);
        pthread_mutex_unlock(&mutex);   

    }

    pthread_exit(NULL);
}

// Take each message off this list and send it over the network
//  to the remote client
void * UDP_output_thread() {

    printf("UDP output thread...\n");


    // INITIALIZE SOCKETS
    struct sockaddr_in sock_out;
    struct in_addr addr_out;
    memset(&sock_out, 0, sizeof(sock_out));
    sock_out.sin_family = AF_INET;
    inet_aton(outputIP, &addr_out);
    sock_out.sin_addr.s_addr = (in_addr_t)addr_out.s_addr;      // htonl = host to network long
    sock_out.sin_port = htons(remotePort);                      // htons = host to network short


    // CREATE AND BIND SOCKET
    int socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); // Create the socket locally
    if (socketDescriptor < 0) {
        perror("Failed to create remote socket\n");
        exit(-1);
    }


    // LOOP
    while (1) {

        // LOCK THREAD, AWAIT CONDITION
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condTx, &mutex);

        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {

            printf("Exiting UDP output thread...\n");
            // CLOSE SOCKET & THREAD
            pthread_mutex_unlock(&mutex);   // Unlock thread
            close(socketDescriptor);
            pthread_exit(NULL);

        }
        // SEND REPLY
        else if (List_count(listTx) > 0) {
            // void *output_void = List_first(listTx);
            char *output = List_first(listTx);
            List_remove(listTx);
            int status = sendto(socketDescriptor, output, strlen(output), 0, 
                    (struct sockaddr *)&sock_out, sizeof(sock_out));
            if (status < 0) {
                perror("Failed to send");
            }
            pthread_mutex_unlock(&mutex);   // Unlock thread

        }
    }


    // CLOSE SOCKET & THREAD
    close(socketDescriptor);
    pthread_exit(NULL);

}

// On receipt of input from the remote s-talk client, puts the 
//  message on the list of messages that need to be printed to
//  the local screen
void * UDP_input_thread() {
    
    printf("UDP input thread...\n");

    // INITIALIZE SOCKETS
    struct sockaddr_in sock_in;
    memset(&sock_in, 0, sizeof(sock_in));
    sock_in.sin_family = AF_INET;
    sock_in.sin_addr.s_addr = htonl(INADDR_ANY);    // htonl = host to network long
    sock_in.sin_port = htons(localPort);            // htons = host to network short

    // CREATE AND BIND SOCKET
    int socketDescriptor = socket(AF_INET, SOCK_DGRAM, 0); // Create the socket locally
    if (socketDescriptor < 0) {
        perror("Failed to create local socket\n");
        exit(-1);
    }
    bind(socketDescriptor, (struct sockaddr*)&sock_in, sizeof(sock_in));    // Open socket
    

    // LOOP
    while (1) {


        // RECEIVE
        struct sockaddr_in sinRemote;   // Output parameter
        memset(&sinRemote, 0, sizeof(sinRemote));
        unsigned int sin_size = sizeof(sinRemote);   // In/out parameter
        char messageRx[BUFFER_SIZE];    // Client data written into here
                                                // This is effectively a buffer for receive
        int bytesRx = recvfrom(socketDescriptor, (char *)messageRx, BUFFER_SIZE, 0,
                                    (struct sockaddr *)&sinRemote, &sin_size);


        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {

            printf("Exiting UDP input thread...\n");
            // CLOSE SOCKET & THREAD
            close(socketDescriptor);
            pthread_exit(NULL);

        }
        // PROCESS MESSAGE
        if (bytesRx > 0) {
            char *message = (char *)malloc(strlen(messageRx));
            strcpy(message, messageRx);

            // LOCK THREAD
            pthread_mutex_lock(&mutex);
            
            if (strncmp(message, CODE_EXIT, 1) == 0) {
                
                printf("Exiting UDP input thread...\n");
                status_exit = true;
                
                // UNLOCK THREAD
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);

            }
            
            List_append(listRx, message);
            pthread_mutex_unlock(&mutex);
        }
    }
    

    // CLOSE SOCKET
    close(socketDescriptor);
    pthread_exit(NULL);
}

// Take each message off of the list and output to the screen
void * screen_output_thread() {

    printf("Screen output thread...\n");


    // LOOP
    while(1) {

        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {
            printf("Exiting screen output thread...\n");
            pthread_exit(NULL);
        }
        else if (List_count(listRx) > 0) {
            char *message = List_first(listRx);
            List_remove(listRx);
            printf("%s", message);
            fflush(stdout);
        }

    }

    pthread_exit(NULL);
}


int main (int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(-1);
    }

    // Set exit status
    status_exit = false;

    // Map ports
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

    // CREATE THREADS
    pthread_create(&tids[KEYBOARD], NULL, keyboard_thread, NULL);
    pthread_create(&tids[UDP_INPUT], NULL, UDP_input_thread, NULL);
    pthread_create(&tids[UDP_OUTPUT], NULL, UDP_output_thread, NULL);
    pthread_create(&tids[SCREEN_OUTPUT], NULL, screen_output_thread, NULL);


    // JOIN THREADS
    pthread_join(tids[KEYBOARD], NULL);
    pthread_join(tids[UDP_INPUT], NULL);
    pthread_join(tids[UDP_OUTPUT], NULL);
    pthread_join(tids[SCREEN_OUTPUT], NULL);


    
    return 0;
}
