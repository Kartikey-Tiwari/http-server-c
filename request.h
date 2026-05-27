#ifndef REQUEST_H
#define REQUEST_H

#include "dynamic_string.h"
#include "headers.h"

typedef struct RequestLine {
  DString *method;
  DString *resource;
  DString *http;
} RequestLine;

typedef enum RequestState {
  REQUEST_INITIALIZED,
  READ_REQUEST_LINE,
  READ_HEADERS,
  READING_BODY,
  REQUEST_ERROR,
  DONE
} RequestState;

typedef struct Request {
  RequestLine rql;
  Headers *headers;
  DString *body;
  int contentLen;
  int bytesRead;
  RequestState state;
} Request;

Request *createRequest();

void printRequest(Request *req);

void freeRequest(Request *req);
#endif
