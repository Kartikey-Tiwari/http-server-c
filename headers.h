#ifndef HEADERS_H
#define HEADERS_H

#include "dynamic_string.h"
#include "hashmap.h"
#include <stdlib.h>

typedef struct Headers {
  hashmap *headers;
} Headers;

Headers *createHeaders();

void setHeader(Headers *headers, DString *headerField, DString *headerValue);

DString *headerLookup(Headers *headers, DString *headerField);

void headersForEach(Headers *headers, void (*fn)(void *, void *, void *),
                    void *data);

void headers_free(Headers *headers);
#endif
