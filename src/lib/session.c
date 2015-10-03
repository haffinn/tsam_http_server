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

    struct sockaddr_in server;
    struct sockaddr_storage client;
    socklen_t client_size;
} session_t;

void setSessionVerb(session_t* session, char* verb) {
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
