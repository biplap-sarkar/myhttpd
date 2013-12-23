/* 
 * All the consts and global variables of the application are declared here
 */
 
#ifndef CONSTS_H
#define CONSTS_H

#include	<semaphore.h>

#define	BUF_LEN	8192
#define DEFAULT_THREAD_COUNT 4
#define DEFAULT_ROOT_DIR "./"
#define DEFAULT_Q_TIME 60
#define DEFAULT_LOG_FILE "log.txt"
#define FCFS 21
#define SJF 22
#define DEBUG 31
#define DEFAULT_PORT 8080
#define HEAD 1
#define GET 2
#define HTTP1_0 10
#define HTTP1_1 11
#define INVALID_TERMINATION -11
#define INVALID_OR_NOT_SUPPORTED_REQUEST -12
#define INVALID_OR_NOT_SUPPORTED_VERSION -13
#define FILE_NOT_FOUND -15
#define INVALID_REQUEST_LINE -14
#define MESSAGE_HEADER_DELIMITER "\r\n"
#define HEADER_DELIMITER "\n"


extern char *logfile;
extern char *rootdir;
extern char *sch;
extern int sched;
extern int mode;

extern sem_t qmutex;
extern sem_t mutex;
extern sem_t isempty;
extern sem_t semrequest;
extern sem_t logmutex;
extern char *host;
extern char *port;
extern char *threadcount;
extern int initialwait;

extern const char const *APP_NAME ;
extern const char const *APP_VERSION ;
extern const char const *filenotfound ;
extern const char const *badrequest ;

#endif
