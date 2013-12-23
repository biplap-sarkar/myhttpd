/*
 * This file contains functions handling http protocol details
 */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>
#include "consts.h"
#include "file_operations.h"
#include "http_request_handler.h"


const char const *filenotfound = "<html><head><body><h1>File not found</h1><body></head></html>";
const char const *badrequest = "<html><head><body><h1>Bad request</h1><body></head></html>";

struct http_request * parse_http_request(struct request *req, char *req_msg){
	struct http_request *http_req = (struct http_request *)malloc(sizeof(struct http_request));
	http_req->file = (char *)malloc(sizeof(char)*BUF_LEN);
	http_req->client_soc = req->client_soc;
	http_req->type = INVALID_REQUEST_LINE;
	http_req->status = INVALID_REQUEST_LINE;
	http_req->qtime = req->qtime;
	http_req->ip = (char *)malloc(sizeof(char)*50);
	http_req->firstline = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(http_req->ip,0,50);
	memset(http_req->firstline,0,BUF_LEN);
	strcpy(http_req->ip,req->ip);
	
	char *header = strtok(req_msg,MESSAGE_HEADER_DELIMITER);
	char *req_line = strtok(header,HEADER_DELIMITER);
	if(req_line==NULL)
		req_line="";
	if(req_line != NULL)
		strcpy(http_req->firstline,req_line);
	char *req_type = strtok(req_line," ");
	if(req_type == NULL)
		req_type = "";
	if(strcmp(req_type,"GET")==0){
		http_req->type = GET;
		http_req->status = GET;
	}
	else if(strcmp(req_type,"HEAD") == 0){
		http_req->type = HEAD;
		http_req->status = HEAD;
	}
		
	char *req_url = strtok(NULL," ");
	if(req_url == NULL)
		req_url = "";
	char *req_ver = strtok(NULL," ");
	if(req_ver == NULL)
		req_ver = "";
	
	if(strcmp(req_ver,"HTTP/1.0")==0)
		http_req->version = HTTP1_0;
	else if(strcmp(req_ver,"HTTP/1.1")==0)
		http_req->version = HTTP1_1;
	else
		http_req->version = INVALID_OR_NOT_SUPPORTED_VERSION;
	
	memset(http_req->file,0,BUF_LEN);
	strcpy(http_req->file, req_url+1);
	
	char tmpfile[256];
	char tmpdir[256];
	int len;
	int isize;
	char *tild = strchr(req_url,'~');
	if(tild != NULL){
		char term[256];
		char home[256];
		struct passwd *uinfo;
		char *termptr = strchr(tild,'/');
		if(termptr == NULL)
			strcpy(term,"/");
		else
			strcpy(term,termptr);
		char *username = strtok(tild+1,"/");
		uinfo = getpwnam(username);
		if(uinfo == NULL){
			strcpy(tmpdir,"/home/");
			strcat(tmpdir,username);
			strcat(tmpdir,"/myhttpd/");
		}
		else{
			strcpy(home,uinfo->pw_dir);
			strcpy(tmpdir,home);
			strcat(tmpdir,"/myhttpd/");
		}
		//chdir(tmpdir);
		strcpy(http_req->file,tmpdir);
		if(term[0]=='/')
			strcat(http_req->file,term+1);
		else
			strcat(http_req->file,term);
	}
	if(strlen(http_req->file)==0)
		strcpy(http_req->file,"./");
	int size = get_file_size(http_req->file);
	http_req->isdir=0;
	if(size > 0){
		http_req->size = size;
		http_req->priority = size;
		if(is_dir(http_req->file)){
			strcpy(tmpfile,http_req->file);
			len = strlen(tmpfile);
			if(tmpfile[len-1]!='/')
				strcat(http_req->file,"/");
			strcpy(tmpfile,http_req->file);
			strcat(tmpfile,"index.html");
			isize = get_file_size(tmpfile);
			if(isize>0){
				strcpy(http_req->file,tmpfile);
				http_req->size = isize;
				http_req->priority = isize;
			}
			else{
				http_req->dirhtml = get_dirlist_html(http_req->file);
				http_req->size = strlen(http_req->dirhtml);
				http_req->priority = 0;
				http_req->isdir = 1;
			}
		}
	}
	else{
		http_req->size = strlen(filenotfound);
		http_req->priority = http_req->size;
		http_req->status = FILE_NOT_FOUND;
	}
	return http_req;
}

int process_http_request(struct http_request *req){
	time_t now = time(0);
	req->ptime = (struct tm*)malloc(sizeof(struct tm));
	localtime_r(&now,req->ptime);
	switch(req->status){
		case GET:{
			process_http_get_request(req->client_soc,req->version,req->file, req->size, req->isdir, req->dirhtml);
			req->status = 200;
			break;
		}
		case HEAD:{
			process_http_head_request(req->client_soc,req->version,req->file, req->size, req->isdir, req->dirhtml);
			req->status = 200;
			break;
		}
		case INVALID_OR_NOT_SUPPORTED_REQUEST:{
			req->status = 400;
			bad_request(req->client_soc);
			break;
		}
		case INVALID_REQUEST_LINE:{
			req->status = 400;
			bad_request(req->client_soc);
			break;
		}
		case FILE_NOT_FOUND:{
			req->status = 404;
			file_not_found(req->client_soc,req->type);
			break;
		}
		default:{
			req->status = 400;
			bad_request(req->client_soc);
			break;
		}
	}
	
	write_log(req);
	if(req->isdir)
		free(req->dirhtml);
	return 0;
}
int bad_request(int soc){
	int len = strlen(badrequest);
	char content_len[20];
	char current_time[256];
	format_current_timestamp(current_time);
	sprintf(content_len,"%d\n",len);
	char *response = (char *)malloc(sizeof(char *)*BUF_LEN);
	memset(response,0,BUF_LEN);
	strcat(response,"HTTP/1.0 400 Bad Request\n");
	strcat(response,"Date: ");
	strcat(response,current_time);
	strcat(response,"\n");
	strcat(response,"Content-Type: text/html\nContent-Length:");
	strcat(response,content_len);
	strcat(response,"Server: ");
	strcat(response,APP_NAME);
	strcat(response," ");
	strcat(response,APP_VERSION);
	strcat(response,"\n");
	strcat(response,"Connection: close\n\r\n");
	strcat(response,badrequest);
	send(soc,response,strlen(response),0);
	free(response);
	close(soc);
	return 0;
}

int file_not_found(int soc, int type){
	int len = strlen(filenotfound);
	char content_len[20];
	char current_time[256];
	format_current_timestamp(current_time);
	sprintf(content_len,"%d\n",len);
	char *response = (char *)malloc(sizeof(char *)*BUF_LEN);
	memset(response,0,BUF_LEN);
	strcat(response,"HTTP/1.0 404 Not Found\n");
	strcat(response,"Date: ");
	strcat(response,current_time);
	strcat(response,"\n");
	strcat(response,"Content-Type: text/html\nContent-Length:");
	strcat(response,"Server: ");
	strcat(response,APP_NAME);
	strcat(response," ");
	strcat(response,APP_VERSION);
	strcat(response,"\n");
	strcat(response,"Connection: close\n\r\n");
	if(type == GET)
		strcat(response,filenotfound);
	send(soc,response,strlen(response),0);
	free(response);
	close(soc);
	return 0;
}
int process_http_get_request(int soc, int version, char *file, int size,int isdir, char *dirhtml){
	send_http_response_header(soc, version, file, size, isdir, dirhtml);
	int result = send_http_response_body(soc, version, file, size, isdir, dirhtml);
	return result;
	close(soc);
}

int process_http_head_request(int soc, int version, char *file, int size,int isdir, char *dirhtml){
	send_http_response_header(soc, version, file, size, isdir, dirhtml);
	close(soc);
}
int send_http_response_header(int soc, int version, char *file, int size, int isdir, char *dirhtml){
	char *response_header = (char *)malloc(sizeof(char)*BUF_LEN);
	memset(response_header, 0, BUF_LEN);
	char content_length[20];
	char last_modified[256];
	char current_time[256];
	if(version == HTTP1_0)
		strcat(response_header,"HTTP/1.0 ");
	else if(version == HTTP1_1)
		strcat(response_header,"HTTP/1.1 ");
	if(size>=0){
		format_mod_timestamp(file,last_modified);
		format_current_timestamp(current_time);
		strcat(response_header,"200 OK\n");
		strcat(response_header,"Date: ");
		strcat(response_header,current_time);
		strcat(response_header,"\n");
		strcat(response_header,"Content-Type: ");
		if(isdir)
			strcat(response_header,"text/html");
		else
			strcat(response_header,get_mime_type(file));
		strcat(response_header,"\n");
		strcat(response_header,"Content-Length: ");
		sprintf(content_length,"%d\n",size);
		strcat(response_header,content_length);
		strcat(response_header,"Last-Modified: ");
		strcat(response_header,last_modified);
		strcat(response_header,"\n");
	}
	else{
		strcat(response_header,"404 Not Found\n");
		strcat(response_header,"Date: ");
		strcat(response_header,current_time);
		strcat(response_header,"\n");
		strcat(response_header,"Content-Type: text/html\n");
		strcat(response_header,"Content-Length: ");
		size = strlen(filenotfound);
		sprintf(content_length,"%d\n",size);
		strcat(response_header,content_length);
	}
	strcat(response_header,"Server: ");
	strcat(response_header,APP_NAME);
	strcat(response_header," ");
	strcat(response_header,APP_VERSION);
	strcat(response_header,"\n");
	strcat(response_header,"Connection: close\n\r\n");
	int headerlen = strlen(response_header);
	send(soc, response_header, headerlen, 0);
	free(response_header);
	return 0;
}
int send_http_response_body(int soc, int version, char *file, int size, int isdir, char *dirhtml){
	if(size>=0){
		if(isdir){
			send(soc,dirhtml,size,0);
		}
		else{
		char *buf = (char *)malloc(sizeof(char)*BUF_LEN);
		int fd = open((file), O_RDONLY);
		int bytes_remaining = size;
		while(bytes_remaining>0){
			if(bytes_remaining > BUF_LEN){
				memset(buf,0,BUF_LEN);
				int bread = 0;
				bread = read(fd,buf,BUF_LEN);
				int flag = 1; 
				int bwritten=0;
				bwritten = send(soc,buf,bread,0);
				bytes_remaining = bytes_remaining-bwritten;
			}
			else{
				memset(buf,0,BUF_LEN);
				int bread=0;
				bread = read(fd,buf,bytes_remaining);
				int flag = 1; 
				int bwritten=0;
				bwritten = send(soc,buf,bread,0);
				bytes_remaining = bytes_remaining-bwritten;
			}
		}
	
		close(fd);
		free(buf);
	}
	}
	else{
		send(soc,filenotfound,strlen(filenotfound),0);
	}
	return 0;
}
