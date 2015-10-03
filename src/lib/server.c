#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define BUFFER_SIZE 65535
#define VERB_SIZE 25
#define RESOURCE_SIZE 255

void server(session_t* session) 
{
    char buffer[BUFFER_SIZE];
    gchar **lines, **tokens;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    int connectFd;
    FILE* file;

    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char *headerFail = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n<h1>Not found</h1>";

    for (;;)
    {
    	g_new0(char, BUFFER_SIZE);

    	if ((connectFd = accept(session->socket_fd, NULL, NULL)) < 0)
    	{
    		perror("Accept failed\n");
    		close(session->socket_fd);
    		exit(1);
    	}

        read(connectFd, buffer, BUFFER_SIZE - 1);

        lines = g_strsplit(buffer, "\n", 3);
        tokens = g_strsplit(lines[0], " ", 3);
        strncpy(verb, tokens[0], VERB_SIZE);
        strncpy(resource, tokens[1], RESOURCE_SIZE);

        file = fopen("htdocs/index.html", "r");

        if ((g_strcmp0(verb, "GET") == 0) || (g_strcmp0(verb, "HEAD") == 0))
        {
       	    send(connectFd, headerOk, strlen(headerOk), 0);

       	    if (g_strcmp0(verb, "GET") == 0)
       	    {
       	    	fread(buffer, BUFFER_SIZE - 1, 1, file);
       	    	send(connectFd, buffer, strlen(buffer), 0);	
       	    }
        }
        else if (g_strcmp0(verb, "POST") == 0)
        {
        	printf("Post request!\n");
        }
        else
        {
        	printf("BAD!\n");
        }

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
