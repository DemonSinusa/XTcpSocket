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
    CPT Rthread, Wthread;
} TCT;

typedef struct _sockclient_type_ {
    int sock, buflen;
    struct addrinfo hints;
    TCT Treadrs;
    Counts count;
    int (*OnRead)(struct _sockclient_type_ *cl, char *buf, int len);
    int (*OnWrite)(struct _sockclient_type_ *cl, int len);
    void (*OnDisconnected)(struct _sockclient_type_ *cl);
    void (*OnErr)(struct _sockclient_type_ *cl, int err);
} SCT;

typedef struct _thread_srvt_ {
    CPT AcptThread;
} TST;

typedef struct _sct_list_ {
    int client, buflen;
    TCT Treadrs;
    Counts count;
    void *ServST;
    struct _sct_list_ *prev, *next;
} LCL;

typedef struct _sockserver_type_ {
    int sock, bufftoclient;
    struct addrinfo hints;
    TST treads;
    Counts count;
    void (*OnConnected)(struct _sockserver_type_ *s, LCL *client);
    int (*OnRead)(struct _sockserver_type_ *s, LCL *cl, char *buf, int len);
    int (*OnWrite)(struct _sockserver_type_ *s, LCL *cl, int len);
    void (*OnDisconnected)(struct _sockserver_type_ *s, LCL *cl);
    void (*OnErr)(struct _sockserver_type_ *s, int err);
    LCL *first, *end;
} SST;

#ifdef __cplusplus
extern "C" {
#endif

#ifndef WIN32
#define closesocket  close
#endif

#ifdef __cplusplus
}
#endif

#endif /* XSOCKET_H */

