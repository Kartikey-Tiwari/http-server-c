#include "response.h"
#include "server.h"
#include "utils.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>

char *static_folder_path = NULL;

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

int main(int argc, char *argv[]) {

  int port = 8080;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--static") == 0) {
      if (i + 1 < argc) {
        static_folder_path = argv[i + 1];
        i++;
      } else {
        printf("Error: --static requires a single folder path (e.g., --static "
               "./public)\n");
        return 1;
      }
    } else if (strcmp(argv[i], "--port") == 0) {
      if (i + 1 < argc) {
        port = atoi(argv[i + 1]);
        if (port < 1024 || port > 65535) {
          printf("Error: port number should be greater than 1023 and less than "
                 "65536");
          return 1;
        }
        i++;
      } else {
        printf("Error: --port requires a port number (e.g., --port 8080)");
        return 1;
      }
    } else {
      break;
    }
  }
  Server *server = createServer(port);
  if (static_folder_path != NULL) {
    printf("Starting in STATIC FILE mode serving folder: %s\n",
           static_folder_path);
    serverListen(server, static_file_handler);
  } else {
    printf("Starting in API mode\n");
    serverListen(server, handler);
  }
  /* serverListen(server, handler); */
  stopListening(server);
  return 0;
}
