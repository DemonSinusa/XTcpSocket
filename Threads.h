/*
 * File:   Threads.h
 * Author: Anton
 *
 * Created on 24 ноября 2019 г., 16:34
 */
#ifndef NEUROTHREADS_H
#define NEUROTHREADS_H
#include "libexport.h"

#ifdef _WIN32
#include <windows.h> //Нужна для потоков в winapi
#else
#include <pthread.h>
#include <signal.h>
#endif

#define STATE_NOT_READY 0
#define STATE_READY  1
#define STATE_EXCITED   2

#define NANO 1000000000L

typedef struct _cross_platform_thread_struct_{
	int status;
#ifdef _WIN32
	HANDLE thread;
	LPDWORD ThreadId;
#else
	pthread_t thread;
#endif
	int errcode;					//errcode потока
	long ThreadTime;                //Аптайм потока c момента предшествующей остановки
}CPT;

typedef struct _thread_attr_core_ {
	CPT *obj;
	void(*entrypoint) (void *);
	void *attrs;
}TAC;


#ifdef __cplusplus
extern "C" {
#endif

#ifdef _WIN32
#define _wCrossThreadExit();	ExitThread(0);
#else
#define _wCrossThreadExit();	pthread_exit(NULL);
#endif

DLL_EXPORT int _wCrossThreadCreate(CPT *tr,void (*entrypoint) (void *),void *attrs);
DLL_EXPORT void _wCrossThreadClose(CPT *tr);
DLL_EXPORT long _wCrossThreadPause(CPT *tr);
DLL_EXPORT long _wCrossThreadResume(CPT *tr);

#ifdef __cplusplus
}
#endif

#endif
