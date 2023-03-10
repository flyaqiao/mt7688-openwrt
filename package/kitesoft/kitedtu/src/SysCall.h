#ifndef __SYS_CALL_H__
#define __SYS_CALL_H__

#ifdef WIN32
#ifdef WIN32
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#include <windows.h>
#define MUTEX_T CRITICAL_SECTION
#else
#include <pthread.h>
#define MUTEX_T pthread_mutex_t
#define _popen popen
#define _pclose pclose
#endif
void Sleep(int ms);
int DelayMs(int ms);
int StartBackgroudTask(void* fn, void* param, int priority);

void OpenLock(MUTEX_T* p);
void CloseLock(MUTEX_T* p);
void Lock(MUTEX_T* p);
void Unlock(MUTEX_T* p);
#endif
