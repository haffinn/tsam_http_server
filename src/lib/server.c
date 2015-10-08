#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <assert.h>

static void check_max(gpointer p, gpointer currentMax)
{
    gint element = GPOINTER_TO_INT(p);
    gint *max = currentMax;

    if (element > *(max))
    {
        *(max) = element;
    } 
}

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
    char buf[1000];                     
    memset(buf, '\0', 1000);
    FILE *file;

    if (g_strcmp0(resource, "/") == 0) {
        file = fopen("htdocs/index.html", "r");
    } else {
        file = fopen("htdocs/style.css", "r");
    }

    fread(buf, 999, 1, file);
    send(connectFd, buf, strlen(buf), 0);
}

void logToFile(char *ip, int port, char* resource, char* verb, int responseCode) {
    FILE* file = fopen("log", "a");

    if (file == NULL) {
        perror("Failed to open logfile.\n");
        exit(1);
    }

    GTimeVal tv;
    g_get_current_time(&tv);
    gchar *timestr = g_time_val_to_iso8601(&tv);

    // Print to file (append)
    fprintf(file, "%s : %s:%d %s\n", timestr, ip, port, verb);

    fprintf(file, "%s : %d\n", resource, responseCode);

    // Free resources
    g_free(timestr);
    fclose(file);
}

int closeSocket(int socket, session_t* session)
{
    close(socket);
    FD_CLR(socket, &session->read_fds);                   
    g_queue_remove(session->q, GINT_TO_POINTER(socket));

    int max = session->listener;
    g_queue_foreach(session->q, check_max, &max);
    return max;
}

void server(session_t* session)
{
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    char buffer[BUFFER_SIZE];
    int selectStatus, currentReadFd, readBytes, newSocket;

    // Queue containing connected stream sockets in LRU.
    // Least recently used connection is last.
    session->q = g_queue_new();

    struct timeval timer;
    timer.tv_sec = 5;
    timer.tv_usec = 0;

    // TODO: Build dynamically
    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

    // Setup read file descriptor set and timer for select()
    fd_set reader;
    FD_ZERO(&reader);
    session->read_fds = newFdSet();

    // Keep track of largest file descriptor
    session->listener = createSocket(session->server);
    int maxFileDescriptor = session->listener;
    FD_SET(session->listener, &session->read_fds);
    
    // Remote address info
    struct sockaddr_storage remoteAddr;
    char remoteIP[INET_ADDRSTRLEN];
    socklen_t remoteAddrLen;

    // Main Loop
    for (;;)
    {
        reader = session->read_fds;
        selectStatus = select(maxFileDescriptor + 1, &reader, NULL, NULL, &timer);

        if (selectStatus == -1)
        {
            fprintf(stderr, "Select failed\n");
            exit(1);
        }
        else if (selectStatus == 0)
        {
            if (g_queue_get_length(session->q) > 0)
            {
                maxFileDescriptor = closeSocket(GPOINTER_TO_INT(g_queue_pop_tail(session->q)), session);
            }
        }

        for(currentReadFd = session->listener + 1; currentReadFd <= maxFileDescriptor; currentReadFd++)
        {
            if (FD_ISSET(currentReadFd, &reader))
            {
                memset(buffer, '\0', BUFFER_SIZE);

                getnameinfo((struct sockaddr *) &remoteAddr, remoteAddrLen, remoteIP, sizeof(remoteIP), NULL, 0, NI_NUMERICHOST);
                readBytes = recv(currentReadFd, buffer, BUFFER_SIZE - 1, 0);;

                if (readBytes <= 0)
                {
                    closeSocket(currentReadFd, session);
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
                    logToFile(remoteIP, session->port, resource, verb, 200);
                    send(currentReadFd, headerOk, strlen(headerOk), 0);

                    if (session->verb == VERB_GET)
                    {
                        handleGetRequest(currentReadFd, resource);
                    }
                }
                else if (session->verb == VERB_POST)
                {
                    logToFile(remoteIP, session->port, resource, verb, 200);
                    buildDom(chunks[1], buffer);
                    send(currentReadFd, headerOk, strlen(headerOk), 0);
                    send(currentReadFd, buffer, strlen(buffer), 0);
                }
                else
                {
                    logToFile(remoteIP, session->port, resource, verb, 200);
                }

                gpointer connection = g_hash_table_lookup(session->headers, "Connection");

                if (connection == NULL || g_strcmp0(connection, "keep-alive") != 0)
                {
                    closeSocket(currentReadFd, session);
                }
                else 
                {                    
                    g_queue_remove(session->q, GINT_TO_POINTER(currentReadFd));
                    g_queue_push_head(session->q, GINT_TO_POINTER(currentReadFd));
                }
            }
        }
        
        if (FD_ISSET(session->listener, &reader))
        {
            remoteAddrLen = sizeof(remoteAddr);
            newSocket = accept(session->listener, (struct sockaddr *) &remoteAddr, &remoteAddrLen);

            if (newSocket == -1)
            {
                perror("accept");
                break;
            }

            g_queue_push_head(session->q, GINT_TO_POINTER(newSocket));
            FD_SET(newSocket, &session->read_fds);

            if (newSocket > maxFileDescriptor)
            {
                maxFileDescriptor = newSocket;
            }
        }
    }

    FD_ZERO(&session->read_fds);
    close(session->listener);
}
