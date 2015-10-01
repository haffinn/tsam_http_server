#include <stdlib.h>
#include <stdio.h>

#include "lib/session.c"
#include "lib/socket.c"
#include "lib/server.c"

/**
 * Kick start!
 */
int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Please prove port number\n");
        return 1;
    }

    session_t session = {
        .port = atoi(argv[1]),
        .directory = "htdocs",
        .block_number = 0,
        .state = 0 // TODO: Hmmm?
    };
   
    session.client_size = sizeof(session.client);
    session.server = createServer(session.port);
    session.socket_fd = createSocket(session.server);

    server(&session);
    return 0;
}
