#include "clientqueue.h"
#include <unistd.h>

void init_queue(ClientQueue *q) {
  q->head = 0;
  q->tail = 0;
  q->count = 0;
  pthread_mutex_init(&q->lock, NULL);
  pthread_cond_init(&q->has_work, NULL);
}

int pop_queue(ClientQueue *q) {
  pthread_mutex_lock(&q->lock);

  while (q->count == 0) {
    pthread_cond_wait(&q->has_work, &q->lock);
  }

  int client_socket = q->sockets[q->head];
  q->head = (q->head + 1) % MAX_QUEUE;
  q->count--;

  pthread_mutex_unlock(&q->lock);

  return client_socket;
}

void push_queue(ClientQueue *q, int client_socket) {
  pthread_mutex_lock(&q->lock);

  if (q->count < MAX_QUEUE) {
    q->sockets[q->tail] = client_socket;
    q->tail = (q->tail + 1) % MAX_QUEUE;
    q->count++;

    pthread_cond_signal(&q->has_work);
  } else {
    close(client_socket);
  }

  pthread_mutex_unlock(&q->lock);
}
