#ifndef UTILS_H
#define UTILS_H

#include "request.h"
#include "response.h"
#include <netinet/in.h>

#define numMethods 8
extern char *httpMethods[];
#define numSpecialChars 15
extern char headerSpecialChars[];

void *get_in_addr(struct sockaddr *sa);

char isValidMethod(const char *method, char **methodsList);

int isValidHeaderField(const char *field);

int isValidHeaderValue(char *fieldValue);

int isValidContentLength(char *val);

void safeSend(int fd, char *buf, int bufSize);

char *itoa(int n);

const char *getMimeType(const char *filepath);

void static_file_handler(Request *req, Response *res);

#endif
