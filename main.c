#include <pthread.h>
#include "config.h"
#include "http.h"
#include "socket.h"

#define DEFAULT_PORT 1337

size_t strlcpy(char *dst, const char *src, size_t dstsize)
{
	size_t len = strlen(src);
	if(dstsize)
	{
		size_t bl = (len < dstsize-1 ? len : dstsize-1);
		((char*)memcpy(dst, src, bl))[bl] = 0;
	}
	return len;
}

long long readFile(char *fileStr, char **buffer)
{
	FILE *filePointer = fopen(fileStr, "rb");

	if(filePointer == NULL)
	{
		buffer = NULL;
		return 0;
	}
	fseek(filePointer, 0, SEEK_END);
	long fsize = ftell(filePointer);
	fseek(filePointer, 0, SEEK_SET);

	char *tmpBuffer = malloc(fsize + 1);
	fread(tmpBuffer, 1, fsize, filePointer);
	buffer[fsize] = '\0';
	fclose(filePointer);
	*buffer = tmpBuffer;
	return fsize;
}

void LOG(char *str)
{
	FILE *fp;
	fp = fopen("log.txt", "a");
	fscanf(fp, "%s",str);
	fclose(fp);
}

void *connection_handler(void *socket_desc)
{
	struct http_response response;
	char client_message[2000];
	int http_status_code = 200;
	int sock = *(int*)socket_desc;

	recv(sock, client_message , sizeof(client_message)/sizeof(client_message[0]), 0);

	struct http_header request = parse_header(client_message);
	if(request.method == NULL || request.file == NULL)
		http_status_code = 400;

	FILE *filePointer;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
	char *fileToRead = NULL;
    filePointer = fopen(".website_files", "r");
    if (filePointer != NULL)
	{
		while ((read = getline(&line, &len, filePointer)) != -1)
		{
			if(line[0] == '#')
				continue;
			strtok(line, "\n");
			if(strcmp(strtok(line, " "), request.file) == 0)
			{
				fclose(filePointer);
				fileToRead = strtok(NULL, " ");
				break;
			}
		}
	}
	else
	{
		perror("Failed to open .website_files");
		http_status_code = 500;
	}

	if(fileToRead == NULL)
		http_status_code = 404;

	switch(http_status_code)
	{
		case 400:
			strlcpy(response.header, "HHTTP/1.0 400 Bad Request",sizeof(response.header)/sizeof(char));
			fileToRead = malloc(strlen("xxx.html")+1);
			strlcpy(fileToRead,"400.html", strlen("xxx.html")+1);
			break;
		case 404:
			strlcpy(response.header, "HTTP/1.0 404 File Not Found",sizeof(response.header)/sizeof(char));
			fileToRead = malloc(strlen("xxx.html")+1);
			strlcpy(fileToRead,"404.html", strlen("xxx.html")+1);
			break;
		case 500:
			strlcpy(response.header, "HTTP/1.0 500 Internal Server Error",sizeof(response.header)/sizeof(char));
			fileToRead = malloc(strlen("xxx.html")+1);
			strlcpy(fileToRead,"500.html", strlen("xxx.html")+1);
			break;
		case 200:
		default:
			strlcpy(response.header, "HTTP/1.0 200 OK",sizeof(response.header)/sizeof(char));
	}
	
	//Open and read the contents of the requested file.
	response.content_length = readFile(fileToRead, &response.data);
	if(response.data == NULL)
	{
		perror("Failed to open file.");
		return 0;
	}

	/* Set a default value for content_type. */	
	strlcpy(response.content_type, "application/octet-stream", sizeof(response.content_type)/sizeof(char));

	strtok(fileToRead, ".");
	char *fileExtension;
	if((fileExtension = strtok(NULL, ".")))
		for(size_t i = 0;i < sizeof(mimes)/sizeof(mimes[0])-1;i++)
			if(strcmp(mimes[i].ext, fileExtension) == 0)
				strlcpy(response.content_type, mimes[i].type, sizeof(response.content_type)/sizeof(char));


	long long resp_length = response.content_length+
			strlen("Content-Type: ")+
			strlen(response.content_type)+
			strlen("\r\n")+
			strlen(response.header)+
			strlen("\r\n")+
			strlen("\r\n")+
			strlen("\r\n");

	char *message = calloc(resp_length,sizeof(char));

	strcat(message,response.header);
	strcat(message, "\r\n");
	strcat(message, "Content-Type: ");
	strcat(message,response.content_type);
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	strcat(message,response.data);
	
	if(write(sock, message, resp_length) == -1)
	{
		perror("Failed to send message via socket.");
	}
	if(response.data)
		free(response.data);
	if(line)
		free(line);

	close(sock);
	sock = -1;

	if(socket_desc)
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
			fprintf(stderr, "Usage: %s [-p PORT] -h(Print this message)\n", argv[0]);
			exit(1);
			break;
		}

	int socket_desc;
	struct sockaddr_in server, client;

	int new_socket, c, *new_sock;
	startServer(&socket_desc, &server, port);

	c = sizeof(struct sockaddr_in);
	while((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		pthread_t sniffer_thread;
		new_sock = malloc(1);
		*new_sock = new_socket;
		
		if(pthread_create(&sniffer_thread, NULL, connection_handler, (void*)new_sock) < 0)
		{
			perror("Unable to create a thread.");
		}
		
		pthread_join( sniffer_thread , NULL);
	}
	close(socket_desc);

	return 0;
}
