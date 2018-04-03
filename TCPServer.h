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
	    void (*OnConnected)(SST *serv, LCL *cl),
	    int (*OnRead)(SST *serv, LCL *cl, char *buf, int len),
	    int (*OnWrite)(SST *serv, LCL *cl, int len),
	    void (*OnDisconnected)(SST *serv, LCL *cl),
	    void (*OnErr)(SST *serv, int err));

    int Listen(SST *, char *host, char *port);

    int SendToClient(LCL *cl, char *buf, int len);

    void FinitServer(SST *);


#ifdef __cplusplus
}
#endif

#endif /* TCPSERVER_H */

