/* queue.c */

#include <stdlib.h>
#include <string.h>

#include "queue.h"

int new_queue(struct QUEUE *queue, size_t capacity, size_t item_size)
{
  if (capacity) {
    queue->data = malloc(sizeof(void *) * capacity * item_size);
    if (! queue->data)
      return 1;
  } else {
    queue->data = NULL;
  }
  queue->capacity = capacity;
  queue->item_size = item_size;
  queue->first_item = 0;
  queue->num_items = 0;
  return 0;
}

void free_queue(struct QUEUE *queue)
{
  free(queue->data);
}

int queue_add(struct QUEUE *queue, void *data)
{
  if (queue->num_items >= queue->capacity)
    return 1;
  
  size_t item_pos = (queue->first_item + queue->num_items) % queue->capacity;
  memcpy((char *)queue->data + item_pos * queue->item_size, data, queue->item_size);
  queue->num_items++;
  return 0;
}

int queue_remove(struct QUEUE *queue, void *data)
{
  if (queue->num_items == 0) {
    memset(data, 0, queue->item_size);
    return 1;
  }
  memcpy(data, (char *)queue->data + queue->first_item * queue->item_size, queue->item_size);
  queue->first_item = (queue->first_item + 1) % queue->capacity;
  queue->num_items--;
  return 0;
}
