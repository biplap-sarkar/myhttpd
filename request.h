/*
 * Header file for declaring the datastructure used in the application
 */
#ifndef REQUEST
#define REQUEST
#include <time.h>
struct http_request{	// Data structure to contain information regarding an http request
	int type;			// Request type, (GET, HEAD etc)
	int version;		// Http version
	int client_soc;		// Client socket
	int size;			// Size of the resource requested
	int priority;		// Priority if the request in decreasin order. Normally equal to file size.
						// but equal to 0 in case of HEAD request or when resource refers to directory.
	char *file;			// Requested resource
	char *firstline;	// Request line of the http request
	int status;			// Status of the request (OK, Invalid request, File Not Found etc)
	struct tm *ptime;	// Timestamp of the time when request is picked up by a worker thread
	struct tm *qtime;	// Timestamp of the time which request is put into shared queue for scheduler
	char *ip;			// IP of the client
	int isdir;			// Flag for determining if requested resource is file or directory.
						// 1 if it is directory, 0 otherwise
	char *dirhtml;		// In case the requested resources is directory, and index.html
						// is not present in that directory, It keeps the directory listing in html format.
};

struct request{			// Data structure to contain information regarding a raw request
	int client_soc;		// Client socket
	struct tm * qtime;	// Timestamp of the time when request is put into queue for scheduler.
	char *ip;			// IP of the client.
};
#endif
