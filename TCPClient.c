/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <string.h>

#ifdef _WIN32
#include <WinSock2.h>
#include <Ws2tcpip.h>
#else
#include <pthread.h>
#include <unistd.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

#include "Threads.h"

#include "TCPClient.h"

SCT *InitClient(int domain, int type, int flags, int protocol, int rbuflen) {
    SCT *cl = NULL;

#ifdef WIN32

    WSADATA wsaData;

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
	fprintf(stderr,"Winsock не инициализирован!\n");
	WSACleanup();
	return NULL;
    } else fprintf(stdout,"Winsock всё ОК!\n");

#endif

    if ((cl = (SCT *) malloc(sizeof (SCT))) != NULL) {
	memset(cl, 0, sizeof (SCT));

	cl->hints.ai_family = domain;
	cl->hints.ai_socktype = type;
	cl->hints.ai_flags = flags;
	cl->hints.ai_protocol = protocol;

	cl->buflen = rbuflen;
    }
    return cl;
}

void FinitClient(SCT *cl) {
    if (cl) {
	//При потоковости рубануть сперва их
	if (cl->Treadrs.Wthread.thread)_wCrossThreadClose(&cl->Treadrs.Wthread);
	if (cl->Treadrs.Rthread.thread)_wCrossThreadClose(&cl->Treadrs.Rthread);
	if (cl->sock > 0)closesocket(cl->sock);
	free(cl);
    }

#ifdef WIN32

    if (WSACleanup())
	fprintf(stderr,"Чот ничистицца...\n");
    else
	fprintf(stdout,"Зачищено!\n");

#endif

}

static int ReadThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    char *bin = (char *) malloc(cl->buflen);
    int readl = 0, all = 0;

    while ((readl = recv(cl->sock, &bin[all], cl->buflen, 0)) >= 0) {
	all += readl;
	if (readl == 0 || readl < cl->buflen) {
	    if (cl->OnRead) {
		if (cl->OnRead(cl, bin, all) != 0) {
		    closesocket(cl->sock);
		    if (cl->OnDisconnected)cl->OnDisconnected(cl);
		    cl->sock = 0;
		    break;
		}
	    }
	    cl->count.AllRead += cl->count.PrevRead;
	    cl->count.PrevRead = all;
	    all = 0;
	}

	bin = realloc(bin, all + cl->buflen);

    }
    free(bin);

    pthread_exit(NULL);
}

int SetCallBacksC(SCT *cl,
	int (*OnRead)(SCT *cl, char *buf, int len),
	int (*OnWrite)(SCT *cl, int len),
	void (*OnDisconnected)(SCT *cl),
	void (*OnErr)(SCT *cl, int err)) {
    int count = 0;

    if (OnRead) {
    	if(cl->OnRead){
	    	_wCrossThreadClose(&cl->Treadrs.Rthread);
    	}else cl->OnRead = OnRead;

		if (_wCrossThreadCreate(&cl->Treadrs.Rthread, ReadThreadMain, cl) != 0) {
			closesocket(cl->sock);
			if (cl->OnDisconnected)cl->OnDisconnected(cl);
			cl->sock = 0;
			if (cl->OnErr)cl->OnErr(cl, -60);
	}
	count++;
    } else cl->OnRead = NULL;
    if (OnWrite) {
	cl->OnWrite = OnWrite;
	count++;
    } else cl->OnWrite = NULL;
    if (OnDisconnected) {
	cl->OnDisconnected = OnDisconnected;
	count++;
    } else cl->OnDisconnected = NULL;
    if (OnErr) {
	cl->OnErr = OnErr;
	count++;
    } else cl->OnErr = NULL;
    return count;
}


static int WriteThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    if (cl->OnWrite)cl->OnWrite(cl, cl->count.PrevWrite);
    _wCrossThreadExit(0);
    return 0;
}

int Connect(SCT *cl, char *host, char *port) {
    int status;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

    if ((status = getaddrinfo(host, port, &cl->hints, &servinfo)) != 0) {
	if (cl->OnErr)cl->OnErr(cl, -10);
	return -1;
    } else {
	//Можно продолжать-???:!
	for (tservinfo = servinfo; tservinfo != NULL; tservinfo = tservinfo->ai_next) {
	    cl->sock = socket(tservinfo->ai_family, tservinfo->ai_socktype,
		    tservinfo->ai_protocol);
	    if (cl->sock == -1)
		continue;

	    if (connect(cl->sock, tservinfo->ai_addr, tservinfo->ai_addrlen) != -1)
		break; // Зашибись

	    closesocket(cl->sock);
	}
	if (tservinfo == NULL) { // А адрес так и не вышел)
	    if (cl->OnErr)cl->OnErr(cl, -20);
	    return -2;
	}
	freeaddrinfo(servinfo);
    }
    return 0;

}

int Send(SCT *cl, char *buf, int len) {
    int total = 0; // Сколько уже
    int bytesleft = len; // Сколько нужно
    int n = 0;

    while (total < len && n != -1) {
	n = send(cl->sock, &buf[total], bytesleft, 0);
	if (n == -1) {
	    if (cl->OnErr)cl->OnErr(cl, -100);
	    break;
	}
	total += n;
	bytesleft -= n;
    }

    if (n >= 0) {
	cl->count.AllWrite += cl->count.PrevWrite;
	cl->count.PrevWrite = total;


	if (_wCrossThreadCreate(&cl->Treadrs.Wthread, WriteThreadMain, cl) != 0) {
	    if (cl->OnErr)cl->OnErr(cl, -50);
	}

    } else {
	closesocket(cl->sock);
	if (cl->OnDisconnected)cl->OnDisconnected(cl);
	cl->sock = 0;
    }
    return total;

}
