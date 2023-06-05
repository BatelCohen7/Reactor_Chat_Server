#include "reactor.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


/*
 * The Running function is the main execution body of the reactor.
 * It listens on registered file descriptors and triggers the corresponding
 * event handlers. If an error occurs, it sets the appropriate errno
 * and prints an error message.
 */
void *Running(void *react_ptr) {
	if (react_ptr == NULL)
	{
		errno = EINVAL;
		fprintf(stderr, "Running() failed: %s\n", strerror(EINVAL));
		return NULL;
	}

	reactorPtr reactor = (reactorPtr)react_ptr;

	while (reactor->running == true)
	{
		size_t size = 0;
        size_t i = 0;
		reactorNodeP curr = reactor->head;
        while (curr != NULL)
		{
			size++;
			curr = curr->next;
		}

		curr = reactor->head;

		reactor->fds = (pollfd_t_ptr)calloc(size, sizeof(pollfd_t));

		if (reactor->fds == NULL)
		{
			fprintf(stderr, "Running() failed: %s\n", strerror(errno));
			return NULL;
		}

		while (curr != NULL)
		{
			(*(reactor->fds + i)).fd = curr->fd;
			(*(reactor->fds + i)).events = POLLIN;

			curr = curr->next;
			i++;
		}

		int reactor_1 = poll(reactor->fds, i, POLL_TIMEOUT);

		if (reactor_1 < 0)
		{
			fprintf(stderr, "poll() failed: %s\n", strerror(errno));
			free(reactor->fds);
			reactor->fds = NULL;
			return NULL;
		}

		else if (reactor_1 == 0)
		{
			fprintf(stdout, "poll() timed out.\n");
			free(reactor->fds);
			reactor->fds = NULL;
			continue;
		}

		for (i = 0; i < size; ++i)
		{
			if ((*(reactor->fds + i)).revents & POLLIN)
			{
				reactorNodeP curr = reactor->head;

				for (unsigned int j = 0; j < i; ++j)
					curr = curr->next;

				void *handler_ret = curr->hd.handler((*(reactor->fds + i)).fd, reactor);

				if (handler_ret == NULL && (*(reactor->fds + i)).fd != reactor->head->fd)
				{
					reactorNodeP curr_node = reactor->head;
					reactorNodeP prev_node = NULL;

					while (curr_node != NULL && curr_node->fd != (*(reactor->fds + i)).fd)
					{
						prev_node = curr_node;
						curr_node = curr_node->next;
					}

					prev_node->next = curr_node->next;

					free(curr_node);
				}

				continue;
			}

            /*((*(reactor->fds + i)).revents & POLLHUP) checks if the POLLHUP event is set in the revents field of the file descriptor.
            ((*(reactor->fds + i)).revents & POLLNVAL) checks if the POLLNVAL event is set in the revents field of the file descriptor.
            ((*(reactor->fds + i)).revents & POLLERR) checks if the POLLERR event is set in the revents field of the file descriptor.
            These events typically indicate various error conditions or disconnection events related to the file descriptor.*/
			else if (((*(reactor->fds + i)).revents & POLLHUP || (*(reactor->fds + i)).revents & POLLNVAL || (*(reactor->fds + i)).revents & POLLERR) && (*(reactor->fds + i)).fd != reactor->head->fd)
			{
				reactorNodeP curr_node = reactor->head;
				reactorNodeP prev_node = NULL;

				while (curr_node != NULL && curr_node->fd != (*(reactor->fds + i)).fd)
				{
					prev_node = curr_node;
					curr_node = curr_node->next;
				}

				prev_node->next = curr_node->next;

				free(curr_node);
			}
		}

		free(reactor->fds);
		reactor->fds = NULL;
	}

	fprintf(stdout, "Reactor thread finished.\n");

	return reactor;
}

/*
 * The createReactor function creates a new reactor instance. It sets up the reactor structure,
 * but doesn't start the reactor's execution thread. If it fails to allocate the reactor structure,
 * it sets the appropriate errno and prints an error message.
 */
void *createReactor() {
	reactorPtr react_ptr = NULL;

	fprintf(stdout, "Creates a new reactor...\n");

	if ((react_ptr = (reactorPtr)malloc(sizeof(reactor_t))) == NULL)
	{
		fprintf(stderr, "malloc() failed: %s\n", strerror(errno));
		return NULL;
	}

	react_ptr->thread = 0;
	react_ptr->head = NULL;
	react_ptr->fds = NULL;
	react_ptr->running = false;

	fprintf(stdout, "Reactor has been created.\n");

	return react_ptr;
}

/*
 * The startReactor function starts the reactor's execution thread.
 * If the reactor is already running or if there are no registered file descriptors,
 * it prints an error message and returns immediately. If it fails to create a new thread,
 * it sets the appropriate errno and prints an error message.
 */
void startReactor(void *reactor1) {
	if (reactor1 == NULL)
	{
		fprintf(stderr, "startReactor() has been failed: %s\n", strerror(EINVAL));
		return;
	}

	reactorPtr reactor = (reactorPtr)reactor1;

	if (reactor->head == NULL)
	{
		fprintf(stderr, "startReactor() has been failed: Tried to start a reactor without registered file descriptors.\n");
		return;
	}

	else if (reactor->running)
	{
		fprintf(stderr, "startReactor() has been failed: Tried to start a reactor that's already running.\n");
		return;
	}

	fprintf(stdout, "Starting reactor thread...\n");

	reactor->running = true;

	int ret_val = pthread_create(&reactor->thread, NULL, Running, reactor1);

	if (ret_val != 0)
	{
		fprintf(stderr, "startReactor() has been failed: pthread_create() failed: %s\n", strerror(ret_val));
		reactor->running = false;
		reactor->thread = 0;
		return;
	}

	fprintf(stdout, "Reactor thread has been started.\n");
}

/*
 * The stopReactor function stops the reactor's execution thread and frees
 * its resources. If the reactor isn't running, it prints an error message and returns immediately.
 * If it fails to cancel the thread or join it, it sets the appropriate errno and prints an error message.
 */
void stopReactor(void *react) {
	if (react == NULL)
	{
		fprintf(stderr, "stopReactor() has been failed: %s\n", strerror(EINVAL));
		return;
	}

	reactorPtr reactor = (reactorPtr)react;
	void *reactor_1 = NULL;

	if (!reactor->running)
	{
		fprintf(stderr, "stopReactor() has been failed: Tried to stop a reactor that's not currently running.\n");
		return;
	}

	fprintf(stdout, "Stopping reactor thread...\n");

	reactor->running = false;

	int reactor_val = pthread_cancel(reactor->thread);

	if (reactor_val != 0)
	{
		fprintf(stderr, "stopReactor() has been failed: pthread_cancel() failed: %s\n", strerror(reactor_val));
		return;
	}

	reactor_val = pthread_join(reactor->thread, &reactor_1);

	if (reactor_val != 0)
	{
		fprintf(stderr, "stopReactor() has been failed: pthread_join() failed: %s\n", strerror(reactor_val));
		return;
	}

	if (reactor_1 == NULL)
	{
		fprintf(stderr, "stopReactor() has been failed: Reactor thread error: %s", strerror(errno));
		return;
	}

	// Free the reactor's file descriptors.
	if (reactor->fds != NULL)
	{
		free(reactor->fds);
		reactor->fds = NULL;
	}
	
	// Reset reactor pthread.
	reactor->thread = 0;

	fprintf(stdout, "Reactor thread has been stopped.\n");
}

/*
 * The addFd function registers a new file descriptor and its corresponding event handler in the reactor.
 * If it fails to allocate a new reactor node, it sets the appropriate errno and prints an error message.
 * If the file descriptor is already closed or the handler is NULL, it sets errno to EINVAL and prints an error message.
 */
void addFd(void *react, int fd, handler_t handler) {
	if (react == NULL || 
        handler == NULL || 
        fd < 0 || 
        fcntl(fd, F_GETFL) == -1 || 
        errno == EBADF)
	{
		fprintf(stderr, "addFd() failed: %s\n", strerror(EINVAL));
		return;
	}

	fprintf(stdout, "Adding a new FD %d to the list.\n", fd);

	reactorPtr reactor = (reactorPtr)react;
	reactorNodeP node = (reactorNodeP)malloc(sizeof(reactorNode));

	if (node == NULL)
	{
		fprintf(stderr, "addFd() failed: malloc() has been failed: %s\n", strerror(errno));
		return;
	}

	node->fd = fd;
	node->hd.handler = handler;
	node->next = NULL;

	if (reactor->head == NULL)
		reactor->head = node;

	else
	{
		reactorNodeP curr = reactor->head;

		while (curr->next != NULL)
			curr = curr->next;

		curr->next = node;
	}

	fprintf(stdout, "Successfuly added FD %d to the list.\n", fd);
}

/*
 * The WaitFor function waits for the reactor's execution thread to finish.
 * If the reactor isn't running, it returns immediately.
 * If it fails to join the thread, it sets the appropriate errno and prints an error message.
 */
void WaitFor(void *react) {
	if (react == NULL)
	{
		fprintf(stderr, "WaitFor() has been failed: %s\n", strerror(EINVAL));
		return;
	}

	reactorPtr reactor = (reactorPtr)react;
	void *reactor_1 = NULL;

	if (!reactor->running)
		return;

	fprintf(stdout, "Reactor thread joined.\n");

	int ret_value = pthread_join(reactor->thread, &reactor_1);
	
	if (ret_value != 0)
	{
		fprintf(stderr, "WaitFor() has been failed: pthread_join() failed: %s\n", strerror(ret_value));
		return;
	}

	if (reactor_1 == NULL)
		fprintf(stderr, "Reactor thread error: %s", strerror(errno));
}