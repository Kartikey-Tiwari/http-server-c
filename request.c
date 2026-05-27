#include "request.h"
#include "dynamic_string.h"
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
  printf("Method: %s\n", rql->method->str);
  printf("Resource: %s\n", rql->resource->str);
  printf("HTTP Version: %s\n", rql->http->str);
}

void print_upper(void *key, void *value, void *user_data) {
  (void)user_data;
  d_str_upper((DString *)key);
  d_str_upper((DString *)value);

  printf("%s: %s\n", ((DString *)key)->str, ((DString *)value)->str);
}

void printRequest(Request *req) {
  printRequestLine(&req->rql);

  printf("----Headers----\n");
  headersForEach(req->headers, print_upper, NULL);

  if (req->contentLen != 0) {
    printf("----Body----\n%s\n----end----\n", req->body->str);
  }
}

void freeRequest(Request *req) {
  d_str_free(req->body);
  headers_free(req->headers);
  d_str_free(req->rql.method);
  d_str_free(req->rql.resource);
  d_str_free(req->rql.http);
}
