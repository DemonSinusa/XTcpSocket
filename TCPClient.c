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
	if (cl->sock > 0)close(cl->sock);
	free(cl);
    }
}

int SetCallBacksC(SCT *cl, int (*OnRead)(SCT *cl, char *buf, int len), int (*OnWrite)(SCT *cl, int len), void (*OnErr)(SCT *cl, int err)) {
    int count = 0;
    if (OnRead) {
	cl->OnRead = OnRead;
	count++;
    }
    if (OnWrite) {
	cl->OnWrite = OnWrite;
	count++;
    }
    if (OnErr) {
	cl->OnErr = OnErr;
	count++;
    }
    return count;
}

static void *ReadThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    char *bin = (char *) malloc(cl->buflen);
    int readl = 0, all = 0, error = 0;
    pthread_detach(cl->treads.Rthread);

    while ((readl = recv(cl->sock, &bin[all], cl->buflen, MSG_WAITALL)) >= 0) {
	all += readl;
	if (readl == 0)
	    if (cl->OnRead) {
		if (cl->OnRead(cl, bin, all) != 0) {
		    close(cl->sock);
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

    return NULL;
}

static void *WriteThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    pthread_detach(cl->treads.Wthread);
    if (cl->OnWrite)cl->OnWrite(cl, cl->count.PrevWrite);
    return NULL;
}

int Connect(SCT *cl, char *host, char *port) {
    int status;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

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

	if (pthread_create(&cl->treads.Rthread, NULL, ReadThreadMain,
		(void *) & cl) != 0) {
	    close(cl->sock);
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
    while (total < len) {
	n = send(cl->sock, buf + total, bytesleft, 0);
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

	if (pthread_create(&cl->treads.Wthread, NULL, WriteThreadMain,
		(void *) & cl) != 0) {
	    if (cl->OnErr)cl->OnErr(cl, -200);
	}

    } else {
	close(cl->sock);
	cl->sock = 0;
    }
    return total;

}