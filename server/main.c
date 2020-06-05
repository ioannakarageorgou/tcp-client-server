#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include "common.h"
#include "utils.h"

ListNode *clients = NULL;

int main(int argc, char *argv[]) {
    int port;
    if (strcmp(argv[1], "-p") == 0) {
        port = atoi(argv[2]);
    } else {
        printf("Please give port number\n");
        exit(-1);
    }


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

    //----------------------------------------------------------

    //initialize signals
    struct sigaction sa;
    sa.sa_handler = my_signal_handler;
    sigfillset(&(sa.sa_mask));

    sigaction(SIGINT, &sa, NULL);

    //----------------------------------------------------------

    extern int make_socket(uint16_t port);
    int sock;
    fd_set active_fd_set, read_fd_set;
    int i;
    struct sockaddr_in clientname;
    size_t size;

    /* Create the socket and set it up to accept connections. */
    sock = make_socket(port);
    if (listen(sock, 1) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Initialize the set of active sockets. */
    FD_ZERO (&active_fd_set);
    FD_SET (sock, &active_fd_set);

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
                if (i == sock) {
                    /* Connection request on original socket. */
                    int new;
                    size = sizeof(clientname);
                    new = accept(sock, (struct sockaddr *) &clientname, &size);

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
                    if (read_from_client(i) < 0) {
                        close(i);
                        FD_CLR (i, &active_fd_set);
                    }
                }
            }
    }
}