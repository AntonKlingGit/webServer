CC		= gcc
CFLAGS	= -Wall -Wextra -Wpedantic -Werror -std=c99 -O3 -g
LIBS	= -pthread

http_server: main.c
	$(CC) $(CFLAGS) $(LIBS) $^ -o $@

clean:
	rm http_server
