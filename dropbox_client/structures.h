//
// Created by jo on 14/5/2019.
//

#ifndef DROPBOX_CLIENT_STRUCTURES_H
#define DROPBOX_CLIENT_STRUCTURES_H

typedef struct{
    char dirName[200];
    int portNum;
    char myIP[200];
    int workerThreads;
    int bufferSize;
    int serverPort;
    char serverIP[200];
}options;

typedef struct{
    char IPaddress[100];
    int portNum;
}Data;

typedef struct{
    char pathname[128];
    int version;
    char IPaddress[200];
    int portNum;
}bufferData;

#endif //DROPBOX_CLIENT_STRUCTURES_H
