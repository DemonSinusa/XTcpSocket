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

#ifndef WIN32
#define INVALID_SOCKET	-1
#define closesocket(x)    close(x)
typedef int SOCKET;

#endif

typedef struct _counters_ {
    int AllRead, AllWrite;
    int PrevRead, PrevWrite;
} Counts;

typedef struct _thread_clnt_ {
    CPT Rthread, Wthread;
} TCT;

typedef struct _sockclient_type_ {
    SOCKET sock;
    int buflen;
    struct addrinfo hints;
    TCT Treadrs;
    Counts count;

    void *ServST;

    int (*Open)(struct _sockclient_type_ *cl, char *host, char *port);
    int (*Read)(struct _sockclient_type_ *cl);
    int (*Write)(struct _sockclient_type_ *cl,char *buf,int len);
    void(*Close)(struct _sockclient_type_ *cl);

    int (*OnRead)(struct _sockclient_type_ *cl, char *buf, int len);
    int (*OnWrite)(struct _sockclient_type_ *cl, int len);
    void (*OnDisconnected)(struct _sockclient_type_ *cl);
    void (*OnErr)(struct _sockclient_type_ *cl, int err);
} SCT;


typedef struct _sct_list_ {
    SCT *Client;
    struct _sct_list_ *prev, *next;
} LCL;

typedef struct _thread_srvt_ {
    CPT AcptThread;
} TST;

typedef struct _sockserver_type_ {
    SOCKET sock;
    int bufftoclient;
    struct addrinfo hints;
    TST threads;
    Counts count;
    int (*Listen)(struct _sockserver_type_ *s, char *host, char *port);
    void (*Todeaf)(struct _sockserver_type_ *s);

    void (*OnConnected)(struct _sockserver_type_ *s, SCT *client);
    int (*OnRead)(struct _sockserver_type_ *s, SCT *cl, char *buf, int len);
    int (*OnWrite)(struct _sockserver_type_ *s, SCT *cl, int len);
    void (*OnDisconnected)(struct _sockserver_type_ *s, SCT *cl);
    void (*OnErr)(struct _sockserver_type_ *s,SCT *cl, int err);
    LCL *first, *end;
} SST;

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif

#endif /* XSOCKET_H */

