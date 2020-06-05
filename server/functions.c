#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "common.h"
#include "list.h"
#include "utils.h"

char * makeTuplesOf2(Data *d)
{
    char port[80];
    sprintf(port,"%d",htons(d->portNum));

//    char ipstr2[INET_ADDRSTRLEN];
//    inet_pton(AF_INET, d->IPaddress, ipstr2);

    char* msg = malloc(256*sizeof(char));
    sprintf(msg," <%s,%s>",d->IPaddress,port);

    return msg;
}

void write_to_client(int filedes, char *buffer) {
    int nbytes;

    nbytes = write(filedes, buffer, strlen(buffer) + 1);
    if (nbytes < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Server: send message: `%s'\n", buffer);
}


void server_Get_Clients(int filedes)
{
    //N = num of tuples = num of connected clients(himself inside)
    int N = sizeL(clients);
    char msg[1024];
    sprintf(msg,"CLIENT_LIST %d",N);

    for(ListNode* i = clients; i!=NULL; i=i->next){
        // make a tuple for each connected client in the list
        Data* d = i->data;
        char * tuple = makeTuplesOf2(d);
        strcat(msg,tuple);
        free(tuple);            // ?????????????????????? not sure
    }

    //send the msg back
    write_to_client(filedes, msg);
}


Data *takeClientInfo(char *buffer) {
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    printf("%s", buffer);

    char *token2 = strtok(buffer, ">");
    char *token1 = strtok(buffer, ",");
    token1++;
    token2 = strtok(NULL, ",");

    Data *d = malloc(sizeof(Data));


//    char ip[INET_ADDRSTRLEN];
//    inet_ntop(AF_INET, token1, ip, sizeof(ip));

    strcpy(d->IPaddress, token1);
    d->portNum = atoi(token2);
    d->portNum = ntohs(d->portNum);
    printf(" ip= %s , port= %d\n", token1, d->portNum);
    return d;
}

void send_ON_OFF_message(Data *to_client, Data *new_client, int flag) {
    //server must connect as client to the receiver client
    //making new session fow every client
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);

    int sock;
    struct sockaddr_in servername;
    //create a socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client)");
        exit(EXIT_FAILURE);
    }

    //connect to the receiver client
    init_sockaddr(&servername, to_client->IPaddress, to_client->portNum);
    if (0 > connect(sock,
                    (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    //send data to the receiver client
    char buf[1024];
    int new_port = htons(new_client->portNum);
//    char ipstr2[INET_ADDRSTRLEN];
//    inet_pton(AF_INET, new_client->IPaddress, ipstr2);

    if(flag == 0){
        sprintf(buf, "USER_ON <%s,%d>", new_client->IPaddress, new_port);
    }
    else{
        sprintf(buf,"USER_OFF <%s,%d>", new_client->IPaddress, new_port);
    }

    write_to_client(sock, buf);

    close(sock);
}


void init_sockaddr(struct sockaddr_in *name,
                   const char *hostname,
                   uint16_t port) {
    struct hostent *hostinfo;

    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    hostinfo = gethostbyname(hostname);
    if (hostinfo == NULL) {
        fprintf(stderr, "Unknown host %s.\n", hostname);
        exit(EXIT_FAILURE);
    }
    name->sin_addr = *(struct in_addr *) hostinfo->h_addr;
}


void reply_to_clients(Data *d, int flag) {

  //send USER_ON  <...> message to all connected clients
    for (ListNode *i = clients; i != NULL; i = i->next) {

        Data *ld = i->data;
        debug_log("examing client: %s , %d",ld->IPaddress, ld->portNum)

//        char str[INET_ADDRSTRLEN];
//        inet_ntop(AF_INET, d->IPaddress, str, INET_ADDRSTRLEN);


        //Dont send msg who to replied
        if ((strcmp(d->IPaddress, ld->IPaddress) != 0) || d->portNum != ld->portNum) {
            send_ON_OFF_message(ld, d,flag);
            //flag = 0 means USER_ON message
            //flag = 1 means USER_OFF message
        }
    }

}

void server_Log_On(char *buffer) {
    Data *clientInfo;
    clientInfo = takeClientInfo(buffer);

    debug_log("Client info: %s  %d", clientInfo->IPaddress, clientInfo->portNum)

    //insert IpAddress and port number into server list
    //check if this client already exists
    if (searchL(clients, clientInfo) == 1) {
        lInsert(&clients, clientInfo);
    }

    //send message to all the other clients
    reply_to_clients(clientInfo,0);
}


void server_Log_Off(int filedes, char* buffer)
{
    Data* clientinfo = takeClientInfo(buffer);

    int flag = 0;

    //search in the client list
    for(ListNode* i=clients; i!=NULL; i=i->next){
        Data* d = i->data;
        if(d->portNum == clientinfo->portNum && strcmp(clientinfo->IPaddress,d->IPaddress) == 0){
            //client found
            flag = 1;
            //delete node from server list
            deleteNode(&clients, d);
            break;
        }
    }

    if(flag == 0){
        //client not found
        char msg[] = "ERROR_IP_PORT_NOT_FOUND_IN_LIST";
        write_to_client(filedes,msg);
    }
    else{
        //client found
        //send to all connected clients
        reply_to_clients(clientinfo,1);       //1 means LOG_OFF messsage
    }
}


int make_socket(uint16_t port) {
    int sock;
    struct sockaddr_in name;

    /* Create the socket. */
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    /* Give the socket a name. */
    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *) &name, sizeof(name)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    return sock;
}

int read_from_client(int filedes) {
    char buffer[MAXMSG];
    int nbytes;

    nbytes = read(filedes, buffer, MAXMSG);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0)
        /* End-of-file. */
        return -1;
    else {
        /* Data read. */
        fprintf(stderr, "Server: got message: `%s'\n", buffer);

        if (strstr(buffer, "LOG_ON") != NULL) {
            server_Log_On(buffer);

        }
        else if(strcmp(buffer,"GET_CLIENTS") == 0){
            server_Get_Clients(filedes);
        }
        else if(strstr(buffer,"LOG_OFF") != NULL){
            server_Log_Off(filedes,buffer);
        }

        return 0;
    }
}


// Returns hostname for the local computer
void checkHostName(int hostname) {
    if (hostname == -1) {
        perror("gethostname");
        exit(1);
    }
}

// Returns host information corresponding to host name
void checkHostEntry(struct hostent *hostentry) {
    if (hostentry == NULL) {
        perror("gethostbyname");
        exit(1);
    }
}

// Converts space-delimited IPv4 addresses
// to dotted-decimal format
void checkIPbuffer(char *IPbuffer) {
    if (NULL == IPbuffer) {
        perror("inet_ntoa");
        exit(1);
    }
}


void my_signal_handler(int sig)
{
    //stop_running();

    freeListSpecial(clients);

    printf("Bye...\n");

    exit(0);
}