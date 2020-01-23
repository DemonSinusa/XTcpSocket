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

typedef struct _thread_attr_core_{
	int (*entrypoint) (void *);
	void *attrs;
	int retval;
}TAC;

typedef struct _cross_platform_thread_struct_{
	int status;
#ifdef _WIN32
	HANDLE thread;
	LPDWORD ThreadId;
#else
	pthread_t thread;
	pthread_attr_t thread_attr;
#endif
	int errcode;					//errcode потока
	long ThreadTime;                //Аптайм потока c момента предшествующей остановки
}CPT;


#ifdef __cplusplus
extern "C" {
#endif
DLL_EXPORT int _wCrossThreadCreate(CPT *tr,int (*entrypoint) (void *),void *attrs);
DLL_EXPORT int _wCrossThreadExit(int code);
DLL_EXPORT void _wCrossThreadClose(CPT *tr);
DLL_EXPORT long _wCrossThreadPause(CPT *tr);
DLL_EXPORT long _wCrossThreadResume(CPT *tr);

#ifdef __cplusplus
}
#endif

#endif
