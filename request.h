#ifndef REQUEST_H
#define REQUEST_H

#include "headers.h"
#include <glib.h>

typedef struct RequestLine {
  char *method;
  char *resource;
  char *http;
} RequestLine;

typedef enum RequestState {
  INITIALIZED,
  READ_REQUEST_LINE,
  READ_HEADERS,
  READING_BODY,
  ERROR_STATE,
  DONE
} RequestState;

typedef struct Request {
  RequestLine rql;
  Headers *headers;
  char *body;
  int contentLen;
  int bytesRead;
  RequestState state;
} Request;

void printRequest(Request *req);

void freeRequest(Request *req);
#endif
