#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
//#include <bits/types/siginfo_t.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
//#include <bits/types/sig_atomic_t.h>
#include "common.h"
#include "utils.h"
#include "ring_buffer.h"

void read_options(int argc, char **argv) {
    memset(&opt, 0, sizeof(opt));       //initialize

    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            strcpy(opt.dirName, argv[i + 1]);
        }
        if (strcmp(argv[i], "-p") == 0) {
            opt.portNum = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-w") == 0) {
            opt.workerThreads = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-b") == 0) {
            opt.bufferSize = atoi(argv[i + 1]);
            RBUF_SIZE = opt.bufferSize;
        }
        if (strcmp(argv[i], "-sp") == 0) {
            opt.serverPort = atoi(argv[i + 1]);
        }
        if (strcmp(argv[i], "-sip") == 0) {
            strcpy(opt.serverIP, argv[i + 1]);
        }
    }
}

void put_data_in_client_list(char *buffer) {
    //you've read: CLIENT_LIST N <IP1,port1>...<IPn,portn>

    //remove "CLIENT_LIST"
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    printf("%s\n", buffer);

    //make a table to store each tuple
    int num = atoi(&buffer[0]);
    char table[num][100];

    buffer = buffer + 2;
    printf("%s\n", buffer);

    for (int i = 0; i < num; i++) {         //initialize table
        table[i][0] = '\0';
    }

    int count = num - 1;
    char *token = strtok(buffer, " ");
    while (token) {
        //printf("token: %s\n", token);
        strcpy(table[count], token);
        count--;
        token = strtok(NULL, " ");
    }

    //edit each tuple and insert in the client list
    for (int i = 0; i < num; i++) {

        char *token2 = strtok(table[i], ">");
        char *token1 = strtok(table[i], ",");
        token1++;
        token2 = strtok(NULL, ",");

        Data *d = malloc(sizeof(Data));

//        char ip[INET_ADDRSTRLEN];
//        inet_ntop(AF_INET, token1, ip, sizeof(ip));
        strcpy(d->IPaddress, token1);
        d->portNum = atoi(token2);
        d->portNum = ntohs(d->portNum);
        debug_log(" ip= %s , port= %d\n", token1, d->portNum)

        if (strcmp(d->IPaddress, opt.myIP) == 0 && d->portNum == opt.portNum) {
            //don't store yourself in the client list
            debug_log("it's me, don't store me ")
            free(d);
            continue;
        } else {
            //if client is not YOU insert him in the client list

            //insert IpAddress and port number into client list
            //check if this client/peer already exists
            if (searchL(client_list, d) == 1) {

                pthread_mutex_lock(&list_mtx);

                lInsert(&client_list, d);

                pthread_mutex_unlock(&list_mtx);
            }

            //insert new client in the buffer
            //tuple kind1 <IPaddr,portNum>
            bufferData *bd = malloc(sizeof(bufferData));
            strcpy(bd->IPaddress, d->IPaddress);
            bd->portNum = d->portNum;
            strcpy(bd->pathname, "EMPTY");
            bd->version = 0;

            //produce an item in the ring buffer
            pthread_mutex_lock(&mtx);

            while (ring_buffer.count >= RBUF_SIZE) {
                debug_log(">> Found Buffer Full \\n")
                pthread_cond_wait(&cond_nonfull, &mtx);
            }
            ringbuf_put(&ring_buffer, bd);

            pthread_cond_broadcast(&cond_nonempty);

            pthread_mutex_unlock(&mtx);
        }
    }
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

//read my input directory and store its files(path) in a list
void read_my_inputDir(ListNode **L, char *mydir) {

    struct stat buffer;

    if (stat(mydir, &buffer) < 0) {
        printf("%s\n", mydir);
        exit(-9);
    }

    DIR *dp;
    struct dirent *sub;

    if ((dp = opendir(mydir)) == NULL) {
        printf("error opendir!");
        exit(-4);
    }
    while ((sub = readdir(dp)) != NULL) {
        //while directory is not empty

        if (strcmp(sub->d_name, ".") == 0 || strcmp(sub->d_name, "..") == 0) {
            // ignore . , ..
            continue;
        }

        char str[4096];
        sprintf(str, "%s/%s", mydir, sub->d_name);

        if (stat(str, &buffer) < 0) {
            printf("%s", str);
        }

        if ((buffer.st_mode & S_IFMT) != S_IFDIR) {
            //file encountered
            //insert file to the list
            lInsert(L, new_str(str));
        } else {
            //subdirectory encountered
            read_my_inputDir(L, str);
        }
    }
    closedir(dp);
}

//take all the files from input dir and make them a list (full path)
ListNode *from_inputDir_to_list() {

    ListNode *myContents = NULL;
    read_my_inputDir(&myContents, opt.dirName);

    if (myContents == NULL) {
        printf("Problem with my input dir amigo\n");
    }

//    //sort the list in order to send them in order
//    MergeSort(&myContents);
    return myContents;
}

unsigned checksum(void *buffer, size_t len, unsigned int seed) {
    unsigned char *buf = (unsigned char *) buffer;
    size_t i;

    for (i = 0; i < len; ++i)
        seed += (unsigned int) (*buf++);
    return seed;
}

// function that returns an integer based on checksum of the file
int hash_file(char *file) {
    FILE *fp;
    size_t len;
    char buf[4096];

    if (NULL == (fp = fopen(file, "rb"))) {
        printf("Unable to open %s for reading\n", file);
        return -1;
    }
    len = fread(buf, sizeof(char), sizeof(buf), fp);
    // printf("%d bytes read\n", len);
    printf("The checksum of %s is %d\n", file, checksum(buf, len, 0));
}


void write_to_socket(int filedes, char *buffer) {
    int nbytes;

    nbytes = write(filedes, buffer, strlen(buffer) + 1);
    if (nbytes < 0) {
        perror("write");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "Client: send message: `%s'\n", buffer);
}

void get_file_list(int filedes) {

    ListNode *myFiles = from_inputDir_to_list();
    char buf[1024];
    sprintf(buf, "FILE_LIST %d", sizeL(myFiles));

    //for every file in the list, find its hash value and make a tuple
    for (ListNode *i = myFiles; i != NULL; i = i->next) {

        char *msg = malloc(256 * sizeof(char));
        sprintf(msg, " <%s,%d>", (char *) i->data, hash_file((char *) i->data));
        strcat(buf, msg);
        free(msg);
    }

    write_to_socket(filedes, buf);

    freeListSpecial(myFiles);
}

int get_file_size(char *filename) {

    // opening the file in read mode
    FILE *fp = fopen(filename, "r");

    // checking if the file exist or not
    if (fp == NULL) {
        printf("File Not Found!\n");
        return -1;
    }

    fseek(fp, 0L, SEEK_END);

    // calculating the size of the file
    int res = ftell(fp);

    // closing the file
    fclose(fp);

    return res;
}

void received_GET_FILE(int filedes, char *buffer) {
    // GET_FILE <pathname,version>
    debug_log("received: %s", buffer)

    //get tuple information
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    printf("%s", buffer);

    char *token2 = strtok(buffer, ">");
    char *token1 = strtok(buffer, ",");
    token1++;
    token2 = strtok(NULL, ",");

    char *tmp2 = strchr(token1, '/');
    if (tmp2 != NULL)
        token1 = tmp2 + 1;


    char pathname[200];
    sprintf(pathname, "%s/%s", opt.dirName, token1);
    int version = atoi(token2);

    debug_log("checking if file=%s exists in local dir", pathname)

    //check if file exists
    if (access(pathname, F_OK) != -1) {
        // file exists

        //check if version has altered
        if ((version != hash_file(pathname)) || version == -1) {
            //version has changed
            debug_log("version has changed")
            //return message FILE_SIZE version n byte0byte1...byten
            char msg[1024];
            sprintf(msg, "FILE_SIZE %d %d ", hash_file(pathname), get_file_size(pathname));

            FILE *fp = fopen(pathname, "r");
            if (fp == NULL) {
                printf("Error! Failed to open %s for reading\n", pathname);
            }
            size_t read;
            char buf[1000];
            buffer[0] = '\0';
            while ((read = fread(buf, 1, 1000, fp)) > 0) {

                strcat(msg, buf);
            }

            fclose(fp);
            debug_log("write to socket:%s", msg)
            write_to_socket(filedes, msg);

        } else {
            //version hasn't changed
            char msg[] = "FILE_UP_TO_DATE";

            write_to_socket(filedes, msg);
        }

    } else {
        // file doesn't exist
        char msg[] = "FILE_NOT_FOUND";
        debug_log("file not found")
        write_to_socket(filedes, msg);
    }
}

void received_USER_OFF(char *buffer) {
    //get tuple information
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    printf("%s", buffer);

    char *token2 = strtok(buffer, ">");
    char *token1 = strtok(buffer, ",");
    token1++;
    token2 = strtok(NULL, ",");

    char ip[INET_ADDRSTRLEN];
//    inet_ntop(AF_INET, token1, ip, sizeof(ip));
    strcpy(ip, token1);
    int port = atoi(token2);
    port = ntohs(port);

    for (ListNode *i = client_list; i != NULL; i = i->next) {

        Data *d = i->data;
        if (strcmp(d->IPaddress, ip) == 0 && d->portNum == port) {
            //client found - delete him from the list
            deleteNode(&client_list, d);
            debug_log("Delete client : %s, %d", ip, port)
            break;
        }
    }
}

void received_USER_ON(char *buffer) {
    //get tuple information
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
    //considered as visised

    //insert IpAddress and port number into client list
    //check if this client/peer already exists
    if (searchL(client_list, d) == 1) {

        pthread_mutex_lock(&list_mtx);

        lInsert(&client_list, d);

        pthread_mutex_unlock(&list_mtx);
    }

    //insert new client in the buffer
    //tuple kind1 <IPaddr,portNum>
    bufferData *bd = malloc(sizeof(bufferData));
    strcpy(bd->IPaddress, d->IPaddress);
    bd->portNum = d->portNum;
    strcpy(bd->pathname, "EMPTY");
    bd->version = 0;

    //produce an item in the ring buffer
    pthread_mutex_lock(&mtx);

    while (ring_buffer.count >= RBUF_SIZE) {
        debug_log(">> Found Buffer Full \\n")
        pthread_cond_wait(&cond_nonfull, &mtx);
    }
    ringbuf_put(&ring_buffer, bd);

    pthread_cond_broadcast(&cond_nonempty);

    pthread_mutex_unlock(&mtx);
}

char *filename_analysis(char *localPath, char *filename) {

    ListNode *l = NULL;

    char *token = strtok(filename, "/");
    while (token) {

        lInsertAtEnd(&l, token);
        token = strtok(NULL, "/");
    }
    //the list will contain the members of full path
    //last entry will have the filename
    //anything in between is a dir and must be constructed if not yet

    char edit[500];
    edit[0] = '\0';

    for (ListNode *j = l; j != NULL; j = j->next) {
        strcat(edit, (char *) j->data);
        strcat(edit, "/");
    }
    edit[strlen(edit) - 1] = '\0';

    l = DeleteLastN(l);             //last node will be the file

    //create the dirs that not exist
    char name[500];
    name[0] = '\0';
    strcpy(name, localPath);
    char newname[500];
    newname[0] = '\0';

    for (ListNode *i = l; i != NULL; i = i->next) {

        sprintf(newname, "%s/%s", name, (char *) i->data);
        struct stat st = {0};
        //stat for checking if the directory exists - if not mkdir
        if (stat(newname, &st) == -1) {

            mkdir(newname, 0700);
        }

        strcpy(name, newname);
        newname[0] = '\0';          //initialize newname
    }
    freeList(l);
    return new_str(edit);
}

//function that checks a path and makes the necessary directories and subdirectories
char *before_store_local(char *filename, char *ip, int port) {

    //make path for this peer
    char localpath[500];
    localpath[0] = '\0';
    sprintf(localpath, "./%s/%s_%d", "PeerDir", ip, port);

    //check if there is dir for peer else make it
    struct stat st = {0};
    //stat for checking if the directory exists - if not mkdir
    if (stat(localpath, &st) == -1) {

        mkdir(localpath, 0700);
    }

    //tokenize filename in order to check if we must create any subdirectory
    char *n = filename_analysis(localpath, filename);

    //complete the mirror dir path
    char finalpath[500];
    finalpath[0] = '\0';
    sprintf(finalpath, "%s/%s", localpath, n);

    free(n);

    // path must be returned- is where the file will be stored in mirror id
    return new_str(finalpath);
}

void store_file(char *filename, char *buffer, char *ip, int port) {
    //FILE_SIZE version n byte0byte1...byten

    //remove "FILE_SIZE"
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    //printf("%s\n", buffer);

    int version = atoi(&buffer[0]);
    //buffer++;
    tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;

    int bytes = atoi(&buffer[0]);
    //buffer++;

    char *tmp2 = strchr(buffer, ' ');
    if (tmp2 != NULL)
        buffer = tmp2 + 1;

    char *tmp3 = strchr(filename, '/');
    if (tmp3 != NULL)
        filename = tmp3 + 1;

//    //now buffer only contains file contents
//    char path[300];
//    sprintf(path, "./%s/%s_%d/%s", "PeerDir", ip, port, filename);
//    debug_log("I'm going to store file=%s",path)

    char *path = before_store_local(filename, ip, port);
    debug_log("I'm going to store file=%s", path)

    errno = 0;
    FILE *fptr;
    fptr = fopen(path, "w+");

    if (fptr == NULL) {
        debug_log("Error! fopen in store_file, fptr = NULL \n")
        debug_log("EEERRROR:%d", errno)
        exit(-2);
    }

    fwrite(buffer, 1, bytes, fptr);

    fclose(fptr);

    free(path);
}

void received_file(int filedes, char *filename, char *ip, int port) {
    char buffer[MAXMSG];
    int nbytes;

    nbytes = read(filedes, buffer, MAXMSG);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0)
        /* End-of-file. */
        return;
    else {
        /* Data read. */
        fprintf(stderr, "Client: got message: `%s'\n", buffer);

        //response from GET_FILE
        if (strcmp(buffer, "FILE_UP_TO_DATE") == 0) {
            return;

        } else if (strcmp(buffer, "FILE_NOT_FOUND") == 0) {
            return;

        } else if (strstr(buffer, "FILE_SIZE") != NULL) {
            //FILE_SIZE version n byte0byte1...byten
            store_file(filename, buffer, ip, port);
            return;
        }
    }
}

int read_from_socket(int filedes) {
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
        fprintf(stderr, "Client: got message: `%s'\n", buffer);

        if (strstr(buffer, "CLIENT_LIST") != NULL) {
            //if you've just read the client list
            put_data_in_client_list(buffer);

        } else if (strcmp(buffer, "GET_FILE_LIST") == 0) {
            get_file_list(filedes);

        } else if (strstr(buffer, "GET_FILE") != NULL) {
            received_GET_FILE(filedes, buffer);

        } else if (strstr(buffer, "USER_OFF") != NULL) {
            received_USER_OFF(buffer);

        } else if (strstr(buffer, "USER_ON") != NULL) {
            received_USER_ON(buffer);
        }
        return 0;
    }
}


void send_LOG_ON_to_server() {
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);

    int sock;
    struct sockaddr_in servername;

    //create socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client)");
        exit(EXIT_FAILURE);
    }

    //connect to server
    init_sockaddr(&servername, opt.serverIP, opt.serverPort);
    if (0 > connect(sock,
                    (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    //send data to server
    char buf[1024];
    int new_port = htons(opt.portNum);

//    char myip[200];
//    strcpy(myip,opt.myIP);
//
//    struct sockaddr_in sa;
//    inet_pton(AF_INET,opt.myIP, &(sa.sin_addr));
//    int n = snprintf(NULL,0,"%u",sa.sin_addr.s_addr);
//    snprintf(myip,n+1,"%u",sa.sin_addr.s_addr);

//    char ipstr2[INET_ADDRSTRLEN];
//    inet_pton(AF_INET, opt.myIP, ipstr2);

    sprintf(buf, "LOG_ON <%s,%d>", opt.myIP, new_port);

    write_to_socket(sock, buf);


    close(sock);
}

void send_GET_CLIENTS_to_server() {
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);
    //connect to server
    int sock;
    struct sockaddr_in servername;

    //create socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client)");
        exit(EXIT_FAILURE);
    }

    //connect to server
    init_sockaddr(&servername, opt.serverIP, opt.serverPort);
    if (0 > connect(sock, (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    //send data to server
    char buf[] = "GET_CLIENTS";

    write_to_socket(sock, buf);

    debug_log("Client: waiting server to send client list")
    //this function will take care of the fetched info
    read_from_socket(sock);

    close(sock);
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

void stop_running() {
    running = false;
}

ListNode *put_data_in_file_list(char *buffer, char *ip, int port) {
    //you've read: FILE_LIST N <pathname1,version1>...<pathnamen,versionn>

    ListNode *files = NULL;

    //remove "FILE_LIST"
    char *tmp = strchr(buffer, ' ');
    if (tmp != NULL)
        buffer = tmp + 1;
    //printf("%s\n", buffer);

    //make a table to store each tuple
    int num = atoi(&buffer[0]);
    char table[num][100];

    buffer = buffer + 2;
    //printf("%s\n", buffer);

    for (int i = 0; i < num; i++) {         //initialize table
        table[i][0] = '\0';
    }

    int count = num - 1;
    char *token = strtok(buffer, " ");
    while (token) {
        //printf("token: %s\n", token);
        strcpy(table[count], token);
        count--;
        token = strtok(NULL, " ");
    }

    //edit each tuple and insert in the client list

    for (int i = 0; i < num; i++) {

        char *token2 = strtok(table[i], ">");
        char *token1 = strtok(table[i], ",");
        token1++;
        token2 = strtok(NULL, ",");


        bufferData *bd = malloc(sizeof(bufferData));

        strcpy(bd->pathname, token1);
        bd->version = atoi(token2);
        strcpy(bd->IPaddress, ip);
        bd->portNum = port;

        debug_log("pathname=%s, version=%d, ip=%s,port=%d", bd->pathname, bd->version, bd->IPaddress, bd->portNum)

        lInsert(&files, bd);
    }
    //files=list that contains <pathname,version,IPaddr,portNum>
    return files;
}

ListNode *read_from_PEER_socket(int filedes, char *ip, int port) {
    char buffer[MAXMSG];
    int nbytes;
    ListNode *files = NULL;

    nbytes = read(filedes, buffer, MAXMSG);
    if (nbytes < 0) {
        /* Read error. */
        perror("read");
        exit(EXIT_FAILURE);
    } else if (nbytes == 0)
        /* End-of-file. */
        return NULL;
    else {
        /* Data read. */
        fprintf(stderr, "Client: got message: `%s'\n", buffer);

        if (strstr(buffer, "FILE_LIST") != NULL) {
            files = put_data_in_file_list(buffer, ip, port);
            return files;
        }

        return NULL;    //sth went wrong
    }
}

ListNode *send_GET_FILE_LIST(char *ip, int port) {
    //making new session
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);

    int sock;
    struct sockaddr_in servername;

    //create a socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client:send get file list)");
        exit(EXIT_FAILURE);
    }

    //connect to the receiver client
    init_sockaddr(&servername, ip, port);
    if (0 > connect(sock,
                    (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    //send data to the receiver client
    char buf[] = "GET_FILE_LIST";

    write_to_socket(sock, buf);

    debug_log("Client:waiting peer to return his file list")

    ListNode *files = NULL;

    files = read_from_PEER_socket(sock, ip, port);  //list of bufferData tuples <pathname,version,IPaddr,portNum>

    close(sock);

    return files;
}

void send_GET_FILE(char *ip, int port, char *pathname, int version) {
    //making new session
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);

    int sock;
    struct sockaddr_in servername;

    //create a socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client:send get file list)");
        exit(EXIT_FAILURE);
    }

    //connect to the receiver client
    init_sockaddr(&servername, ip, port);
    if (0 > connect(sock,
                    (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    char buf[200];
    sprintf(buf, "GET_FILE <%s,%d>", pathname, version);

    write_to_socket(sock, buf);

    debug_log("Client:waiting peer to return file=%s", pathname)

    received_file(sock, pathname, ip, port);

    close(sock);
}

void tuple_kind1(bufferData *item) {

    ListNode *files = send_GET_FILE_LIST(item->IPaddress,
                                         item->portNum);

    if (files == NULL) {
        debug_log("PEER HASN'T SENT ANY FILES")
    } else {
        //put the tuples <pathname,version,IPaddr,portNum> in the ring buffer
        //writer
        for (ListNode *i = files; i != NULL; i = i->next) {

            while (ring_buffer.count >= RBUF_SIZE) {
                debug_log(">> Found buffer full ")
                pthread_cond_wait(&cond_nonfull, &mtx);
            }

            if(running == false){
                //when you wake up check if user wants you to stop
                debug_log("Thread exiting...")
                pthread_exit(NULL);
            }

            bufferData *bd = i->data;
            ringbuf_put(&ring_buffer, bd);
        }
        pthread_cond_broadcast(&cond_nonempty);
    }
}

void tuple_kind2(bufferData *item) {

    Data *A = malloc(sizeof(Data));
    strcpy(A->IPaddress, item->IPaddress);
    A->portNum = item->portNum;

    if (searchL(client_list, A) == 0) {
        //client is ON (exists in client list)

        char *file = malloc(200 * sizeof(char));
        strcpy(file, item->pathname);

        char *tmp = strchr(file, '/');      //remove peer folder from path
        if (tmp != NULL)
            file = tmp + 1;

        char filename[200];
        sprintf(filename, "%s/%s_%d/%s", "PeerDir", item->IPaddress, item->portNum, file);

        if (access(filename, F_OK) != -1) {
            debug_log("file exists in local dir")
            //file exists in local dir:PeerDir
            send_GET_FILE(item->IPaddress, item->portNum, file, hash_file(filename));

        } else {
            debug_log("file exists in local dir")
            //file doesn't exists in local dir:PeerDir
            send_GET_FILE(item->IPaddress, item->portNum, file, -1);
        }
    }
    free(A);

}

volatile sig_atomic_t sig_int = 0;

void *workerThreads(void *ptr) {
    //spthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    while (running) {         //run until you receive ctrl c

        pthread_mutex_lock(&mtx);

        //reader
        while (ring_buffer.count <= 0 && sig_int == 0) {
            debug_log(">> Found Buffer Empty ")
            pthread_cond_wait(&cond_nonempty, &mtx);
        }

        if(sig_int == 1){
            //when you wake up check if user wants you to stop
            debug_log("Thread exiting...")
            pthread_mutex_unlock(&mtx);
            pthread_exit(NULL);
        }

        bufferData *item = ringbuf_get(&ring_buffer);

        //check what type of tuple you got
        if (strcmp(item->pathname, "EMPTY") == 0 && item->version == 0) {
            //tuple kind1 <IPaddr,portNum>
            tuple_kind1(item);

        } else {
            // tuple kind2 <pathname,version,IPaddr,portNum>
            tuple_kind2(item);
        }
        //free(item);       //double free or corruption!

        // continue reader thread
        pthread_cond_broadcast(&cond_nonfull);
        pthread_mutex_unlock(&mtx);
        debug_log("Worker thread complete a round of work!")
    }
    debug_log("Thread exiting...")
    pthread_exit(NULL);
}


void send_LOG_OFF_to_server() {
    extern void init_sockaddr(struct sockaddr_in *name,
                              const char *hostname,
                              uint16_t port);

    int sock;
    struct sockaddr_in servername;

    //create socket
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket (client)");
        exit(EXIT_FAILURE);
    }

    //connect to server
    init_sockaddr(&servername, opt.serverIP, opt.serverPort);
    if (0 > connect(sock,
                    (struct sockaddr *) &servername,
                    sizeof(servername))) {
        perror("connect (client)");
        exit(EXIT_FAILURE);
    }

    //send data to server
    char buf[1024];
    int new_port = htons(opt.portNum);
//    char ipstr2[INET_ADDRSTRLEN];
//    inet_pton(AF_INET, opt.myIP, ipstr2);

//    char myip[200];
//    strcpy(myip,opt.myIP);
//
//    struct sockaddr_in sa;
//    inet_pton(AF_INET,opt.myIP, &(sa.sin_addr));
//    int n = snprintf(NULL,0,"%u",sa.sin_addr.s_addr);
//    snprintf(myip,n+1,"%u",sa.sin_addr.s_addr);

    sprintf(buf, "LOG_OFF <%s,%d>", opt.myIP, new_port);

    write_to_socket(sock, buf);

    close(sock);
}


void my_signal_handler(int sig) {

    send_LOG_OFF_to_server();

    ringbuf_flush(&ring_buffer, RBUF_CLEAR);

    freeListSpecial(client_list);

    stop_running();             //stop the worker threads

    sig_int = 1;

    pthread_cond_broadcast(&cond_nonfull);
    pthread_cond_broadcast(&cond_nonempty);
    pthread_mutex_unlock(&mtx);

    int err;

    for (int i = 0; i < opt.workerThreads; i++){

//        pthread_cond_broadcast(&cond_nonfull);
//        pthread_cond_broadcast(&cond_nonempty);

        if (err = pthread_join(*(tids + i), NULL)) {
            /* Wait for thread termination */
            perror2("pthread_join", err);
            exit(1);
        }
    }

    pthread_cond_destroy(&cond_nonempty);
    pthread_cond_destroy(&cond_nonfull);
    pthread_mutex_destroy(&mtx);
    pthread_mutex_destroy(&list_mtx);

    printf("Bye...\n");

    exit(0);
}