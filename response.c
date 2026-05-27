#include "response.h"
#include "dynamic_string.h"
#include "headers.h"
#include "utils.h"
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

Response *createResponse(int fd) {
  Response *res = (Response *)malloc(sizeof(Response));
  res->sockfd = fd;
  res->state = RESPONSE_INITIALIZED;
  res->headers = createHeaders();

  setHeader(res->headers, d_str_new("Connection"), d_str_new("close"));
  return res;
}

void freeResponse(Response *res) {
  headers_free(res->headers);
  free(res);
}

int writeStatusLine(Response *res, StatusCode status) {
  if (res->state != RESPONSE_INITIALIZED) {
    res->state = RESPONSE_ERROR;
    return 0;
  }
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
  safeSend(res->sockfd, msg, strlen(msg));
  res->state = STATUSLINE_SENT;
  return 1;
}

void writeHeader(void *key, void *value, void *fd) {
  Response *res = (Response *)fd;
  DString *header = d_str_dup((DString *)key);
  d_str_append(header, ": ");
  d_str_append(header, ((DString *)value)->str);
  d_str_append(header, "\r\n");

  safeSend(res->sockfd, header->str, header->len);
  d_str_free(header);
}

int writeHeaders(Response *res) {
  if (res->state != STATUSLINE_SENT) {
    res->state = RESPONSE_ERROR;
    return 0;
  }
  headersForEach(res->headers, writeHeader, res);
  safeSend(res->sockfd, "\r\n", 2);
  res->state = HEADERS_SENT;
  return 1;
}

int writeBody(Response *res, char *body, int bodyLen) {
  if (res->state != HEADERS_SENT) {
    res->state = RESPONSE_ERROR;
    return 0;
  }

  DString *key = d_str_new("content-length");
  DString *contentLen = headerLookup(res->headers, key);
  d_str_free(key);
  if (contentLen == NULL) {
    res->state = RESPONSE_ERROR;
    return 0;
  }

  int len = atoi(contentLen->str);

  if (len != bodyLen) {
    res->state = RESPONSE_ERROR;
    return 0;
  }

  safeSend(res->sockfd, body, bodyLen);
  res->state = BODY_SENT;
  return 1;
}

void responseEnd(Response *res) {
  switch (res->state) {
  case RESPONSE_INITIALIZED:
    writeStatusLine(res, OK);
    writeHeaders(res);
  case STATUSLINE_SENT:
    safeSend(res->sockfd, "\r\n", 2);
    break;
  case HEADERS_SENT:
  case BODY_SENT:
  case RESPONSE_ERROR:
  default:;
  };
  res->state = RESPONSE_DONE;
  close(res->sockfd);
}

char *httpCodeToHTML(StatusCode status) {
  switch (status) {
  case OK:
    return "<html>\n"
           "<head>\n"
           "<title>200 OK</title>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Success!</h1>\n"
           "<p>Your request was an absolute banger.</p>\n"
           "</body>\n"
           "</html>";
  case BAD_REQUEST:
    return "<html>\n"
           "<head>\n"
           "<title>400 Bad Request</title>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Bad Request</h1>\n"
           "<p>Your request honestly kinda sucked.</p>\n"
           "</body>\n"
           "</html>";
  case NOT_FOUND:
    return "<html>\n"
           "<head>\n"
           "<title>404 Not Found</title>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Bad Request</h1>\n"
           "<p>Oops! The resource you requested does not exist on this "
           "server.</p>\n"
           "</body>\n"
           "</html>";
  case SERVER_ERROR:
    return "<html>\n"
           "<head>\n"
           "<title>500 Internal Server Error</title>\n"
           "</head>\n"
           "<body>\n"
           "<h1>Internal Server Error</h1>\n"
           "<p>Okay, you know what? This one is on me.</p>\n"
           "</body>\n"
           "</html>";
  case HTTP_UNSUPPORTED_VER:
    return "<html>\n"
           "<head>\n"
           "<title>505 HTTP Version Not Supported</title>\n"
           "</head>\n"
           "<body>\n"
           "<h1>HTTP Version Not Supported</h1>\n"
           "<p>Okay, you know what? This one is on me.</p>\n"
           "</body>\n"
           "</html>";
  default:;
    return "";
  }
}
