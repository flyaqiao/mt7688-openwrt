#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "SysCall.h"

static MUTEX_T m_CacheMutex;
static int m_iCacheFd;
struct {
  int Write;
  int Read;
} CacheDataHeader;

typedef struct {
  uint32_t Magic : 16;
  uint32_t Count : 16;
  uint32_t ReadyTime : 8;
  uint32_t RunTime : 8;
  uint32_t AlertTime : 8;
  uint32_t State : 4;
  uint32_t RE : 4;
  uint32_t BeginTime;
  uint32_t EndTime;
} CacheData;
#define CACHE_DATA_ADDR 1024
#define CACHE_DATA_SIZE sizeof(CacheData)
#define CACHE_DATA_COUNT (31 * 1024 / CACHE_DATA_SIZE)
#define CACHE_DATA_MAGIC 0x5A93

static int AT24CXX_Write(int addr, unsigned char *buf, int len)
{
  //写操作
  lseek(m_iCacheFd, addr, SEEK_SET); //定位地址，地址是0x40
  if (write(m_iCacheFd, buf, len) < 0)//写入数据
    return -1;
  return 0;
}
static int AT24CXX_Read(int addr, unsigned char *buf, int len)
{
  //读操作
  lseek(m_iCacheFd, addr, SEEK_SET);//准备读，首先定位地址，因为前面写入的时候更新了当前文件偏移量，所以这边需要重新定位到0x40.
  if (read(m_iCacheFd, buf, len) < 0)//读数据 
    return -1;
  return 0;
}
static void ScanCacheData(void)
{
  int i;
  uint16_t magic = 0;
  /*临界区上锁*/
  Lock(&m_CacheMutex);
  CacheDataHeader.Read = 0;
  CacheDataHeader.Write = 0;
  for (i = 0; i < CACHE_DATA_COUNT; i++) {
    if (AT24CXX_Read(CACHE_DATA_ADDR + i * CACHE_DATA_SIZE, (void *)&magic, sizeof(magic)) != 0)
      break;
    if (magic == CACHE_DATA_MAGIC && CacheDataHeader.Read > i) {
      CacheDataHeader.Read = i;
      break;
    }
  }
  if (CacheDataHeader.Read < 0)
    CacheDataHeader.Read = 0;
  if (CacheDataHeader.Write < 0)
    CacheDataHeader.Write = 0;
  /*临界区释放锁*/
  Unlock(&m_CacheMutex);
  printf("Cache Pos: %d %d\r\n", CacheDataHeader.Read, CacheDataHeader.Write);
}
void InitCache()
{
  OpenLock(&m_CacheMutex);
  m_iCacheFd = open("/sys/bus/i2c/devices/0-0050/eeprom", O_RDWR);//打开文件
  if (m_iCacheFd < 0)
    printf("####i2c test device open failed####/n");
  StartBackgroudTask(ScanCacheData, (void*)0, 0);
}
int send_run_data(uint16_t run_time, uint16_t alert_time, uint16_t ready_time, time_t begin, time_t end, uint16_t state, uint16_t RE, uint16_t count)
{
  CacheData data;
  data.Magic = CACHE_DATA_MAGIC;
  data.ReadyTime = ready_time;
  data.RunTime = run_time;
  data.AlertTime = alert_time;
  data.State = state;
  data.Count = count;
  data.BeginTime = begin;
  data.EndTime = end;
  data.RE = RE;
  /*临界区上锁*/
  Lock(&m_CacheMutex);
  AT24CXX_Write(CACHE_DATA_ADDR + CacheDataHeader.Write * CACHE_DATA_SIZE, (void *)&data, sizeof(data));
  CacheDataHeader.Write++;
  if (CacheDataHeader.Write >= CACHE_DATA_COUNT)
    CacheDataHeader.Write = 0;
  /*临界区释放锁*/
  Unlock(&m_CacheMutex);
  printf("data size = %d cur pos = [%d, %d]\r\n", sizeof(data), CacheDataHeader.Read, CacheDataHeader.Write);
  //at_send_mqtt_log("cur pos = [%d, %d]", CacheDataHeader.Read, CacheDataHeader.Write);
  //tx_semaphore_put(&at_ready_sem);
  return 0;
}
