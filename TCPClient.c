/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/select.h>
#include <sys/time.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>

#include "TCPClient.h"

SCT *InitClient(int domain, int type, int flags, int protocol, int rbuflen) {
    SCT *cl = NULL;
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
	if (cl->treads.Wthread)pthread_cancel(cl->treads.Wthread);
	if (cl->treads.Rthread)pthread_cancel(cl->treads.Rthread);
	if (cl->sock > 0)close(cl->sock);
	free(cl);
    }
}

int SetCallBacksC(SCT *cl,
	int (*OnRead)(SCT *cl, char *buf, int len),
	int (*OnWrite)(SCT *cl, int len),
	void (*OnDisconnected)(SCT *cl),
	void (*OnErr)(SCT *cl, int err)) {
    int count = 0;
    if (OnRead) {
	cl->OnRead = OnRead;
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

static void *ReadThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    char *bin = (char *) malloc(cl->buflen);
    int readl = 0, all = 0, error = 0;
    //MSG_WAITALL
    while ((readl = recv(cl->sock, &bin[all], cl->buflen, 0)) >= 0) {
	all += readl;
	if (readl == 0 || readl < cl->buflen)
	    if (cl->OnRead) {
		if (cl->OnRead(cl, bin, all) != 0) {
		    close(cl->sock);
		    if (cl->OnDisconnected)cl->OnDisconnected(cl);
		    cl->sock = 0;
		    break;
		} else {
		    cl->count.AllRead += cl->count.PrevRead;
		    cl->count.PrevRead = all;
		    all = 0;
		}
	    }

	bin = realloc(bin, all + cl->buflen);

    }

    if (readl < 0 && errno != EINTR) {
	if (cl->OnErr)cl->OnErr(cl, -30);
    } else {
	cl->count.AllRead += cl->count.PrevRead;
	cl->count.PrevRead = all;
    }

    pthread_exit(NULL);
}

static void *WriteThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    if (cl->OnWrite)cl->OnWrite(cl, cl->count.PrevWrite);
    pthread_exit(NULL);
}

int Connect(SCT *cl, char *host, char *port) {
    int status;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

    int one = 0;
    pthread_attr_t tattr;

    if ((status = getaddrinfo(host, port, &cl->hints, &servinfo)) != 0) {
	if (cl->OnErr)cl->OnErr(cl, -1);
	return -1;
    } else {
	//Можно продолжать-???:!
	for (tservinfo = servinfo; tservinfo != NULL; tservinfo = tservinfo->ai_next) {
	    cl->sock = socket(tservinfo->ai_family, tservinfo->ai_socktype,
		    tservinfo->ai_protocol);
	    if (cl->sock == -1)
		continue;

	    if (connect(cl->sock, tservinfo->ai_addr, tservinfo->ai_addrlen) != -1)
		break; /* Success */

	    close(cl->sock);
	}
	if (tservinfo == NULL) { /* No address succeeded */
	    if (cl->OnErr)cl->OnErr(cl, -2);
	    return -2;
	}
	freeaddrinfo(servinfo);

	pthread_attr_init(&tattr);
	if ((one = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED))) {
	    if (cl->OnErr)cl->OnErr(cl, -4);
	}

	if (pthread_create(&cl->treads.Rthread, &tattr, ReadThreadMain, cl) != 0) {
	    close(cl->sock);
	    if (cl->OnDisconnected)cl->OnDisconnected(cl);
	    cl->sock = 0;
	    if (cl->OnErr)cl->OnErr(cl, -3);
	}
    }
    return 0;

}

int Send(SCT *cl, char *buf, int len) {
    int total = 0; // Сколько уже
    int bytesleft = len; // Сколько нужно
    int n = 0;

    int one = 0;
    pthread_attr_t tattr;

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

	pthread_attr_init(&tattr);
	if ((one = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED))) {
	    if (cl->OnErr)cl->OnErr(cl, -300);
	}

	if (pthread_create(&cl->treads.Wthread, &tattr, WriteThreadMain, cl) != 0) {
	    if (cl->OnErr)cl->OnErr(cl, -200);
	}

    } else {
	close(cl->sock);
	if (cl->OnDisconnected)cl->OnDisconnected(cl);
	cl->sock = 0;
    }
    return total;

}