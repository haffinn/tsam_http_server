#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static void check_max(gpointer p, gpointer currentMax)
{
    gint element = GPOINTER_TO_INT(p);
    gint *max = currentMax;

    if (element > *(max))
    {
        *(max) = element;
    } 
}

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

void generateDOM(session_t* session, int connectFd, char* resource, gchar* color, bool hasBG, gchar* statusCode, bool isSlashTest, GHashTable* variables, GString* header, char* postData)
{
	GString* DOM = g_string_sized_new (0);

	g_string_append_printf(DOM, "%s", "<!DOCTYPE html>\r\n<html>\r\n");

	if(g_strcmp0(statusCode, "200") == 0)
	{
		if(hasBG)
		{
			g_string_append_printf(DOM, "<body style=\"background-color: %s \">", color);
		}
		else
		{
			g_string_append_printf(DOM, "%s", "<body>");
		}

		// TODO --  ATH GetIP Addr og getport virkar ekki :/
		// Laga þetta html - það er eitthvað pínu skrítið held ég...
		gchar* host = g_hash_table_lookup(session->headers, "Host");
		g_string_append_printf(DOM, "\r\n\t%s%s \r\n\t%s:%d\r\n", host, resource, getIpAdress(session), getPort(session));

		if (isSlashTest)
		{
			GList* keys = g_hash_table_get_keys(variables);
			GList* values = g_hash_table_get_values(variables);

			int size = g_hash_table_size(variables);
			int i;

			for (i = 0; i < size; i++)
			{
				gchar* key = g_list_nth_data(keys, i);
				gchar* val = g_list_nth_data(values, i);
				printf("\nite: %d\n", i);
				printf("key: %s\n", key);
				printf("val: %s\n", val);
				if (i == 0)
				{
					g_string_append_printf(DOM, "\n\t%s = %s<br/>\n\t", key, val);
				}
				else 
				{
					g_string_append_printf(DOM, "%s = %s<br/>\n\t", key, val);
				}
			}
			g_string_append_printf(DOM, "%s", "\r");

			g_list_free_1(keys);
			g_list_free_1(values);
		}
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		g_string_append_printf(DOM, "%s", "\t<h2>404</h2>\r\n\t<p>Oops! The page you requested was not found!</p>\r\n");
	}

	if (session->verb == VERB_POST)
	{
		g_string_append_printf(DOM, "\t<div>%s</div>\n", postData);
	}
	g_string_append_printf(DOM, "%s", "</body>\r\n</html>");

	// printf("#######Header: \n '%s'\n", header->str);
	// printf("#######HTML: \n '%s'\n", DOM->str);

	g_string_append_printf(header, "Content-Length: %lu\r\n\r\n", DOM->len);

	if (session->verb == VERB_HEAD)
	{
		send(connectFd, header->str, header->len, 0);
	}
	if (session->verb == VERB_GET || session->verb == VERB_POST)
	{
		send(connectFd, header->str, header->len, 0);
		send(connectFd, DOM->str, DOM->len, 0);
	}

	// Todo free up GString objects...
}

void generateResponse(session_t* session, int connectFd, char* resource, gchar* color, bool hasBG, gchar* statusCode, bool isSlashTest, GHashTable* variables, char* postData)
{
	GString* header = g_string_sized_new(0);
	if(g_strcmp0(statusCode, "200") == 0)
	{
		g_string_append_printf(header, "%s", "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nMax-Age: 3600\r\n");
		if (hasBG)
		{
			g_string_append_printf (header, "Set-Cookie: color=%s\r\n", color);
		}
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		g_string_append_printf(header,"%s", "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n");
	}
	generateDOM(session, connectFd, resource, color, hasBG, statusCode, isSlashTest, variables, header, postData);
}

GHashTable* parseQueryString(gchar* queryString)
{
    GHashTable* query = g_hash_table_new(g_str_hash, g_str_equal);

    if (queryString != NULL)
    {
        gchar** pairs = g_strsplit(queryString, "&", 100);
        gchar** pair;
        int size = g_strv_length(pairs), i;

        for (i = 0; i < size; i++)
        {  
            pair = g_strsplit(pairs[i], "=", 2);
            g_hash_table_insert(query, pair[0], pair[1]);
        }
    }
    return query;
}

void handleRequest(session_t* session, int connectFd, char* resource, char* postData)
{

    // TODO: destroy query hash table?
    // TODO: add POST and HEAD - Haffi

    gchar** data = g_strsplit(resource, "?", 2);
    gchar* file = data[0];
    GHashTable* query = parseQueryString(data[1]);

    bool hasBG = false;
    bool isSlashTest = false;


    // if resource is empty, like this: localhost:port/
    if (data[1] == NULL && (g_strcmp0(data[0], "/") == 0))
    {
        // TODO: skoða port og IP   path missing: session->path before resource
        generateResponse(session, connectFd, resource, NULL, false, "200", isSlashTest, query, postData);

    }
    else
    {
        if (g_strcmp0(file, "/color") == 0)
        {
            gchar* color = g_hash_table_lookup(query, "bg");
           
            if (color != NULL)
            {
            	// URI contains /color?bg=x
                hasBG = true;
                generateResponse(session, connectFd, resource, color, hasBG, "200", isSlashTest, query, postData);
            }
            else
            {
                // URI does not contains bg=(...)
                // Check if request contains cookie
                // Cookie:color=red ---> [color = color=red]
                color = g_hash_table_lookup(session->headers, "Cookie");
                
               
                if (color == NULL)
                {
                    generateResponse(session, connectFd, resource, NULL, hasBG, "200", isSlashTest, query, postData);
                }
                else
                {
                	hasBG = true;
                	// colorvalue(key, value) ---> [color] [red]
                	gchar** cookie = g_strsplit(color, "=", 2);
                	if (cookie[1] != NULL)
                	{
                		gchar* myColor = cookie[1];
                		generateResponse(session, connectFd, resource, myColor, hasBG, "200", isSlashTest, query, postData);
                	}
                }
            }
        }
        else if (g_strcmp0(file, "/test") == 0)
        {
        	isSlashTest = true;
        	generateResponse(session, connectFd, resource, NULL, hasBG, "200", isSlashTest, query, postData);
        }
        else
        {
        	generateResponse(session, connectFd, resource, NULL, false, "404", isSlashTest, query, postData);
        }   
    }
}

void logToFile(session_t* session, int socket, char* resource, char* verb, int responseCode) {
    FILE* file = fopen("httpd.log", "a");
    connection_t *c =(connection_t *)g_hash_table_lookup(session->connections, GINT_TO_POINTER(socket));

    if (c == NULL)
    {
        return;
    }

    if (file == NULL) {
        perror("Failed to open logfile.\n");
        return;
    }

    GTimeVal tv;
    g_get_current_time(&tv);
    tv.tv_usec = 0;
    gchar *timestr = g_time_val_to_iso8601(&tv);

    // Print to file (append)
    fprintf(file, "%s : %s:%d %s\n", timestr, c->ip, c->port, verb);
    fprintf(file, "%s : %d\n\n", resource, responseCode);

    // Free resources
    g_free(timestr);
    fclose(file);
}

int closeSocket(int socket, session_t* session)
{
    close(socket);
    FD_CLR(socket, &session->read_fds);                   
    g_queue_remove(session->q, GINT_TO_POINTER(socket));

    int max = session->listener;
    g_queue_foreach(session->q, check_max, &max);
    return max;
}

void server(session_t* session)
{
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE], protocol[PROTOCOL_SIZE];
    char buffer[BUFFER_SIZE];
    int selectStatus, currentReadFd, readBytes;

    session->q = g_queue_new();

    struct timeval timer;
    timer.tv_sec = 30;
    timer.tv_usec = 0;

    // Setup read file descriptor set and timer for select()
    fd_set reader;
    FD_ZERO(&reader);
    session->read_fds = newFdSet();

    // Keep track of largest file descriptor
    session->listener = createSocket(session->server);
    session->maxFd = session->listener;
    session->connections = g_hash_table_new (g_direct_hash, g_direct_equal);
    FD_SET(session->listener, &session->read_fds);

    // Main Loop
    for (;;)
    {
        reader = session->read_fds;
        selectStatus = select(session->maxFd + 1, &reader, NULL, NULL, &timer);

        if (selectStatus == -1)
        {
            fprintf(stderr, "Select failed\n");
            exit(1);
        }
        // Handle timeouts
        else if (selectStatus == 0)
        {
            if (g_queue_get_length(session->q) > 0)
            {
                session->maxFd = closeSocket(GPOINTER_TO_INT(g_queue_pop_tail(session->q)), session);
                continue;
            }
        }

        // There's something to read
        for(currentReadFd = 0; currentReadFd <= session->maxFd; currentReadFd++)
        {
            if (!FD_ISSET(currentReadFd, &reader))
            {
                continue;
            }

            if (currentReadFd == session->listener)
            {
                newConnection(session);
            }
            else
            {
                memset(buffer, '\0', BUFFER_SIZE);
                memset(verb, '\0', VERB_SIZE);
                memset(resource, '\0', RESOURCE_SIZE);
                memset(protocol, '\0', PROTOCOL_SIZE);

                readBytes = recv(currentReadFd, buffer, BUFFER_SIZE - 1, 0);;

                if (readBytes <= 0)
                {
                    closeSocket(currentReadFd, session);
                    continue;
                }

                chunks = g_strsplit(buffer, "\r\n\r\n", 2);
                lines = g_strsplit(chunks[0], "\r\n", 20);
                tokens = g_strsplit(lines[0], " ", 3);
                strncpy(verb, tokens[0], strlen(tokens[0]));
                strncpy(resource, tokens[1], strlen(tokens[1]));
                strncpy(protocol, tokens[2], strlen(tokens[2]));
                
                setSessionHeaders(session, lines);
                setSessionVerb(session, verb);

                if (session->verb == VERB_HEAD || session->verb == VERB_GET)
                {
                    logToFile(session, currentReadFd, resource, verb, 200);
                    handleRequest(session, currentReadFd, resource, NULL);
                }
                else if (session->verb == VERB_POST)
                {
                    logToFile(session, currentReadFd, resource, verb, 200);
                    handleRequest(session, currentReadFd, resource, chunks[1]);
                }
                else
                {
                    logToFile(session, currentReadFd, resource, verb, 200);
                }

                gchar* connection = (gchar *) g_hash_table_lookup(session->headers, "Connection");

                g_queue_remove(session->q, GINT_TO_POINTER(currentReadFd));
                g_queue_push_head(session->q, GINT_TO_POINTER(currentReadFd));

                if ((g_strcmp0(protocol, "HTTP/1.0") == 0 && g_strcmp0(connection, "keep-alive") != 0) || (g_strcmp0(connection, "close") == 0))
                {
                    closeSocket(currentReadFd, session);
                }
            }
        }
    }

    g_hash_table_destroy(session->connections);
    FD_ZERO(&session->read_fds);
    close(session->listener);
}
