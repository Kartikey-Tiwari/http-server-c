#include "utils.h"
#include <ctype.h>
#include <string.h>

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
