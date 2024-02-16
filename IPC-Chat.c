#include "List.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
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


*/


#define NUM_THREADS 4
#define NUM_LISTS 2

// NOTE: THIS PORT WILL BE ENTERED BY THE USER
#define PORT 23432



// On receipt of input, adds the input to the list of messages
//  to be sent to the remote s-talk client
void * keyboard_thread () {


    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// Take each message off this list and send it over the network
//  to the remote client
void * UDP_output_thread() {


    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// On receipt of input from the remote s-talk client, puts the 
//  message on the list of messages that need to be printed to
//  the local screen
void * UDP_input_thread() {


    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}

// Take each message off of the list and output to the screen
void * screen_output_thread() {


    pthread_exit(0);    // Instead of 0, we can also return a something in this line
}


int main (int argc, char **argv[]) {

    if (argc < 4) {
        printf("Usage: s-talk [my port number] [remote machine name] [remote port number]\n");
        exit(-1);
    }

    // INITIALIZE
    struct sockaddr_in sock_in;
    memset(&sock_in, 0, sizeof(sock_in));
    sock_in.sin_family = AF_INET;
    sock_in.sin_addr.s_addr = htonl(INADDR_ANY);    // htonl = host to network long
    sock_in.sin_port = htons(PORT); // htons = host to network short

    // CREATE AND BIND SOCKET
    int socketDescriptor = socket(PF_INET, SOCK_DGRAM, 0); // Create the socket locally
    bind(socketDescriptor, (struct sockaddr*)&sock_in, sizeof(sock_in));    // Open socket

    while (1) {

        // RECEIVE
        struct sockaddr_in sinRemote;   // Output parameter
        unsigned int sin_len = sizeof(sinRemote);   // In/out parameter
        char messageRx[LIST_MAX_NUM_NODES];    // Client data written into here
                                    // This is effectively a buffer for receive
        int bytesRx = recvfrom(socketDescriptor, messageRx, LIST_MAX_NUM_NODES, 0,
                                    (struct sockaddr *)&sinRemote, &sin_len);

        // Null terminate the string
        int terminateIdx = (bytesRx < LIST_MAX_NUM_NODES) ? bytesRx : LIST_MAX_NUM_NODES - 1;
        messageRx[terminateIdx] = 0;
        printf("Message received (%d bytes): \n>> %s\n", bytesRx, messageRx);

        // PROCESS MESSAGE

        // CREATE REPLY
        char messageTx[LIST_MAX_NUM_NODES];
        sprintf(messageTx, "Hello\n");

        // SEND REPLY
        sin_len = sizeof(sinRemote);
        sendto(socketDescriptor, messageTx, strlen(messageTx), 0, 
                (struct sockaddr *)&sinRemote, sin_len);    // We will have the client's IP address and port

    }

    // CLOSE SOCKET
    close(socketDescriptor);

    

                            





    // Thread ID, must be unique for each thread we create
    pthread_t tids[NUM_THREADS];



    
    return 0;
}