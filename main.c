#include <arpa/inet.h>
#include <ctype.h>
#include <glib-2.0/glib.h>
#include <glib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

#define numMethods 8
char *httpMethods[numMethods] = {"GET",    "HEAD",    "POST",    "PUT",
                                 "DELETE", "CONNECT", "OPTIONS", "TRACE"};
#define numSpecialChars 15
char headerSpecialChars[15] = {'!', '#', '$', '%', '&', '\'', '*', '+',
                               '-', '.', '^', '_', '`', '|',  '~'};

typedef struct RequestLine {
  char *method;
  char *resource;
  char *http;
} RequestLine;

void printRequestLine(RequestLine *rql) {
  printf("----Request Line----\n");
  printf("Method: %s\n", rql->method);
  printf("Resource: %s\n", rql->resource);
  printf("HTTP Version: %s\n", rql->http);
}

typedef struct Request {
  RequestLine rql;
  GHashTable *headers;
  int state;
} Request;

void print_upper(gpointer key, gpointer value, gpointer user_data) {
  char *key_upper = g_ascii_strup((char *)key, -1);
  char *value_upper = g_ascii_strup((char *)value, -1);

  printf("%s: %s\n", key_upper, value_upper);

  g_free(key_upper);
  g_free(value_upper);
}

void printRequest(Request *req) {
  printRequestLine(&req->rql);

  printf("----Headers----\n");
  g_hash_table_foreach(req->headers, print_upper, NULL);
}

char isValidMethod(const char *method, char **methodsList) {
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
  SUCCESS_READ_HEADER,
  SUCCESS_HEADERS_DONE,
  READ_NOT_PARSED,
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
    if (!isValidMethod(method, httpMethods)) {
      printf("bad request\n");
      rv = REQUEST_METHOD_ERROR;
      g_strfreev(parts);
      return rv;
    }

    char *version = parts[2];

    if (strcmp("HTTP/1.1", version) != 0) {
      printf("bad request\n");
      rv = REQUEST_HTTP_VER_ERROR;
      g_strfreev(parts);
      return rv;
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
    }
  }
  g_strfreev(parts);
  return rv;
}

int isValidHeaderField(const char *field) {
  for (int i = 0; field[i] != '\0'; i++) {
    if (!isalnum(field[i])) {
      char found = 0;
      for (int j = 0; j < numSpecialChars; j++) {
        if (headerSpecialChars[j] == field[i]) {
          found = 1;
        }
      }
      if (found == 0) {
        return 0;
      }
    }
  }
  return 1;
}

int isValidHeaderValue(char *fieldValue) {
  for (char *c = fieldValue; *c; c++) {
    if ((*c <= 31 && *c != '\t') || *c == 127) {
      return 0;
    }
  }
  return 1;
}

ErrorCode readHeaderLine(GString *line, GHashTable *headers) {
  ErrorCode rv = SUCCESS_READ_HEADER;
  if (*(line->str) == '\0') {
    rv = SUCCESS_HEADERS_DONE;
    return rv;
  }
  char **parts = g_strsplit(line->str, ":", 2);
  int i;
  for (i = 0; parts[i] != NULL; i++) {
  }
  if (i == 2) {
    char *field = g_strchug(parts[0]);
    if (!isValidHeaderField(field)) {
      rv = HEADER_FORMAT_ERROR;
      g_strfreev(parts);
      return rv;
    }

    for (char *c = parts[0]; *c; c++) {
      *c = tolower(*c);
    }

    char *value = g_strstrip(parts[1]);
    if (!isValidHeaderValue(value)) {
      rv = HEADER_FORMAT_ERROR;
      g_strfreev(parts);
      return rv;
    }

    char *prevValue = g_hash_table_lookup(headers, field);
    if (prevValue == NULL) {
      g_hash_table_insert(headers, g_strdup(field), g_strdup(value));
    } else {
      GString *newValue = g_string_new(prevValue);
      g_string_append(newValue, ",");
      g_string_append(newValue, value);
      g_hash_table_insert(headers, g_strdup(field), g_strdup(newValue->str));
      g_string_free(newValue, TRUE);
    }
  } else {
    rv = HEADER_FORMAT_ERROR;
  }
  g_strfreev(parts);
  return rv;
}

ErrorCode parse(char *buf, int *bufSize, int *lookForStartingLF, GString *line,
                Request *req) {
  char *crlf = strstr(buf, "\r\n");
  if (crlf != NULL) {
    *crlf = '\0';
  }

  int rv;

  // last read end character was '\r' and this read first char is '\n'
  if (*lookForStartingLF == 1 && buf[0] == '\n') {
    if (req->state == 0) {
      rv = readRequestLine(line, &req->rql);
    } else if (req->state == 1) {
      rv = readHeaderLine(line, req->headers);
    }
    line = g_string_truncate(line, 0);
    memmove(buf, buf + 1, *bufSize - 1);
    buf[*bufSize - 1] = '\0';
    if (rv == SUCCESS_READ_REQ_LINE) {
      req->state = 1;
    } else if (rv == SUCCESS_HEADERS_DONE) {
      req->state = 2;
    }
    *lookForStartingLF = 0;
    return rv;
  }
  // delimiter crlf not found
  else if (crlf == NULL) {
    if (buf[*bufSize - 1] == '\r') {
      *lookForStartingLF = 1;
    } else {
      *lookForStartingLF = 0;
    }
    g_string_append(line, buf);
    rv = READ_NOT_PARSED;
    return rv;
  }
  // crlf found
  else {
    *lookForStartingLF = 0;
    g_string_append(line, buf);
    if (req->state == 0) {
      rv = readRequestLine(line, &req->rql);
    } else if (req->state == 1) {
      rv = readHeaderLine(line, req->headers);
    }
    line = g_string_truncate(line, 0);
    *bufSize = *bufSize - (crlf - buf + 2);
    memmove(buf, crlf + 2, *bufSize);
    buf[*bufSize] = '\0';
    if (rv == SUCCESS_READ_REQ_LINE) {
      req->state = 1;
    } else if (rv == SUCCESS_HEADERS_DONE) {
      req->state = 2;
    }
    return rv;
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
      req.headers =
          g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

      while ((rv = recv(new_fd, buf, 8, 0)) > 0) {
        buf[rv] = '\0';

        int k = parse(buf, &rv, &lookForStartingLF, line, &req);
        while (k != READ_NOT_PARSED && k != SUCCESS_HEADERS_DONE) {
          k = parse(buf, &rv, &lookForStartingLF, line, &req);
        }
        if (k == SUCCESS_HEADERS_DONE) {
          break;
        }
      }
      printRequest(&req);
      g_string_free(line, TRUE);
      g_hash_table_destroy(req.headers);
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }

  close(sockfd);
  exit(0);
  return 0;
}
