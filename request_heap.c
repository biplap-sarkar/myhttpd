/*
 * Implements Min Heap for keeping http requests during SJF
 */
#include <stdio.h>
#include <stdlib.h>
#include "request.h"
#include "request_heap.h"


int heapsize = 0;
struct heap_node *heap = NULL;

int insert_into_heap(struct http_request *req){
	if(heapsize%BUF_LEN == 0)
		heap = (struct heap_node *)realloc(heap, sizeof(struct heap_node)*(heapsize+BUF_LEN));
	int i = heapsize;
	heapsize = heapsize + 1;
	while(i>0 && heap[PARENT(i)].req->priority > req->priority){
		heap[i].req = heap[PARENT(i)].req;
		i = PARENT(i);
	} 
	heap[i].req = req;
	return 0;
}

int min_heapify(int index){
	int smallest;
	struct http_request *temp;
	if(LEFT_CHILD(index) < heapsize && heap[LEFT_CHILD(index)].req->priority < heap[index].req->priority)
		smallest = LEFT_CHILD(index);
	else
		smallest = index;
	if(RIGHT_CHILD(index) < heapsize && heap[RIGHT_CHILD(index)].req->priority < heap[smallest].req->priority)
		smallest = RIGHT_CHILD(index);
	if(smallest != index){
		temp = heap[index].req;
		heap[index].req = heap[smallest].req;
		heap[smallest].req = temp;
		min_heapify(smallest);
	}
	return 0;
}

struct http_request * extract_shortest(){
	struct http_request *shortest = heap[0].req;
	heapsize = heapsize - 1;
	heap[0]=heap[heapsize];
	min_heapify(0);
	return shortest;
}
