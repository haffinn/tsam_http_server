#include <sys/socket.h>
#include <netinet/in.h>

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

    GHashTable* headers;

    struct sockaddr_in server;
    struct sockaddr_storage client;
    socklen_t client_size;
} session_t;

void setSessionHeaders(session_t *session, gchar **lines)
{
    int headersCount = g_strv_length(lines) - 1;
    session->headers = g_hash_table_new(g_str_hash, g_str_equal);

    while (headersCount-- > 1)
    {
        gchar **header  = g_strsplit(lines[headersCount], ": ", 2);
        g_hash_table_insert(session->headers, header[0], header[1]);
        printf("%s: %s\n", header[0], header[1]);
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
