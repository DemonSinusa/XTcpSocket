/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   TCPServer.h
 * Author: senjord
 *
 * Created on 27 марта 2018 г., 14:09
 */

#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "XSocket.h"

#ifdef __cplusplus
extern "C" {
#endif

    SST *InitServer(int domain, int type, int flags, int protocol, int rbuflen);

    int SetCallBacksS(SST *serv,
	    void (*OnConnected)(SST *serv, SCT *cl),
	    int (*OnRead)(SST *serv, SCT *cl, char *buf, int len),
	    int (*OnWrite)(SST *serv, SCT *cl, int len),
	    void (*OnDisconnected)(SST *serv, SCT *cl),
	    void (*OnErr)(SST *serv,SCT *cl, int err));

    int Listen(SST *, char *host, char *port);
    void Todeaf(SST *serv);

    int SendToClient(SCT *cl, char *buf, int len);

    void FinitServer(SST *);


#ifdef __cplusplus
}
#endif

#endif /* TCPSERVER_H */

