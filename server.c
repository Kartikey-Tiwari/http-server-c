#include "server.h"
#include "headers.h"
#include "parser.h"
#include "request.h"
#include "response.h"
#include "utils.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t keep_running = 1;

typedef struct WorkerArgs {
  int new_fd;
  void (*handler)(Request *req, Response *res);
} WorkerArgs;

void *threadFunc(void *arg) {
  WorkerArgs *workerArgs = (WorkerArgs *)arg;
  int new_fd = workerArgs->new_fd;
  void (*handler)(Request *req, Response *res) = workerArgs->handler;

  Response *res = createResponse(new_fd);

  Request *req = createRequest();
  ReturnCode rc = readRequestFromClient(new_fd, &req, &keep_running);

  if (req->state == REQUEST_ERROR || rc == INTERRUPTED) {
    StatusCode status = OK;
    switch (rc) {
    case REQUEST_METHOD_ERROR:
    case REQUEST_INVALID_CHAR_ERROR:
    case HEADER_FORMAT_ERROR:
    case BODY_LENGTH_MISMATCH:
      if (rc == REQUEST_METHOD_ERROR) {
        printf("request method error\n");
      } else if (rc == REQUEST_INVALID_CHAR_ERROR) {
        printf("request invalid character error\n");
      } else if (rc == HEADER_FORMAT_ERROR) {
        printf("Header format error\n");
      } else if (rc == BODY_LENGTH_MISMATCH) {
        printf("body length mismatch\n");
      }
      printf("bad request!\n");
      status = BAD_REQUEST;
      break;
    case REQUEST_HTTP_VER_ERROR:
      printf("http version not supported\n");
      status = HTTP_UNSUPPORTED_VER;
      break;
    case INTERRUPTED:
      printf("server interrupted\n");
      status = SERVER_ERROR;
      break;
    default:;
    }
    writeStatusLine(res, status);
    setHeader(res->headers, d_str_new("Content-Type"),
              d_str_new("text/html"));
    char *body = httpCodeToHTML(status);
    int bodyLen = strlen(body);
    char *len = itoa(bodyLen);
    setHeader(res->headers, d_str_new("Content-Length"), d_str_new(len));
    free(len);
    writeHeaders(res);
    writeBody(res, body, bodyLen);
    responseEnd(res);
  } else {
    if (req->state == DONE) {
      printRequest(req);
      handler(req, res);
      if (res->state != RESPONSE_DONE) {
        responseEnd(res);
      }
    }
  }
  freeRequest(req);
  freeResponse(res);
  free(workerArgs);
  return NULL;
}

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
  struct sigaction sa_int, sa_chld, sa_pipe;
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

  sa_pipe.sa_handler = SIG_IGN;
  sigemptyset(&sa_pipe.sa_mask);
  sa_pipe.sa_flags = 0;
  if (sigaction(SIGPIPE, &sa_pipe, NULL) == -1) {
    perror("sigaction sigpipe");
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

void serverListen(Server *server,
                  void (*handler)(Request *req, Response *res)) {
  if (listen(server->sockfd, 1024) < 0) {
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
      break;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              s, sizeof s);
    printf("\n[Server]: got connection from %s\n", s);

    WorkerArgs *args = malloc(sizeof(WorkerArgs));
    args->new_fd = new_fd;
    args->handler = handler;

    pthread_t thread;
    if (pthread_create(&thread, NULL, threadFunc, args) != 0) {
      perror("pthread_create");
      free(args);
      close(new_fd);
    } else {
      pthread_detach(thread);
    }
  }
}

void stopListening(Server *server) {
  printf("server stopped\n");
  close(server->sockfd);
  exit(0);
}
