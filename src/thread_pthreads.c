/* thread_pthreads.c */

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "thread.h"
#include "queue.h"

struct THREAD {
  pthread_t thread;
  void (*func)(void *data);
  void *func_data;
};

struct CHANNEL {
  pthread_mutex_t mutex;
  pthread_cond_t write_event;
  pthread_cond_t read_event;
  struct QUEUE queue;
  void *data;
  bool has_reader;
  bool has_writer;
};

void thread_sleep(unsigned int msec)
{
  usleep((useconds_t)msec * 1000);
}

static void *thread_start_func(void *data)
{
  struct THREAD *thread = data;
  thread->func(thread->func_data);
  return NULL;
}

struct THREAD *start_thread(void (*func)(void *data), void *data)
{
  struct THREAD *thread = malloc(sizeof *thread);
  if (! thread)
    return NULL;
  thread->func = func;
  thread->func_data = data;
  if (pthread_create(&thread->thread, NULL, thread_start_func, thread) != 0) {
    free(thread);
    return NULL;
  }
  return thread;
}

int join_thread(struct THREAD *thread)
{
  int ret = pthread_join(thread->thread, NULL);
  free(thread);
  return (ret == 0) ? 0 : 1;
}

struct CHANNEL *new_chan(size_t capacity, size_t item_size)
{
  struct CHANNEL *chan = malloc(sizeof *chan);
  if (! chan)
    return NULL;
  chan->has_reader = false;
  chan->has_writer = false;

  if (new_queue(&chan->queue, capacity, item_size) != 0)
    goto err;

  chan->data = malloc(item_size);
  if (! chan->data)
    goto err;

  if (pthread_mutex_init(&chan->mutex, NULL) != 0)
    goto err;

  if (pthread_cond_init(&chan->read_event, NULL) != 0) {
    pthread_mutex_destroy(&chan->mutex);
    goto err;
  }

  if (pthread_cond_init(&chan->write_event, NULL) != 0) {
    pthread_cond_destroy(&chan->read_event);
    pthread_mutex_destroy(&chan->mutex);
    goto err;
  }
  
  return chan;

 err:
  free(chan->data);
  free_queue(&chan->queue);
  free(chan);
  return NULL;
}

void free_chan(struct CHANNEL *chan)
{
  pthread_cond_destroy(&chan->write_event);
  pthread_cond_destroy(&chan->read_event);
  pthread_mutex_destroy(&chan->mutex);
  free_queue(&chan->queue);
  free(chan->data);
  free(chan);
}

static void sync_send(struct CHANNEL *chan, void *data)
{
  //printf(" sync send for %p\n", chan);
  pthread_mutex_lock(&chan->mutex);
  chan->has_writer = true;
  memcpy(chan->data, data, chan->queue.item_size);
  if (chan->has_reader)
    pthread_cond_signal(&chan->write_event);
  pthread_cond_wait(&chan->read_event, &chan->mutex);
  pthread_mutex_unlock(&chan->mutex);
}

static int sync_recv(struct CHANNEL *chan, void *data, int block)
{
  //printf(" sync recv for %p\n", chan);
  pthread_mutex_lock(&chan->mutex);

  if (! block && ! chan->has_writer) {
    pthread_mutex_unlock(&chan->mutex);
    return 1;
  }
  
  chan->has_reader = true;
  while (! chan->has_writer)
    pthread_cond_wait(&chan->write_event, &chan->mutex);
  chan->has_reader = false;
  chan->has_writer = false;
  memcpy(data, chan->data, chan->queue.item_size);
  memset(chan->data, 0, chan->queue.item_size);

  pthread_cond_signal(&chan->read_event);
  pthread_mutex_unlock(&chan->mutex);
  return 0;
}

static void async_send(struct CHANNEL *chan, void *data)
{
  //printf("async send for %p\n", chan);
  pthread_mutex_lock(&chan->mutex);
    
  chan->has_writer = true;
  while (chan->queue.num_items == chan->queue.capacity)
    pthread_cond_wait(&chan->read_event, &chan->mutex);
  chan->has_writer = false;
  
  queue_add(&chan->queue, data);
  if (chan->has_reader)
    pthread_cond_signal(&chan->write_event);
  pthread_mutex_unlock(&chan->mutex);
}

static int async_recv(struct CHANNEL *chan, void *data, int block)
{
  //printf("async recv for %p\n", chan);
  pthread_mutex_lock(&chan->mutex);

  if (! block && chan->queue.num_items == 0) {
    pthread_mutex_unlock(&chan->mutex);
    return 1;
  }
  
  chan->has_reader = true;
  while (chan->queue.num_items == 0)
    pthread_cond_wait(&chan->write_event, &chan->mutex);
  chan->has_reader = false;

  queue_remove(&chan->queue, data);
  if (chan->has_writer)
    pthread_cond_signal(&chan->read_event);
  pthread_mutex_unlock(&chan->mutex);
  return 0;
}

void chan_send(struct CHANNEL *chan, void *data)
{
  if (chan->queue.capacity == 0)
    sync_send(chan, data);
  else
    async_send(chan, data);
}

int chan_recv(struct CHANNEL *chan, void *data, int block)
{
  if (chan->queue.capacity == 0)
    return sync_recv(chan, data, block);
  else
    return async_recv(chan, data, block);
}
