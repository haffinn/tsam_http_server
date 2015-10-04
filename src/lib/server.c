#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

#define VERB_DEFAULT 0
#define VERB_GET 1
#define VERB_HEAD 2
#define VERB_POST 3

#define BUFFER_SIZE 65535
#define VERB_SIZE 25
#define RESOURCE_SIZE 255


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
    // TODO: setja inn slóð, ip addressu og port nr
    // send(connectFd, "<!doctype html><html><body><p>http://localhost/<br/>127.0.0.1:2182</p><body/><html/>\n", 86, 0);

    GHashTable* parameters;

    gchar **seperateByQuestionMark, **seperateByAmpersant, **seperateByEqual;
    gchar *color;

    // test?arg=foo&arg2=bar   --->   [test], [arg=foo&arg2=bar]
    seperateByQuestionMark = g_strsplit(resource, "?", 2);

    //printf("0: %s 1: %s\n", seperateByQuestionMark[0], seperateByQuestionMark[1]);


    if (seperateByQuestionMark[1] != NULL)
    {
        gchar* stringAfterSplit = seperateByQuestionMark[1];

        // arg=foo&arg2=bar   --->  [arg=foo] [arg2=bar]   
        seperateByAmpersant = g_strsplit(stringAfterSplit, "&", 100);
        //printf("seperateByAmpersant[0]: %s -> seperateByAmpersant[1]: %s\n", seperateByAmpersant[0], seperateByAmpersant[1]);

        if (seperateByAmpersant[1] != NULL)
        {
            //printf("%s\n", "Það er ? merki í slóðinni");

            parameters = g_hash_table_new(g_str_hash, g_str_equal);

            int size = g_strv_length(seperateByAmpersant);
            int i = size;
            parameters = g_hash_table_new(g_str_hash, g_str_equal);

            gchar **header;

            send(connectFd, "<!doctype html>\n<html>\n<body>\n\t<p>", 38, 0);
            while (i-- > 0)
            {
                header  = g_strsplit(seperateByAmpersant[i], "=", 2);
                g_hash_table_insert(parameters, header[0], header[1]);
                printf("%s -> %s\n", header[0], header[1]);

                //int length = g_strv_length(returnString);
                //printf("length: %s\n", length);
                //send(connectFd, returnString, 200, 0);

                // if (i == size)
                // {

                // }
                // else
                // {

                // }
                


            }
            send(connectFd, "\n\t<p/>\n<body/>\n<html/>\n", 26, 0);
            
            //printf("%s\n", secondPartOfString);

            //gchar* returnString = g_strconcat(firstPartOfString, thirdPartOfString);
            //printf("%s\n", returnString);
            
        }
        else
        {
            gchar **array;
            array = g_strsplit(seperateByAmpersant[0], "=", 2);
            //printf("array[0]: %s array[1]: %s\n", array[0], array[1]);
            if (g_strcmp0(array[0], "bg") == 0)
            {
                color = array[1];
                //printf("color: %s\n", array[1]);
                // html with color
                gchar* returnString = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<body style=\"background-color:", color, "\">\n<body/>\n<html/>\n");
                send(connectFd, returnString, 88, 0);
            }
            else
            {
                // html with parameter
                gchar* returnString = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<p>", array[0], " = ", array[1], "<p/>\n<body/>\n<html/>\n");
                send(connectFd, returnString, 81, 0);
            }
        }
    }
    else
    {
        //printf("%s\n", "Ekkert ? merki í slóð");
        send(connectFd, "<!doctype html>\n<html>\n<body>\n\t<p>http://localhost/<br/>\n\t127.0.0.1:2182</p>\n<body/>\n<html/>\n", 86, 0);
    }
}

void server(session_t* session)
{
    char buffer[BUFFER_SIZE];
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    int connectFd;
    FILE* file;

    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char *headerFail = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n<h1>Not found</h1>";

    for (;;)
    {
    	g_new0(char, BUFFER_SIZE);

    	if ((connectFd = accept(session->socket_fd, NULL, NULL)) < 0)
    	{
    		perror("Accept failed\n");
    		close(session->socket_fd);
    		exit(1);
    	}

        read(connectFd, buffer, BUFFER_SIZE - 1);

        chunks = g_strsplit(buffer, "\r\n\r\n", 2);
        lines = g_strsplit(chunks[0], "\r\n", 3);
        tokens = g_strsplit(lines[0], " ", 3);
        strncpy(verb, tokens[0], VERB_SIZE);
        strncpy(resource, tokens[1], RESOURCE_SIZE);

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

    	GTimeVal tv;
    	gchar *timestr;
    	g_get_current_time(&tv);
    	timestr = g_time_val_to_iso8601(&tv);

        printf("%s : %s:%d %s\n", timestr, getIpAdress(session), getPort(session), verb);
        printf("%s : %s\n", resource, "200"); // TODO: reponse code
        g_free(timestr);

        file = fopen("htdocs/index.html", "r");

        if (session->verb == VERB_HEAD || session->verb == VERB_GET)
        {
       	    send(connectFd, headerOk, strlen(headerOk), 0);

       	    if (session->verb == VERB_GET)
       	    {
                handleGetRequest(connectFd, resource);
       	    }
        }
        else if (session->verb == VERB_POST)
        {
   	        buildDom(chunks[1], buffer);
       	    send(connectFd, headerOk, strlen(headerOk), 0);
       	    send(connectFd, buffer, strlen(buffer), 0);
        }
        else
        {
        	printf("BAD!\n");
        }

    	if (shutdown(connectFd, SHUT_RDWR) == -1)
    	{
    		perror("Shutdown failed\n");
    		close(connectFd);
    		close(session->socket_fd);
    		exit(1);
    	}

    	close(connectFd);
    }

    close(session->socket_fd);
}
