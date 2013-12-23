static char svnid[] = "$Id: myhttpd.c 1 2013-10-10 03:18:54Z biplap $";

/* 
 * This is the main file where sheduler thread, worker threads are created and executed and requests are 
 * queued in shared synchronized datastructures and are processed
 */
 
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netdb.h>
#include	<netinet/in.h>
#include	<inttypes.h>
#include	<semaphore.h>
#include	<time.h>
#include 	<arpa/inet.h>
#include	"consts.h"
#include    "request.h"
#include	"request_queue.h"
#include 	"http_request_queue.h"
#include 	"http_request_handler.h"
#include	"request_heap.h"


const char const *APP_NAME = "myhttpd";
const char const *APP_VERSION = "1.0.0-50097584";

char *progname;



void usage();
int setup_client();
int setup_server();
int create_thread_pool(int);
void* process_request(void *);
int put_request_in_readyqueue(struct http_request *req);

char *logfile = NULL;
char *rootdir = NULL;
char *sch = NULL;
int sched;
int mode;

int s, sock, ch, server, done, bytes, aflg;
int soctype = SOCK_STREAM;
sem_t qmutex;
sem_t mutex;
sem_t isempty;
sem_t semrequest;
sem_t logmutex;
char *host = NULL;
char *port = NULL;
char *threadcount = NULL;
int initialwait = DEFAULT_Q_TIME;
extern char *optarg;
extern int optind;

int
main(int argc,char *argv[])
{
	fd_set ready;
	struct sockaddr_in msgfrom;
	int msgsize;
	int client;
	union {
		uint32_t addr;
		char bytes[4];
	} fromaddr;

	if ((progname = rindex(argv[0], '/')) == NULL)
		progname = argv[0];
	else
		progname++;
	
	while ((ch = getopt(argc, argv, "dhl:p:r:t:n:s:")) != -1)
		switch(ch) {
			case 'd':
				mode = DEBUG;
				break;
			case 's':
				sch = optarg;
				if((strcmp(sch,"FCFS")==0) || (strcmp(sch,"fcfs")==0))
					sched = FCFS;
				else if((strcmp(sch,"SJF")==0) || (strcmp(sch,"sjf")==0))
					sched = SJF;
				else{
					printf("Warning: Invalid scheduling option, switching to FCFS\n");
					sched = FCFS;
				}
				break;
			case 'p':
				port = optarg;
				break;
			case 'h':
				usage();
				break;
			case 'n':
				threadcount = optarg;
				break;
			case 'l':
				logfile = optarg;
				break;
			case 'r':
				rootdir = optarg;
				break;
			case 't':
				initialwait = atoi(optarg);
				break;
			case '?':
			default:
				usage();
		}
	
	if(mode != DEBUG){
		int pid = fork();
		if(pid>0){
			exit(0);
		}
	}
	else{
		printf("Running in debug mode\n");
	}
		
	if(sched != FCFS && sched != SJF)
		sched = FCFS;
	sem_init(&mutex,0,1);
	sem_init(&qmutex,0,1);
	sem_init(&isempty,0,0);
	sem_init(&semrequest,0,0);
	sem_init(&logmutex,0,1);
	
	set_working_directory();
	if(logfile == NULL)
		logfile=DEFAULT_LOG_FILE;
	
	/*
	 * Create thread pool.
	 */
	 if(mode != DEBUG){
		 create_scheduler_thread();
		if(threadcount == NULL)
			create_thread_pool(DEFAULT_THREAD_COUNT);
		else
			create_thread_pool(atoi(threadcount));
	}

/*
 * Create socket on local host.
 */
	if ((s = socket(AF_INET, soctype, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	
	sock = setup_server();
	printf("Started the server\n");
	struct sockaddr_in remote;
	char ip[50];
	while(1){
		client = accept_clients(&remote);
		memset(ip,0,50);
		strcpy(ip,inet_ntoa(remote.sin_addr));
		if(mode != DEBUG)
			queue_request(client,ip);
		else{
			debug_process(client,ip);
		}
	}
	return(0);
}

/*
 * Receives requests from client socket
 */
/*
int receive_request(int soc){
	printf("main()\n");
	int brecvd;
	memset(buf,0,BUF_LEN);
	int len;
	char tmpbuf[BUF_LEN];
	do{
		memset(tmpbuf,0,BUF_LEN);
		brecvd = recv(soc, tmpbuf, BUF_LEN, 0);
		
		if(brecvd <= 0)
			break;
		strcat(buf,tmpbuf);
		len = strlen(buf);
		printf("last 4 chars %x %x %x %x received:recieve_request()\n",buf[len-4],buf[len-3],buf[len-2],buf[len-1]);
	}while(!(((buf[len-4])=='\r' && buf[len-3]=='\n' && buf[len-2]=='\r' && buf[len-1]=='\n')||
		buf[len-2]=='\n' && buf[len-1]=='\n'));
	//queue_request(soc);
	return 0;
}*/

/*
 * Insert a new request in the request queue
 */
int queue_request(int soc, char *ip){
	struct request_node * newnode;
	struct request * newrequest;
	newnode = (struct request_node *)malloc(sizeof(struct request_node));
	time_t now = time(0);
	
	
	newrequest = (struct request *)malloc(sizeof(struct request));
	newrequest->ip = (char *)malloc(sizeof(char)*50);
	memset(newrequest->ip,0,50);
	strcpy(newrequest->ip,ip);
	newrequest->client_soc = soc;
	newrequest->qtime = (struct tm*)malloc(sizeof(struct tm));
	localtime_r(&now,newrequest->qtime); 
	strcpy(newrequest->ip,ip);
	
	newnode->req = newrequest;
	sem_wait(&qmutex);
	enqueue_request(newnode);
	sem_post(&qmutex);
	sem_post(&semrequest);
	return 0;
}

debug_process(int client,char *ip){
	char buf[BUF_LEN];
	struct request * newrequest;
	struct http_request *http_req;
	int recvd=0;
	time_t now = time(0);
	char *req_msg = (char *)malloc(sizeof(char)*BUF_LEN);
	
	newrequest = (struct request *)malloc(sizeof(struct request));
	newrequest->ip = (char *)malloc(sizeof(char)*50);
	memset(newrequest->ip,0,50);
	strcpy(newrequest->ip,ip);
	newrequest->client_soc = client;
	newrequest->qtime = (struct tm *)malloc(sizeof(struct tm));
	localtime_r(&now,newrequest->qtime);
	strcpy(newrequest->ip,ip);
	
	memset(buf,0,BUF_LEN);
	recvd = recv(newrequest->client_soc, buf, BUF_LEN, 0);
	if(recvd==0){
		free(req_msg);
		free(newrequest->ip);
		free(newrequest->qtime);
		free(newrequest);
		return;
	}
	strcpy(req_msg,buf);
	
	
	http_req = parse_http_request(newrequest,req_msg);
	process_http_request(http_req);
	free(http_req->ip);
	free(http_req->firstline);
	free(http_req->file);
	free(http_req->qtime);
	free(http_req->ptime);
	free(http_req);
	free(req_msg);
	free(newrequest->ip);
	free(newrequest);
	
}

/*
 * Parses requests from request queue for http requests
 */
void * parse_request(void * args){
	struct request_node *node;
	struct http_request *http_req;
	char buf[BUF_LEN];
	int bytes;
	while(1){
		sem_wait(&semrequest);
		sem_wait(&qmutex);
		node = dequeue_request();
		sem_post(&qmutex);
		
		memset(buf,0,BUF_LEN);
		bytes = recv(node->req->client_soc, buf, BUF_LEN, 0);
		if(bytes > 0){
		
			char *req_msg = (char *)malloc(sizeof(char)*BUF_LEN);
			strncpy(req_msg,buf,BUF_LEN);
	
			http_req = parse_http_request(node->req,req_msg);
			put_request_in_readyqueue(http_req);
		
			free(req_msg);
		}
		free(node->req->ip);
		free(node->req);
		free(node);
	}
}

/*
 * Puts an http request in ready queue
 */
int put_request_in_readyqueue(struct http_request *req){
	struct http_request_node *http_req_node;
	sem_wait(&mutex);
	switch(sched){
		case FCFS:
			http_req_node = (struct http_request_node *)malloc(sizeof(struct http_request_node));
			http_req_node->req=req;
			enqueue(http_req_node);
			break;
		case SJF:
			insert_into_heap(req);
			break;
	}
	sem_post(&mutex);
	sem_post(&isempty);
}

int set_working_directory(){
	if(rootdir==NULL)
		rootdir = DEFAULT_ROOT_DIR;
	return chdir(rootdir);
}

/*
 * Process a request from ready queue
 */
void* process_request(void *args){
	sleep(initialwait);
	struct http_request *http_req;
	struct http_request_node *http_req_node;
	while(1){
		sem_wait(&isempty);
		sem_wait(&mutex);
		
		switch(sched){
			case FCFS:
				http_req_node = dequeue();
				http_req = http_req_node->req;
				free(http_req_node);
				break;
			case SJF:
				http_req = extract_shortest();
				break;
		}
		sem_post(&mutex);
		process_http_request(http_req);
		free(http_req->file);
		free(http_req->firstline);
		free(http_req->ip);
		free(http_req->ptime);
		free(http_req->qtime);
		free(http_req);
	}
}

/*
 * Creates thread pool for worker threads
 */
int create_thread_pool(int count){
	int i;
	pthread_t tid[count];
	int err;
	for(i=0;i<count;i++){
		err = pthread_create(&(tid[i]),NULL,&process_request, NULL);
		if(err != 0)
			printf("\ncan't create thread :[%s]\n", strerror(err));
		//else
		//	printf("\n thread number %d created\n",i);
	}
}

/*
 * Creates thread for putting requests in ready queue
 */
int create_scheduler_thread(){
	pthread_t tid;
	int err;
	err = pthread_create(&tid,NULL,&parse_request, NULL);
	if(err != 0)
		printf("\ncan't create scheduler thread : [%s]\n", strerror(err));
	//else
	//	printf("\n scheduler thread created\n");
}




/*
 * setup_server() - set up socket for mode of soc running as a server.
 */

int setup_server() {
	struct sockaddr_in serv, remote;
	struct servent *se;
	int newsock, len;

	len = sizeof(remote);
	memset((void *)&serv, 0, sizeof(serv));
	serv.sin_family = AF_INET;
	if (port == NULL)
		serv.sin_port = htons(DEFAULT_PORT);
	else if (isdigit(*port))
		serv.sin_port = htons(atoi(port));
	else {
		if ((se = getservbyname(port, (char *)NULL)) < (struct servent *) 0) {
			perror(port);
			exit(1);
		}
		serv.sin_port = se->s_port;
	}
	if (bind(s, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
		perror("bind");
		exit(1);
	}
	if (getsockname(s, (struct sockaddr *) &remote, &len) < 0) {
		perror("getsockname");
		exit(1);
	}
	fprintf(stderr, "Port number is %d\n", ntohs(remote.sin_port));
	listen(s, 1);
}

int accept_clients(struct sockaddr_in *remote){
	int newsock;
	struct sockaddr_in serv;
	int len = sizeof(remote);
	if (getsockname(s, (struct sockaddr *) remote, &len) < 0) {
		perror("getsockname");
		exit(1);
	}
	if (soctype == SOCK_STREAM) {
		//fprintf(stderr, "Entering accept() waiting for connection.\n");
		newsock = accept(s, (struct sockaddr *) remote, &len);
	}
	return(newsock);
}


/*
 * usage - print usage string and exit
 */

void
usage()
{
	printf("%s version %s\n", APP_NAME, APP_VERSION);
	printf("usage: %s  [−d] [−h] [−l file] [−p port] [−r dir] [−t time] [−n threadnum] [−s sched]\n", APP_NAME);
	printf("Arguments:\n");
	printf("−d : Enter debugging mode.\n");
	printf("−h : Print a usage summary with all options and exit.\n");
	printf("−l file : Log all requests to the given file. Default is log.txt in application root directory.\n");
	printf("−p port : Listen on the given port. Default is 8080\n", APP_NAME);
	printf("−r dir : Set the root directory for the http server to dir. Default is current execution directory.\n");
	printf("−t time : Set the queuing time to time seconds. Default is 60 seconds.\n");
	printf("−n threadnum : Set number of threads waiting ready in the execution thread pool to threadnum.Default is 4.\n");
	printf("−s sched : Set the scheduling policy. It can be either FCFS or SJF. Default is FCFS\n");
	exit(0);
}

