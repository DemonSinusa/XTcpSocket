/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   TCPClient.h
 * Author: senjord
 *
 * Created on 27 марта 2018 г., 14:09
 */

#ifndef TCPCLIENT_H
#define TCPCLIENT_H

#include "XSocket.h"



#ifdef __cplusplus
extern "C" {
#endif

    SCT *InitClient(int domain, int type, int flags, int protocol); //В параметры можно закинуть всякое флаговое и\ли бинарное дерьмиЩ

    int SetCallBacksC(SCT *cl, int (*OnRead)(char *buf, int len), int (*OnWrite)(int len), void (*OnErr)(int err));
    int Connect(SCT *cl, char *host, char *port);

    int Send(SCT *cl, char *buf, int len);

    void FinitClient(SCT *cl);

#ifdef __cplusplus
}
#endif

#endif /* TCPCLIENT_H */

