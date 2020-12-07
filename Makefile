CC		= gcc
CFLAGS	= -Wall -Wextra -Werror -pedantic -std=c99 -O3
LIBS	= -lpthread -lbsd

http_server: main.c socket.c http.c
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

clean:
	rm http_server
