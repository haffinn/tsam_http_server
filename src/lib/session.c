#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

typedef union Number
{
    unsigned short value;
    char bytes[2];
} number_t;

typedef struct session
{
    char* filename;
    char* directory;
    char* path;
    char buffer[1024];

    FILE* file;

    int port;
    int state;
    int verb;
    int listener;
    int maxFd;
    int connection_fd;

    struct timeval timer;
    
    fd_set read_fds;
    GHashTable* headers;
    GHashTable* connections;
    GQueue *q;
    
    struct sockaddr_in server;
    struct sockaddr_storage client;
    socklen_t client_size;
} session_t;

struct timeval newTimer() {
    struct timeval timer;
    timer.tv_sec = 3;
    timer.tv_usec = 0;
    return timer;
}

fd_set newFdSet() {
    fd_set rfds;
    FD_ZERO(&rfds);
    return rfds;
}

void setSessionHeaders(session_t *session, gchar **lines)
{
    int headersCount = g_strv_length(lines);
    session->headers = g_hash_table_new(g_str_hash, g_str_equal);

    while (headersCount-- > 1)
    {
        gchar **header  = g_strsplit(lines[headersCount], ": ", 2);
        g_hash_table_insert(session->headers, header[0], header[1]);
    }
}

void setSessionVerb(session_t* session, char* verb)
{
    if (g_strcmp0(verb, "GET") == 0)
	{
	    session->verb = VERB_GET;
	}
	else if (g_strcmp0(verb, "HEAD") == 0)
    {
        session->verb = VERB_HEAD;
    }
    else if (g_strcmp0(verb, "POST") == 0)
    {
        session->verb = VERB_POST;
    }
}

int newConnection(session_t* session)
{
    struct sockaddr_storage remoteAddr;
    char remoteIP[INET_ADDRSTRLEN];
    socklen_t remoteAddrLen = sizeof(remoteAddr);
    int newSocket = accept(session->listener, (struct sockaddr *) &remoteAddr, &remoteAddrLen);

    if (newSocket == -1)
    {
        perror("accept");
        return -1;
    }

    g_queue_push_head(session->q, GINT_TO_POINTER(newSocket));
    FD_SET(newSocket, &session->read_fds);

    if (newSocket > session->maxFd)
    {
        session->maxFd = newSocket;
    }

    getnameinfo((struct sockaddr *) &remoteAddr, remoteAddrLen, remoteIP, sizeof(remoteIP), NULL, 0, NI_NUMERICHOST);
    connection_t *c = malloc(sizeof(connection_t));

    c->socket = newSocket;
    c->port = session->port;
    memcpy(c->ip, remoteIP, INET_ADDRSTRLEN);

    g_hash_table_insert(session->connections, GINT_TO_POINTER(newSocket), c);
    return newSocket;
}
