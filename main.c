#include "response.h"
#include "server.h"
#include "utils.h"
#include <string.h>

void handler(Request *req, Response *res) {
  char *body;
  StatusCode status;
  if (strcmp(req->rql.resource, "/") == 0) {
    status = OK;
  } else {
    status = NOT_FOUND;
  }
  body = httpCodeToHTML(status);
  writeStatusLine(res, status);
  setHeader(res->headers, "Content-Type", "text/html");
  int bodyLen = strlen(body);
  char *len = itoa(bodyLen);
  setHeader(res->headers, "Content-Length", len);
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
