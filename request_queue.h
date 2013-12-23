/*
 * Header for request queue
 */
#ifndef REQUEST_NODE_INCLUDED
#define REQUEST_NODE_INCLUDED
struct request_node{
	struct request *req;
	struct request_node *next;
};
extern struct request_node *req_front;
extern struct request_node *req_rear;
void enqueue_request(struct request_node *);
struct request_node * dequeue_request();
#endif 

