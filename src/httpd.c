#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

#define VERB_DEFAULT 0
#define VERB_GET 1
#define VERB_HEAD 2
#define VERB_POST 3
#define BUFFER_SIZE 65535
#define VERB_SIZE 25
#define RESOURCE_SIZE 255

#include "lib/session.c"
#include "lib/socket.c"
#include "lib/server.c"

/**
 * Kick start!
 */
int main(int argc, char **argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Please provide port number\n");
        return 1;
    }

    session_t session =
    {
        .port = atoi(argv[1]),
        .directory = "htdocs",
        .verb = 0,
        .state = 0
    };

    session.client_size = sizeof(session.client);
    session.server = createServer(session.port);
    session.socket_fd = createSocket(session.server);

    server(&session);
    return 0;
}
