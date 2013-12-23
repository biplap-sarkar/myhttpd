/*
 * Implements FCFS queue for http request
 */
#include	<stdlib.h>
#include 	"http_request_queue.h"

struct http_request_node *front = NULL;
struct http_request_node *rear = NULL;
void enqueue(struct http_request_node * node){
	if(rear == NULL){
		node->next = NULL;
		rear = node;
		front = node;
	}
	else{
		rear->next = node;
		rear = node;
	}
}

struct http_request_node * dequeue(){
	struct http_request_node *node = front;
	if(front == rear){
		node = front;
		front = rear = NULL;
	}
	else{
		node = front;
		front = front->next;
	}
	return node;
}
