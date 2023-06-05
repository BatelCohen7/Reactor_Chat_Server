
# Reactor Chat Server</div>
Task 4 : Linux practice - Operating systems course.</div>
</div>
The Reactor Chat Server is an implementation of a chat server using the Reactor pattern.</div> It supports an unlimited number of clients and allows clients to connect and exchange messages in a chat-like environment.</div>

## Introduction</div>

The **Reactor Chat Server** implements a server-client architecture where multiple clients can connect to the server and exchange messages.</div> It utilizes the Reactor pattern to efficiently handle multiple client connections using a single thread.</div> The server leverages the `select` or `poll` mechanism to monitor and process events on the registered file descriptors (client sockets).</div>

## API</div>

The Reactor Chat Server provides the following API functions:</div>

- `void* createReactor()`: Creates a new Reactor instance and returns a pointer to it.</div>
- `void stopReactor(void* this)`: Stops the Reactor if it is active.</div>
- `void startReactor(void* this)`: Starts the Reactor and begins handling client connections.</div>
- `void addFd(void* this, int fd, handler_t handler)`: Adds a file descriptor to the Reactor with the specified handler function.</div>
- `void WaitFor(void* this)`: Waits for the Reactor thread to finish.</div>

## Dependencies</div>

The Reactor Chat Server has the following dependencies:</div>

- C compiler (GCC)</div>
- Linux machine (Ubuntu 22.04 )</div>
- Make</div>

## Installation</div>

To install the Reactor Chat Server, follow the steps below:</div>

1. Clone the repository: `git clone <git@github.com:BatelCohen7/Reactor_Chat_Server.git>`.</div>
2. Navigate to the project directory: `cd reactor-chat-server`.</div>
3. Compile the server using the provided Makefile: `make`.</div>

## Configuration</div>

The Reactor Chat Server can be configured by modifying the server source code (`react_server.c`). </div> The server configuration options include:</div>

- Server port: Modify the `SERVER_PORT` constant in `react_server.c` to specify the desired port number for the server.</div>

## Running</div>

Run the reactor server:  ./react_server</div>
![12](https://github.com/BatelCohen7/Reactor_Chat_Server/assets/93344134/a1428866-f898-44b7-8615-b27525c5fb4d)


## Author: </div>
[Batel Cohen Yerushalmi](https://github.com/BatelCohen7 "Batel Cohen Yerushalmi") 

