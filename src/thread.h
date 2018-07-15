/* thread.h */

#ifndef THREAD_H_FILE
#define THREAD_H_FILE

struct CHANNEL;

struct CHANNEL *new_chan(size_t capacity, size_t item_size);
void free_chan(struct CHANNEL *chan);
void chan_send(struct CHANNEL *chan, void *data);
int chan_recv(struct CHANNEL *chan, void *data, int block);

struct THREAD;

struct THREAD *start_thread(void (*func)(void *data), void *data);
int join_thread(struct THREAD *thread);
void thread_sleep(unsigned int msec);

#endif /* THREAD_H_FILE */
