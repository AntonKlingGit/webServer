#include <pthread.h>
#include <bsd/string.h>
#include <getopt.h>
#include "config.h"
#include "http.h"
#include "socket.h"

#define DEFAULT_PORT 1337

void *connection_handler(void *socket_desc)
{
	struct http_response response;
	char client_message[2000];
	int http_status_code = 200;
	int sock = *(int*)socket_desc;

	recv(sock, client_message, 2000, 0);

	struct http_header request = parse_header(client_message);
	if(request.method == NULL || request.file == NULL)
		http_status_code = 400;

	FILE *filePointer;
	char line[255];
	char *fileToRead = NULL;
	filePointer = fopen(".website_files", "r");
	if (filePointer)
	{
		while (fgets(line, 255, filePointer))
		{
			if(line[0] == '#')
				continue;
			strtok(line, "\n");
			if(strcmp(strtok(line, " "), request.file) == 0)
			{
				fileToRead = strtok(NULL, " ");
				break;
			}
		}
		fclose(filePointer);
	}
	else
	{
		perror("Failed to open .website_files");
		http_status_code = 500;
	}

	if(!fileToRead)
	{
		fileToRead = malloc(strlen("400.html")+1);
		if(!fileToRead)
		{
			perror("malloc");
			goto exit_func;
		}
		if(http_status_code == 200) /* aslong as no other error has occured we
									   know that the error is that the
									   requested file does not exist.*/
			http_status_code = 404;
	}

	switch(http_status_code)
	{
		case 400:
			strlcpy(response.header, "HTTP/1.0 400 Bad Request",
					sizeof(response.header)/sizeof(char));
			strlcpy(fileToRead,"400.html", strlen("400.html")+1);
			break;
		case 404:
			strlcpy(response.header, "HTTP/1.0 404 File Not Found",
					sizeof(response.header)/sizeof(char));
			strlcpy(fileToRead,"404.html", strlen("404.html")+1);
			break;
		case 500:
			strlcpy(response.header, "HTTP/1.0 500 Internal Server Error",
					sizeof(response.header)/sizeof(char));
			strlcpy(fileToRead,"500.html", strlen("500.html")+1);
			break;
		case 200:
		default:
			strlcpy(response.header, "HTTP/1.0 200 OK",
					sizeof(response.header)/sizeof(char));
	}
	
	/* Open and read the contents of the requested file. */
	FILE *fp;
	if(!(fp = fopen(fileToRead, "rb")))
	{
		perror("fopen");
		goto exit_func;
	}
	fseek(fp, 0, SEEK_END);
	response.content_length = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	if(!(response.data = malloc(response.content_length + 1)))
	{
		perror("malloc");
		goto exit_func;
	}
	fread(response.data, 1, response.content_length, fp);
	fclose(fp);

	/* Set a default value for content_type. */	
	strlcpy(response.content_type, "application/octet-stream",
			sizeof(response.content_type)/sizeof(char));

	strtok(fileToRead, ".");
	char *fileExtension;
	if((fileExtension = strtok(NULL, ".")))
		for(size_t i = 0;i < sizeof(mimes)/sizeof(mimes[0])-1;i++)
			if(strcmp(mimes[i].ext, fileExtension) == 0)
				strlcpy(response.content_type, mimes[i].type,
						sizeof(response.content_type)/sizeof(char));

	

	long long resp_length = response.content_length+
			strlen("Content-Type: ")+
			strlen(response.content_type)+
			strlen("\r\n")+
			strlen(response.header)+
			strlen("\r\n")+
			strlen("\r\n")+
			strlen("\r\n");

	char *message;
	if(!(message = malloc(resp_length * sizeof(char))))
	{
		perror("malloc");
		goto exit_func;
	}

	strcpy(message,response.header);
	strcat(message, "\r\n");
	strcat(message, "Content-Type: ");
	strcat(message,response.content_type);
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	memcpy(message+resp_length-response.content_length,response.data,
			response.content_length);
	
	if(write(sock, message, resp_length) == -1)
	{
		perror("write");
	}

exit_func:
	free(response.data);
	close(sock);
	free(socket_desc);
	return 0;
}

int main(int argc, char **argv)
{
	short port = DEFAULT_PORT;
	int ch;
	while ((ch = getopt(argc, argv, "p:h")) != -1)
		switch((char)ch)
		{
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
			fprintf(stderr,
					"Usage: %s [-p PORT] -h(Print this message)\n", argv[0]);
			exit(1);
			break;
		}

	int socket_desc;
	struct sockaddr_in server, client;

	int new_socket, c, *new_sock;
	startServer(&socket_desc, &server, port);

	c = sizeof(struct sockaddr_in);
	while((new_socket =
			accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		pthread_t sniffer_thread;
		if(!(new_sock = malloc(1))) return 1;
		*new_sock = new_socket;
		
		if(pthread_create(&sniffer_thread,
						NULL, connection_handler, (void*)new_sock) < 0)
		{
			perror("Unable to create a thread.");
			return 1;
		}
		
		pthread_join( sniffer_thread , NULL);
	}
	close(socket_desc);

	return 0;
}
