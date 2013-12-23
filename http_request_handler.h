/*
 * Header file for http request handler
 */
#ifndef HTTP_REQUEST_HANDLER
#define HTTP_REQUEST_HANDLER

#include "request.h"

struct http_request * parse_http_request(struct request *req,char *);
int process_http_request(struct http_request *);
int process_http_get_request(int soc, int version, char *url, int size, int isdir, char *dirhtml);
int process_http_head_request(int soc, int version, char *url, int size, int isdir, char *dirhtml);
int send_http_response_header(int soc, int version, char *url, int size, int isdir, char *dirhtml);
int send_http_response_body(int soc, int version, char *url, int size, int isdir, char *dirhtml);
#endif
