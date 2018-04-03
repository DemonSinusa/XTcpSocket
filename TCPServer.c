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

#include "TCPServer.h"

//Работа со списком клиентов (++/--) только

LCL *AddPlease(LCL *first) {
    LCL *it = NULL;
    if ((it = (LCL *) malloc(sizeof (LCL))) != NULL)
	memset(it, 0, sizeof (LCL));

    if (first) {
	it->ServST = first->ServST;
	if (first->next) {
	    //в середину
	    it->next = first->next;
	    it->prev = first;
	    first->next->prev = it;
	    first->next = it;
	} else {
	    //в конец
	    it->prev = first;
	    first->next = it;
	    ((SST *) first->ServST)->end = it;
	}
    }
    return it;
}

void DelPlease(LCL *it) {
    SST *s = (SST *) it->ServST;
    if (s->first == it)s->first = it->next;
    if (s->end == it)s->end = it->prev;

    if (it->prev) {
	it->prev->next = it->next;
    }
    if (it->next) {
	it->next->prev = it->prev;
    }
    free(it);
}

//-----------------------------------------

SST *InitServer(int domain, int type, int flags, int protocol, int rbuflen) {
    SST *serv = NULL;
    if ((serv = (SST *) malloc(sizeof (SST))) != NULL) {
	memset(serv, 0, sizeof (SST));

	serv->hints.ai_family = domain;
	serv->hints.ai_socktype = type;
	serv->hints.ai_flags = flags;
	serv->hints.ai_protocol = protocol;

	serv->bufftoclient = rbuflen;
    }
    return serv;
}

void FinitServer(SST *serv) {
    LCL *tmp = NULL;
    if (serv) {
	while (serv->first) {
	    tmp = serv->first->next;
	    close(serv->first->client);
	    free(serv->first);
	    serv->first = tmp;
	}
	free(serv);
    }
}

int SetCallBacksS(SST *serv,
	void (*OnConnected)(SST *serv, LCL *cl),
	int (*OnRead)(SST *serv, LCL *cl, char *buf, int len),
	int (*OnWrite)(SST *serv, LCL *cl, int len),
	void (*OnDisconnected)(SST *serv, LCL *cl),
	void (*OnErr)(SST *serv, int err)) {
    int count = 0;
    if (OnConnected) {
	serv->OnConnected = OnConnected;
	count++;
    }
    if (OnRead) {
	serv->OnRead = OnRead;
	count++;
    }
    if (OnWrite) {
	serv->OnWrite = OnWrite;
	count++;
    }
    if (OnDisconnected) {
	serv->OnDisconnected = OnDisconnected;
	count++;
    }
    if (OnErr) {
	serv->OnErr = OnErr;
	count++;
    }
    return count;
}

static void *MainReadero(void *data) {
    LCL *it = (LCL *) data;
    SST *serv = (SST *) it->ServST;
    pthread_detach(it->Rthread);

    char *bin = (char *) malloc(it->buflen);
    int readl = 0, all = 0;

    while ((readl = recv(it->client, &bin[all], it->buflen, MSG_WAITALL)) >= 0) {
	all += readl;
	if (readl == 0)
	    if (serv->OnRead) {
		if (serv->OnRead(serv, it, bin, all) != 0) {
		    close(it->client);
		    if (serv->OnDisconnected)serv->OnDisconnected(serv, it);
		    DelPlease(it);
		    break;
		} else {
		    it->count.AllRead += it->count.PrevRead;
		    it->count.PrevRead = all;
		    serv->count.AllRead += serv->count.PrevRead;
		    serv->count.PrevRead = all;
		    all = 0;
		}
	    }

	bin = realloc(bin, all + it->buflen);

    }

    if (readl < 0 && errno != EINTR) {
	if (serv->OnErr)serv->OnErr(serv, -100);
    } else {
	it->count.AllRead += it->count.PrevRead;
	it->count.PrevRead = all;
	serv->count.AllRead += serv->count.PrevRead;
	serv->count.PrevRead = all;
    }

    return NULL;
}

static void *MainWritero(void *data) {
    LCL *it = (LCL *) data;
    SST *serv = (SST *) it->ServST;
    pthread_detach(it->Wthread);

    if (serv->OnWrite)serv->OnWrite(serv, it, it->count.PrevWrite);

    return NULL;
}

static void *MainAccepto(void *data) {
    SST *serv = (SST *) data;
    LCL *temp = NULL;
    pthread_detach(serv->treads.AcptThread);
    int client = 0;

    while ((client = accept(serv->sock, NULL, NULL)) > 0) {

	if (!serv->first) {
	    serv->first = serv->end = temp = AddPlease(NULL);
	    serv->first->ServST = &serv;
	} else temp = AddPlease(temp);

	temp->client = client;
	temp->buflen = serv->bufftoclient;

	if (serv->OnConnected)serv->OnConnected(serv, temp);
	else if (serv->OnErr)serv->OnErr(serv, -20); //Важный кальбэк упущен.

	if (pthread_create(&temp->Rthread, NULL, MainReadero,
		(void *) & temp) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -10);
	    close(temp->client);
	    if (serv->OnDisconnected)serv->OnDisconnected(serv, temp);
	    DelPlease(temp);
	}

    }
    return NULL;
}

int Listen(SST *serv, char *host, char *port) {
    int status;
    int one = 1;
    char *hostint = NULL;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

    if (!host || strlen(host) < 3)hostint = (char *) "localhost";
    else hostint = host;

    if ((status = getaddrinfo(hostint, port, &serv->hints, &servinfo)) != 0) {
	if (serv->OnErr)serv->OnErr(serv, -1);
	return -1;
    } else {
	//Можно продолжать-???:!
	for (tservinfo = servinfo; tservinfo != NULL; tservinfo = tservinfo->ai_next) {
	    serv->sock = socket(tservinfo->ai_family, tservinfo->ai_socktype,
		    tservinfo->ai_protocol);
	    if (serv->sock == -1)
		continue;

	    if (setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof (one)) < 0) {
		if (serv->OnErr)serv->OnErr(serv, -4);
	    }

	    if (bind(serv->sock, tservinfo->ai_addr, tservinfo->ai_addrlen) != -1)
		break; /* Success */

	    close(serv->sock);
	}

	if (tservinfo == NULL) { /* No address succeeded */
	    if (serv->OnErr)serv->OnErr(serv, -2);
	    return -2;
	} else if (listen(serv->sock, 16) < 0) {
	    if (serv->OnErr)serv->OnErr(serv, -3);
	    close(serv->sock);
	    serv->sock = 0;
	    freeaddrinfo(servinfo);
	    return -3;
	}
	freeaddrinfo(servinfo);


	if (pthread_create(&serv->treads.AcptThread, NULL, MainAccepto,
		(void *) & serv) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -5);
	    close(serv->sock);
	    serv->sock = 0;
	}

    }
    return 0;
}

int SendToClient(LCL *cl, char *buf, int len) {
    int total = 0; // Сколько уже
    int bytesleft = len; // Сколько нужно
    int n = 0;
    SST *serv = (SST *) cl->ServST;

    while (total < len) {
	n = send(cl->client, buf + total, bytesleft, 0);
	if (n == -1) {
	    if (serv->OnErr)serv->OnErr(serv, -1000);
	    break;
	}
	total += n;
	bytesleft -= n;
    }

    if (n >= 0) {
	cl->count.AllWrite += cl->count.PrevWrite;
	cl->count.PrevWrite = total;

	serv->count.AllWrite += cl->count.PrevWrite;
	serv->count.PrevWrite = cl->count.PrevWrite;

	if (pthread_create(&cl->Wthread, NULL, MainWritero,
		(void *) & cl) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -2000);
	}

    } else {
	close(cl->client);
	DelPlease(cl);
    }
    return total;

}
