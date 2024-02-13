#include "List.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// On receipt of input, adds the input to the list of messages
//  to be sent to the remote s-talk client
void * keyboard_thread () {

}

// Take each message off this list and send it over the network
//  to the remote client
void * UDP_output_thread() {

}

// On receipt of input from the remote s-talk client, puts the 
//  message on the list of messages that need to be printed to
//  the local screen
void * UDP_input_thread() {

}

// Take each message off of the list and output to the screen
void * screen_output_thread() {

}