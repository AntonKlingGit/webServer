#include "socket.h"

void startServer(int *socket_desc, struct sockaddr_in *server, short port)
{
	if(socket_desc == NULL)
	{
		perror("socket_desc = NULL");
		exit(1);
	}

	if(server == NULL)
	{
		perror("server = NULL");
		exit(1);
	}

	*socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (*socket_desc == -1)
	{
		perror("Unable to create socket.");
		exit(1);
	}

	server->sin_family = AF_INET;
	server->sin_addr.s_addr = INADDR_ANY;
	server->sin_port = htons(port);

	if(bind(*socket_desc,(struct sockaddr *)server, sizeof(*server)) < 0)
	{
		perror("Failed to assign address to socket.");
		exit(1);
	}

	if(listen(*socket_desc, 3) != 0)
	{
		perror("listen() error.");
		exit(1);
	}
}
