#include "response.h"
#include "utils.h"
#include <string.h>
#include <sys/socket.h>

void writeStatusLine(int fd, StatusCode status) {
  char *msg;
  switch (status) {
  case OK:
    msg = "HTTP/1.1 200 OK\r\n";
    break;
  case BAD_REQUEST:
    msg = "HTTP/1.1 400 Bad Request\r\n";
    break;
  case SERVER_ERROR:
  default:
    msg = "HTTP/1.1 500 Internal Server Error\r\n";
  }
  safeSend(fd, msg, strlen(msg));
}

GHashTable *getDefaultHeaders(int contentLen) {
  GHashTable *headers =
      g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

  g_hash_table_insert(headers, g_strdup("Content-Length"),
                      g_strdup_printf("%d", contentLen));
  g_hash_table_insert(headers, g_strdup("Connection"), g_strdup("close"));
  g_hash_table_insert(headers, g_strdup("Content-Type"),
                      g_strdup("text/plain"));

  return headers;
}

void writeHeader(gpointer key, gpointer value, gpointer fd) {
  GString *header = g_string_new((char *)key);
  g_string_append(header, ": ");
  g_string_append(header, (char *)value);
  g_string_append(header, "\r\n");

  safeSend(*(int *)fd, header->str, header->len);
  g_string_free(header, TRUE);
}

void writeHeaders(int fd, GHashTable *headers) {
  g_hash_table_foreach(headers, writeHeader, &fd);
  safeSend(fd, "\r\n", 2);
}
