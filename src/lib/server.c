#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <assert.h>

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

void handleGetRequest(int connectFd, char *resource)
{
    send(connectFd, "<html/>\n", 9, 0);
}

void logToFile(session_t *session, char* resource, char* verb, int responseCode) {
    FILE* file = fopen("log", "a");

    if (file == NULL) {
        perror("Failed to open logfile.\n");
        exit(1);
    }

    GTimeVal tv;
    g_get_current_time(&tv);
    gchar *timestr = g_time_val_to_iso8601(&tv);

    // Print to file (append)
    fprintf(file, "%s : %s:%d %s\n",
        timestr, getIpAdress(session), getPort(session), verb);

    fprintf(file, "%s : %d\n", resource, responseCode);

    // Free resources
    g_free(timestr);
    fclose(file);
}

void server(session_t* session)
{
    char buffer[BUFFER_SIZE];
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    int connectFd;
    int selectStatus;
    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

    struct sockaddr_storage remote; // client address
    socklen_t remoteSize;
    char remoteIP[INET_ADDRSTRLEN];

    for (;;)
    {
        memset(buffer, '\0', BUFFER_SIZE);
        session->timer = newTimer();
        session->read_fds = newReadFds(session);
        selectStatus = select(
            session->socket_fd + 1,
            &session->read_fds,
            NULL,
            NULL,
            &session->timer
        );

    	if (selectStatus > 0) {
            assert(FD_ISSET(session->socket_fd, &session->read_fds));

            if ((connectFd = accept(session->socket_fd, NULL, NULL)) < 0)
        	{
        		perror("Accept failed\n");
        		close(session->socket_fd);
        		exit(1);
        	}
            
            printf("new connection from %s on socket %d\n",
                            inet_ntop(remote.ss_family,
                                &(((struct sockaddr_in*) &remote)->sin_addr),
                                remoteIP, INET_ADDRSTRLEN),
                            session->socket_fd);

            read(connectFd, buffer, BUFFER_SIZE - 1);
            
            chunks = g_strsplit(buffer, "\r\n\r\n", 2);
            lines = g_strsplit(chunks[0], "\r\n", 20);
            tokens = g_strsplit(lines[0], " ", 3);
            strncpy(verb, tokens[0], VERB_SIZE);
            strncpy(resource, tokens[1], RESOURCE_SIZE);

            setSessionHeaders(session, lines);
            setSessionVerb(session, verb);

            if (session->verb == VERB_HEAD || session->verb == VERB_GET)
            {
                logToFile(session, resource, verb, 200);
           	    send(connectFd, headerOk, strlen(headerOk), 0);

           	    if (session->verb == VERB_GET)
           	    {
                    handleGetRequest(connectFd, resource);
           	    }
            }
            else if (session->verb == VERB_POST)
            {
                logToFile(session, resource, verb, 200);
       	        buildDom(chunks[1], buffer);
           	    send(connectFd, headerOk, strlen(headerOk), 0);
           	    send(connectFd, buffer, strlen(buffer), 0);
            }
            else
            {
                 logToFile(session, resource, verb, 500);
            }

        	if (shutdown(connectFd, SHUT_RDWR) == -1)
        	{
        		perror("Shutdown failed\n");
        		close(connectFd);
        		close(session->socket_fd);
        		exit(1);
        	}

        	close(connectFd);
            g_hash_table_destroy(session->headers);
    	}
    }

    close(session->socket_fd);
}
