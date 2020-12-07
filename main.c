#include <pthread.h>
#include <bsd/string.h>
#include <getopt.h>
#include "config.h"
#include "http.h"
#include "socket.h"

int parse_website_file(char *file, char **output)
{
	*output = NULL;
	char line[255];
	FILE *filePointer;
	filePointer = fopen(".website_files", "r");
	if (!filePointer)
	{
		perror("fopen");
		return 0;
	}

	while (fgets(line, 255, filePointer))
	{
		if(line[0] == '#')
			continue;
		strtok(line, "\n");
		if(strcmp(strtok(line, " "), file) == 0)
		{
			char *buffer = strtok(NULL, " ");
			*output = malloc(strlen(buffer)+1);
			strlcpy(*output, buffer, strlen(buffer)+1);
			break;
		}
	}
	fclose(filePointer);
	return 1;
}

// TODO: Divide this function into smaller pieces
void *connection_handler(void *socket_desc)
{
	struct http_response response;
	char client_message[2000];
	int http_status_code = 200;
	int sock = *(int*)socket_desc;
	
	strlcpy(response.version, "HTTP/1.0 ",
				sizeof(response.version));

	recv(sock, client_message, 2000, 0);

	struct http_header request = parse_header(client_message);
	if(request.method == NULL || request.file == NULL)
	{
		http_status_code = 400;
		goto request_error_handler;
	}

	char *requested_file;
	if(!parse_website_file(request.file, &requested_file))
	{
		http_status_code = 500;;
		goto request_error_handler;
	}
	else if(!requested_file)
	{
		http_status_code = 404;
		goto request_error_handler;
	}
	
	strlcpy(response.status, "200 OK",
			sizeof(response.status));

	if(0)
	{
request_error_handler:
		requested_file = malloc(strlen("xxx.html")+1);
		if(!requested_file)
		{
			perror("malloc");
			goto exit_func;
		}

		switch(http_status_code)
		{
		case 400:
			strlcpy(response.status, "400 Bad Request",
					sizeof(response.status));
			strlcpy(requested_file,"400.html", strlen("400.html")+1);
			break;
		case 404:
			strlcpy(response.status, "404 File Not Found",
					sizeof(response.status));
			strlcpy(requested_file,"404.html", strlen("404.html")+1);
			break;
		case 500:
			strlcpy(response.status, "500 Internal Server Error",
					sizeof(response.status));
			strlcpy(requested_file,"500.html", strlen("500.html")+1);
			break;
		}
	}

	// Open and read the contents of the requested file.
	FILE *fp;
	if(!(fp = fopen(requested_file, "rb")))
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

	// Set a default value for content_type.
	strlcpy(response.content_type, "application/octet-stream",
			sizeof(response.content_type));

	strtok(requested_file, ".");
	char *fileExtension;
	if((fileExtension = strtok(NULL, ".")))
	{
		for(size_t i = 0;i < sizeof(mimes)/sizeof(mimes[0])-1;i++)
		{
			if(strcmp(mimes[i].ext, fileExtension) == 0)
			{
				strlcpy(response.content_type, mimes[i].type,
						sizeof(response.content_type));
			}
		}
	}

	long long respLength = response.content_length+
			strlen("Content-Type: ")+
			strlen(response.content_type)+
			strlen("\r\n")+
			strlen(response.version)+
			strlen(response.status)+
			strlen("\r\n")+
			strlen("\r\n")+
			strlen("\r\n")-2;

	char *message;
	if(!(message = malloc(respLength)))
	{
		perror("malloc");
		goto exit_func;
	}

	strcpy(message,response.version);
	strcat(message,response.status);
	strcat(message, "\r\n");
	strcat(message, "Content-Type: ");
	strcat(message,response.content_type);
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	memcpy(message+respLength-response.content_length,response.data,
			response.content_length+1);
	
	if(write(sock, message, respLength) == -1)
		perror("write");

exit_func:
	free(requested_file);
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
			return 0;
			break;
		}

	int socket_desc;
	struct sockaddr_in server, client;

	int new_socket, c, *new_sock;
	start_server(&socket_desc, &server, port);

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
