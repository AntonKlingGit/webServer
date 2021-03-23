#define _GNU_SOURCE
#include <pthread.h>
#include <string.h>
#include <getopt.h>
#include "config.h"
#include <unistd.h>
#include <assert.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define RECV_BUFFER 2000

void *connection_handler(void *socket_desc);
long read_file(char **file, char **buffer, int *status_code);
int parse_header(char *header, char **file);
void get_mime(const char *file, char content_type[200]);

void *connection_handler(void *socket_desc)
{
	int sock = *(int*)socket_desc;

	char *requested_file;
	char *response_data = NULL;
	char response_version[50] = "HTTP/1.0 ",
		 response_status[200] = "200 OK",
	     response_content_type[200];
	long long response_content_length;

	char recv_buffer[RECV_BUFFER];
	if(recv(sock, recv_buffer, RECV_BUFFER, 0) == -1)
	{
		perror("recv");
		goto exit_func;
	}

	strncpy(response_status, "200 OK", sizeof(response_status));

	if(!parse_header(recv_buffer, &requested_file))
		strncpy(response_status, "400 Bad Request", sizeof(response_status));

	int status_code;
	if((response_content_length =
			read_file(&requested_file, &response_data, &status_code)) < 0 ||
			NULL == response_data)
		goto exit_func;

	if(200 != status_code)
		strncpy(response_status, "404 File Not Found",
				sizeof(response_status));

	get_mime(requested_file, response_content_type);

	size_t respLength = response_content_length+
			strlen("Content-Type: \r\n")+
			strlen(response_content_type)+
			strlen(response_version)+
			strlen(response_status)+
			strlen("\r\n\r\n\r\n")-2;

	char *message;
	if(!(message = malloc(respLength)))
	{
		perror("malloc");
		goto exit_func;
	}

	sprintf(message, "%s%s\r\nContent-Type: %s\r\n\r\n\r\n",response_version, response_status,
			response_content_type);
	/*
		This is using memcpy instead of sprintf since response_data may
		contain binary data and therefore not be null terminated.
	*/
	memcpy(message+respLength-response_content_length,
			response_data, response_content_length);  
	
	if(write(sock, message, respLength) == -1)
		perror("write");

	free(message);
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
	if ((socket_desc = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port);

	if(bind(socket_desc,(struct sockaddr*)&server, sizeof(server)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if(listen(socket_desc, 3) != 0)
	{
		perror("listen");
		exit(1);
	}

	const struct timeval sock_timeout={
		.tv_sec=SOCK_S_TIMEOUT,
		.tv_usec=SOCK_US_TIMEOUT};

	setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char*)&sock_timeout, sizeof(sock_timeout));

	c = sizeof(struct sockaddr_in);
	while((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)))
	{
		if(new_socket == -1) continue;
		pthread_t connection_thread;
		if(!(new_sock = malloc(sizeof(int*)))) return 1;
		*new_sock = new_socket;
		
		if(pthread_create(&connection_thread, NULL, connection_handler, (void*)new_sock) < 0)
		{
			perror("pthread_create");
			return 1;
		}
		
		pthread_join(connection_thread, NULL);
	}
	close(socket_desc);

	return 0;
}

int parse_header(char *header, char **file)
{
	if(!strtok(header, " ")        ||
	   !(*file = strtok(NULL, "")) ||
	   !(*file = strtok(*file, " ")))
	{
		*file = "/400.html";
		return 0;
	}

	if(**file == '\0' || (**file == '/' && (*file)[1] == '\0'))
		*file = "/index.html";

	return 1;
}

void get_mime(const char *file, char content_type[200])
{
	char fileExtension[strlen(file)+1];
	strcpy(fileExtension, file);
	fileExtension[strlen(file)] = '\0';

	if(strtok(fileExtension, ".") != NULL)
	{
		const char * ext = strtok(NULL, ".");
		if(ext == NULL)
			goto exit;

		for(size_t i = 0;i < sizeof(mimes)/sizeof(mimes[0])-1;i++)
			if(strcmp(mimes[i].ext, ext) == 0)
			{
				strncpy(content_type, mimes[i].type, sizeof(char[200])-1);
				content_type[sizeof(char[200])-1] = 0;
				return;
			}
	}

exit:
	strncpy(content_type, "application/octet-stream", sizeof(char[200])-1);
	content_type[sizeof(char[200])-1] = 0;
}

long read_file(char **file, char **buffer, int *status_code)
{
	if(strcmp(*file, "/404.html") != 0)
		*status_code = 200;

	FILE *fp;
	if(!(fp = fopen(*file, "rb")))
	{
		if(strcmp(*file, "/404.html") == 0)
			return -1;

		*status_code = 404;
		*file = "/404.html";
		return read_file(file, buffer, status_code);
	}

	fseek(fp, 0, SEEK_END);
	long length = ftell(fp);
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
