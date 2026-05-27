#include "response.h"
#include "server.h"
#include "utils.h"
#include <string.h>

void handler(Request *req, Response *res) {
  char *body;
  StatusCode status;
  if (strcmp(req->rql.resource->str, "/") == 0) {
    status = OK;
  } else {
    status = NOT_FOUND;
  }
  body = httpCodeToHTML(status);
  writeStatusLine(res, status);
  setHeader(res->headers, d_str_new("Content-Type"), d_str_new("text/html"));
  int bodyLen = strlen(body);
  char *len = itoa(bodyLen);
  setHeader(res->headers, d_str_new("Content-Length"), d_str_new(len));
  free(len);
  writeHeaders(res);
  writeBody(res, body, bodyLen);
  responseEnd(res);
}

int main() {
  Server *server = createServer(8080);
  serverListen(server, handler);
  stopListening(server);
  return 0;
}
