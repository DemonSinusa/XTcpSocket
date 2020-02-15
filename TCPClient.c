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

void ReadThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    char *bin = (char *)malloc(cl->buflen);
    int readl = 0, all = 0;


    while((readl = recv(cl->sock, &bin[all], cl->buflen, 0))>=0){
				all+=readl;
				if(readl==0){
#ifdef _DEBUG
						fprintf(stdout,"Прочиталось 0 от %d",(int)cl->sock);
#endif
					if(cl->OnRead){
                        if(cl->OnRead(cl,bin,all)!=0){
                            cl->Close(cl);
                            break;
                        }
					}
					all=0;
                    continue;
				}else{
					cl->count.PrevRead=all;
					cl->count.AllRead+=cl->count.PrevRead;
					if(readl<cl->buflen){	//Признак дочитанности
						if(cl->OnRead){
                        if(cl->OnRead(cl,bin,all)!=0){
                            cl->Close(cl);
                            break;
                        }
					}
					//Дочитали, отработали,очистили и создали новый буфер.
					all=0;
					free(bin);
					bin=(char *)malloc(cl->buflen);

					}else{		//Признак необходимости дописать буфер, теперь без одинаковых выделений
						bin=realloc(bin,all+cl->buflen);
					}
				}
    }

    if(readl<0&&cl->sock!=0){
		    if (cl->OnErr)cl->OnErr(cl, -100);
		    	cl->Close(cl);
    }

    free(bin);

}

void WriteThreadMain(void *clntSock) {
    SCT *cl = (SCT *) clntSock;
    if (cl->OnWrite)cl->OnWrite(cl, cl->count.PrevWrite);
	free(cl);
}

int read_client(SCT *cl){
	if (_wCrossThreadCreate(&cl->Treadrs.Rthread, ReadThreadMain, cl) != 0) {
			if (cl->OnErr)cl->OnErr(cl, -60);
		return 0;
	}else return 1;
}

int write_client(SCT *cl,char *buf,int len){
	int total = 0; // Сколько уже
    int bytesleft = len; // Сколько нужно
    int sendl=0;
	SCT *s_cl = NULL;

    while(bytesleft){
		    sendl = send(cl->sock, &buf[total], bytesleft, 0);
		    if(sendl>=0){				//Нормальный режим передачи
				total += sendl;
				bytesleft -= sendl;
		    }else{						//Ошибка
		    	if (cl->OnErr)cl->OnErr(cl, -100);
		    	cl->Close(cl);
		    	break;
		    }
    }
    if(bytesleft==0){
    	cl->count.PrevWrite=total;
		cl->count.AllWrite+=cl->count.PrevWrite;
		if (cl->sock != 0) {
			s_cl = (SCT*)malloc(sizeof(SCT));
			memcpy(s_cl, cl, sizeof(SCT));
			if (_wCrossThreadCreate(&s_cl->Treadrs.Wthread, WriteThreadMain, s_cl) != 0) {
				if (cl->OnErr)cl->OnErr(cl, -60);
				free(s_cl);
			}
			cl->Treadrs.Wthread = s_cl->Treadrs.Wthread;
		}
    }else total=-1;

    return total;
}

int open_client(SCT *cl, char *host, char *port){
	int status=0,Ok=0;
    struct addrinfo *clientinfo = NULL, *tclientinfo = NULL; // указатель на результаты вызова

    if ((status = getaddrinfo(host, port, &cl->hints, &clientinfo)) != 0) {
	if (cl->OnErr)cl->OnErr(cl, -10);
#ifdef _DEBUG
#ifdef WIN32
	fprintf(stderr,"getaddrinfo(%s,%s): %ws\n",host,port, gai_strerror(status));
	#else
	fprintf(stderr,"getaddrinfo(%s,%s): %s\n",host,port, gai_strerror(status));
	#endif // WIN32
#endif // _DEBUG_
		return -1;
    } else {
	//Можно продолжать-???:!
	for (tclientinfo = clientinfo; tclientinfo != NULL; tclientinfo = tclientinfo->ai_next) {

	    if ((cl->sock = socket(tclientinfo->ai_family, tclientinfo->ai_socktype,tclientinfo->ai_protocol))==-1)continue;

	    if (connect(cl->sock, tclientinfo->ai_addr, (int)tclientinfo->ai_addrlen) != -1){
			Ok=1;
			break; // Зашибись
	    }

	    closesocket(cl->sock);
	}
	freeaddrinfo(clientinfo);

	if (!Ok) { // А адрес так и не вышел)
	    if (cl->OnErr)cl->OnErr(cl, -20);
	    return -2;
	}

    }
	return 0;
}

void close_client(SCT *cl){
	if(cl){
		//При потоковости рубануть сперва залупленых
	//	if (cl->Treadrs.Wthread.thread)_wCrossThreadClose(&cl->Treadrs.Wthread);
		if (cl->Treadrs.Rthread.thread)_wCrossThreadClose(&cl->Treadrs.Rthread);
		closesocket(cl->sock);
		if (cl->OnDisconnected)cl->OnDisconnected(cl);
		cl->sock = 0;
	}
}

SCT *InitClient(int domain, int type, int flags, int protocol, int rbuflen) {
    SCT *cl = NULL;

#ifdef WIN32_note

    WSADATA wsaData;

    if (WSAStartup(WINSOCK_VERSION, &wsaData)) {
	fprintf(stderr,"Winsock не инициализирован!\n");
	WSACleanup();
	return NULL;
    } else fprintf(stdout,"Winsock всё ОК!\n");

#endif

    if ((cl = (SCT *) malloc(sizeof (SCT))) != NULL) {
	memset(cl, 0x00, sizeof (SCT));

	cl->Close=close_client;
	cl->Read=read_client;
	cl->Write=write_client;
	cl->Open=open_client;

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
		if (cl->sock != 0)closesocket(cl->sock);
	free(cl);
    }
#ifdef WIN32_note

    if (WSACleanup())
	fprintf(stderr,"Чот ничистицца...\n");
    else
	fprintf(stdout,"Зачищено!\n");

#endif

}



int SetCallBacksC(SCT *cl,
	int (*OnRead)(SCT *cl, char *buf, int len),
	int (*OnWrite)(SCT *cl, int len),
	void (*OnDisconnected)(SCT *cl),
	void (*OnErr)(SCT *cl, int err)) {
    int count = 0;

    if (OnRead) {
    	if(cl->OnRead&&cl->Treadrs.Rthread.thread){
	    	_wCrossThreadClose(&cl->Treadrs.Rthread);
	    	cl->Treadrs.Rthread.thread=0;
    	}
    	cl->OnRead = OnRead;
		count++;
    } else cl->OnRead = NULL;


    if (OnWrite) {
    	if(cl->OnWrite&&cl->Treadrs.Wthread.thread){
                _wCrossThreadClose(&cl->Treadrs.Wthread);
				cl->Treadrs.Wthread.thread=0;
    	}
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


int Open(SCT *cl, char *host, char *port) {
	if(cl)
    return cl->Open(cl,host,port);
    else return -1;
}

int Start_Read(SCT *cl,int rbuflen){
	if(cl){
		if(rbuflen>0)cl->buflen=rbuflen;
		return cl->Read(cl);
	}
	return -1;
}

int Send(SCT *cl, char *buf, int len) {
	if(cl)return cl->Write(cl,buf,len);
	else return 0;
}

void Close(SCT *cl){
	if(cl)cl->Close(cl);
}
