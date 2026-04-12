#include "parser.h"
#include "headers.h"
#include "request.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

ReturnCode readTillCRLF(char *buf, int *bufSize, int *lookForStartingLF,
                        GString *line, Request *req,
                        ReturnCode (*fn)(GString *, Request *)) {
  char *crlf = strstr(buf, "\r\n");

  int rv;

  // last read end character was '\r' and this read first char is '\n'
  if (*lookForStartingLF == 1) {
    if (buf[0] == '\n') {
      rv = fn(line, req);
      line = g_string_truncate(line, 0);
      memmove(buf, buf + 1, *bufSize - 1);
      buf[*bufSize - 1] = '\0';
      *bufSize = *bufSize - 1;
      *lookForStartingLF = 0;
    } else {
      if (req->state == REQUEST_INITIALIZED) {
        rv = REQUEST_INVALID_CHAR_ERROR;
      } else if (req->state == READ_REQUEST_LINE) {
        rv = HEADER_FORMAT_ERROR;
      }
    }
    return rv;
  }
  // delimiter crlf not found
  else if (crlf == NULL) {
    if (buf[*bufSize - 1] == '\r') {
      buf[*bufSize - 1] = '\0';
      *lookForStartingLF = 1;
    } else {
      *lookForStartingLF = 0;
    }
    g_string_append(line, buf);
    *bufSize = 0;
    rv = READ_NOT_PARSED;
    return rv;
  }
  // crlf found
  else {
    *lookForStartingLF = 0;
    *crlf = '\0';
    g_string_append(line, buf);
    *crlf = '\r';
    rv = fn(line, req);
    line = g_string_truncate(line, 0);
    *bufSize = *bufSize - (crlf - buf + 2);
    memmove(buf, crlf + 2, *bufSize);
    buf[*bufSize] = '\0';
    return rv;
  }
}

ReturnCode readRequestLine(GString *line, Request *req) {
  ReturnCode rv = SUCCESS_READ_REQ_LINE;
  char **parts = g_strsplit(line->str, " ", -1);
  int i;
  for (i = 0; parts[i] != NULL; i++) {
  }
  if (i == 3) {
    char *method = parts[0];
    if (!isValidMethod(method, httpMethods)) {
      printf("request method error\n");
      rv = REQUEST_METHOD_ERROR;
      g_strfreev(parts);
      return rv;
    }

    char *version = parts[2];

    if (strcmp("HTTP/1.1", version) != 0) {
      printf("http version error\n");
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

      req->rql.method = g_strdup(parts[0]);
      req->rql.resource = g_strdup(parts[1]);
      req->rql.http = g_strdup(version + j + 1);
    }
  }
  g_strfreev(parts);
  return rv;
}

ReturnCode readHeaderLine(GString *line, Request *req) {
  ReturnCode rv = SUCCESS_READ_HEADER;
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

    addToHeader(req->headers, field, value);
  } else {
    rv = HEADER_FORMAT_ERROR;
  }
  g_strfreev(parts);
  return rv;
}

ReturnCode readBodyWithLen(char *buf, int *bufSize, GString *line,
                           Request *req) {
  int bytesLeft = req->contentLen - req->bytesRead;

  if (bytesLeft - *bufSize < 0) {
    return BODY_LENGTH_MISMATCH;
  }

  g_string_append(line, buf);
  req->bytesRead += *bufSize;
  bytesLeft -= *bufSize;
  *bufSize = 0;

  if (!bytesLeft) {
    req->body = g_strdup(line->str);
    return SUCCESS_READ_BODY;
  }

  return READ_BODY;
}

ParseAction parse(char *buf, int *bufSize, int *lookForStartingLF,
                  GString *line, Request *req, ReturnCode *err) {
  switch (req->state) {
  case REQUEST_INITIALIZED:
    *err = readTillCRLF(buf, bufSize, lookForStartingLF, line, req,
                        readRequestLine);
    if (*err == SUCCESS_READ_REQ_LINE) {
      req->state = READ_REQUEST_LINE;
    }
    break;
  case READ_REQUEST_LINE:
    *err = readTillCRLF(buf, bufSize, lookForStartingLF, line, req,
                        readHeaderLine);
    if (*err == SUCCESS_HEADERS_DONE) {
      req->state = READ_HEADERS;
    }
    break;
  case READ_HEADERS:
    char *value = headerLookup(req->headers, "content-length");
    if (value == NULL) {
      req->state = DONE;
      *err = NOTHING_TO_DO;
      break;
    }
    if (isValidContentLength(value)) {
      int len = atoi(value);
      req->contentLen = len;
      req->bytesRead = 0;
      req->state = READING_BODY;
      *err = PREPARED_BODY_PARSER;
    } else {
      *err = HEADER_FORMAT_ERROR;
    }
    break;
  case READING_BODY:
    *err = readBodyWithLen(buf, bufSize, line, req);
    if (*err == SUCCESS_READ_BODY) {
      req->state = DONE;
    }
    break;
  case DONE:
    *err = NOTHING_TO_DO;
    break;
  default:
  }
  if (req->state == DONE) {
    return ACTION_DONE;
  }

  if (*err == REQUEST_METHOD_ERROR || *err == REQUEST_HTTP_VER_ERROR ||
      *err == REQUEST_INVALID_CHAR_ERROR || *err == HEADER_FORMAT_ERROR ||
      *err == BODY_LENGTH_MISMATCH || req->state == REQUEST_ERROR) {
    req->state = REQUEST_ERROR;
    return ACTION_ERROR;
  }

  if (*err == READ_NOT_PARSED || *err == READ_BODY) {
    return ACTION_NEED_MORE_DATA;
  }

  return ACTION_CONTINUE;
}

ReturnCode readRequestFromClient(int fd, Request **request,
                                 volatile sig_atomic_t *keep_running) {
  char buf[9];
  buf[8] = '\0';

  int bytesReceived, lookForStartingLF = 0;
  GString *line = g_string_new(NULL);
  ReturnCode rc;

  while (*keep_running) {
    bytesReceived = recv(fd, buf, 8, 0);
    if (bytesReceived < 0) {
      if (errno == EINTR) {
        break;
      }
      perror("recv failed");
      break;
    } else if (bytesReceived == 0) {
      break;
    }
    int rv = bytesReceived;
    buf[rv] = '\0';

    ParseAction action =
        parse(buf, &rv, &lookForStartingLF, line, *request, &rc);
    while (action == ACTION_CONTINUE) {
      action = parse(buf, &rv, &lookForStartingLF, line, *request, &rc);
    }

    if (action == ACTION_DONE || action == ACTION_ERROR) {
      break;
    }
  }
  if (!(*keep_running)) {
    rc = INTERRUPTED;
  }
  g_string_free(line, TRUE);

  return rc;
}
