/* queue.h */

#ifndef QUEUE_H_FILE
#define QUEUE_H_FILE

struct QUEUE {
  size_t capacity;
  size_t first_item;
  size_t num_items;
  size_t item_size;
  void **data;
};

int new_queue(struct QUEUE *queue, size_t capacity, size_t item_size);
void free_queue(struct QUEUE *queue);
int queue_add(struct QUEUE *queue, void *data);
int queue_remove(struct QUEUE *queue, void *data);

#endif /* QUEUE_H_FILE */
