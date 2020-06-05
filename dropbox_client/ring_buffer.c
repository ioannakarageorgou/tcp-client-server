#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ring_buffer.h"
#include "utils.h"

char *new_str(const char *a) {
    size_t len = strlen(a);

    char *b = (char *) malloc(len * sizeof(char));
    strcpy(b, a);
    return b;
}

// initialize the queue
void ringbuf_init(rbuf_t *_this) {
    // clear the _thisfer and init the values
    // and sets head = tail in one go
//    memset( _this, 0, sizeof(*_this) );
    _this->count = 0;
    _this->head = 0;
    _this->tail = 0;

    _this->buf = malloc(RBUF_SIZE * sizeof(bufferData));   //buf[RBUF_SIZE]
}

// determine if the queue is empty
bool ringbuf_empty(rbuf_t *_this) {
    // test if the queue is empty
    // 0 returns true
    // nonzero false
    return (0 == _this->count);
}

// determine if the queue is full
bool ringbuf_full(rbuf_t *_this) {
    // full when no of elements exceed the max size
    return (_this->count >= RBUF_SIZE);
}

// fetch an item from the queue at tail
bufferData *ringbuf_get(rbuf_t *_this) {
    bufferData *item = malloc(sizeof(bufferData));
    if (_this->count > 0) {
        // get item element
        item = &_this->buf[_this->tail];
        // advance the tail
        _this->tail = ringbuf_adv(_this->tail, RBUF_SIZE);
        // reduce the total count
        --_this->count;
    } else {
        // the queue is empty
        strcpy(item->pathname, "RBUF_EMPTY");
    }
    return item;
}

// insert an item to the queue at head
void ringbuf_put(rbuf_t *_this, bufferData *item) {
    if (_this->count < RBUF_SIZE) {
        // set the item at head position
        _this->buf[_this->head] = *item;
        // advance the head
        _this->head = ringbuf_adv(_this->head, RBUF_SIZE);
        // increase the total count
        ++_this->count;
        debug_log("RING_BUFFER: just inserted->%s|%d", item->IPaddress, item->portNum)
    } else {
        //found full buffer...
        debug_log("FULL...%s", item->pathname)
    }
}

// peek at the first element in the queue
bufferData *ringbuf_peek(rbuf_t *_this) {
    if (_this->count != 0) {
        return &_this->buf[_this->tail];
    }
}

// print the contents
void ringbuf_print(rbuf_t *_this) {
    for (int i = 0; i < RBUF_SIZE; i++) {
        debug_log("%s ", _this->buf[i].pathname)
    }
    printf("\n");
}

// flush the queue and clear the buffer
void ringbuf_flush(rbuf_t *_this, rbuf_opt_e clear) {
    for (int i = 0; i < _this->count; i++) {
        free(&_this->buf[i]);
    }
    free(_this->buf);

    _this->count = 0;
    _this->head = 0;
    _this->tail = 0;
}

// advance the ring buffer index
static unsigned int ringbuf_adv(const unsigned int value, const unsigned int max) {
    unsigned int index = value + 1;
    if (index >= max) {
        index = 0;
    }
    return index;
}


