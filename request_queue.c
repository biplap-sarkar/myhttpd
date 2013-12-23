/*
 * Implements queue for keeping connection requests. 
 */
#include	<stdlib.h>
#include 	"request_queue.h"

struct request_node *req_front = NULL;
struct request_node *req_rear = NULL;
void enqueue_request(struct request_node * node){
	if(req_rear == NULL){
		node->next = NULL;
		req_rear = node;
		req_front = node;
	}
	else{
		req_rear->next = node;
		req_rear = node;
	}
}

struct request_node * dequeue_request(){
	struct request_node *node = req_front;
	if(req_front == req_rear){
		node = req_front;
		req_front = req_rear = NULL;
	}
	else{
		node = req_front;
		req_front = req_front->next;
	}
	return node;
}



