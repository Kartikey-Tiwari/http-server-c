#ifndef RESPONSE_H
#define RESPONSE_H

#include "headers.h"

typedef enum StatusCode {
  OK = 200,
  BAD_REQUEST = 400,
  NOT_FOUND = 404,
  SERVER_ERROR = 500,
  SERVER_UNAVAILABLE = 503,
  HTTP_UNSUPPORTED_VER = 505,
} StatusCode;

typedef enum ResponseState {
  RESPONSE_INITIALIZED,
  STATUSLINE_SENT,
  HEADERS_SENT,
  BODY_SENT,
  RESPONSE_DONE,
  RESPONSE_ERROR,
} ResponseState;

typedef struct Response {
  int sockfd;
  ResponseState state;
  Headers *headers;
} Response;

Response *createResponse(int fd);

void freeResponse(Response *res);

int writeStatusLine(Response *res, StatusCode status);

int writeHeaders(Response *res);

int writeBody(Response *res, char *body, int bodyLen);

void responseEnd(Response *res);

char *httpCodeToHTML(StatusCode status);
#endif
