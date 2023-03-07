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
unsigned long GetTickCount();
#define _popen popen
#define _pclose pclose
#endif
#define MAX_LEN 128
typedef struct {
  time_t time;
  unsigned int code;
  unsigned int weft;
  unsigned int rsv[2];
  char msg[MAX_LEN];
} LOGITEM;

int DelayMs(int ms);
int StartBackgroudTask(void* fn, void* param);

void OpenLock(MUTEX_T* p);
void CloseLock(MUTEX_T* p);
void Lock(MUTEX_T* p);
void Unlock(MUTEX_T* p);
void lprintf(const char *fmt, ...);
int GetLogItems(LOGITEM* pItems, int start, int count);
int GetLogCount();
#endif
