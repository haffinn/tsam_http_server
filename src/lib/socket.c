#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>

struct sockaddr_in createServer(int port)
{
    struct sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    return server;
}

int createSocket(struct sockaddr_in server)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd == -1)
    {
        perror("Cannot create socket\n");
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *) &server, (socklen_t) sizeof(server)) == -1)
    {
        perror("Bind failed\n");
        close(sockfd);
        exit(1);
    }

    if (listen(sockfd, 20) == -1)
    {
        perror("Listen failed\n");
        close(sockfd);
        exit(1);
    }

    return sockfd;
}
