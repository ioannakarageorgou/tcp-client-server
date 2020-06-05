//
// Created by jo on 16/5/2019.
//

#ifndef UNTITLED_COMMON_H
#define UNTITLED_COMMON_H

#include "structures.h"
#include "list.h"

#define MAXMSG  512

extern ListNode * clients;

int make_socket (uint16_t port);
int read_from_client (int filedes);
void checkHostName(int hostname);
void checkHostEntry(struct hostent * hostentry);
void checkIPbuffer(char *IPbuffer);
void my_signal_handler(int sig);

#endif //UNTITLED_COMMON_H
