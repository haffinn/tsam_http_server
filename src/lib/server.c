#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

void server(session_t* session) 
{
    printf("Port %d\n", session->port);

    for (;;)
    {
    	int connectFd = accept(session->socket_fd, NULL, NULL);

    	if (connectFd < 0)
    	{
    		perror("Accept failed\n");
    		close(session->socket_fd);
    		exit(1);
    	}

    	printf("connected to %d\n", connectFd);

    	if (shutdown(connectFd, SHUT_RDWR) == -1)
    	{
    		perror("Shutdown failed\n");
    		close(connectFd);
    		close(session->socket_fd);
    		exit(1);
    	}

    	close(connectFd);
    }

    close(session->socket_fd);
}
