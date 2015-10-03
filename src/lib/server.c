#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define BUFFER_SIZE 65535
#define VERB_SIZE 25
#define RESOURCE_SIZE 255


unsigned short getPort(session_t* session) {
    struct sockaddr_in* socket_address = (struct sockaddr_in*) &session->client;
    return socket_address->sin_port;
}

char* getIpAdress(session_t* session) {
    struct sockaddr_in* socket_address = (struct sockaddr_in*) &session->client;
    int ip_address = socket_address->sin_addr.s_addr;
    char ip_string[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip_address, ip_string, INET_ADDRSTRLEN);
    char* p = ip_string;
    return p;
}

void buildDom(char* data, char* buffer)
{
	memset(buffer, 0, BUFFER_SIZE);
	snprintf(buffer, strlen(data) + 64, "<!doctype html>\n<html>\n<body>\n\t<div>%s</div>\n</body>\n</html>", data);
}

void server(session_t* session) 
{
    char buffer[BUFFER_SIZE];
    gchar **lines, **tokens, **chunks;
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

        chunks = g_strsplit(buffer, "\r\n\r\n", 2);
        lines = g_strsplit(chunks[0], "\r\n", 3);
        tokens = g_strsplit(lines[0], " ", 3);
        strncpy(verb, tokens[0], VERB_SIZE);
        strncpy(resource, tokens[1], RESOURCE_SIZE);

		GTimeVal tv;
		gchar *timestr;
    	g_get_current_time(&tv);
    	timestr = g_time_val_to_iso8601(&tv);

        printf("%s : %s:%d %s\n", timestr, getIpAdress(session), getPort(session), verb);
        fflush(stdout);
        g_free(timestr);

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
       	    send(connectFd, headerOk, strlen(headerOk), 0);
        	buildDom(chunks[1], buffer);
       	    send(connectFd, buffer, strlen(buffer), 0);
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
