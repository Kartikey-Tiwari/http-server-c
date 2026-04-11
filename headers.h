#ifndef HEADERS_H
#define HEADERS_H

#include <glib.h>
#include <stdlib.h>

typedef struct Headers {
  GHashTable *headers;
} Headers;

Headers *createHeaders();

void setHeader(Headers *headers, char *headerField, char *headerValue);

char *headerLookup(Headers *headers, char *headerField);

void addToHeader(Headers *headers, char *headerField, char *headerValue);

void headersForEach(Headers *headers, void (*fn)(void *, void *, void *),
                    void *data);

void headers_free(Headers *headers);
#endif
