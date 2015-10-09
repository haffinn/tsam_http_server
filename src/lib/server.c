#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
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

void buildDom(char* data, char* buffer)
{
	memset(buffer, 0, BUFFER_SIZE);
	snprintf(buffer, strlen(data) + 64, "<!doctype html>\n<html>\n<body>\n\t<div>%s</div>\n</body>\n</html>", data);
}

void generateDOM(int connectFd, gchar* color, bool hasBG, gchar* statusCode, GHashTable* variables, GString* header)
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
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		g_string_append_printf(DOM, "%s", "\t<h2>404</h2>\r\n\t<p>Oops! The page you requested was not found!</p>\r\n");
	}
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
	if(g_strcmp0(statusCode, "200") == 0)
	{
		g_string_append_printf(header, "%s", "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nMax-Age: 3600\r\n");
		// header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nMax-Age: 3600\r\n";

		if (hasBG)
		{
			g_string_append_printf (header, "Set-Cookie: color=%s\r\n", color);
		}
	}
	else if(g_strcmp0(statusCode, "404") == 0)
	{
		g_string_append_printf(header,"%s", "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n");
	}
	generateDOM(connectFd, color, hasBG, statusCode, variables, header);
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
        generateResponse(connectFd, NULL, false, "200", query);

    }
    else
    {
        if (g_strcmp0(file, "/color") == 0)
        {
            gchar* color = g_hash_table_lookup(query, "bg");
           
            if (color != NULL)
            {
                hasBG = true;
                generateResponse(connectFd, color, hasBG, "200", query);
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
                    generateResponse(connectFd, NULL, false, "200", query);
                }
                else
                {
                	generateResponse(connectFd, color, true, "200", query);
                }
            }
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
        	generateResponse(connectFd, NULL, false, "200", query);
        }   
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

        //printf("%s\n", chunks[0]);

        setSessionHeaders(session, lines);
        setSessionVerb(session, verb);

        if (session->verb == VERB_HEAD || session->verb == VERB_GET)
        {
            logToFile(session, resource, verb, 200);
       	    //send(connectFd, headerOk, strlen(headerOk), 0);

       	    if (session->verb == VERB_GET)
       	    {
                handleGetRequest(session, connectFd, resource);
       	    }
        }
        else if (session->verb == VERB_POST)
        {
            logToFile(session, resource, verb, 200);
   	        buildDom(chunks[1], buffer);
       	    //send(connectFd, headerOk, strlen(headerOk), 0);
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
    //g_hash_table_destroy(cookies);
    close(session->socket_fd);
}
