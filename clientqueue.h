#ifndef CLIENTQUEUE_H
#define CLIENTQUEUE_H
#include <pthread.h>
#define MAX_QUEUE 1024

typedef struct {
  int sockets[MAX_QUEUE];
  int head;
  int tail;
  int count;

  pthread_mutex_t lock;
  pthread_cond_t has_work;
} ClientQueue;

void init_queue(ClientQueue *q);
int pop_queue(ClientQueue *q);
void push_queue(ClientQueue *q, int client_socket);

#endif
