#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdbool.h>

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

// void buildDom(char* data, char* buffer)
// {
// 	memset(buffer, 0, BUFFER_SIZE);
// 	snprintf(buffer, strlen(data) + 64, "<!doctype html>\n<html>\n<body>\n\t<div>%s</div>\n</body>\n</html>", data);
// }

void printResource(int connectFd, char* resource, bool isPost, char* buffer)
{
	// TODO: change hardcoded data
	if (isPost)
	{
		char* returnString = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<p>http://localhost", resource, "<br/>\n\t127.0.0.1:2182</p>\n\n\t", buffer, "\n<body/>\n<html/>\n", (gchar*) NULL);
		send(connectFd, returnString, strlen(returnString), 0);
	}
	else
	{
		char* returnString = g_strconcat("<!doctype html>\n<html>\n<body>\n\t<p>http://localhost", resource, "<br/>\n\t127.0.0.1:2182</p>\n<body/>\n<html/>\n", (gchar*) NULL);
		send(connectFd, returnString, strlen(returnString), 0);
	}
}

void checkforparams(int connectFd, bool isColor, gchar* strAfterQuestionMark)
{
	printf("%s\n", "inside checkforparams");
	gchar** seperateByAmpersant; //, seperateByEqual;
	GHashTable* parameters = g_hash_table_new(g_str_hash, g_str_equal);

	// arg=foo&arg2=bar&arg3=myarg   --->  [arg=foo] [arg2=bar&arg3=myarg]
	seperateByAmpersant = g_strsplit(strAfterQuestionMark, "&", 2);
	printf("%s\n", "1");
	printf("%s\n", seperateByAmpersant[0]);

	// TODO: While setning virkar ekki rétt... :(
	// TODO: Ef ekkert "&" -> Tjékka ef bara 1 argument
	while (seperateByAmpersant[1] != NULL)
	{
		printf("%s\n", "2");
		printf("%s\n", seperateByAmpersant[1]);
		if (g_strcmp0(seperateByAmpersant[1], "") != 0)
		{
			printf("%s\n", "3");
			// Resource includes something like "arg=foo&"
			// Print 404?
			printf("%s\n", "Resource includes something like 'arg=foo&'");
		}
		else 
		{
			printf("%s\n", "4");
			gchar** arguments = g_strsplit(seperateByAmpersant[0], "=", 2);

			if(arguments[1] != NULL)
			{
				printf("%s\n", "5");
				if (g_strcmp0(arguments[1], "") != 0)
				{
					printf("%s\n", "6");
					// Resource includes something like "arg="
					// Print 404?
					printf("%s\n", "Resource includes something like 'arg='");
				}
				else 
				{
					printf("%s\n", "7");
					if (isColor && g_strcmp0(arguments[0], "bg") == 0)
					{
						printf("%s\n", "8");
						printf("%s\n", "bg spotted");
						//color = arguments[0];
						// g_strconcat("<body style=\"background-color:", color, (gchar *)NULL);

					}
					else 
					{
						printf("%s\n", "9");
						// Add args to HTML
						g_hash_table_insert(parameters, arguments[0], arguments[1]);
						gchar* returnString = g_strconcat("<br/>\n\t", arguments[0], " = ", arguments[1], (gchar *)NULL);
						send(connectFd, returnString, strlen(returnString), 0);
					}
				}
			}
			printf("%s\n", "10");
			seperateByAmpersant = g_strsplit(seperateByAmpersant[1], "&", 2);
		}
	}
	printf("%s\n", "done");
}

void handleURI(session_t* session, int connectFd, char *resource, char* data, char* buffer)
{
	gchar **seperateByQM; //, **seperateByAmpersant, **seperateByEqual;
	// gchar *color;

	// char* myIp = getIpAdress(session);
	// unsigned short myPort = getPort(session);

	bool isPost = false;

	if (data != NULL)
	{
		isPost = true;
		memset(buffer, 0, BUFFER_SIZE);
		snprintf(buffer, strlen(data) + 11, "<div>%s</div>", data);
	}

	// test?arg=foo&arg2=bar   --->   [test], [arg=foo&arg2=bar]
	seperateByQM = g_strsplit(resource, "?", 2);

	if ((seperateByQM[1] != NULL) && (g_strcmp0(seperateByQM[1], "") != 0))
	{
		//Eitthvað hægra megin við "?"
		if (g_strcmp0(seperateByQM[0], "/color") == 0)
		{
			// "color" er vinstra megin við "?"
			checkforparams(connectFd, true, seperateByQM[1]);
		}
		else
		{
			// Ekki color
			// Check for parameters
			checkforparams(connectFd, false, seperateByQM[1]);
		}
	}
	else
	{
		//Ekkert hægra megin við "?" eða ekkert "?"
		// Print resource
		printf("%s\n", "no '?'");
	}
}

void handleGetRequest(session_t* session, int connectFd, char* resource)
{
	handleURI(session, connectFd, resource, NULL, NULL);
}

void handlePostRequest(session_t* session, int connectFd, char* resource, char* data, char* buffer)
{
	handleURI(session, connectFd, resource, data, buffer);
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
    	// g_new0(char, BUFFER_SIZE);
		memset(buffer, 0, BUFFER_SIZE);

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

            handlePostRequest(session, connectFd, resource, chunks[1], buffer);

   	        // buildDom(chunks[1], buffer);
       	    // send(connectFd, headerOk, strlen(headerOk), 0);
       	    // send(connectFd, buffer, strlen(buffer), 0);
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
