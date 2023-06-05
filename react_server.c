#include "reactor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// Global variables
void* reactor = NULL;                 // Pointer to the reactor event handler
uint32_t clientCount = 0;            // Count of active client connections
uint64_t totalBytesReceived = 0;    // Total number of bytes received from clients
uint64_t totalBytesSent = 0;       // Total number of bytes sent to clients

int main(void) {
    // Server address setup
    struct sockaddr_in serverAddress = {
        .sin_family = AF_INET,
        .sin_port = htons(SERVER_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    int server_fd = -1;   // Server file descriptor
    int reuse = 1;       // Socket option to allow reuse of local addresses

    fprintf(stdout, "License info\n");

    // Register signal handler for SIGINT (Ctrl+C) signals
    signal(SIGINT, signal_handler);

    fprintf(stdout, "Starting server...\n");

    // Create socket and check for errors
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        fprintf(stderr, "socket() failed: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    // Set socket option to reuse address and check for errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int)) < 0) {
        fprintf(stderr, "setsockopt(SO_REUSEADDR) failed: %s\n", strerror(errno));
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Bind socket to server address and check for errors
    if (bind(server_fd, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        fprintf(stderr, "bind() failed: %s\n", strerror(errno));
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Start listening on the socket and check for errors
    if (listen(server_fd, MAX_BUFFER) < 0) {
        fprintf(stderr, "listen() failed: %s\n", strerror(errno));
        close(server_fd);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Server started successfully.\n");

    fprintf(stdout, "Server listening on port \033[0;32m%d\033[0;37m.\n", SERVER_PORT);

     // Create an event reactor
    reactor = createReactor();
    if (reactor == NULL) {
        fprintf(stderr, "createReactor() failed: %s\n", strerror(ENOSPC));
        close(server_fd);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Adding server socket to reactor...\n");

    // Add server file descriptor to the reactor with server_handler as callback
    addFd(reactor, server_fd, server_handler);

    // Check if file descriptor was added successfully to the reactor
    if (((reactorPtr)reactor)->head == NULL) {
        fprintf(stderr, "addFd() failed: %s\n", strerror(errno));
        close(server_fd);
        free(reactor);
        return EXIT_FAILURE;
    }

    fprintf(stdout, "Server socket added to reactor successfully.\n");

    // Start the reactor
    startReactor(reactor);
    
    // Wait for the reactor to finish
    WaitFor(reactor);

    // Call the signal handler function directly to clean up
    signal_handler();

    return EXIT_SUCCESS;
}

// Function called when SIGINT signal is received
void signal_handler() {
    fprintf(stdout, "Server shutting down...\n");

     // Check if the reactor exists
    if (reactor != NULL) {
         // If the reactor is running, stop it
        if (((reactorPtr)reactor)->running)
            stopReactor(reactor);

        fprintf(stdout, "Closing all sockets and freeing memory...\n");

        // Free memory of all elements in the reactor linked list
        reactorNodeP current = ((reactorPtr)reactor)->head;
        reactorNodeP previous = NULL;

        while (current != NULL) {
            previous = current;
            current = current->next;

            close(previous->fd);
            free(previous);
        }

        // Free the reactor
        free(reactor);

       
        fprintf(stdout, " Client count in this session: %d\n", clientCount);
        fprintf(stdout, "Total bytes received in this session: %lu bytes (%lu KB / %lu MB).\n", 
                totalBytesReceived, totalBytesReceived / 1024, (totalBytesReceived / 1024) / 1024);
        fprintf(stdout, "Total bytes sent in this session: %lu bytes (%lu KB / %lu MB).\n",
                totalBytesSent, totalBytesSent / 1024, (totalBytesSent / 1024) / 1024);

        if (clientCount > 0) {
            fprintf(stdout, "Average bytes received per client: %lu bytes (%lu KB / %lu MB).\n", 
                    totalBytesReceived / clientCount, (totalBytesReceived / clientCount) / 1024,
                    ((totalBytesReceived / clientCount) / 1024) / 1024);
            fprintf(stdout, "Average bytes sent per client: %lu bytes (%lu KB / %lu MB).\n",
                    totalBytesSent / clientCount, (totalBytesSent / clientCount) / 1024,
                    ((totalBytesSent / clientCount) / 1024) / 1024);
        }
    } else {
        fprintf(stdout, "Reactor wasn't created, no memory cleanup needed.\n");
    }

    fprintf(stdout, "Server is now offline, goodbye.\n");

    // Exit program
    exit(EXIT_SUCCESS);
}

// Handles client requests
void *clientHandler(int fd, void *reactor) {
    char *buffer = (char *)calloc(MAX_BUFFER, sizeof(char));

    if (buffer == NULL) {
        fprintf(stderr, "calloc() failed: %s\n",strerror(errno));
        close(fd);
        return NULL;
    }

    int bytes_read = recv(fd, buffer, MAX_BUFFER, 0);

    if (bytes_read <= 0) {
        if (bytes_read < 0)
            fprintf(stderr, "recv() failed: %s\n",strerror(errno));
        else
            fprintf(stdout, "Client %d disconnected.\n",  fd);

        free(buffer);
        close(fd);
        return NULL;
    }

      totalBytesReceived += bytes_read;

    // Make sure the buffer is null-terminated, so we can print it.
    if (bytes_read < MAX_BUFFER)
        buffer[bytes_read] = '\0';
    else
        buffer[MAX_BUFFER - 1] = '\0';

 
    for (int i = 0; i < bytes_read - 3; i++) {
        if ((buffer[i] == 0x1b) && (buffer[i + 1] == 0x5b) && (buffer[i + 2] == 0x41 || buffer[i + 2] == 0x42 ||
                                                              buffer[i + 2] == 0x43 || buffer[i + 2] == 0x44)) {
            buffer[i] = 0x20;
            buffer[i + 1] = 0x20;
            buffer[i + 2] = 0x20;

            i += 2;
        }
    }

    
    if (SERVER_PRINT_MSGS) {
        fprintf(stdout, "Client %d: %s\n", fd, buffer);
    }

    
    if (SERVER_RELAY) {
        reactorNodeP current = ((reactorPtr)reactor)->head->next;

        char *bufferCopy = (char *)calloc(bytes_read + SERVER_RLY_MSG_LEN, sizeof(char));

        if (bufferCopy == NULL) {
            fprintf(stderr, "calloc() failed: %s\n",strerror(errno));
            free(buffer);
            return NULL;
        }

        snprintf(bufferCopy, bytes_read + SERVER_RLY_MSG_LEN, "Message from client %d: %s", fd, buffer);

        while (current != NULL) {
            if (current->fd != fd) {
                int bytes_written = send(current->fd, bufferCopy, bytes_read + SERVER_RLY_MSG_LEN, 0);

                if (bytes_written < 0) {
                    fprintf(stderr, " send() failed: %s\n",  strerror(errno));
                    free(buffer);
                    return NULL;
                } else if (bytes_written == 0) {
                    fprintf(stderr, "Client %d disconnected, expected to be removed in the next poll() round.\n",
                             current->fd);
                } else if (bytes_written < (bytes_read + SERVER_RLY_MSG_LEN)) {
                    fprintf(stderr, " send() sent fewer bytes than expected, check your network.\n");
                } else {
                    totalBytesSent += bytes_written;
                }
            }

            current = current->next;
        }

        free(bufferCopy);
    }

    free(buffer);

    return reactor;
}

// Handles new client connections
void *server_handler(int fd, void *react) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);

    reactorPtr  myReactor = (reactorPtr)react;

    
    if ( myReactor == NULL) {
        fprintf(stderr, "Server handler error: %s\n", strerror(EINVAL));
        return NULL;
    }

    int clientFd = accept(fd, (struct sockaddr *)&clientAddr, &clientLen);

    
    if (clientFd < 0) {
        fprintf(stderr, "accept() failed: %s\n", strerror(errno));
        return NULL;
    }

    fprintf(stdout, "Client %s:%d connected, Reference ID: %d\n", inet_ntoa(clientAddr.sin_addr),
            ntohs(clientAddr.sin_port), clientFd);

    // Add the client to the reactor.
    addFd( myReactor, clientFd, clientHandler);

    clientCount++;

    return react;
}