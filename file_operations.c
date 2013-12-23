/* 
 * Contains functions for file operations
 */

#include <sys/stat.h>
#include <time.h>
#include <fcntl.h> 
#include <semaphore.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include "consts.h"
#include "file_operations.h"
#include "http_request_handler.h"


int get_file_size(char *filename){
	int size = -1;
	struct stat st;
	int result = stat(filename, &st);
	if(result==0)
		size = st.st_size;
	return size;
}

struct tm *last_mod_timestamp(char *filename){
	struct stat st;
	struct tm *timestamp;
	int result = stat(filename, &st);
	if(result==0){
		timestamp = gmtime(&st.st_mtime);
	}
	return timestamp;
}
void format_mod_timestamp(char *filename, char *f_timestamp){
	char * format = "%a, %d %b %Y %H:%M:%S %Z";
	struct tm *timestamp = last_mod_timestamp(filename);
	strftime(f_timestamp, BUF_LEN, format, timestamp);
}

void format_current_timestamp(char *c_timestamp){
	char * format = "%a, %d %b %Y %H:%M:%S %Z";
	time_t now = time(0);
	struct tm timestamp;
	gmtime_r(&now,&timestamp);
	strftime(c_timestamp, BUF_LEN, format, &timestamp);
}

int is_dir(char* path) {
    struct stat buf;
    stat(path, &buf);
    return S_ISDIR(buf.st_mode);
}
void write_log(struct http_request *req){
	if(mode == DEBUG)
		write_log_to_stdout(req);
	else
		write_log_to_file(req);
}
void write_log_to_stdout(struct http_request *req){
	char * format = "%d/%b/%Y:%H:%M:%S %z";
	char qtime[100];
	char ptime[100];
	int status;
	switch(req->status){
		case 400:
			req->size = strlen(filenotfound);
			break;
		case 404:
			req->size = strlen(badrequest);
			break;
	}
	strftime(qtime, 100, format, req->qtime);
	strftime(ptime, 100, format, req->ptime);
	printf("%s [%s] [%s] \"%s\" %d %d\n",req->ip,qtime,ptime,req->firstline,req->status,req->size);
}
void write_log_to_file(struct http_request *req){
	char * format = "%d/%b/%Y:%H:%M:%S %z";
	char *buf = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(buf,0,BUF_LEN);
	char qtime[100];
	char ptime[100];
	int status;
	switch(req->status){
		case 400:
			req->size = strlen(filenotfound);
			break;
		case 404:
			req->size = strlen(badrequest);
			break;
	}
	strftime(qtime, 100, format, req->qtime);
	strftime(ptime, 100, format, req->ptime);
	sprintf(buf,"%s [%s] [%s] \"%s\" %d %d\n",req->ip,qtime,ptime,req->firstline,req->status,req->size);
	wait(&logmutex);
	int fd = open(logfile, O_RDWR | (O_APPEND |O_CREAT) ,0777);
	write(fd,buf,strlen(buf));
	close(fd);
	signal(&logmutex);
	free(buf);
}

char * get_dirlist_html(char *dir){
	struct dirent **namelist;
	int i,n;
	char *dirhtml = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(dirhtml,0,BUF_LEN);
	char *modtime = (char *)malloc(sizeof(char)*100);
	char absfile[256];
	char *size = (char *)malloc(sizeof(char)*20);
	strcpy(dirhtml,"<html><head><title>Index of ");
	strcat(dirhtml,dir);
	strcat(dirhtml,"</title></head><body><table><tr><th>File</th><th>Last Modified</th><th>Size</th></tr>");
    n = scandir(dir,&namelist, 0, alphasort);
    if (n < 0)
        perror("scandir");
    else {
        for (i = 0; i < n; i++) {
			//if((strcmp(namelist[i]->d_name,".")==0) || (strcmp(namelist[i]->d_name,"..")==0))
			if(*(namelist[i]->d_name)=='.'){
				free(namelist[i]);
				continue;
			}
			memset(modtime,0,100);
			memset(size,0,20);
			strcat(dirhtml,"<tr><td>");
			//int len = strlen(DEFAULT_ROOT_DIR);
			//strcat(dirhtml,dir+len);
			strcat(dirhtml,namelist[i]->d_name);
			strcat(dirhtml,"</td><td>");
			strcpy(absfile,dir);
			strcat(absfile,namelist[i]->d_name);
			format_mod_timestamp(absfile, modtime);
			strcat(dirhtml,modtime);
			strcat(dirhtml,"</td><td>");
			int fsize = get_file_size(absfile);
			sprintf(size,"%d",fsize);
			strcat(dirhtml,size);
			strcat(dirhtml,"</td><tr>");
            //printf("%s %d\n", namelist[i]->d_name,get_file_size(namelist[i]->d_name));
            free(namelist[i]);
            }
        }
    strcat(dirhtml,"</table></body></html>");
    free(size);
    free(modtime);
    free(namelist);
    return dirhtml;
}

char * get_mime_type(char *file){
	char *ext = strrchr(file,'.');
	if(ext==NULL)
		return "text/html";
	ext = ext + 1;
	if(ext==NULL)
		return "text/plain";
	else if(strcmp(ext,"txt")==0)
		return "text/plain";
	else if(strcmp(ext,"c")==0)
		return "text/x-c";
	else if(strcmp(ext,"css")==0)
		return "text/css";
	else if(strcmp(ext,"js")==0)
		return "application/javascript";
	else if(strcmp(ext,"html")==0)
		return "text/html";
	else if(strcmp(ext,"gif")==0)
		return "image/gif";
	else if(strcmp(ext,"GIF")==0)
		return "image/gif";
	else if(strcmp(ext,"jpeg")==0)
		return "image/jpeg";
	else if(strcmp(ext,"JPEG")==0)
		return "image/jpeg";
	else if(strcmp(ext,"JPG")==0)
		return "image/jpeg";
	else if(strcmp(ext,"jpg")==0)
		return "image/jpeg";
	else if(strcmp(ext,"bmp")==0)
		return "image/bmp";
}
