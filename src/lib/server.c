#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

void server(session_t* session) 
{
    char buffer[6000];
    gchar **lines, **tokens;
    char verb[25], resource[255];
    int connectFd;
    FILE* file;

    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char *headerFail = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n<h1>Not found</h1>";

    for (;;)
    {
    	if ((connectFd = accept(session->socket_fd, NULL, NULL)) < 0)
    	{
    		perror("Accept failed\n");
    		close(session->socket_fd);
    		exit(1);
    	}

        read(connectFd, buffer, 5999);

        lines = g_strsplit(buffer, "\n", 3);
        tokens = g_strsplit(lines[0], " ", 3);
        strncpy(verb, tokens[0], 25);
        strncpy(resource, tokens[1], 255);

        file = fopen("htdocs/index.html", "r");

        if (file != NULL && (!g_strcmp0(resource, "/") || !g_strcmp0(resource, "/index.html")))
        {
            fread(buffer, 5999, 1, file);
            send(connectFd, headerOk, strlen(headerOk), 0);
            send(connectFd, buffer, strlen(buffer), 0);
        } 
        else
        {
            send(connectFd, headerFail, strlen(headerFail), 0);
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
