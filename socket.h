#ifndef SOCKET_H
#define SOCKET_H
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

void init_server(int *socket_desc, struct sockaddr_in *server, short port);
#endif
