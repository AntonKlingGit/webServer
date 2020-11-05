#include "http.h"

struct http_header parse_header(char* header)
{
	struct http_header parsed_header;
	if((parsed_header.method = strtok(header, " ")) != NULL)
	{
		parsed_header.file = strtok(NULL, " ");
	}	

	return parsed_header;
}
