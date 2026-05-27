#include "dynamic_string.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

DString *d_str_new(const char *str) {
  size_t len = strlen(str);

  DString *res = (DString *)malloc(sizeof(DString));
  if (!res) {
    return res;
  }
  res->allocated_size = len + 1;
  res->len = len;
  res->str = malloc(sizeof(char) * res->allocated_size);
  if (!res->str) {
    free(res);
    return NULL;
  }
  strcpy(res->str, str);
  return res;
};

DString *d_str_dup(const DString *str) {
  DString *dup = malloc(sizeof(DString));
  if (!dup) {
    return dup;
  }
  dup->allocated_size = str->len + 1;
  dup->len = str->len;
  dup->str = strdup(str->str);
  if (!dup->str) {
    free(dup);
    return NULL;
  }
  return dup;
}

void d_str_free(DString *str) {
  if (!str || !str->str)
    return;
  free(str->str);
  free(str);
}

int d_str_resize(DString *str, size_t desiredSize) {
  if (!str || desiredSize < str->allocated_size) {
    return 0;
  }
  size_t newAllocated = str->allocated_size * 2;
  if (newAllocated < desiredSize) {
    newAllocated = desiredSize;
  }

  str->str = realloc(str->str, newAllocated);
  if (!str->str) {
    return 0;
  }

  str->allocated_size = newAllocated;
  return 1;
}

int d_str_equal(const DString *str1, const DString *str2) {
  return strcmp(str1->str, str2->str);
}

// http://www.cse.yorku.ca/~oz/hash.html
uint32_t d_str_hash(const DString *str) {
  uint32_t hash = 5381;
  int c;

  char *p = str->str;

  while ((c = *p++))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

int d_str_append(DString *to_str, const char *from_str) {
  if (!from_str || *from_str == '\0')
    return 0;
  size_t len = strlen(from_str);
  if (to_str->allocated_size - to_str->len < len + 1) {
    if (!d_str_resize(to_str, to_str->allocated_size + len + 1)) {
      return 0;
    }
  }
  strcpy(to_str->str + to_str->len, from_str);
  to_str->len += len;
  return 1;
}

void d_str_truncate(DString *str, size_t len) {
  if (len + 1 > str->allocated_size) {
    return;
  }

  str->len = len;
  str->str[len] = '\0';
}

void d_str_trim(DString *str) {
  if (!str || !str->str || !str->len) {
    return;
  }

  char *start = str->str;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    start++;
  }

  if (*start == '\0') {
    str->str[0] = '\0';
    str->len = 0;
    return;
  }

  size_t end = str->len - 1;
  while (end > 0 && isspace((unsigned char)str->str[end])) {
    end--;
  }

  size_t newLen = (str->str + end) - start + 1;

  memmove(str->str, start, newLen);

  str->str[newLen] = '\0';
  str->len = newLen;
}

void d_str_ltrim(DString *str) {
  if (!str || !str->str || !str->len) {
    return;
  }

  char *start = str->str;
  while (*start != '\0' && isspace((unsigned char)*start)) {
    start++;
  }

  if (*start == '\0') {
    str->str[0] = '\0';
    str->len = 0;
    return;
  }

  size_t newLen = str->str + str->len - start;

  memmove(str->str, start, newLen);
  str->str[newLen] = '\0';
  str->len = newLen;
}

void d_str_lower(DString *str) {
  for (size_t i = 0; i < str->len; i++) {
    str->str[i] = tolower(str->str[i]);
  }
}

void d_str_upper(DString *str) {
  for (size_t i = 0; i < str->len; i++) {
    str->str[i] = toupper(str->str[i]);
  }
}

DString **d_str_split(const DString *str, const char *delimiter,
                      int numSplits) {
  if (!str || !str->str || !delimiter)
    return NULL;

  size_t delim_len = strlen(delimiter);

  if (delim_len == 0) {
    DString **result = malloc(2 * sizeof(DString *));
    result[0] = d_str_new(str->str);
    result[1] = NULL;
    return result;
  }

  int count = 1;
  const char *tmp = str->str;
  while ((tmp = strstr(tmp, delimiter)) != NULL) {
    if (count == numSplits) {
      break;
    }
    count++;
    tmp += delim_len;
  }

  DString **result = malloc((count + 1) * sizeof(DString *));
  if (!result)
    return NULL;

  int i = 0;
  char *start = str->str;
  char *end;

  while ((end = strstr(start, delimiter)) != NULL) {
    if (i == count - 1) {
      break;
    }
    char temp = end[0];
    end[0] = '\0';
    DString *newStr = d_str_new(start);

    result[i] = newStr;
    end[0] = temp;

    i++;
    start = end + delim_len;
  }

  result[i] = d_str_new(start);

  result[i + 1] = NULL;

  return result;
}

void d_str_free_multiple(DString **parts) {
  if (!parts)
    return;
  DString **temp = parts;
  while (*temp != NULL) {
    d_str_free(*temp);
    temp++;
  }
  free(parts);
}
