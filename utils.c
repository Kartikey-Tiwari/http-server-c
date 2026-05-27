#include "utils.h"
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>

char *httpMethods[] = {"GET",    "HEAD",    "POST",    "PUT",
                       "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char headerSpecialChars[] = {'!', '#', '$', '%', '&', '\'', '*', '+',
                             '-', '.', '^', '_', '`', '|',  '~'};
extern char *static_folder_path;

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }

  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

char isValidMethod(const char *method, char **methodsList) {
  for (int i = 0; i < numMethods; i++) {
    if (!strcmp(method, methodsList[i])) {
      return 1;
    }
  }
  return 0;
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

int isValidContentLength(char *val) {
  for (char *c = val; *c; c++) {
    if (!isdigit(*c)) {
      return 0;
    }
  }
  return 1;
}

void safeSend(int fd, char *buf, int bufSize) {
  int rv, bytesLeft = bufSize;
  while ((rv = send(fd, buf + bufSize - bytesLeft, bytesLeft, 0)) != -1 &&
         bytesLeft != 0) {
    bytesLeft -= rv;
  }
}

char *itoa(int n) {
  char *buf = malloc(sizeof(char) * 11);
  int r, idx = 0;
  if (n == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }
  while (n) {
    r = n % 10;
    n = n / 10;
    buf[idx++] = '0' + r;
  }
  buf[idx] = '\0';
  idx--;

  int x = 0;
  char temp;
  while (x <= idx) {
    temp = buf[x];
    buf[x++] = buf[idx];
    buf[idx--] = temp;
  }
  return buf;
}

const char *getMimeType(const char *filepath) {
  const char *dot = strrchr(filepath, '.');

  if (!dot || dot == filepath) {
    return "application/octet-stream";
  }

  // Web Core
  if (strcmp(dot, ".html") == 0 || strcmp(dot, ".htm") == 0)
    return "text/html";
  if (strcmp(dot, ".css") == 0)
    return "text/css";
  if (strcmp(dot, ".js") == 0)
    return "application/javascript";
  if (strcmp(dot, ".json") == 0)
    return "application/json";
  if (strcmp(dot, ".xml") == 0)
    return "application/xml";

  // Documents
  if (strcmp(dot, ".pdf") == 0)
    return "application/pdf";
  if (strcmp(dot, ".txt") == 0)
    return "text/plain";
  if (strcmp(dot, ".csv") == 0)
    return "text/csv";

  // Images
  if (strcmp(dot, ".jpg") == 0 || strcmp(dot, ".jpeg") == 0)
    return "image/jpeg";
  if (strcmp(dot, ".png") == 0)
    return "image/png";
  if (strcmp(dot, ".gif") == 0)
    return "image/gif";
  if (strcmp(dot, ".svg") == 0)
    return "image/svg+xml";
  if (strcmp(dot, ".ico") == 0)
    return "image/x-icon";
  if (strcmp(dot, ".webp") == 0)
    return "image/webp";

  // Video (Allows direct browser streaming)
  if (strcmp(dot, ".mp4") == 0)
    return "video/mp4";
  if (strcmp(dot, ".webm") == 0)
    return "video/webm";
  if (strcmp(dot, ".ogv") == 0)
    return "video/ogg";

  // Audio
  if (strcmp(dot, ".mp3") == 0)
    return "audio/mpeg";
  if (strcmp(dot, ".wav") == 0)
    return "audio/wav";
  if (strcmp(dot, ".ogg") == 0)
    return "audio/ogg";
  if (strcmp(dot, ".aac") == 0)
    return "audio/aac";

  // Web Fonts
  if (strcmp(dot, ".woff") == 0)
    return "font/woff";
  if (strcmp(dot, ".woff2") == 0)
    return "font/woff2";
  if (strcmp(dot, ".ttf") == 0)
    return "font/ttf";
  if (strcmp(dot, ".otf") == 0)
    return "font/otf";

  // Compressed Archives
  if (strcmp(dot, ".zip") == 0)
    return "application/zip";
  if (strcmp(dot, ".tar") == 0)
    return "application/x-tar";
  if (strcmp(dot, ".gz") == 0)
    return "application/gzip";
  if (strcmp(dot, ".rar") == 0)
    return "application/vnd.rar";

  // Default Fallback
  return "application/octet-stream";
}

void static_file_handler(Request *req, Response *res) {
  char filepath[1024];

  if (strcmp(req->rql.resource->str, "/") == 0) {
    snprintf(filepath, sizeof(filepath), "%s/index.html", static_folder_path);
  } else {
    snprintf(filepath, sizeof(filepath), "%s%s", static_folder_path,
             req->rql.resource->str);
  }

  int file_fd = open(filepath, O_RDONLY);
  if (file_fd < 0) {
    char *body = httpCodeToHTML(NOT_FOUND);
    writeStatusLine(res, NOT_FOUND);
    setHeader(res->headers, d_str_new("Content-Type"), d_str_new("text/html"));
    int bodyLen = strlen(body);
    char *len = itoa(bodyLen);
    setHeader(res->headers, d_str_new("Content-Length"), d_str_new(len));
    free(len);
    writeHeaders(res);
    writeBody(res, body, bodyLen);
    responseEnd(res);
    return;
  }

  struct stat stat_buf;
  fstat(file_fd, &stat_buf);
  long filesize = stat_buf.st_size;

  writeStatusLine(res, OK);
  const char *mimeType = getMimeType(filepath);
  setHeader(res->headers, d_str_new("Content-Type"), d_str_new(mimeType));
  setHeader(res->headers, d_str_new("Connection"), d_str_new("close"));
  char *len = itoa(filesize);
  setHeader(res->headers, d_str_new("Content-Length"), d_str_new(len));
  writeHeaders(res);
  free(len);

  off_t offset = 0;
  while (offset < filesize) {
    ssize_t n = sendfile(res->sockfd, file_fd, &offset, filesize - offset);

    if (n <= 0) {
      perror("sendfile failed");
      break;
    }
  }
  responseEnd(res);
}
