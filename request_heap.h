/*
 * Header for http heap datastructure
 */
#ifndef REQUEST_HEAP
#define REQUEST_HEAP
#include "request.h"
#include "consts.h"

#define LEFT_CHILD(i) (2*(i)+1)
#define RIGHT_CHILD(i) (2*(i)+2)
#define PARENT(i) (i/2)
struct heap_node{
	struct http_request *req;
};
extern int heapsize;
int insert_into_heap(struct http_request *req);
int min_heapify(int index);
struct http_request * extract_shortest();
#endif
