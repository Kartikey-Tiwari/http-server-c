#ifndef RESPONSE_H
#define RESPONSE_H

#include <glib.h>

typedef enum StatusCode {
  OK = 200,
  BAD_REQUEST = 400,
  SERVER_ERROR = 500,
} StatusCode;

void writeStatusLine(int fd, StatusCode status);

GHashTable *getDefaultHeaders(int contentLen);

void writeHeaders(int fd, GHashTable *headers);
#endif
