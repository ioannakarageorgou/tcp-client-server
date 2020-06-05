#ifndef DROPBOX_CLIENT_RING_BUFFER_H
#define DROPBOX_CLIENT_RING_BUFFER_H
//source: https://gist.github.com/hosaka/985abd49cd737479a5fc

#include <stdbool.h>
#include "common.h"

//#define RBUF_SIZE 12
extern int RBUF_SIZE;

// buffer structure
typedef struct ring_buf_s
{
    bufferData* buf;
    int head;         // new data is written at this position in the buffer
    int tail;         // data is read from this position in the buffer
    int count;        // total number of elements in the queue <= RBUF_SIZE
} rbuf_t;

extern rbuf_t ring_buffer;

// ring buffer options
typedef enum
{
    RBUF_CLEAR,
    RBUF_NO_CLEAR
} rbuf_opt_e;

// buffer messages
typedef enum
{
    RBUF_EMPTY = -1,
    RBUF_FULL
} rbuf_msg_e;

// API

char *new_str(const char *a);

// initialize the queue
void ringbuf_init(rbuf_t* _this);

// determine if the queue is empty
bool ringbuf_empty(rbuf_t* _this);

// determine if the queue is full
bool ringbuf_full(rbuf_t* _this);

// fetch an item from the queue at tail
bufferData* ringbuf_get(rbuf_t* _this);

// insert an item to the queue at head
void ringbuf_put(rbuf_t* _this, bufferData* item);

// peek at the first element in the queue
bufferData* ringbuf_peek(rbuf_t* _this);

// flush the queue and clear the buffer
void ringbuf_flush(rbuf_t* _this, rbuf_opt_e clear);

// print the contents
void ringbuf_print(rbuf_t* _this);

// advance the ring buffer index
static unsigned int ringbuf_adv (const unsigned int value, const unsigned int max_val);


#endif //DROPBOX_CLIENT_RING_BUFFER_H
