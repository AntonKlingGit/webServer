#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

#define DEFAULT_PORT 80

//Currently the web server only cares about the method and the file that is requested.
struct http_header
{
	char* method, *file;
};

struct http_header parse_header(char* header)
{
	struct http_header parsed_header;
	if((parsed_header.method = strtok(header, " ")) != NULL)
	{
		parsed_header.file = strtok(NULL, " ");
	}	

	return parsed_header;
}

//This checks wether the requested file is listed in the .website_files file
char* getFile(char *fileStr)
{
	FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen(".website_files", "r");
    if (fp == NULL)
	{
		perror("Failed to open .website_files");
		return "500.html";
	}

    while ((read = getline(&line, &len, fp)) != -1)
	{
		if(strcmp(strtok(line, " "), fileStr) == 0)
		{
			fclose(fp);
			char* return_value = strtok(NULL, " ");				
			// Return the file to read
			return return_value;	
			if(line)
				free(line);
		}
    }
	fclose(fp);
	return "404.html";
}

int socket_desc;
struct sockaddr_in server, client;

void startServer(short port)
{
	socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_desc == -1)
	{
		perror("Unable to create socket.");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if(bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0)
	{
		perror("Failed to assign address to socket.");
		exit(1);
	}

	listen(socket_desc, 3);
}

void *connection_handler(void *socket_desc)
{
	char client_message[2000];
	char *http_server_header = "HTTP/1.0 200 OK\r\n\r\n";
	int sock = *(int*)socket_desc;

	//Get message sent by browser
	recv(sock, client_message , sizeof(client_message)/sizeof(client_message[0]), 0);
	struct http_header request = parse_header(client_message);
	if(request.method == NULL || request.file == NULL)
	{
		http_server_header = "HTTP/1.0 400 Bad Request\r\n\r\n";
	}

	char *fileToRead;
	fileToRead = getFile(request.file);
	if(strcmp(fileToRead,"404.html") == 0)
	{
		//Send back the desierd content
		http_server_header = "HTTP/1.0 404 Not Found\r\n\r\n";
	}
	else if(strcmp(fileToRead,"500.html") == 0)
	{
		http_server_header = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
	}

	
	//Open and read the contents of the requested file.
	FILE *f = fopen(fileToRead, "rb");
	
	if(f != NULL)
	{
		fseek(f, 0, SEEK_END);
		long fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		char *buffer = malloc(fsize + 1);
		fread(buffer, 1, fsize, f);
		buffer[fsize] = '\0';
		fclose(f);

		char *message = malloc(strlen(buffer)+strlen(http_server_header));
		memset(message, 0, sizeof(message));
		strcat(message,http_server_header);
		strcat(message,buffer);
		free(buffer);
		//Send back the desierd content
		if(write(sock, message, strlen(message)) > 0)
		{
			perror("Failed to send message via socket.");
		}
		memset(client_message, 0, strlen(client_message));
		free(message);
	}
	else
	{
		perror("Failed to open file");
		//Incase everything fails this ensures that the server will atleast send a 500 back to the client.
		http_server_header = "HTTP/1.0 500 Internal Server Error\r\n\r\n";
		if(write(sock, http_server_header, strlen(http_server_header)) > 0)
		{
			perror("Failed to send message via socket.");
		}
	}
	
	memset(client_message, 0, strlen(client_message));
	close(sock);
	free(socket_desc);
	return 0;
}

int main(int argc, char **argv)
{
	short port = DEFAULT_PORT;
	int ch;
	while ((ch = getopt(argc, argv, "p:h")) != -1)
		switch((char)ch) {
		case 'p':
			port = atoi(optarg);
			break;
		case 'h':
			fprintf(stderr, "Usage: %s [-p PORT] -h(Print this message)\n", argv[0]);
			exit(1);
			break;
		}	

	int new_socket, c, *new_sock;

	startServer(port);

	puts("Waiting for incoming connections");
	c = sizeof(struct sockaddr_in);
	while((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		puts("A new client has connected");
		printf("%s\n", inet_ntoa(client.sin_addr));

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
