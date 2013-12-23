/*
 * Header file for file operations
 */
#ifndef FILE_OPERATIONS
#define FILE_OPERATIONS
#include "request.h"

int get_file_size(char *filename);
struct tm *last_mod_timestamp(char *filename);
char * get_dirlist_html(char *dir);
void format_mod_timestamp(char *filename, char *f_timestamp);
void write_log_to_file(struct http_request *req);
void write_log_to_stdout(struct http_request *req);
char * get_mime_type(char *file);
#endif
