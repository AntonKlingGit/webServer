#ifndef HTTP_H
#define HTTP_H
#include <string.h>

//Currently the web server only cares about the method and the file that is requested.
struct http_header
{
	char* method, *file;
};
struct http_response
{
	char header[100];
	char content_type[200];
	long long content_length; 
	char* data;
};

struct http_header parse_header(char* header);
#endif
