#define DEFAULT_PORT 1337
#define WEBSITE_ROOT "./site"

#define SOCK_S_TIMEOUT 5 // Seconds
#define SOCK_US_TIMEOUT 0 // Microseconds

/* mime-types */
static const struct {
 	char *ext;
	char *type;
}mimes[] = {
	{ "xml",   "application/xml; charset=utf-8" },
	{ "xhtml", "application/xhtml+xml; charset=utf-8" },
	{ "html",  "text/html; charset=utf-8" },
	{ "htm",   "text/html; charset=utf-8" },
	{ "css",   "text/css; charset=utf-8" },
	{ "txt",   "text/plain; charset=utf-8" },
	{ "md",    "text/plain; charset=utf-8" },
	{ "c",     "text/plain; charset=utf-8" },
	{ "h",     "text/plain; charset=utf-8" },
	{ "gz",    "application/x-gtar" },
	{ "tar",   "application/tar" },
	{ "pdf",   "application/x-pdf" },
	{ "png",   "image/png" },
	{ "gif",   "image/gif" },
	{ "jpeg",  "image/jpg" },
	{ "jpg",   "image/jpg" },
	{ "iso",   "application/x-iso9660-image" },
	{ "webp",  "image/webp" },
	{ "svg",   "image/svg+xml; charset=utf-8" },
	{ "flac",  "audio/flac" },
	{ "mp3",   "audio/mpeg" },
	{ "ogg",   "audio/ogg" },
	{ "mp4",   "video/mp4" },
	{ "ogv",   "video/ogg" },
	{ "webm",  "video/webm" },
};
