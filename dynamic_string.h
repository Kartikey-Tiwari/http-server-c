#ifndef DYNAMIC_STRING_H
#define DYNAMIC_STRING_H

#include <stddef.h>
#include <stdint.h>

typedef struct DString {
  size_t len;
  size_t allocated_size;
  char *str;
} DString;

DString *d_str_new(const char *str);
DString *d_str_dup(const DString *str);
void d_str_free(DString *str);

int d_str_equal(const DString *str1, const DString *str2);
uint32_t d_str_hash(const DString *str);

int d_str_append(DString *to_str, const char *from_str);
void d_str_truncate(DString *str, size_t len);

void d_str_trim(DString *str);
void d_str_ltrim(DString *str);
void d_str_lower(DString *str);
void d_str_upper(DString *str);

DString **d_str_split(const DString *str, const char *delimiter, int numSplits);
void d_str_free_multiple(DString **parts);

#endif
