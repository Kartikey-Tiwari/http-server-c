#ifndef SERVER_H
#define SERVER_H

typedef struct Server {
  int sockfd;
  int port;
} Server;

Server *createServer(int port);

void serverListen(Server *server);

void stopListening(Server *server);

#endif
