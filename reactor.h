#ifndef _REACTOR_H
#define _REACTOR_H

#include <stdio.h>
#include <stdbool.h>
#include <poll.h>
#include <stdlib.h>
#include <pthread.h>

#define SERVER_PORT 7895

#define MAX_CLIENTS 8192

#define MAX_BUFFER 1024

#define POLL_TIMEOUT -1

#define SERVER_RELAY    1

#define SERVER_PRINT_MSGS	1

#define SERVER_RLY_MSG_LEN	33


// Typedefs
typedef void *(*handler_t)(int fd, void *react);
typedef struct _reactorNode reactorNode, *reactorNodeP;
typedef struct _reactor reactor_t, *reactorPtr;
typedef struct pollfd pollfd_t, *pollfd_t_ptr;


// Structures Section 
    struct _reactorNode{
        int fd;

        union _hdlr_func_union{
            handler_t handler;
            void *handler_ptr;
        } hd;

        reactorNodeP next;
    };

    struct _reactor{

        pthread_t thread;

        reactorNodeP head;

        pollfd_t_ptr fds;

        bool running;
    };


// Functions
void *createReactor();
void startReactor(void *react);
void stopReactor(void *react);
void addFd(void *react, int fd, handler_t handler);
void WaitFor(void *react);
void signal_handler();
void *client_handler(int fd, void *react);
void *server_handler(int fd, void *react);

#endif

