
static char svnid[] = "$Id: httpserver.c 1 2013-10-10 03:18:54Z biplap $";

#define	BUF_LEN	8192
#define DEFAULT_THREAD_COUNT 4
#define FIFO 21
#define SJF 22

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
#include    "request.h"
#include 	"http_request_queue.h"

char *progname;
char buf[BUF_LEN];

void usage();
int setup_client();
int setup_server();
int create_thread_pool(int);
void* process_request(void *);
int put_request_in_readyqueue(int client_soc,int type,int version, char *file, int size);

int s, sock, ch, server, done, bytes, aflg;
int soctype = SOCK_STREAM;
sem_t mutex;
sem_t isempty;
sem_t semrequest;
char *host = NULL;
char *port = NULL;
char *threadcount = NULL;
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
	int pid = fork();
	if(pid>0){
		exit(0);
	}
	while ((ch = getopt(argc, argv, "adsp:h:n:")) != -1)
		switch(ch) {
			case 'a':
				aflg++;		/* print address in output */
				break;
			case 'd':
				soctype = SOCK_DGRAM;
				break;
			case 's':
				server = 1;
				break;
			case 'p':
				port = optarg;
				break;
			case 'h':
				host = optarg;
				break;
			case 'n':
				threadcount = optarg;
				break;
			case '?':
			default:
				usage();
		}
	argc -= optind;
	if (argc != 0)
		usage();
	if (!server && (host == NULL || port == NULL))
		usage();
	if (server && host != NULL)
		usage();
	
	sem_init(&mutex,0,1);
	sem_init(&isempty,0,0);
	sem_init(&semrequest,0,0);
	/*
	 * Create thread pool.
	 */
	 if(threadcount == NULL)
		create_thread_pool(DEFAULT_THREAD_COUNT);
	 else
		create_thread_pool(atoi(threadcount));
	 create_scheduler_thread();

/*
 * Create socket on local host.
 */
	if ((s = socket(AF_INET, soctype, 0)) < 0) {
		perror("socket");
		exit(1);
	}
	if (!server)
		sock = setup_client();
	else
		sock = setup_server();
	while(1){
		printf("Started the server\n");
		client = accept_clients();
		read_request(client);
	}
	
/*
 * Set up select(2) on both socket and terminal, anything that comes
 * in on socket goes to terminal, anything that gets typed on terminal
 * goes out socket...
 */
 /* not needed for now */
 /*
	while (!done) {
		FD_ZERO(&ready);
		FD_SET(sock, &ready);
		FD_SET(fileno(stdin), &ready);
		if (select((sock + 1), &ready, 0, 0, 0) < 0) {
			perror("select");
			exit(1);
		}
		if (FD_ISSET(fileno(stdin), &ready)) {
			if ((bytes = read(fileno(stdin), buf, BUF_LEN)) <= 0)
				done++;
			send(sock, buf, bytes, 0);
		}
		msgsize = sizeof(msgfrom);
		if (FD_ISSET(sock, &ready)) {
			if ((bytes = recvfrom(sock, buf, BUF_LEN, 0, (struct sockaddr *)&msgfrom, &msgsize)) <= 0) {
				done++;
			} else if (aflg) {
				fromaddr.addr = ntohl(msgfrom.sin_addr.s_addr);
				fprintf(stderr, "%d.%d.%d.%d: ", 0xff & (unsigned int)fromaddr.bytes[0],
			    	0xff & (unsigned int)fromaddr.bytes[1],
			    	0xff & (unsigned int)fromaddr.bytes[2],
			    	0xff & (unsigned int)fromaddr.bytes[3]);
			}
			write(fileno(stdout), buf, bytes);
		}
	}*/
	return(0);
}

int read_request(int client_soc){
	memset(buf,0,BUF_LEN);
	recv(client_soc, buf, BUF_LEN, 0);
	queue_request(client_soc);
	//node = dequeue();
	//printf("%s\n",node->request->request);
}
int queue_request(int client_soc){
	struct request *newrequest = (struct request*)malloc(sizeof(struct request));
	newrequest->url = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(newrequest->url,0,BUF_LEN);
	strcpy(newrequest->url,buf);
	newrequest->client_soc = client_soc;
	struct request_node *newnode = (struct request_node*)malloc(sizeof(struct request_node));
	newnode->request = newrequest;
	enqueue_request(newnode);
	sem_post(&semrequest);
}
int put_request_in_readyqueue(int client_soc,int type,int version, char *file, int size){
	struct http_request *newrequest = (struct http_request *)malloc(sizeof(struct http_request));
	newrequest->file = file;
	newrequest->client_soc = client_soc;
	newrequest->size = size;
	newrequest->type = type;
	newrequest->version = version;
	struct http_request_node *node = (struct http_request_node *)malloc(sizeof(struct http_request_node));
	node->request = newrequest;
	enqueue(node);
	sem_post(&isempty);
}

int create_thread_pool(int count){
	int i;
	pthread_t tid[count];
	int err;
	for(i=0;i<count;i++){
		err = pthread_create(&(tid[i]),NULL,&process_http_request, NULL);
		if(err != 0)
			printf("\ncan't create thread :[%s]\n", strerror(err));
		else
			printf("\n thread number %d created\n",i);
	}
}

int create_scheduler_thread(){
	pthread_t tid;
	int err;
	err = pthread_create(&(tid[i]),NULL,&schedule_request, NULL);
	if(err != 0)
		printf("\ncan't create scheduler thread : [%s]\n", strerror(err));
	else
		printf("\n scheduler thread created\n");
}

void* schedule_request(void *arg){
	struct request_node *node;
	int result;
	//printf("worker thread %d\n",id);
	while(1){
		sem_wait(&semrequest);
		sem_wait(&mutex);
		node = dequeue_request();
		sem_post(&mutex);
		//printf("received request in process_request(): %s\n",node->request->request);
		result = process_http_request(node->request);
		printf("request handled successfully by thread %d with result %d\n",id,result);
		free(node->request->file);
		free(node->request);
		free(node);
		//printf("%s\n",node->request->request);
	}
	char *file = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(file, 0, BUF_LEN);
	int type;
	int version;
	int size;
	printf("parsing request %s : read_request()\n",buf);
	parse_http_request(buf,&type,&file,&version);
	size = get_file_size(file+1);
	printf("request with file %s, type %d, version %d, size %d : read_request()\n",file,type,version,size);
	put_request_in_readyqueue(client_soc,type,version,file,size);
}

void* process_http_request(void *arg){
	pthread_t id = pthread_self();
	struct http_request_node *node;
	int result;
	//printf("worker thread %d\n",id);
	while(1){
		sem_wait(&isempty);
		sem_wait(&mutex);
		node = dequeue();
		sem_post(&mutex);
		//printf("received request in process_request(): %s\n",node->request->request);
		result = process_http_request(node->request);
		printf("request handled successfully by thread %d with result %d\n",id,result);
		free(node->request->file);
		free(node->request);
		free(node);
		//printf("%s\n",node->request->request);
	}
}

/*
 * setup_client() - set up socket for the mode of soc running as a
 *		client connecting to a port on a remote machine.
 */

int setup_client() {

	struct hostent *hp, *gethostbyname();
	struct sockaddr_in serv;
	struct servent *se;

/*
 * Look up name of remote machine, getting its address.
 */
	if ((hp = gethostbyname(host)) == NULL) {
		fprintf(stderr, "%s: %s unknown host\n", progname, host);
		exit(1);
	}
/*
 * Set up the information needed for the socket to be bound to a socket on
 * a remote host.  Needs address family to use, the address of the remote
 * host (obtained above), and the port on the remote host to connect to.
 */
	serv.sin_family = AF_INET;
	memcpy(&serv.sin_addr, hp->h_addr, hp->h_length);
	if (isdigit(*port))
		serv.sin_port = htons(atoi(port));
	else {
		if ((se = getservbyname(port, (char *)NULL)) < (struct servent *) 0) {
			perror(port);
			exit(1);
		}
		serv.sin_port = se->s_port;
	}
/*
 * Try to connect the sockets...
 */
	if (connect(s, (struct sockaddr *) &serv, sizeof(serv)) < 0) {
		perror("connect");
		exit(1);
	} else
		fprintf(stderr, "Connected...\n");
	return(s);
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
		serv.sin_port = htons(0);
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

int accept_clients(){
	int newsock;
	struct sockaddr_in serv, remote;
	int len = sizeof(remote);
	if (getsockname(s, (struct sockaddr *) &remote, &len) < 0) {
		perror("getsockname");
		exit(1);
	}
	if (soctype == SOCK_STREAM) {
		fprintf(stderr, "Entering accept() waiting for connection.\n");
		newsock = accept(s, (struct sockaddr *) &remote, &len);
	}
	return(newsock);
}

/*
 * usage - print usage string and exit
 */

void
usage()
{
	fprintf(stderr, "usage: %s -h host -p port\n", progname);
	fprintf(stderr, "usage: %s -s [-p port]\n", progname);
	exit(1);
}
