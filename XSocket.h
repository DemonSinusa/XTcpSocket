/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   XSocket.h
 * Author: senjord
 *
 * Created on 27 марта 2018 г., 14:09
 */

#ifndef XSOCKET_H
#define XSOCKET_H

typedef struct _counters_ {
    int AllRead, AllWrite;
    int PrevRead, PrevWrite;
} Counts;

typedef struct _thread_clnt_ {
    pthread_t Rthread, Wthread;
    //    pthread_t ErrThread;
} TCT;

typedef struct _sockclient_type_ {
    int sock, buflen;
    //    struct sockaddr_in addr_info;
    struct addrinfo hints;
    TCT treads;
    Counts count;
    int (*OnRead)(char *buf, int len);
    int (*OnWrite)(int len);
    void (*OnErr)(int err);
} SCT;

typedef struct _thread_srvt_ {
    pthread_t AcptThread;
    //    pthread_t ErrThread;
} TST;

typedef struct _sct_list_ {
    int client, buflen;
    pthread_t Rthread, Wthread;
    Counts count;
    void *ServST;
    struct _sct_list_ *prev, *next;
} LCL;

typedef struct _sockserver_type_ {
    int sock, bufftoclient;
    struct addrinfo hints;
    TST treads;
    Counts count;
    void (*OnConnected)(LCL *client);
    int (*OnRead)(LCL *cl, char *buf, int len);
    int (*OnWrite)(LCL *cl, int len);
    void (*OnErr)(int err);
    LCL *first, *end;
} SST;

#ifdef __cplusplus
extern "C" {
#endif




#ifdef __cplusplus
}
#endif

#endif /* XSOCKET_H */
