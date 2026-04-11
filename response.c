#include "response.h"
#include "headers.h"
#include "utils.h"
#include <string.h>
#include <sys/socket.h>

void writeStatusLine(int fd, StatusCode status) {
  char *msg;
  switch (status) {
  case OK:
    msg = "HTTP/1.1 200 OK\r\n";
    break;
  case BAD_REQUEST:
    msg = "HTTP/1.1 400 Bad Request\r\n";
    break;
  case SERVER_ERROR:
  default:
    msg = "HTTP/1.1 500 Internal Server Error\r\n";
  }
  safeSend(fd, msg, strlen(msg));
}

Headers *getDefaultHeaders(int contentLen) {
  Headers *headers = createHeaders();
  char *len = g_strdup_printf("%d", contentLen);

  setHeader(headers, "Content-Length", len);
  g_free(len);

  setHeader(headers, "Connection", "close");
  setHeader(headers, "Content-Type", "text/plain");

  return headers;
}

void writeHeader(void *key, void *value, void *fd) {
  GString *header = g_string_new((char *)key);
  g_string_append(header, ": ");
  g_string_append(header, (char *)value);
  g_string_append(header, "\r\n");

  safeSend(*(int *)fd, header->str, header->len);
  g_string_free(header, TRUE);
}

void writeHeaders(int fd, Headers *headers) {
  headersForEach(headers, writeHeader, &fd);
  safeSend(fd, "\r\n", 2);
}
