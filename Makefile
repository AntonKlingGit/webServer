CC		= gcc
CFLAGS	= -Wall -Wextra -Werror -pedantic
LIBS	= -lpthread

http_server: main.c socket.c http.c
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@
