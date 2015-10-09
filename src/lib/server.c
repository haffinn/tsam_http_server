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

void generateDOM(int connectFd, gchar* color, bool hasBG, gchar* statusCode, GHashTable* variables, GString* header)
{
	printf("%s\n", "inside gen DOM");
	printf("%s\n", "7");
	//gchar* DOM;
	GString* DOM = g_string_sized_new (0);

	g_string_append_printf(DOM, "%s", "<!DOCTYPE html>\r\n<html>\r\n");
	// DOM = "<!doctype html>\r\n<html>\r\n"

	if(g_strcmp0(statusCode, "200") == 0)
	{
		printf("%s\n", "8");
		if(hasBG)
		{
			printf("%s\n", "9");
			// gchar* result = g_strconcat("<!doctype html>\r\n<html>\r\n<body style=\"background-color: ", color, "\">", (char *) NULL);
			g_string_append_printf(DOM, "<body style=\"background-color: %s \">", color);
		}
		else
		{
			printf("%s\n", "10");
			g_string_append_printf(DOM, "%s", "<body>");
		}
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		printf("%s\n", "11");
		g_string_append_printf(DOM, "%s", "\t<h2>404</h2>\r\n\t<p>Oops! The page you requested was not found!</p>\r\n");
		// gchar *notFound = "<!doctype html>\r\n<html>\r\n<body>\r\n\t<h2>404</h2>\r\n\t<p>Oops! The page you requested was not found!</p>\r\n</body>\r\n</html>";
		// send(connectFd, notFound, strlen(notFound), 0);
	}
	printf("%s\n", "12");
	g_string_append_printf(DOM, "%s", "</body>\r\n</html>");

	printf("#######Header: \n '%s'\n", header->str);
	printf("#######HTML: \n '%s'\n", DOM->str);

	g_string_append_printf(header, "Content-Length: %lu\r\n\r\n", DOM->len);


	send(connectFd, header->str, header->len, 0);

	// TODO bara senda ef GET/POST - Ekki ef HEAD
	send(connectFd, DOM->str, DOM->len, 0);
}

void generateResponse(int connectFd, gchar* color, bool hasBG, gchar* statusCode, GHashTable* variables)
{
	GString* header = g_string_sized_new(0);
	// gchar* header;
	printf("%s\n", "inside gen response");
	printf("%s\n", "1");
	if(g_strcmp0(statusCode, "200") == 0)
	{
		printf("%s\n", "2");
		g_string_append_printf(header, "%s", "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nMax-Age: 3600\r\n");
		// header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nMax-Age: 3600\r\n";

		if (hasBG)
		{
			printf("%s\n", "3");
			g_string_append_printf (header, "Set-Cookie: color=%s\r\n", color);
			// header = g_strconcat(header, "Set-Cookie: color=", color, "\r\n", (char *) NULL);
		}
		// else
		// {
		// 	g_string_append_printf(header, "%s", "\r\n");
		// }
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		printf("%s\n", "4");
		g_string_append_printf(header,"%s", "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n");
		// header = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n";
	}

	printf("%s\n", "5");
	generateDOM(connectFd, color, hasBG, statusCode, variables, header);
	printf("%s\n", "done");
}

// void generateHeader(int connectFd, gchar* color, bool hasBG)
// {
//     gchar *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
//     gchar *cookie = g_strconcat("Set-Cookie: color=", color, " Max-Age:3600\r\n", (char *) NULL);


//     if (hasBG)
//     {
//         gchar *returnString = g_strconcat(headerOk, cookie, "\r\n", (char *) NULL);
//         send(connectFd, returnString, strlen(returnString), 0);
//         printf("%s\n", "Goes into hasBG");
//     }
//     else
//     {
//         gchar *returnString = g_strconcat(headerOk, "\r\n", (char *) NULL);
//         send(connectFd, returnString, strlen(returnString), 0);
//         printf("%s\n", "Does not go into hasBG");
//     }
//     // GString *headerString = g_string_sized_new(0);
//     // g_string_append_printf(headerString,"%s", headerOk);
//     // if(user added parameters)
//     // {
//     //     g_string_append_printf(headerString,"Set-Cookie: color=%s; Max-Age:3600\r\n", color);
//     // }

//     // g_string_append(headerString, "\r\n");

//     // g_string_free(headerString);

// }

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

void handleGetRequest(session_t* session, int connectFd, char* resource)
{

    // TODO: destroy query hash table?
    // TODO: add POST and HEAD - Haffi

    char buffer[6000];
    memset(buffer, '\0', 6000);
    gchar** data = g_strsplit(resource, "?", 2);
    gchar* file = data[0];
    GHashTable* query = parseQueryString(data[1]);
    int size = g_hash_table_size(query);
    bool hasBG = false; 
    //bool hasParam = false;
    GList *keys = g_hash_table_get_keys(query);
    GList* values = g_hash_table_get_values(query);
    keys = g_list_reverse(keys);
    values = g_list_reverse(values);

    // if resource is empty, like this: localhost:port/
    if (data[1] == NULL && (g_strcmp0(data[0], "/") == 0))
    {
        // TODO: skoða port og IP   path missing: session->path before resource
        // gchar *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-length:9999\r\nHost:localhost:4074\r\n";
        // send(connectFd, headerOk, strlen(headerOk), 0);

        // gchar* result = g_strconcat("<!doctype html>\r\n<html>\r\n<body><body/>\r\n<html/>\n", (char *) NULL);
        // send(connectFd, result, strlen(result), 0);

        generateResponse(connectFd, NULL, false, "200", query);

    }
    else
    {
        if (g_strcmp0(file, "/color") == 0)
        {
            gchar* color = g_hash_table_lookup(query, "bg");
            //printf("COLOR: %s\n", color);
            // generateHeader(connectFd, color, hasBG);
           
            if (color != NULL)
            {
                hasBG = true;
                generateResponse(connectFd, color, hasBG, "200", query);

                // gchar* colorHeader = g_strconcat("color=", color, (gchar*) NULL);
                // g_hash_table_insert(session->headers, "Set-Cookie", colorHeader);

                // gchar* setCookieHeader = g_strconcat("Set-Cookie:", colorHeader, "\r\n", (gchar *) NULL);
                // send(connectFd, setCookieHeader, strlen(setCookieHeader), 0);

                // printf("\n\nSetting cookie as: %s\n\n\n", colorHeader);
                // // g_hash_table_insert(cookieID, &connectFd, color);
            	
            	
                // // set-cookie
                // //g_hash_table_insert(header, "Set-Cookie", color);
                // //gchar* cookie = g_hash_table_lookup(header, "Set-Cookie");
                // //printf("COOKIE: %s\n", cookie);
                
                // gchar* result = g_strconcat("<!doctype html>\r\n<html>\r\n<body style=\"background-color: ", color, "\">", (char *) NULL);
                // send(connectFd, result, strlen(result), 0);
            }
            else
            {
                // SKOÐA!!!!
                color = g_hash_table_lookup(session->headers, "Cookie");

                gchar** colorValue = g_strsplit(color, "=", 2);
                gchar* myValue = colorValue[1];

                printf("COLOR: %s\n", color);
                
                if (color == NULL)
                {
                    //color = "white";
                    // gchar* result = g_strconcat("<!doctype html>\r\n<html>\r\n<body>", (char *) NULL);
                    // send(connectFd, result, strlen(result), 0);

                    generateResponse(connectFd, NULL, false, "200", query);
                }
                else
                {
                	generateResponse(connectFd, color, true, "200", query);
                	// gchar* result = g_strconcat("<!doctype html>\r\n<html>\r\n<body style=\"background-color: ", myValue, "\">", (char *) NULL);
               		// send(connectFd, result, strlen(result), 0);
                }
            }
            
            // send(connectFd, "\r\n</body>\r\n</html>\r\n", 22, 0);
            
            // // ef param líka, þá prenta þá
            // if (hasBG)
            // {
            //     if (g_hash_table_size(query) > 1)
            //     {
            //         hasParam = true;
            //     }
            // }
            // else
            // {
            //     if (g_hash_table_size(query) > 0)
            //     {
            //         hasParam = true;
            //     }
            // }

            // if (hasParam)
            // {
            //     int i;

            //     for (i = 0; i < size; i++)
            //     {
            //         if (!g_strcmp0(g_list_nth_data (keys, i), "bg") == 0)
            //         {
            //             gchar* result = g_strconcat("<br/>\n\t", g_list_nth_data (keys, i), " = ", g_list_nth_data (values, i), (char *)NULL);
            //             send(connectFd, result, strlen(result), 0);
            //         }
            //     }
            // }

        }
        else if (g_strcmp0(file, "/test") == 0)
        {
            gchar *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
            send(connectFd, headerOk, strlen(headerOk), 0);
            // TODO: skoða port og IP   path missing: session->path before resource
            gchar* topHtml = g_strconcat("<!doctype html>\r\n<html>\r\n<body>\r\n\t<p>", resource, "<br/>\r\n\t", getIpAdress(session), ":", getPort(session), (char *) NULL);
            send(connectFd, topHtml, strlen(topHtml), 0);
            
            int i;
            
            for (i = 0; i < size; i++)
            {
                gchar* result = g_strconcat("<br/>\n\t", g_list_nth_data (keys, i), " = ", g_list_nth_data (values, i), (char *)NULL);
                send(connectFd, result, strlen(result), 0);
            }
            send(connectFd, "<p/>\n<body/>\n<html/>\n", 26, 0);
        }
        else
        {
            // gchar *headerNotFound = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n";
            // send(connectFd, headerNotFound, strlen(headerNotFound), 0);
            // send(connectFd, "<!doctype html>\r\n<html>\r\n<body>\r\n\t<h2>404</h2>\r\n\t<p>Oops! The page you requested was not found!</p>\r\n</body>\r\n</html>", 131, 0);
        	generateResponse(connectFd, NULL, false, "200", query);
        }   
    }
}

void logToFile(char *ip, int port, char* resource, char* verb, int responseCode) {
    FILE* file = fopen("log", "a");

    if (file == NULL) {
        perror("Failed to open logfile.\n");
        exit(1);
    }

    GTimeVal tv;
    g_get_current_time(&tv);
    gchar *timestr = g_time_val_to_iso8601(&tv);

    // Print to file (append)
    fprintf(file, "%s : %s:%d %s\n", timestr, ip, port, verb);

    fprintf(file, "%s : %d\n", resource, responseCode);

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

    //GHashTable* cookies;
    //cookies = g_hash_table_new(g_int_hash, g_str_equal);

    //char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n";
    
    /* COPYRIGHT HRAFNKELL*/
    // GString *headerString = g_string_sized_new(0);
    // g_string_append_printf(headerString,"%s", headerOk);
    // if(user added parameters)
    // {
    //     g_string_append_printf(headerString,"Set-Cookie: color=%s; Max-Age:3600\r\n", color);
    // }

    // g_string_append(headerString, "\r\n");

    // g_string_free(headerString);
    /* DONT STEAL, YOU WOULDNT KILL A BABY */

void server(session_t* session)
{
    gchar **lines, **tokens, **chunks;
    char verb[VERB_SIZE], resource[RESOURCE_SIZE], protocol[PROTOCOL_SIZE];
    char buffer[BUFFER_SIZE];
    int selectStatus, currentReadFd, readBytes;

    // Queue containing connected stream sockets in LRU.
    // Least recently used connection is last.
    session->q = g_queue_new();

    struct timeval timer;
    timer.tv_sec = 30;
    timer.tv_usec = 0;

    // TODO: Build dynamically
    char *headerOk = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 425\r\n\r\n";

    // Setup read file descriptor set and timer for select()
    fd_set reader;
    FD_ZERO(&reader);
    session->read_fds = newFdSet();

    // Keep track of largest file descriptor
    session->listener = createSocket(session->server);
    session->maxFd = session->listener;
    FD_SET(session->listener, &session->read_fds);
    
    // Remote address info
    struct sockaddr_storage remoteAddr;
    char remoteIP[INET_ADDRSTRLEN];

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
            
            socklen_t remoteAddrLen = sizeof(remoteAddr);

            if (currentReadFd == session->listener)
            {
                newConnection(session, remoteAddr, remoteAddrLen);
            }
            else
            {
                memset(buffer, '\0', BUFFER_SIZE);
                memset(verb, '\0', VERB_SIZE);
                memset(resource, '\0', RESOURCE_SIZE);
                memset(protocol, '\0', PROTOCOL_SIZE);

                getnameinfo((struct sockaddr *) &remoteAddr, remoteAddrLen, remoteIP, sizeof(remoteIP), NULL, 0, NI_NUMERICHOST);
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
                    logToFile(remoteIP, session->port, resource, verb, 200);

                    if (session->verb == VERB_GET)
                    {
                        handleGetRequest(currentReadFd, resource);
                    }
                }
                else if (session->verb == VERB_POST)
                {
                    logToFile(remoteIP, session->port, resource, verb, 200);
                    buildDom(chunks[1], buffer);
                    send(connectFd, buffer, strlen(buffer), 0);
                }
                else
                {
                    logToFile(remoteIP, session->port, resource, verb, 200);
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

    FD_ZERO(&session->read_fds);
    close(session->listener);
}
