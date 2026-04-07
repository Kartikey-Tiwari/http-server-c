#include <arpa/inet.h>
#include <glib-2.0/glib.h>
#include <glib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

const int numMethods = 8;
char *httpMethods[8] = {"GET",    "HEAD",    "POST",    "PUT",
                        "DELETE", "CONNECT", "OPTIONS", "TRACE"};

typedef struct RequestLine {
  char *method;
  char *resource;
  char *http;
} RequestLine;

typedef struct Request {
  RequestLine rql;
  GHashTable *headers;
  int state;
} Request;

char isValidMethod(const char *method, char **methodsList, int numMethods) {
  for (int i = 0; i < numMethods; i++) {
    if (!strcmp(method, methodsList[i])) {
      return 1;
    }
  }
  return 0;
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

typedef enum {
  SUCCESS_READ_REQ_LINE = 0,
  SUCCESS_READ_HEADERS,
  REQUEST_METHOD_ERROR,
  REQUEST_HTTP_VER_ERROR,
  HEADER_FORMAT_ERROR
} ErrorCode;

ErrorCode readRequestLine(GString *line, RequestLine *rql) {
  ErrorCode rv = SUCCESS_READ_REQ_LINE;
  char **parts = g_strsplit(line->str, " ", -1);
  int i;
  for (i = 0; parts[i] != NULL; i++) {
  }
  if (i == 3) {
    char *method = parts[0];
    if (!isValidMethod(method, httpMethods, numMethods)) {
      printf("bad request\n");
      rv = REQUEST_METHOD_ERROR;
    }

    char *version = parts[2];

    if (strcmp("HTTP/1.1", version) != 0) {
      printf("bad request\n");
      rv = REQUEST_HTTP_VER_ERROR;
    }

    if (rv == SUCCESS_READ_REQ_LINE) {
      int j;
      for (j = 0; version[j] != '\0'; j++) {
        if (version[j] == '/') {
          break;
        }
      }

      rql->method = g_strdup(parts[0]);
      rql->resource = g_strdup(parts[1]);
      rql->http = g_strdup(version + j + 1);
      printf("%s\n%s\n%s\n", rql->method, rql->resource, rql->http);
    }
  }
  g_strfreev(parts);
  g_string_free(line, TRUE);
  return rv;
}

ErrorCode parse(char *buf, int bufSize, int *lookForStartingLF, GString *line,
                Request *req) {
  char *crlf = strstr(buf, "\r\n");
  if (crlf != NULL) {
    *crlf = '\0';
  }

  int rv;

  if (req->state == 0) {
    // last read end character was '\r' and this read first char is '\n'
    if (*lookForStartingLF == 1 && buf[0] == '\n') {
      rv = readRequestLine(line, &req->rql);
      line = g_string_new(NULL);
      g_string_append(line, buf + 1);
      req->state = 1;
      *lookForStartingLF = 0;
      return rv;
    }
    // delimiter crlf not found
    else if (crlf == NULL) {
      if (buf[bufSize] == '\r') {
        *lookForStartingLF = 1;
      } else {
        *lookForStartingLF = 0;
      }
      g_string_append(line, buf);
    }
    // crlf found
    else {
      g_string_append(line, buf);
      rv = readRequestLine(line, &req->rql);
      line = g_string_new(NULL);
      g_string_append(line, crlf + 2);
      return rv;
    }
  } else if (req->state == 1) {
  }
}

int main() {
  int sockfd, rv;
  struct sockaddr_in address;

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(8080);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == 0) {
    perror("socket: couldn't open");
    exit(1);
  }
  if (bind(sockfd, (struct sockaddr *)&address, sizeof address) < 0) {
    perror("bind: failed");
    exit(1);
  }

  if (listen(sockfd, 10) < 0) {
    perror("listen: failed");
    exit(1);
  }

  struct sockaddr_storage their_addr;
  socklen_t their_addr_size = sizeof their_addr;
  int new_fd;
  char s[INET6_ADDRSTRLEN];

  while (1) {
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &their_addr_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) {
      close(sockfd);

      char buf[9];
      buf[8] = '\0';

      int rv, lookForStartingLF = 0;
      GString *line = g_string_new(NULL);
      Request req = {0};
      req.state = 0;
      req.headers = g_hash_table_new(g_str_hash, g_str_equal);

      while ((rv = recv(new_fd, buf, 8, 0)) > 0) {
        buf[rv] = '\0';

        int k = parse(buf, rv, &lookForStartingLF, line, &req);
        if (k == SUCCESS_READ_REQ_LINE) {
          break;
        }
      }
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }

  close(sockfd);
  exit(0);
  return 0;
}
