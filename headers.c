#include "headers.h"
#include "dynamic_string.h"
#include "hashmap.h"

uint32_t wrap_str_hash(const void *key) { return d_str_hash((DString *)key); }

int wrap_str_equal(const void *key1, const void *key2) {
  return d_str_equal((DString *)key1, (DString *)key2);
}

void wrap_str_free(void *item) { d_str_free((DString *)item); }

Headers *createHeaders() {
  Headers *headers = malloc(sizeof(Headers));
  headers->headers = createHashmap(wrap_str_hash, wrap_str_equal, wrap_str_free,
                                   wrap_str_free);
  return headers;
}

DString *headerLookup(Headers *headers, DString *headerField) {
  if (headerField == NULL) {
    return NULL;
  }
  d_str_lower(headerField);
  DString *val = hashmap_lookup(headers->headers, headerField);
  return val;
}

void setHeader(Headers *headers, DString *headerField, DString *headerValue) {
  if (headerField == NULL) {
    return;
  }
  d_str_lower(headerField);

  DString *prevValue = headerLookup(headers, headerField);
  if (prevValue == NULL) {
    hashmap_insert(headers->headers, headerField, headerValue);
  } else {
    d_str_append(prevValue, ",");
    d_str_append(prevValue, headerValue->str);

    d_str_free(headerField);
    d_str_free(headerValue);
  }
}

void headersForEach(Headers *headers, void (*fn)(void *, void *, void *),
                    void *data) {
  hashmap_foreach(headers->headers, fn, data);
}

void headers_free(Headers *headers) {
  hashmap_destroy(headers->headers);
  free(headers);
}
