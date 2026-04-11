#include "headers.h"

Headers *createHeaders() {
  Headers *headers = malloc(sizeof(Headers));
  headers->headers =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
  return headers;
}
void setHeader(Headers *headers, char *headerField, char *headerValue) {
  g_hash_table_insert(headers->headers, g_strdup(headerField),
                      g_strdup(headerValue));
}

char *headerLookup(Headers *headers, char *headerField) {
  return g_hash_table_lookup(headers->headers, headerField);
}

void addToHeader(Headers *headers, char *headerField, char *headerValue) {
  char *prevValue = headerLookup(headers, headerField);
  if (prevValue == NULL) {
    setHeader(headers, headerField, headerValue);
  } else {
    GString *newValue = g_string_new(prevValue);
    g_string_append(newValue, ",");
    g_string_append(newValue, headerValue);
    g_hash_table_insert(headers->headers, g_strdup(headerField),
                        g_strdup(newValue->str));
    g_string_free(newValue, TRUE);
  }
}

void headersForEach(Headers *headers, void (*fn)(void *, void *, void *),
                    void *data) {
  g_hash_table_foreach(headers->headers, fn, data);
}

void headers_free(Headers *headers) {
  g_hash_table_destroy(headers->headers);
  free(headers);
}
