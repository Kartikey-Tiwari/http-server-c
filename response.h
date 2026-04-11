#ifndef RESPONSE_H
#define RESPONSE_H

#include "headers.h"
#include <glib.h>

typedef enum StatusCode {
  OK = 200,
  BAD_REQUEST = 400,
  SERVER_ERROR = 500,
} StatusCode;

void writeStatusLine(int fd, StatusCode status);

Headers *getDefaultHeaders(int contentLen);

void writeHeaders(int fd, Headers *headers);
#endif
