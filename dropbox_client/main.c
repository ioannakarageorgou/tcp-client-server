#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <signal.h>
#include "common.h"
#include "ring_buffer.h"
#include "utils.h"

//valgrind --tool=memcheck --leak-check=yes --show-reachable=yes --num-callers=20 --track-fds=yes --fair-sched=yes

ListNode *client_list = NULL;
options opt;
int RBUF_SIZE = 0;
rbuf_t ring_buffer;
bool running = true;

pthread_mutex_t list_mtx;
pthread_mutex_t mtx;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
pthread_t *tids;
//volatile sig_atomic_t sig_int = 0;

int main(int argc, char **argv) {

    read_options(argc, argv);

    char hostbuffer[256];
    char *IPbuffer;
    struct hostent *host_entry;
    int hostname;

    // To retrieve hostname
    hostname = gethostname(hostbuffer, sizeof(hostbuffer));
    checkHostName(hostname);

    // To retrieve host information
    host_entry = gethostbyname(hostbuffer);
    checkHostEntry(host_entry);

    // To convert an Internet network
    // address into ASCII string
    IPbuffer = inet_ntoa(*((struct in_addr *)
            host_entry->h_addr_list[0]));

    printf("Hostname: %s\n", hostbuffer);
    printf("Host IP: %s\n", IPbuffer);
    strcpy(opt.myIP, IPbuffer);

    //----------------------------------------------------------------
    //initialize the ring buffer
    ringbuf_init(&ring_buffer);

    //initialize mutexes
    pthread_mutex_init(&list_mtx, 0);
    pthread_mutex_init(&mtx, 0);
    pthread_cond_init(&cond_nonempty, 0);
    pthread_cond_init(&cond_nonfull, 0);

    //create a directory to store peer's files
    int dir = mkdir("PeerDir", 0777);

    //initialize signals
    struct sigaction sa;
    sa.sa_handler = my_signal_handler;
    sigfillset(&(sa.sa_mask));

    sigaction(SIGINT, &sa, NULL);

    //-----------------------------------------------------------------

    //STEP 1
    send_LOG_ON_to_server();

//    //STEP 2
//    send_GET_CLIENTS_to_server();

    //-----------------------------------------------------------------

    int err;


    if ((tids = malloc(opt.workerThreads * sizeof(pthread_t))) == NULL) {
        perror("malloc");
        exit(1);
    }

    for (int i = 0; i < opt.workerThreads; i++) {
        if (err = pthread_create(tids + i, NULL, workerThreads, 0)) {
            /* Create a thread */
            perror2("pthread_create", err);
            exit(1);
        }
    }

    //----------------------------------------------------------------

    //setting up own port for listening incoming connections
    extern int make_socket(uint16_t port);
    int listen_sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    struct sockaddr_in clientname;
    size_t size;

    /* Create the socket and set it up to accept connections. */
    listen_sock = make_socket(opt.portNum);
    if (listen(listen_sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (listen_sock, &active_fd_set);

    //STEP 2
    send_GET_CLIENTS_to_server();


    while (1) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(EXIT_FAILURE);
        }

        /* Service all the sockets with input pending. */
        for (i = 0; i < FD_SETSIZE; ++i)
            if (FD_ISSET (i, &read_fd_set)) {
                if (i == listen_sock) {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(listen_sock,
                                 (struct sockaddr *) &clientname,
                                 &size);

                    printf("Connection Established\n");
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &(clientname.sin_addr), ip, INET_ADDRSTRLEN);
                    printf("connection established with IP : %s and PORT : %d\n", ip, ntohs(clientname.sin_port));
                    debug_log("connection established with IP : %s and PORT : %d\n", ip, ntohs(clientname.sin_port))


                    if (new < 0) {
                        perror("accept");
                        exit(EXIT_FAILURE);
                    }
                    fprintf(stderr,
                            "Server: connect from host %s, port %hd.\n",
                            inet_ntoa(clientname.sin_addr),
                            ntohs(clientname.sin_port));
                    FD_SET (new, &active_fd_set);
                } else {
                    /* Data arriving on an already-connected socket. */
                    if (read_from_socket(i) < 0) {
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }

}