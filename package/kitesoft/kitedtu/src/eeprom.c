#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include "SysCall.h"
#include "typedef.h"

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
static void ScanCacheData(void* arg)
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
void LoadParameter(void);
void InitCache()
{
  OpenLock(&m_CacheMutex);
  m_iCacheFd = open("/sys/bus/i2c/devices/0-0050/eeprom", O_RDWR);//打开文件
  if (m_iCacheFd < 0)
    printf("####i2c test device open failed####/n");
  LoadParameter();
  StartBackgroudTask(ScanCacheData, (void*)0, 99);
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
  printf("%s(G:%d,R:%d,Y:%d,[%ld,%ld],%d,%d,%d)\r\n", __func__, run_time, alert_time, ready_time, begin, end, state, RE, count);
  /*临界区上锁*/
  Lock(&m_CacheMutex);
  AT24CXX_Write(CACHE_DATA_ADDR + CacheDataHeader.Write * CACHE_DATA_SIZE, (void *)&data, sizeof(data));
  CacheDataHeader.Write++;
  if (CacheDataHeader.Write >= CACHE_DATA_COUNT)
    CacheDataHeader.Write = 0;
  /*临界区释放锁*/
  Unlock(&m_CacheMutex);
  printf("data size = %d cur pos = [%d, %d]\r\n", sizeof(data), CacheDataHeader.Read, CacheDataHeader.Write);
  return 0;
}
static unsigned char keys[] = "J3#o&Kn4";
#define POLY32 0x04C11DB7
void Rc4Encrypt(const unsigned char *pSrc, unsigned char *pDst, int iSize, unsigned char szKeys[8]);
void Rc4Decrypt(const unsigned char *pSrc, unsigned char *pDst, int iSize, unsigned char szKeys[8]);
unsigned long CalcCRC32(unsigned char *buf, int dlen, int poly, unsigned long CRCinit);
static void ParameterCheck(void)
{
  if (m_Parameter.ReportInterval == 0 || m_Parameter.ReportInterval > 180)
    m_Parameter.ReportInterval = 60;
  if (m_Parameter.RunDelay == 0 || m_Parameter.RunDelay > 30000)
    m_Parameter.RunDelay = 3000;
}
void SaveParameter(void)
{
  unsigned char tmp[sizeof(PARAMETER)];
  OpenLock(&m_CacheMutex);
  /* CRC32 usage. */
  m_Parameter.Magic = 0x85868483;
  ParameterCheck();
  m_Parameter.Crc = CalcCRC32((unsigned char *)&m_Parameter, sizeof(m_Parameter) - sizeof(uint32_t), POLY32, 0);
  Rc4Encrypt((unsigned char *)&m_Parameter, tmp, sizeof(m_Parameter), keys);
  AT24CXX_Write(0, (uint8_t *)tmp, sizeof(m_Parameter));
  /*临界区释放锁*/
  Unlock(&m_CacheMutex);
}
void LoadParameter(void)
{
  unsigned char tmp[sizeof(PARAMETER)];
  uint32_t u32CRC;
  /*临界区上锁*/
  Lock(&m_CacheMutex);
  AT24CXX_Read(0, (uint8_t *)tmp, sizeof(m_Parameter));
  /*临界区释放锁*/
  Unlock(&m_CacheMutex);
  Rc4Decrypt(tmp, (unsigned char *)&m_Parameter, sizeof(m_Parameter), keys);
  /* CRC32 usage. */
  u32CRC = CalcCRC32((unsigned char *)&m_Parameter, sizeof(m_Parameter) - sizeof(uint32_t), POLY32, 0);
  if (m_Parameter.Magic != 0x85868483 || u32CRC != m_Parameter.Crc) {
    memset((void *)&m_Parameter, 0, sizeof(PARAMETER));
    strcpy(m_Parameter.MqttServer, "1.15.67.50");
    m_Parameter.MqttPort = 1883;
    strcpy(m_Parameter.MqttUser, "mt7688");
    strcpy(m_Parameter.MqttPwd, "12345678");
    strcpy(m_Parameter.longitude, "30.326558");
    strcpy(m_Parameter.latitude, "120.088792");
    printf("Parameter Init\r\n");
  }
  ParameterCheck();
  printf("MachType = %d ReportInterval = %d RunDelay = %d\r\n", m_Parameter.MachType, m_Parameter.ReportInterval, m_Parameter.RunDelay);
}
#ifdef MQTT_SUPPORT
void SetMqttPwd(char *pwd)
{
  strcpy(m_Parameter.MqttPwd, pwd);
  SaveParameter();
}
#endif
static int shellMachTypeGet()
{
  return m_Parameter.MachType;
}
static int shellMachTypeSet(int val)
{
  m_Parameter.MachType = val;
  SaveParameter();
  return 0;
}