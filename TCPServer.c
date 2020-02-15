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
#include "TCPServer.h"

//Работа со списком клиентов (++/--) только
//Добавляем элемент следом от позиции to
LCL *Add(SCT *item,LCL *to,SST *serv){
	SCT *it=item;
	LCL *poz=to,*it_s=NULL;
	SST *s=serv;
	if(it&&s){
        if ((it_s = (LCL *)malloc(sizeof(LCL))) != NULL) {
			memset(it_s, 0x00, sizeof(LCL));
			it->ServST=s;
			it_s->Client=it;
            if(poz){
				it_s->next=poz->next;
				it_s->prev=poz;
				if(poz->next)poz->next->prev=it_s;
            }
        }
	}
	return it_s;
}

//Удаляем элемент по позиции возвращаем предидущий иначе последующий
LCL *Del(LCL *item){
	LCL *it=item,*ret=NULL;
    if(it->next){
    	it->next->prev=it->prev;
		ret=it->next;
    }
    if(it->prev){
		it->prev->next=it->next;
		ret=it->prev;
    }
    free(it);
    return ret;
}

//-----------------------------------------
//Серверно клиентские кальбаки

int cl_OnRead(SCT *cl, char *buf, int len){
	SST *serv=(SST *)cl->ServST;
	if(serv){
	if(serv->OnRead)return serv->OnRead(serv,cl,buf,len);
	else return 0;
	}
	return -1;
}
int cl_OnWrite(SCT *cl, int len){
	SST *serv=(SST *)cl->ServST;
	if(serv){
	if(serv->OnWrite)return serv->OnWrite(serv,cl,len);
	else return 0;
	}
	return -1;
}
void cl_OnDisconnected(SCT *cl){
	SST *serv=(SST *)cl->ServST;
	LCL *cur=NULL;
	if(serv){
	if(serv->OnDisconnected) serv->OnDisconnected(serv,cl);
	if((cur=serv->first))do{
		if(cur->Client==cl){
			if(serv->first==cur)serv->first=serv->first->next;
			if(serv->end==cur)serv->end=serv->end->prev;
			FinitClient(cur->Client);
            Del(cur);
			break;
		}
	}while((cur=cur->next));
	}
}
void cl_OnErr(SCT *cl, int err){
	SST *serv=(SST *)cl->ServST;
	if(serv){
	if(serv->OnErr)serv->OnErr(serv,cl,err);
	}
}

LCL *OnConnect(SST *serv,SOCKET sock){
	SST *s=serv;
	LCL *cur=NULL;
	SCT *item=NULL;
	if(serv&&sock>0){
        if((item=InitClient(s->hints.ai_family,s->hints.ai_socktype,0,s->hints.ai_protocol,s->bufftoclient))){
        		item->sock=sock;

				//Кальбаки расставляем тут
				item->OnErr = cl_OnErr;
				item->OnDisconnected = cl_OnDisconnected;
				item->OnWrite = cl_OnWrite;
				item->OnRead = cl_OnRead;

        		if(serv->first){
					if((cur=Add(item,s->end,s)))s->end=cur;
					else FinitClient(item);
        		}else{
        			if((cur=Add(item,NULL,s)))s->first=s->end=cur;
					else FinitClient(item);
        		}
        }
	}
	return cur;
}

//-----------------------------


SST *InitServer(int domain, int type, int flags, int protocol, int rbuflen) {
    SST *serv = NULL;

#ifdef WIN32_note

    WSADATA wsaData;

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
	fprintf(stderr,"Winsock не инициализирован!\n");
	WSACleanup();
	return NULL;
    } else fprintf(stdout,"Winsock всё ОК!\n");

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
		serv->first->Client->OnDisconnected(serv->first->Client);
//	    FinitClient(serv->first->Client);
//	    free(serv->first);
	    serv->first = tmp;
	}
	closesocket(serv->sock);
	free(serv);
    }

#ifdef WIN32_note

    if (WSACleanup())
	fprintf(stderr,"Чот ничистицца...\n");
    else
	fprintf(stdout,"Зачищено!\n");

#endif
}

int SetCallBacksS(SST *serv,
	void (*OnConnected)(SST *serv, SCT *cl),
	int (*OnRead)(SST *serv, SCT *cl, char *buf, int len),
	int (*OnWrite)(SST *serv, SCT *cl, int len),
	void (*OnDisconnected)(SST *serv, SCT *cl),
	void (*OnErr)(SST *serv,SCT *cl, int err)) {
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


void MainAccepto(void *data) {
    SST *serv = (SST *) data;
    LCL *temp = NULL;
    SOCKET client = 0;
	unsigned int addrlen = 0;
    serv->hints.ai_addr = (struct sockaddr *)malloc(sizeof(struct sockaddr));
	memset(serv->hints.ai_addr, 0x00, sizeof(struct sockaddr));
	serv->hints.ai_addrlen= addrlen = sizeof(struct sockaddr);

    while ((client = accept(serv->sock, serv->hints.ai_addr, &addrlen)) != INVALID_SOCKET) {
		serv->hints.ai_addrlen = addrlen;

	if((temp=OnConnect(serv,client))){
		if(serv->OnConnected)serv->OnConnected(serv,temp->Client);
        else if(serv->OnErr)serv->OnErr(serv,NULL,-20);	//Важный кальбэк упущен.
		temp->Client->Read(temp->Client);
	}
    }
    free(serv->hints.ai_addr);

}

int Listen(SST *serv, char *host, char *port) {
    int status=0,retval=0;
    int one = 1,Ok=0;
    char *hostint = NULL;
    struct addrinfo *servinfo = NULL, *tservinfo = NULL; // указатель на результаты вызова

    if(serv->hints.ai_flags&~AI_PASSIVE){
    if(host&&strlen(host) > 3){
		hostint=host;
    }else hostint =(char *)"localhost";
    }

    if ((status = getaddrinfo(hostint, port, &serv->hints, &servinfo)) != 0) {
	if (serv->OnErr)serv->OnErr(serv,NULL, -10);
#ifdef _DEBUG
#ifdef WIN32
	fprintf(stderr,"getaddrinfo(%s,%s): %ws\n",host,port, gai_strerror(status));
	#else
	fprintf(stderr,"getaddrinfo(%s,%s): %s\n",host,port, gai_strerror(status));
	#endif // WIN32
#endif // _DEBUG_
	return -1;
    } else {
	for (tservinfo = servinfo; tservinfo != NULL; tservinfo = tservinfo->ai_next) {

	    if ((serv->sock = socket(tservinfo->ai_family, tservinfo->ai_socktype,tservinfo->ai_protocol))==-1)continue;

	    if (setsockopt(serv->sock, SOL_SOCKET, SO_REUSEADDR, (char *) &one, sizeof (one)) < 0) {
		if (serv->OnErr)serv->OnErr(serv,NULL, -30);
	    }
	    if (bind(serv->sock, tservinfo->ai_addr, (int)tservinfo->ai_addrlen) != -1){
			Ok=1;
			break; //Всё ОК
	    }

	    closesocket(serv->sock);
	}
	freeaddrinfo(servinfo);

	if (Ok) { //Успешно забиндялись
		if (listen(serv->sock, 16) < 0) {
	    	if (serv->OnErr)serv->OnErr(serv,NULL, -40);
	    		retval=-3;
		}else{	//Все прошло успешно слушаем.
			if ((status = _wCrossThreadCreate(&serv->threads.AcptThread,MainAccepto, serv)) != 0) {
	    		if (serv->OnErr)serv->OnErr(serv,NULL, -60);
	    		retval=-4;
			}
		}
	} else{ //Не забиндились
		 if (serv->OnErr)serv->OnErr(serv,NULL, -20);
	    retval= -2;
	}

    }

    if(retval<0){
		closesocket(serv->sock);
	    serv->sock = 0;
    }

    return retval;
}

void Todeaf(SST *serv){
	_wCrossThreadClose(&serv->threads.AcptThread);
	closesocket(serv->sock);
	serv->sock = 0;
}


int SendToClient(SCT *cl, char *buf, int len) {
    return cl->Write(cl,buf,len);
}


