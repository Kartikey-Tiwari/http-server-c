#include "utils.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

char *httpMethods[] = {"GET",    "HEAD",    "POST",    "PUT",
                       "DELETE", "CONNECT", "OPTIONS", "TRACE"};
char headerSpecialChars[] = {'!', '#', '$', '%', '&', '\'', '*', '+',
                             '-', '.', '^', '_', '`', '|',  '~'};

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
