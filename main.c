#include "server.h"

int main() {
  Server *server = createServer(8080);
  serverListen(server);
  stopListening(server);
  return 0;
}
