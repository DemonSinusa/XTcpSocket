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

    DLL_EXPORT SCT *InitClient(int domain, int type, int flags, int protocol, int rbuflen);

    DLL_EXPORT int SetCallBacksC(SCT *cl,
	    int (*OnRead)(SCT *cl, char *buf, int len),
	    int (*OnWrite)(SCT *cl, int len),
	    void (*OnDisconnected)(SCT *cl),
	    void (*OnErr)(SCT *cl, int err));

    DLL_EXPORT int Open(SCT *cl, char *host, char *port);

    DLL_EXPORT int Start_Read(SCT *cl,int rbuflen);

    DLL_EXPORT int Send(SCT *cl, char *buf, int len);

    DLL_EXPORT void Close(SCT *cl);

    DLL_EXPORT void FinitClient(SCT *cl);

#ifdef __cplusplus
}
#endif

#endif /* TCPCLIENT_H */

