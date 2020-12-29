#include "socket.h"

void init_server(int *socket_desc, struct sockaddr_in *server, short port)
{
	if(socket_desc == NULL)
		exit(1);

	if(server == NULL)
		exit(1);

	*socket_desc = socket(AF_INET, SOCK_STREAM, 0);
	if (*socket_desc == -1)
	{
		perror("socket");
		exit(1);
	}

	server->sin_family = AF_INET;
	server->sin_addr.s_addr = INADDR_ANY;
	server->sin_port = htons(port);

	if(bind(*socket_desc,(struct sockaddr *)server, sizeof(*server)) < 0)
	{
		perror("bind");
		exit(1);
	}

	if(listen(*socket_desc, 3) != 0)
	{
		perror("listen");
		exit(1);
	}
}
