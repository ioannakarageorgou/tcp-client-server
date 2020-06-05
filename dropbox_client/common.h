//
// Created by jo on 14/5/2019.
//

#ifndef DROPBOX_CLIENT_COMMON_H
#define DROPBOX_CLIENT_COMMON_H

#include "structures.h"
#include "list.h"
#include "ring_buffer.h"
#include <unistd.h>
#include <pthread.h>

#define perror2(s,e) fprintf(stderr, "%s: %s\n", s, strerror(e))

#define MAXMSG  1024


extern ListNode* client_list;
extern options opt;
extern bool running;

extern pthread_mutex_t list_mtx;
extern pthread_mutex_t mtx;
extern pthread_cond_t cond_nonempty;
extern pthread_cond_t cond_nonfull;
extern pthread_t * tids;
//extern volatile sig_atomic_t sig_int = 0;

void read_options(int argc, char **argv);
void send_LOG_ON_to_server();
void send_GET_CLIENTS_to_server();
int read_from_socket(int filedes);
void checkIPbuffer(char *IPbuffer);
void checkHostEntry(struct hostent * hostentry);
void checkHostName(int hostname);
void* workerThreads(void *ptr);
void my_signal_handler(int sig);

#endif //DROPBOX_CLIENT_COMMON_H
