#include <arpa/inet.h>
#include <glib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "parser.h"
#include "request.h"
#include "utils.h"

int main() {
  int sockfd, rv;
  struct sockaddr_in address;

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(8080);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == 0) {
    perror("socket: couldn't open");
    exit(1);
  }
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (bind(sockfd, (struct sockaddr *)&address, sizeof address) < 0) {
    perror("bind: failed");
    exit(1);
  }

  if (listen(sockfd, 10) < 0) {
    perror("listen: failed");
    exit(1);
  }

  struct sockaddr_storage their_addr;
  socklen_t their_addr_size = sizeof their_addr;
  int new_fd;
  char s[INET6_ADDRSTRLEN];

  while (1) {
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &their_addr_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) {
      close(sockfd);

      char buf[9];
      buf[8] = '\0';

      int bytesReceived, lookForStartingLF = 0;
      GString *line = g_string_new(NULL);
      Request req = {0};
      req.state = INITIALIZED;
      req.headers =
          g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

      while ((bytesReceived = recv(new_fd, buf, 8, 0)) >= 0) {
        int rv = bytesReceived;
        buf[rv] = '\0';

        ParseAction action = parse(buf, &rv, &lookForStartingLF, line, &req);
        while (action == ACTION_CONTINUE) {
          action = parse(buf, &rv, &lookForStartingLF, line, &req);
        }

        if (action == ACTION_DONE || action == ACTION_ERROR) {
          break;
        }
      }
      if (req.state == DONE) {
        printRequest(&req);
      }
      g_string_free(line, TRUE);
      g_free(req.body);
      g_hash_table_destroy(req.headers);
      req.headers = NULL;
      g_free(req.rql.method);
      g_free(req.rql.resource);
      g_free(req.rql.http);
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }

  close(sockfd);
  exit(0);
  return 0;
}
