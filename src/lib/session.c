#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

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
    int socket_fd;
    int connection_fd;

    struct timeval timer;
    fd_set read_fds;
    GHashTable* headers;

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

fd_set newReadFds(session_t *session) {
    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(session->socket_fd, &rfds);
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

void closeSession(session_t *session) {
    printf("...Closing connection...\n");

    if (shutdown(session->connection_fd, SHUT_RDWR) == -1)
	{
		perror("Shutdown failed\n");
		close(session->connection_fd);
		close(session->socket_fd);
		exit(1);
	}

	close(session->connection_fd);
    g_hash_table_destroy(session->headers);
}

void acceptNewSession(session_t *session)
{
    if ((session->connection_fd = accept(session->socket_fd, NULL, NULL)) < 0)
    {
        perror("Accept failed\n");
        close(session->socket_fd);
        exit(1);
    }

    printf("new connection on socket %d\n", session->connection_fd);
}
