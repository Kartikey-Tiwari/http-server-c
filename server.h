#ifndef SERVER_H
#define SERVER_H

#include "request.h"
#include "response.h"
typedef struct Server {
  int sockfd;
  int port;
} Server;

Server *createServer(int port);

void serverListen(Server *server, void (*)(Request *, Response *));

void stopListening(Server *server);

#endif
