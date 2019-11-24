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
#ifdef WIN32

    WSADATA wsaData;

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
	printf("Winsock не инициализирован!\n");
	WSACleanup();
	return NULL;
    } else printf("Winsock всё ОК!\n");

#endif
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
	    closesocket(serv->first->client);
	    free(serv->first);
	    serv->first = tmp;
	}
	closesocket(serv->sock);
	free(serv);
    }

#ifdef WIN32

    if (WSACleanup())
	printf("Чот ничистицца...\n");
    else
	printf("Зачищено!\n");

#endif
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
    } else serv->OnConnected = NULL;
    if (OnRead) {
	serv->OnRead = OnRead;
	count++;
    } else serv->OnRead = NULL;
    if (OnWrite) {
	serv->OnWrite = OnWrite;
	count++;
    } else serv->OnWrite = NULL;
    if (OnDisconnected) {
	serv->OnDisconnected = OnDisconnected;
	count++;
    } else serv->OnDisconnected = NULL;
    if (OnErr) {
	serv->OnErr = OnErr;
	count++;
    } else serv->OnErr = NULL;
    return count;
}

static void *MainReadero(void *data) {
    LCL *it = (LCL *) data;
    SST *serv = (SST *) it->ServST;

    char *bin = (char *) malloc(it->buflen);
    int readl = 0, all = 0;

    //MSG_WAITALL - чего ты ЖДЁЁЁЁшшш? ЧЕГО ты ждеЕЁЁёшш????!!!
    while ((readl = recv(it->client, &bin[all], it->buflen, 0)) >= 0) {
	all += readl;
	if (readl == 0 || readl < it->buflen) {
	    if (serv->OnRead) {
		if (serv->OnRead(serv, it, bin, all) != 0) {
		    closesocket(it->client);
		    if (serv->OnDisconnected)serv->OnDisconnected(serv, it);
		    DelPlease(it);
		    break;
		}
	    }
	    it->count.AllRead += it->count.PrevRead;
	    it->count.PrevRead = all;
	    serv->count.AllRead += serv->count.PrevRead;
	    serv->count.PrevRead = all;
	    all = 0;
	}

	bin = realloc(bin, all + it->buflen);

    }

    free(bin);

    pthread_exit(NULL);
}

static void *MainWritero(void *data) {
    LCL *it = (LCL *) data;
    SST *serv = (SST *) it->ServST;

    if (serv->OnWrite)serv->OnWrite(serv, it, it->count.PrevWrite);

    pthread_exit(NULL);
}

static void *MainAccepto(void *data) {
    SST *serv = (SST *) data;
    LCL *temp = NULL;
    int client = 0;
    int one = 0;
    pthread_attr_t tattr;

    while ((client = accept(serv->sock, serv->hints.ai_addr, &serv->hints.ai_addrlen)) > 0) {

	if (!serv->first) {
	    serv->first = serv->end = temp = AddPlease(NULL);
	    serv->first->ServST = serv;
	} else temp = AddPlease(temp);

	temp->client = client;
	temp->buflen = serv->bufftoclient;

	if (serv->OnConnected)serv->OnConnected(serv, temp);
	else if (serv->OnErr)serv->OnErr(serv, -20); //Важный кальбэк упущен.

	pthread_attr_init(&tattr);
	if ((one = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED))) {
	    if (serv->OnErr)serv->OnErr(serv, -30);
	}

	if (pthread_create(&temp->Rthread, NULL, MainReadero, temp) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -10);
	    closesocket(temp->client);
	    if (serv->OnDisconnected)serv->OnDisconnected(serv, temp);
	    DelPlease(temp);
	}

    }
    pthread_exit(NULL);
}

int Listen(SST *serv, char *host, char *port) {
    int status;
    int one = 1;
    pthread_attr_t tattr;
    char *hostint = NULL;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

    if (host && strlen(host) < 3)hostint = (char *) "localhost";
    else hostint = host;

    if ((status = getaddrinfo(hostint, port, &serv->hints, &servinfo)) != 0) {
	if (serv->OnErr)serv->OnErr(serv, -10);
	return -1;
    } else {
	for (tservinfo = servinfo; tservinfo != NULL; tservinfo = tservinfo->ai_next) {
	    serv->sock = socket(tservinfo->ai_family, tservinfo->ai_socktype,
		    tservinfo->ai_protocol);
	    if (serv->sock == -1)
		continue;

	    if (setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof (one)) < 0) {
		if (serv->OnErr)serv->OnErr(serv, -30);
	    }

	    if (bind(serv->sock, tservinfo->ai_addr, tservinfo->ai_addrlen) != -1)
		break; //Всё ОК

	    closesocket(serv->sock);
	}

	if (tservinfo == NULL) { //А адресов то небыло
	    if (serv->OnErr)serv->OnErr(serv, -20);
	    return -2;
	} else if (listen(serv->sock, 16) < 0) {
	    if (serv->OnErr)serv->OnErr(serv, -40);
	    closesocket(serv->sock);
	    serv->sock = 0;
	    freeaddrinfo(servinfo);
	    return -3;
	}
	freeaddrinfo(servinfo);

	pthread_attr_init(&tattr);
	if ((one = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED))) {
	    if (serv->OnErr)serv->OnErr(serv, -50);
	}
	if ((status = pthread_create(&serv->treads.AcptThread, &tattr, MainAccepto, serv)) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -60);
	    closesocket(serv->sock);
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
    int one = 0;
    pthread_attr_t tattr;

    while (total < len && n != -1) {
	n = send(cl->client, &buf[total], bytesleft, 0);
	if (n == -1) {
	    if (serv->OnErr)serv->OnErr(serv, -100);
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

	pthread_attr_init(&tattr);
	if ((one = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED))) {
	    if (serv->OnErr)serv->OnErr(serv, -50);
	}

	if (pthread_create(&cl->Wthread, &tattr, MainWritero, cl) != 0) {
	    if (serv->OnErr)serv->OnErr(serv, -60);
	}

    } else {
	closesocket(cl->client);
	DelPlease(cl);
    }
    return total;

}
