#include "server.h"
#include "headers.h"
#include "parser.h"
#include "request.h"
#include "response.h"
#include "utils.h"

#include <arpa/inet.h>
#include <asm-generic/errno-base.h>
#include <errno.h>
#include <glib.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t keep_running = 1;

void sigint_handler(int sig) {
  (void)sig;
  printf("\n[Server] Shutdown signal received. Stopping...\n");
  keep_running = 0;
}

void sigchld_handler(int s) {
  (void)s;

  int saved_errno = errno;

  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;

  errno = saved_errno;
}

Server *createServer(int port) {
  struct sigaction sa_int, sa_chld;
  sa_int.sa_handler = sigint_handler;
  sigemptyset(&sa_int.sa_mask);
  sa_int.sa_flags = 0;
  if (sigaction(SIGINT, &sa_int, NULL) == -1) {
    perror("sigaction sigint");
    exit(1);
  }

  sa_chld.sa_handler = sigchld_handler;
  sigemptyset(&sa_chld.sa_mask);
  sa_chld.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
    perror("sigaction sigchld");
    exit(1);
  }

  Server *server = malloc(sizeof(Server));
  int sockfd;
  struct sockaddr_in address;

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = htonl(INADDR_ANY);
  address.sin_port = htons(port);

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd == 0) {
    perror("socket: couldn't open");
    exit(1);
  }

  server->sockfd = sockfd;
  int opt = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
  if (bind(sockfd, (struct sockaddr *)&address, sizeof address) < 0) {
    perror("bind: failed");
    exit(1);
  }
  server->port = port;
  return server;
}

Request *readRequestFromClient(int new_fd) {
  char buf[9];
  buf[8] = '\0';

  int bytesReceived, lookForStartingLF = 0;
  GString *line = g_string_new(NULL);
  Request *req = malloc(sizeof(Request));
  req->state = INITIALIZED;
  req->headers = createHeaders();

  while (keep_running) {
    bytesReceived = recv(new_fd, buf, 8, 0);
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

    ParseAction action = parse(buf, &rv, &lookForStartingLF, line, req);
    while (action == ACTION_CONTINUE) {
      action = parse(buf, &rv, &lookForStartingLF, line, req);
    }

    if (action == ACTION_DONE || action == ACTION_ERROR) {
      break;
    }
  }
  if (req->state == ERROR_STATE) {
    writeStatusLine(new_fd, BAD_REQUEST);
    Headers *headers = getDefaultHeaders(0);
    writeHeaders(new_fd, headers);
    headers_free(headers);
    close(new_fd);
    exit(0);
  } else if (!keep_running) {
    if (req->state == INITIALIZED) {
      printf("[Child %d] Idle connection closed gracefully.\n", getpid());
      writeStatusLine(new_fd, SERVER_ERROR);
      Headers *headers = getDefaultHeaders(0);
      writeHeaders(new_fd, headers);
      headers_free(headers);
      close(new_fd);
      exit(0);
    } else if (req->state != DONE) {
      printf("[Child %d] Interrupted mid-request. Sending 503...\n", getpid());

      writeStatusLine(new_fd, SERVER_ERROR);
      Headers *headers = getDefaultHeaders(0);
      writeHeaders(new_fd, headers);
      headers_free(headers);
      close(new_fd);
      exit(0);
    }
  }
  g_string_free(line, TRUE);

  if (req->state == DONE) {
    return req;
  } else {
    return NULL;
  }
}

void serverListen(Server *server) {
  if (listen(server->sockfd, 10) < 0) {
    perror("listen: failed");
    exit(1);
  }

  struct sockaddr_storage their_addr;
  socklen_t their_addr_size = sizeof their_addr;
  int new_fd;
  char s[INET6_ADDRSTRLEN];

  while (keep_running) {
    new_fd = accept(server->sockfd, (struct sockaddr *)&their_addr,
                    &their_addr_size);
    if (new_fd == -1) {
      if (errno == EINTR) {
        break;
      }
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("server: got connection from %s\n", s);

    if (!fork()) {
      close(server->sockfd);

      Request *req = readRequestFromClient(new_fd);
      int status = BAD_REQUEST;
      if (req->state == DONE) {
        status = OK;
        printRequest(req);
      }
      writeStatusLine(new_fd, status);
      Headers *headers = getDefaultHeaders(0);
      writeHeaders(new_fd, headers);
      headers_free(headers);

      freeRequest(req);
      close(new_fd);
      exit(0);
    }
    close(new_fd);
  }
}

void stopListening(Server *server) {
  printf("server stopped");
  close(server->sockfd);
  exit(0);
}
