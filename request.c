#include "request.h"
#include "headers.h"
#include <stdio.h>

Request *createRequest() {
  Request *req = (Request *)malloc(sizeof(Request));
  req->headers = createHeaders();
  req->body = NULL;
  req->contentLen = 0;
  req->bytesRead = 0;
  req->state = REQUEST_INITIALIZED;
  return req;
}

void printRequestLine(RequestLine *rql) {
  printf("----Request Line----\n");
  printf("Method: %s\n", rql->method);
  printf("Resource: %s\n", rql->resource);
  printf("HTTP Version: %s\n", rql->http);
}

void print_upper(gpointer key, gpointer value, gpointer user_data) {
  (void)user_data;
  char *key_upper = g_ascii_strup((char *)key, -1);
  char *value_upper = g_ascii_strup((char *)value, -1);

  printf("%s: %s\n", key_upper, value_upper);

  g_free(key_upper);
  g_free(value_upper);
}

void printRequest(Request *req) {
  printRequestLine(&req->rql);

  printf("----Headers----\n");
  headersForEach(req->headers, print_upper, NULL);

  if (req->contentLen != 0) {
    printf("----Body----\n%s\n----end----\n", req->body);
  }
}

void freeRequest(Request *req) {
  g_free(req->body);
  headers_free(req->headers);
  g_free(req->rql.method);
  g_free(req->rql.resource);
  g_free(req->rql.http);
}
