/* thread_win32.c */

#define WIN32_LEAN_AND_MEAN 1
#define _WIN32_WINNT 0x0600

#include <windows.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "thread.h"
#include "queue.h"

struct THREAD {
  HANDLE handle;
  DWORD id;
  void (*func)(void *data);
  void *func_data;
};

struct CHANNEL {
  CRITICAL_SECTION cs;
  CONDITION_VARIABLE write_event;
  CONDITION_VARIABLE read_event;
  struct QUEUE queue;
  void *data;
  bool has_reader;
  bool has_writer;
};

void thread_sleep(unsigned int msec)
{
  Sleep(msec);
}

static DWORD WINAPI thread_start_func(void *data)
{
  struct THREAD *thread = data;
  thread->func(thread->func_data);
  return 0;
}

struct THREAD *start_thread(void (*func)(void *data), void *data)
{
  struct THREAD *thread = malloc(sizeof *thread);
  if (! thread)
    return NULL;
  thread->func = func;
  thread->func_data = data;
  thread->handle = CreateThread(NULL, 0, thread_start_func, thread, 0, &thread->id);
  if (thread->handle == NULL) {
    free(thread);
    return NULL;
  }
  return thread;
}

int join_thread(struct THREAD *thread)
{
  int ret = WaitForSingleObject(thread->handle, INFINITE);
  CloseHandle(thread->handle);
  free(thread);
  return (ret == WAIT_OBJECT_0) ? 0 : 1;
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
  
  InitializeConditionVariable(&chan->read_event);
  InitializeConditionVariable(&chan->write_event);
  if (! InitializeCriticalSectionAndSpinCount(&chan->cs, 1024))
    goto err;

  return chan;

 err:
  free(chan->data);
  free_queue(&chan->queue);
  free(chan);
  return NULL;
}

void free_chan(struct CHANNEL *chan)
{
  DeleteCriticalSection(&chan->cs);
  free_queue(&chan->queue);
  free(chan->data);
  free(chan);
}

static void sync_send(struct CHANNEL *chan, void *data)
{
  //printf(" sync send for %p\n", chan);
  EnterCriticalSection(&chan->cs);
  chan->has_writer = true;
  memcpy(chan->data, data, chan->queue.item_size);
  if (chan->has_reader)
    WakeConditionVariable(&chan->write_event);
  SleepConditionVariableCS(&chan->read_event, &chan->cs, INFINITE);
  LeaveCriticalSection(&chan->cs);
}

static int sync_recv(struct CHANNEL *chan, void *data, int block)
{
  //printf(" sync recv for %p\n", chan);
  EnterCriticalSection(&chan->cs);

  if (! block && ! chan->has_writer) {
    LeaveCriticalSection(&chan->cs);
    return 1;
  }
  
  chan->has_reader = true;
  while (! chan->has_writer)
    SleepConditionVariableCS(&chan->write_event, &chan->cs, INFINITE);
  chan->has_reader = false;
  chan->has_writer = false;
  memcpy(data, chan->data, chan->queue.item_size);
  memset(chan->data, 0, chan->queue.item_size);

  WakeConditionVariable(&chan->read_event);
  LeaveCriticalSection(&chan->cs);
  return 0;
}

static void async_send(struct CHANNEL *chan, void *data)
{
  //printf("async send for %p\n", chan);
  EnterCriticalSection(&chan->cs);
    
  chan->has_writer = true;
  while (chan->queue.num_items == chan->queue.capacity)
    SleepConditionVariableCS(&chan->read_event, &chan->cs, INFINITE);
  chan->has_writer = false;
  
  queue_add(&chan->queue, data);
  if (chan->has_reader)
    WakeConditionVariable(&chan->write_event);
  LeaveCriticalSection(&chan->cs);
}

static int async_recv(struct CHANNEL *chan, void *data, int block)
{
  //printf("async recv for %p\n", chan);
  EnterCriticalSection(&chan->cs);

  if (! block && chan->queue.num_items == 0) {
    LeaveCriticalSection(&chan->cs);
    return 1;
  }
  
  chan->has_reader = true;
  while (chan->queue.num_items == 0)
    SleepConditionVariableCS(&chan->write_event, &chan->cs, INFINITE);
  chan->has_reader = false;

  queue_remove(&chan->queue, data);
  if (chan->has_writer)
    WakeConditionVariable(&chan->read_event);
  LeaveCriticalSection(&chan->cs);
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
