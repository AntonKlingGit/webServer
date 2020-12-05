CC		= gcc
CFLAGS	= -Wall -Wextra -Werror -pedantic -std=c99
LIBS	= -lpthread -lbsd

http_server: main.c socket.c http.c
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
