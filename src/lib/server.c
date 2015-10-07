#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
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
    printf("Get request for %s\n", resource);
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
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    char buffer[BUFFER_SIZE];
    int selectStatus, currentReadFd, readBytes, newSocket;
    int countSockets = 0;
    int connectedSockets[200];

    struct timeval timer;
    timer.tv_sec = 3;
    timer.tv_usec = 0;

    // TODO: Build dynamically
    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

    // Setup read file descriptor set and timer for select()
    fd_set master;
    FD_ZERO(&master);

    // Keep track of largest file descriptor
    int listener = createSocket(session->server);
    int maxFileDescriptor = listener;
    FD_SET(listener, &master);

    // Remote address info
    struct sockaddr_storage remoteAddr;
    char remoteIP[INET_ADDRSTRLEN];
    socklen_t remoteAddrLen;

    // Main Loop
    printf("Starting server\n");
    for (;;)
    {
        memset(buffer, '\0', BUFFER_SIZE);

        selectStatus = select(maxFileDescriptor + 1, &master, NULL, NULL, &timer);

        if (selectStatus == -1)
        {
            fprintf(stderr, "Select failed\n");
            exit(1);
        }
        else if (selectStatus == 0)
        {
            if (countSockets > 0)
            {
                maxFileDescriptor = connectedSockets[--countSockets];
                close(connectedSockets[countSockets]);
            }
        }
        else
        {
            for(currentReadFd = 0; currentReadFd <= maxFileDescriptor; currentReadFd++)
            {
                if (FD_ISSET(currentReadFd, &master))
                {
                    if (currentReadFd == listener)
                    {
                        printf("New foo\n");

                        remoteAddrLen = sizeof(remoteAddr);
                        newSocket = accept(listener, (struct sockaddr *) &remoteAddr, &remoteAddrLen);

                        if (newSocket == -1)
                        {
                            perror("accept");
                            break;
                        }

                        FD_SET(newSocket, &master);

                        if (newSocket > maxFileDescriptor)
                        {
                            maxFileDescriptor = newSocket;
                        }

                        connectedSockets[countSockets++] = newSocket;

                        getnameinfo((struct sockaddr *) &remoteAddr, remoteAddrLen, remoteIP, sizeof(remoteIP), NULL, 0, NI_NUMERICHOST);
                        printf("new connection from %s on socket %d\n", remoteIP, newSocket);
                    }
                    else
                    {
                        readBytes = recv(currentReadFd, buffer, BUFFER_SIZE - 1, 0);

                        if (readBytes <= 0)
                        {
                            continue;
                        }

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
                       	    send(currentReadFd, headerOk, strlen(headerOk), 0);

                       	    if (session->verb == VERB_GET)
                       	    {
                                handleGetRequest(currentReadFd, resource);
                       	    }
                        }
                        else if (session->verb == VERB_POST)
                        {
                            logToFile(session, resource, verb, 200);
                   	        buildDom(chunks[1], buffer);
                       	    send(currentReadFd, headerOk, strlen(headerOk), 0);
                       	    send(currentReadFd, buffer, strlen(buffer), 0);
                        }
                        else
                        {
                             logToFile(session, resource, verb, 500);
                        }

                        // gpointer connection = g_hash_table_lookup(session->headers, "Connection");
                        //
                        // if (connection != NULL && g_strcmp0(connection, "close") == 0)
                        // {
                        //     printf("Close %d from foonat\n", currentReadFd);
                        //     close(currentReadFd);
                        //     FD_CLR(currentReadFd, &master);
                        //     countSockets--;
                        // }
                    }
                }
            }

















            // assert(FD_ISSET(listener, &master));
            // acceptNewSession(session);
    	}
    }

    FD_ZERO(&master);
    close(listener);
}
