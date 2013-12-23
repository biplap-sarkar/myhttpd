/*
 * Header for http request queue
 */
#ifndef HTTP_REQUEST_NODE_INCLUDED
#define HTTP_REQUEST_NODE_INCLUDED
struct http_request_node{
	struct http_request *req;
	struct http_request_node *next;
};
extern struct http_request_node *front;
extern struct http_request_node *rear;
void enqueue(struct http_request_node *);
struct http_request_node * dequeue();
#endif 
