#define _GNU_SOURCE
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include "config.h"
#include "socket.h"
#include <unistd.h>

void *connection_handler(void *socket_desc);
long read_file(const char *file, char **buffer, int *status_code);
int parse_header(char *header, char **file);
const char* get_mime(const char *file);

void *connection_handler(void *socket_desc)
{
	int sock = *(int*)socket_desc;

	char *requested_file;

	char *response_data = NULL;
	char response_version[50] = "HTTP/1.0 ",
		 response_status[200] = "200 OK",
	     response_content_type[200];
	long long response_content_length;

	char recv_buffer[2000];
	recv(sock, recv_buffer, 2000, 0);

	strncpy(response_status, "200 OK", sizeof(response_status));

	if(!parse_header(recv_buffer, &requested_file))
		strncpy(response_status, "400 Bad Request", sizeof(response_status));

	int status_code;
	if((response_content_length =
			read_file(requested_file, &response_data, &status_code)) < 0)
		goto exit_func;

	if(NULL == response_data)
		goto exit_func;

	if(404 == status_code)
	{
		strncpy(response_status, "404 File Not Found", sizeof(response_status));
		strncpy(response_content_type, get_mime("/404.html"), 200-1);
	}
	else
		strncpy(response_content_type, get_mime(requested_file), 200-1);

	size_t respLength = response_content_length+
			strlen("Content-Type: ")+
			strlen(response_content_type)+
			strlen("\r\n")+
			strlen(response_version)+
			strlen(response_status)+
			strlen("\r\n")+
			strlen("\r\n")+
			strlen("\r\n")-2;

	char *message;
	if(!(message = malloc(respLength)))
	{
		perror("malloc");
		goto exit_func;
	}

	strcpy(message,response_version);
	strcat(message,response_status);
	strcat(message, "\r\n");
	strcat(message, "Content-Type: ");
	strcat(message,response_content_type);
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	strcat(message, "\r\n");
	/*
		This is using memcpy instead of strcat since response_data may
		contain binary data and therefore not be null terminated.
	*/
	memcpy(message+respLength-response_content_length,
			response_data, response_content_length+1);  
	
	if(write(sock, message, respLength) == -1)
		perror("write");

exit_func:
	free(response_data);
	close(sock);
	free(socket_desc);
	return 0;
}

int main(int argc, char **argv)
{
	/* TODO: Drop privleges */
	if(getuid() != 0)
	{
		fprintf(stderr,
				"Error: Program must be started as root.");
		return 1;
	}

	if(chroot(WEBSITE_ROOT) != 0)
	{
		perror("chroot");
		return 1;
	}

	if(chdir("/") != 0) /* 
						   I am unsure if this even can fail.
						   But I will keep it here just in case.
						*/
	{
		perror("chdir");
		return 1;
	}
	
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
	init_server(&socket_desc, &server, port);

	c = sizeof(struct sockaddr_in);
	while((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		pthread_t connection_thread;
		if(!(new_sock = malloc(1))) return 1;
		*new_sock = new_socket;
		
		if(pthread_create(&connection_thread, NULL, connection_handler, (void*)new_sock) < 0)
		{
			perror("Unable to create a thread.");
			return 1;
		}
		
		pthread_join(connection_thread, NULL);
	}
	close(socket_desc);

	return 0;
}

int parse_header(char *header, char **file)
{
	if(!strtok(header, " ") || !(*file = strtok(NULL, "")) || !(*file = strtok(*file, " ")))
	{
		*file ="/400.html";
		return 0;
	}

	if(strcmp(*file, "") == 0 || strcmp(*file, "/") == 0)
		*file = "/index.html";

	return 1;
}

const char* get_mime(const char *file)
{
	char fileExtension[strlen(file)+1];
	strcpy(fileExtension, file);
	fileExtension[strlen(file)] = '\0';
	if(strtok(fileExtension, ".") == NULL)
		return "application/octet-stream";

	if((strncpy(fileExtension, strtok(NULL, "."), strlen(file))))
	{
		for(size_t i = 0;i < sizeof(mimes)/sizeof(mimes[0])-1;i++)
		{
			if(strcmp(mimes[i].ext, fileExtension) == 0)
				return mimes[i].type;
		}
	}
	return "application/octet-stream";
}

long read_file(const char *file, char **buffer, int *status_code)
{
	*status_code = 200;
	long length;
	FILE *fp;
	if(!(fp = fopen(file, "rb")))
	{
		if(strcmp(file, "/404.html") != 0)
		{
			*status_code = 404;
			return read_file("/404.html", buffer, status_code);
		}
		return -1;
	}
	fseek(fp, 0, SEEK_END);
	length = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if(!(*buffer = malloc(length + 1)))
	{
		perror("malloc");
		return -1;
	}
	fread(*buffer, 1, length, fp);
	fclose(fp);
	
	return length;
}
