#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>

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

void handleGetRequest(session_t* session, int connectFd, char *resource)
{
    // TODO: setja inn slóð, ip addressu og port nr
    // 
    GHashTable* parameters;

    gchar **seperateByQuestionMark, **seperateByAmpersant;
    gchar *color;

    char* myIp = getIpAdress(session);
    unsigned short myPort = getPort(session);

    // printf("path: %s\n", session->path);
    // printf("dir: %s\n", session->directory);
    // printf("IP: %s\n", myIp);
    // printf("port: %d\n", myPort);
    // printf("filename: %s\n", session->filename);


    // test?arg=foo&arg2=bar   --->   [test], [arg=foo&arg2=bar]
    seperateByQuestionMark = g_strsplit(resource, "?", 2);

    if (seperateByQuestionMark[1] != NULL)
    {
        gchar* stringAfterSplit = seperateByQuestionMark[1];

        // arg=foo&arg2=bar   --->  [arg=foo] [arg2=bar]   
        seperateByAmpersant = g_strsplit(stringAfterSplit, "&", 100);

        if (seperateByAmpersant[1] != NULL)
        {
            parameters = g_hash_table_new(g_str_hash, g_str_equal);

            int size = g_strv_length(seperateByAmpersant);
            int i;
            parameters = g_hash_table_new(g_str_hash, g_str_equal);

            gchar **header;

            gchar* result = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<p>", "(TODO - session->path)", resource, "<br/>\n\t", myIp, ":", "(TODO - myPort)", (char *) NULL);
            send(connectFd, result, strlen(result), 0);
            // printf("%s\n", result);
            // fflush(stdout); 

            //send(connectFd, "<!doctype html>\n<html>\n<body>\n\t<p>http://localhost:2000/?arg1=one&arg2=two&arg3=three<br/>\n\t127.0.0.1:1043", 110, 0);
            for (i = 0; i < size; i++)
            {
                header  = g_strsplit(seperateByAmpersant[i], "=", 2);
                g_hash_table_insert(parameters, header[0], header[1]);
                //printf("%s -> %s\n", header[0], header[1]);

                gchar* result = g_strconcat("<br/>\n\t", header[0], " = ", header[1], (char *)NULL);
                send(connectFd, result, strlen(result), 0);
            }
            send(connectFd, "<p/>\n<body/>\n<html/>\n", 26, 0);
        }
        else
        {
            gchar **array;
            array = g_strsplit(seperateByAmpersant[0], "=", 2);

            if (g_strcmp0(array[0], "bg") == 0)
            {
                color = array[1];
                // html with color
                gchar* returnString = g_strconcat("<!doctype html>\n<html>\n<body style=\"background-color:", color, "\">\n<body/>\n<html/>\n", (char *)NULL);
                send(connectFd, returnString, 88, 0);
            }
            else
            {
                // html with parameter
                gchar* returnString = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<p>", array[0], " = ", array[1], "<p/>\n<body/>\n<html/>\n", (char *)NULL);
                send(connectFd, returnString, 81, 0);
            }
        }
    }
    else
    {
        //ekkert spurningamerki í slóð
        // TODO: breyta harðkóðuðum resource, ip og port í rétt...
        send(connectFd, "<!doctype html>\n<html>\n<body>\n\t<p>http://localhost/<br/>\n\t127.0.0.1:2182</p>\n<body/>\n<html/>\n", 102, 0);
    }
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
    char buffer[BUFFER_SIZE];
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE];
    int connectFd;

    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

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
        lines = g_strsplit(chunks[0], "\r\n", 20);
        tokens = g_strsplit(lines[0], " ", 3);
        strncpy(verb, tokens[0], VERB_SIZE);
        strncpy(resource, tokens[1], RESOURCE_SIZE);

        // printf("resource: %s\n", resource);
        // printf("---- end of resource -----%s\n");


        setSessionHeaders(session, lines);
        setSessionVerb(session, verb);

        if (session->verb == VERB_HEAD || session->verb == VERB_GET)
        {
            logToFile(session, resource, verb, 200);
       	    send(connectFd, headerOk, strlen(headerOk), 0);

       	    if (session->verb == VERB_GET)
       	    {
                handleGetRequest(session, connectFd, resource);
       	    }
        }
        else if (session->verb == VERB_POST)
        {
            logToFile(session, resource, verb, 200);
   	        buildDom(chunks[1], buffer);
       	    send(connectFd, headerOk, strlen(headerOk), 0);
       	    send(connectFd, buffer, strlen(buffer), 0);
        }
        else
        {
             logToFile(session, resource, verb, 500);
        }

    	if (shutdown(connectFd, SHUT_RDWR) == -1)
    	{
    		perror("Shutdown failed\n");
    		close(connectFd);
    		close(session->socket_fd);
    		exit(1);
    	}

    	close(connectFd);
        g_hash_table_destroy(session->headers);
    }

    close(session->socket_fd);
}
