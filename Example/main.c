/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/*
 * File:   main.cpp
 * Author: senjord
 *
 * Created on 27 марта 2018 г., 10:18
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


SST *serv = NULL;

char *logalhst = (char *) "localhost";
char *zeport = (char *) "12345";
char overbuffera[256];
int globalclient=0;

/*
 *btg.pool.minergate.com
 */


void OnDisconnectC(SCT *cl) {
    printf("Клиент №%d отрублен\n", cl->sock);
    FinitClient(cl);
    globalclient--;
}

int OnReadC(SCT *cl, char *buf, int len) {
    printf("Клиент №%d словил: %s\n", cl->sock, buf);
    if (Send(cl, buf, len) == len) {
	return 0;
    } else return -1;
}

int OnWriteC(SCT *cl, int len) {
    printf("Серверу от №%d отправлено:%dБайт\n", cl->sock, cl->count.PrevWrite);
    return 0;
}

void OnErrC(SCT *cl, int err) {
    if (cl->sock > 0)
	printf("Ошибка от клиента №%d под №%d\n", cl->sock, err);
    else printf("Ошибка клиента %d, подключение не удалось.\n", err);
}

void OnConnectedS(SST *serv, SCT *cl) {
    globalclient++;
    printf("Подрубился клиент №%d\n Итого их %d\n", cl->sock, globalclient);

}

int OnReadS(SST *serv, SCT *cl, char *buf, int len) {
    printf("Клиент №%d ответил на %dБайт:%s\n", cl->sock, len, buf);
    if (SendToClient(cl, buf, len) == len) {
	SCT *FreeNeutronNew = InitClient(AF_UNSPEC, SOCK_STREAM, 0, IPPROTO_TCP, 32767);
	SetCallBacksC(FreeNeutronNew, OnReadC, OnWriteC, OnDisconnectC, OnErrC);
	if (!Open(FreeNeutronNew, logalhst, zeport)) {
		FreeNeutronNew->Read(FreeNeutronNew);
	    snprintf(overbuffera, 255, "Следующий после №%d- №%d, говорит серверок №%d\n", cl->sock, FreeNeutronNew->sock, serv->sock);
	    Send(FreeNeutronNew, overbuffera, strlen(overbuffera));
	} else printf("Создание не прижилось...( активных клиентов по прежнему:%d\n", globalclient);
	return 0;
    } else return -1;
}

int OnWriteS(SST *serv, SCT *cl, int len) {
    printf("Клиенту №%d записано-%dБайт\n", cl->sock, cl->count.PrevWrite);
    return 0;
}

void OnErrS(SST *serv,SCT *cl, int err) {
	if(!cl)
    printf("Ошибка №%d сервер:%d\n", err, serv->sock);
    else printf("Ошибка №%d сервер:%d,клиент:%d\n", err, serv->sock,cl->sock);
}

int main(int argc, char** argv) {
    //   TCPServer serv(3257, new_client);
    serv = InitServer(AF_UNSPEC, SOCK_STREAM, AI_PASSIVE, IPPROTO_TCP, 32768);
    SetCallBacksS(serv,
	    OnConnectedS,
	    OnReadS,
	    OnWriteS,
	    NULL,
	    OnErrS);

    Listen(serv, NULL, zeport);

  //      sleep(3);

    printf("Запускаюсь...");
    SCT *FreeNeutron = InitClient(AF_UNSPEC, SOCK_STREAM, 0, IPPROTO_TCP, 32767);
    printf("Инициализирован...");
    SetCallBacksC(FreeNeutron, OnReadC, OnWriteC, OnDisconnectC, OnErrC);
    printf("Закалбачен...");
    if (!Open(FreeNeutron, logalhst, zeport)) {
	FreeNeutron->Read(FreeNeutron);
	printf("Успешно подрублено.");
	char *f = (char *)"GET / HTTP/1.1 Host: ya.ru";
	printf("Посылка->%s\n", f);
	Send(FreeNeutron, f, strlen(f));
    }


    getchar();
    FinitServer(serv);
    return 0;
}

