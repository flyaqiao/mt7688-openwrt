#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <fcntl.h>
#include <assert.h>
#include "SysCall.h"
#ifndef WIN32
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#endif

void OpenLock(MUTEX_T* p)
{
  /* 互斥初始化. */
#ifdef WIN32
  InitializeCriticalSection(p);
#else
  pthread_mutex_init(p, NULL);
#endif
}
void CloseLock(MUTEX_T* p)
{
#ifdef WIN32
  DeleteCriticalSection(p);
#else
  pthread_mutex_destroy(p);
#endif
}
void Lock(MUTEX_T* p)
{
  /* 获取互斥锁 */
#ifdef WIN32
  EnterCriticalSection(p);
#else
  pthread_mutex_lock(p);
#endif
}
void Unlock(MUTEX_T* p)
{
  /* 释放互斥锁. */
#ifdef WIN32
  LeaveCriticalSection(p);
#else
  pthread_mutex_unlock(p);
#endif
}
int DelayMs(int ms)
{
#ifdef WIN32
  Sleep(ms);       /*Just to let the system breathe */
#else
  usleep(ms * 1000);
#endif
}

int StartBackgroudTask(void* fn, void* param, int priority)
{
#ifdef WIN32
  HANDLE hThread;
  DWORD  threadId;
  hThread = CreateThread(NULL, 0, fn, param, 0, &threadId); // 创建线程
  CloseHandle(hThread);
  return 0;
#else
  int ret;
  pthread_t tid;
  pthread_attr_t a; //线程属性
  struct sched_param param1;
  pthread_attr_init(&a);  //初始化线程属性
  //2、自己决定调度策略
  pthread_attr_setinheritsched(&a, PTHREAD_EXPLICIT_SCHED);
  //3、设置调度策略
  pthread_attr_setschedpolicy(&a, SCHED_FIFO);
  //4、设置优先级
  param1.sched_priority = priority;//sched_get_priority_max(SCHED_FIFO);// 线程1设置优先级为10
  pthread_attr_setschedparam(&a, &param1);
  //pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);      //设置线程属性
  ret = pthread_create(&tid, &a, fn, param);
  if (ret != 0) {
    printf("Thread create fail 1\r\n");
    return ret;
  }
  /* 销毁一个目标结构，并且使它在重新初始化之前不能重新使用 */
  //pthread_attr_destroy (&a);
  ret = pthread_detach(tid);
  if (ret != 0) {
    printf("Thread create fail 2\r\n");
    return ret;
  }
  return 0;
#endif
}
static char GetPrintChar(unsigned char ch)
{
  if (isprint(ch))
    return ch;
  return '.';
}
void PrintMemory8(unsigned int startAddr, unsigned int size)
{
  unsigned int i, j;
  for (i = 0; i < ((size + 15) / 16); i++) {
    printf("0x%08X ", startAddr + i * 16);
    for (j = 0; j < 16; j++)
      printf("%02X ", *(unsigned char*)(startAddr + i * 16 + j));
    for (j = 0; j < 16; j++)
      printf("%c", GetPrintChar(*(unsigned char*)(startAddr + i * 16 + j)));
    printf("\n");
  }
}
#ifndef WIN32
void Sleep(int ms)
{
  sleep(ms);
}
#endif
int execmd(char *input, char *output, int maxlen)
{
  int reslen;
  FILE *pipe;
  if (NULL == input || NULL == output)
    return -1;
  memset(output, 0, maxlen);
  pipe = _popen(input, "r");
  if (NULL == pipe) {
#ifndef WIN32
    printf("popen fail %d [ %s ]\r\n", errno, input);
#endif
    return -2;
  }
  reslen = fread(output, sizeof(char), maxlen, pipe);
  _pclose(pipe);
  return reslen;
}
