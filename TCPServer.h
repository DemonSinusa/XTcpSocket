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

    SST *InitServer(); //В параметры можно закинуть всякое флаговое и\ли бинарное дерьмиЩ

    int SetCallBacksS(SST *serv,
	    void (*OnConnected)(LCL *cl),
	    int (*OnRead)(LCL *cl, char *buf, int len),
	    int (*OnWrite)(LCL *cl, int len),
	    void (*OnErr)(int err));

    int Listen(SST *, char *host, char *port);

    int SendToClient(LCL *cl, char *buf, int len);

    void FinitServer(SST *);


#ifdef __cplusplus
}
#endif

#endif /* TCPSERVER_H */

