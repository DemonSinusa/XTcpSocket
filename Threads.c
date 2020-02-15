/*
 * File:   Threads.cpp
 * Author: Anton
 *
 * Created on 24 ноября 2019 г., 16:34
 */
//#include "stdafx.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "Threads.h"

#ifdef _WIN32

DWORD WINAPI NormThread(CONST LPVOID lpParam) {
	TAC *tac = (TAC *)lpParam;
	((TAC *)tac)->entrypoint(((TAC *)tac)->attrs);
	tac->obj->thread = NULL;
	free(tac);
	_wCrossThreadExit();
}

DLL_EXPORT int _wCrossThreadCreate(CPT *tr, void (*entrypoint) (void *),void *attrs){
	tr->errcode=0;
	TAC *nt = (TAC *)malloc(sizeof(TAC));
	nt->entrypoint = entrypoint;
	nt->attrs = attrs;
	nt->obj = tr;
	if((tr->thread=CreateThread(NULL, 0, NormThread, nt, 0, tr->ThreadId))){
		tr->status=STATE_EXCITED;
	}else{
		free(nt);
		tr->errcode=GetLastError();
	}
	return tr->errcode;
}


DLL_EXPORT void _wCrossThreadClose(CPT *tr){
	CloseHandle(tr->thread);
	tr->thread = NULL;
}
DLL_EXPORT long _wCrossThreadPause(CPT *tr){
	if(tr->status==STATE_EXCITED){
	tr->ThreadTime=SuspendThread(tr->thread);
	tr->status=STATE_READY;
	}
	return tr->ThreadTime;
}
DLL_EXPORT long _wCrossThreadResume(CPT *tr){
	if(tr->status==STATE_READY){
	tr->ThreadTime=ResumeThread(tr->thread);
	tr->status=STATE_EXCITED;
	}
	return tr->ThreadTime;
}

#else

static void *NormThread(void *tac){
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    ((TAC *)tac)->entrypoint(((TAC *)tac)->attrs);
	((TAC *)tac)->obj->thread = 0;
    free(tac);
    _wCrossThreadExit();
}

DLL_EXPORT int _wCrossThreadCreate(CPT *tr,void (*entrypoint) (void *),void *attrs){
	clockid_t c_id;
	struct timespec mt1;
	tr->errcode=0;

	TAC *nt=(TAC *)malloc(sizeof(TAC));
	nt->entrypoint=entrypoint;
	nt->attrs=attrs;
	nt->obj = tr;
	tr->errcode=pthread_create(&tr->thread,NULL,NormThread,nt);
	 		if(tr->thread!=0){
	 			tr->status=STATE_EXCITED;
	 			if(!pthread_getcpuclockid(tr->thread, &c_id)){
					if(!clock_gettime(c_id,&mt1))tr->ThreadTime=mt1.tv_sec*NANO+mt1.tv_nsec;
				}
			}
	return tr->errcode;
}

DLL_EXPORT void _wCrossThreadClose(CPT *tr){
	void *statusp=NULL;
	tr->errcode=pthread_cancel(tr->thread);
	pthread_join(tr->thread, &statusp);
	tr->thread = 0;
}

long _wCrossThreadPause(CPT *tr){
	clockid_t c_id;
	struct timespec mt1;
	if(tr->status==STATE_EXCITED){
	if(!pthread_getcpuclockid(tr->thread, &c_id)){
		if(!clock_gettime(c_id,&mt1)){
			tr->ThreadTime=(mt1.tv_sec*NANO+mt1.tv_nsec)-tr->ThreadTime;
		}
	}
	pthread_kill(tr->thread, SIGSTOP);
	tr->status=STATE_READY;
	}
	return tr->ThreadTime;
}
long _wCrossThreadResume(CPT *tr){
	if(tr->status==STATE_READY){
			pthread_kill(tr->thread, SIGCONT);
			tr->status=STATE_EXCITED;
	}
	return tr->ThreadTime;
}
#endif // __linux__

