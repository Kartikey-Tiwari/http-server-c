#ifndef PARSER_H
#define PARSER_H

#include "request.h"
#include <signal.h>

typedef enum ParseAction {
  ACTION_CONTINUE,
  ACTION_NEED_MORE_DATA,
  ACTION_DONE,
  ACTION_ERROR
} ParseAction;

typedef enum ReturnCode {
  SUCCESS_READ_REQ_LINE = 0,
  SUCCESS_READ_HEADER,
  SUCCESS_HEADERS_DONE,
  READ_NOT_PARSED,
  REQUEST_METHOD_ERROR,
  REQUEST_HTTP_VER_ERROR,
  REQUEST_INVALID_CHAR_ERROR,
  HEADER_FORMAT_ERROR,
  PREPARED_BODY_PARSER,
  READ_BODY,
  BODY_LENGTH_MISMATCH,
  SUCCESS_READ_BODY,
  NOTHING_TO_DO,
  INTERRUPTED
} ReturnCode;

ReturnCode readRequestFromClient(int fd, Request **req,
                                 volatile sig_atomic_t *keep_running);
#endif
