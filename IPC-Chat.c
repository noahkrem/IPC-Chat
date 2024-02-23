#include "List.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/select.h>
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
#define BUFFER_SIZE 256



// CONSTANTS
const char CODE_EXIT[] = "!\n";             // Used to determine when we should exit the program
const char LOCAL_HOST[] = "127.0.0.1";  // Used to communicate with two terminals on the same host


// GLOBALS
List *listRx;
List *listTx;
char *localPort;
char *remotePort;
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


void free_fn(void *item) {
    free(item);
    item = NULL;
}

// Function for non-blocking user input
// Source: https://web.archive.org/web/20170407122137/http://cc.byexamples.com/2007/04/08/non-blocking-user-input-in-loop-without-ncurses/
int kbhit() {

    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);     // STDIN_FILENO is equivalent to 0
    select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    return FD_ISSET(STDIN_FILENO, &fds);
}


// On receipt of input, adds the input to the list of messages
//  to be sent to the remote s-talk client
void * keyboard_thread () {
    
    printf("Type your messages here!\n");
    printf("Received messages are denoted by '>>'\n\n");

    // LOOP
    while(1) {

        while (!kbhit()) {
            
            // Exit status recognized, initiate exit procedure
            if (status_exit == true) {
                printf("Exiting keyboard thread...\n");
                pthread_mutex_lock(&mutex);
                char *exitMessage = "!\n";
                List_append(listTx, exitMessage);
                pthread_cond_signal(&condTx);
                pthread_mutex_unlock(&mutex);
                return NULL;
            }
            usleep(100);
        }

        char messageTx[BUFFER_SIZE]; // Buffer for input from keyboard
        fgets(messageTx, BUFFER_SIZE, stdin);
        char *newMessage = messageTx;
        newMessage[strlen(messageTx)] = '\0';

        // Exit status recognized, initiate exit procedure
        if (strcmp(messageTx, CODE_EXIT) == 0) {
            
            printf("Exiting keyboard thread...\n");
            status_exit = true;

            // LOCK THREAD
            pthread_mutex_lock(&mutex);     
            List_append(listTx, newMessage);
            
            // UNLOCK THREAD
            pthread_cond_signal(&condRx);
            pthread_cond_signal(&condTx);

            // Wait for output thread to complete before exiting.
            // Doing so removes the issue of invalid reads by the output thread upon exit 
            pthread_cond_wait(&condTx, &mutex);

            pthread_mutex_unlock(&mutex);


            return NULL;
        }

        // LOCK THREAD
        pthread_mutex_lock(&mutex);     
        List_append(listTx, newMessage);

        // UNLOCK THREAD
        pthread_cond_signal(&condTx);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

// Take each message off this list and send it over the network
//  to the remote client
void * UDP_output_thread() {

    // INITIALIZE SOCKETS
    int sockfd, rv, bytesTx;
    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    if((rv = getaddrinfo(outputIP, remotePort, &hints, &servinfo)) != 0) {
        fprintf(stderr, "UDP Output: getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }

    // Loop through all the results and make a socket
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("Output UDP: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return NULL;
    }

    // LOOP
    while (1) {

        // LOCK THREAD, AWAIT CONDITION
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condTx, &mutex);

        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {

            printf("Exiting UDP output thread...\n");

            char *output = List_first(listTx);
            List_remove(listTx);

            int status;
            if ((status = sendto(sockfd, output, strlen(output), 0, 
                                p->ai_addr, p->ai_addrlen)) == -1) {

                perror("UDP Output: sendto");
                exit(1);
            }

            // CLOSE SOCKET & THREAD
            pthread_cond_signal(&condTx);
            pthread_mutex_unlock(&mutex);   // Unlock thread
            freeaddrinfo(servinfo);
            close(sockfd);
            return NULL;
        }
        // SEND REPLY
        else if (List_count(listTx) > 0) {

            char *output = List_first(listTx);
            List_remove(listTx);

            int status = sendto(sockfd, output, strlen(output), 0, 
                                p->ai_addr, p->ai_addrlen);
            if (status < 0) {
                perror("Failed to send");
            }
            
            // UNLOCK
            pthread_mutex_unlock(&mutex);   // Unlock thread
        }
    }

    // CLOSE SOCKET & THREAD
    freeaddrinfo(servinfo);
    close(sockfd);
    return NULL;

}

// On receipt of input from the remote s-talk client, puts the 
//  message on the list of messages that need to be printed to
//  the local screen
void * UDP_input_thread() {

    int sockfd, rv, bytesRx;
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr;
    socklen_t addr_len;
    char s[INET6_ADDRSTRLEN];
    char messageRx[BUFFER_SIZE];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;    // Use local IP

    if ((rv = getaddrinfo(NULL, localPort, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return NULL;
    }

    // Loop through all the results and bind to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("UDP Input: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("UDP Input: bind");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "UDP Input: failed to bind to socket\n");
        return NULL;
    }

    freeaddrinfo(servinfo); // No longer need this structure
    

    // LOOP
    while (1) {

        // RECEIVE        
        addr_len = sizeof their_addr;
        if ((bytesRx = recvfrom(sockfd, (char *)messageRx, BUFFER_SIZE-1, 0, 
                                    (struct sockaddr *)&their_addr, &addr_len)) == -1 ) {

            perror("UDP Input: recvfrom");
            exit(1);
        }
        messageRx[bytesRx] = '\0';

        // TESTING 
        // printf("UDP Input: packet is %d bytes long\n", bytesRx);


        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {

            printf("Exiting UDP input thread...\n");
            
            // CLOSE SOCKET & THREAD
            close(sockfd);
            return NULL;
        }
        // PROCESS MESSAGE
        if (bytesRx > 0) {

            char *message = (char *)malloc(strlen(messageRx) + 1);
            strcpy(message, messageRx);
            message[strlen(messageRx)] = '\0';
            strncpy(messageRx, "", strlen(messageRx));
            
            if (strcmp(message, CODE_EXIT) == 0) {
                
                printf("Exiting UDP input thread...\n");
                status_exit = true;
                
                pthread_cond_signal(&condRx);

                // FREE
                free(message);
                message = NULL;

                // CLOSE SOCKET
                close(sockfd);
                return NULL;

            }

            pthread_mutex_lock(&mutex);
            List_append(listRx, message);
            pthread_cond_signal(&condRx);
            pthread_mutex_unlock(&mutex);
        }
    }
    
    // CLOSE SOCKET
    close(sockfd);
    return NULL;
}

// Take each message off of the list and output to the screen
void * screen_output_thread() {

    // LOOP
    while(1) {


        // LOCK THREAD
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condRx, &mutex);

        // Exit status recognized, initiate exit procedure
        if (status_exit == true) {
            printf("Exiting screen output thread...\n");

            pthread_mutex_unlock(&mutex);
            return NULL;
        }
        else if (List_count(listRx) > 0) {
            
            char *message = List_first(listRx);
            List_remove(listRx);

            // UNLOCK THREAD
            pthread_mutex_unlock(&mutex);

            // PRINT MESSAGE
            printf(">> %s", message);

            // FREE
            free(message);
            message = NULL;

        }

    }

    return NULL;
}


int main (int argc, char *argv[]) {

    if (argc < 4) {
        printf("Usage: s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(-1);
    }

    // Set exit status
    status_exit = false;

    // Map ports & IP
    localPort = argv[1];
    remotePort = argv[3];
    outputIP = argv[2];
    

    // CREATE LISTS
    listRx = List_create();
    listTx = List_create();


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


    // FREE
    List_free(listRx, free_fn);
    List_free(listTx, free_fn);

    
    return 0;
}