#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include <sys/time.h>
#include "gpio.h"
#include "SysCall.h"
#include "typedef.h"

static MUTEX_T m_AtportMutex;
#define DEFAULT_TIMEOUT 3000
static int ccid_ok = 0;
static int location_ok = 0;
static int csq = 0;
static int OpenCom()
{
  int serial_fd;
  struct termios old_cfg;
  struct termios new_cfg;
  char szComm[32];
  sprintf(szComm, "/dev/ttyUSB1");
  //1.打开串口设备文件
  serial_fd = open(szComm, O_RDWR | O_NOCTTY | O_NDELAY);
  if (serial_fd == -1) {
    sprintf(szComm, "/dev/ttyUSB2");
    serial_fd = open(szComm, O_RDWR | O_NOCTTY | O_NDELAY);
    if (serial_fd == -1) {
      printf("open fail\n");
      return -1;
    }
  }
  //2.备份现有的串口配置
  if (-1 == tcgetattr(serial_fd, &old_cfg)) {
    printf("tcgetattr\n");
    return -2;
  }
  //3.原始模式
  new_cfg = old_cfg;
  cfmakeraw(&new_cfg);
  //4.配置波特率
  //cfsetispeed(&new_cfg, B115200);
  //cfsetospeed(&new_cfg, B115200);
  cfsetspeed(&new_cfg, B115200);
  //5.设置控制标志
  new_cfg.c_cflag |= CREAD | CLOCAL;//使能数据接收和本地模式
  /* struct termio newtio; */
  new_cfg.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); /*Input*/
  new_cfg.c_oflag &= ~OPOST; /*Output*/
  //6.设置帧结构
  new_cfg.c_cflag &= ~CSTOPB;//1位停止位
  new_cfg.c_cflag &= ~CSIZE;//去掉数据位屏蔽
  new_cfg.c_cflag |= CS8;//8位数据位
  new_cfg.c_cflag &= ~PARENB;//无校验
  //7.设置阻塞模式
  //fcntl(serial_fd, F_SETFL, 0); //设为阻塞
  //收到1字节解除阻塞
  new_cfg.c_cc[VTIME] = 0;
  new_cfg.c_cc[VMIN] = 0;
  tcflush(serial_fd, TCIFLUSH);
  //8.使能配置生效
  if (-1 == tcsetattr(serial_fd, TCSANOW, &new_cfg)) {
    printf("tcgetattr\n");
    return -3;
  }
  return serial_fd;
}
static void CloseCom(int serial_fd)
{
  close(serial_fd);
}
int at_send_printf(char *resp, int size, const char *fmt, ...)
{
  char line[128];                                    //发送行
  int ret = 0;
  int serial_fd;
  va_list args;
  Lock(&m_AtportMutex);
  serial_fd = OpenCom();
  if (serial_fd < 0) {
    Unlock(&m_AtportMutex);
    return -3;
  }
  tcflush(serial_fd, TCIOFLUSH);
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  write(serial_fd, line, strlen(line));
  va_end(args);
  if (resp) {
    memset(resp, 0, size);
    uint32_t timeout = GetTickCount() + DEFAULT_TIMEOUT;
    while (1) {
      int len = read(serial_fd, resp + ret, size - ret);
      if (len > 0) {
        ret += len;
        if (strstr(resp, "OK") != NULL)
          break;
        if (strstr(resp, "ERROR") != NULL) {
          ret = -1;
          break;
        }
      }
      if (timeout < GetTickCount()) {
        ret = -2;
        break;
      }
    }
  }
  CloseCom(serial_fd);
  Unlock(&m_AtportMutex);
  return ret;
}
void GprsGetLocation(void)
{
  char buf[256];
  if (ccid_ok == 0 && at_send_printf(buf, sizeof(buf), "AT*I\r\n") > 0) {
    char *p = strstr(buf, "CCID");
    if (p && sscanf(p, "%*[^:]: %s", m_Parameter.CCID) == 1) {
      int MqttReconnect();
      printf("CCID: %s\r\n", m_Parameter.CCID);
      MqttReconnect();
      ccid_ok = 1;
    }
  }
  if (location_ok == 0) {
    at_send_printf(buf, sizeof(buf), "AT+CGDCONT?\r\n");
    at_send_printf(buf, sizeof(buf), "AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
    at_send_printf(buf, sizeof(buf), "AT+SAPBR=3,1,\"APN\",\"\"\r\n");
    at_send_printf(buf, sizeof(buf), "AT+SAPBR=1,1\r\n");
    at_send_printf(buf, sizeof(buf), "AT+SAPBR=2,1\r\n");
    if (at_send_printf(buf, sizeof(buf), "AT+CIPGSMLOC=1,1\r\n") > 0) {
      int n;
      char *p = strstr(buf, "CIPGSMLOC");
      if (p && sscanf(p, "%*[^:]: %d,%[^,],%[^,],", &n, m_Parameter.longitude, m_Parameter.latitude) == 3) {
        printf("POS:%s,%s\r\n", m_Parameter.longitude, m_Parameter.latitude);
        location_ok = 1;
      }
    }
  }
  if (at_send_printf(buf, sizeof(buf), "AT+CSQ\r\n") > 0) {
    char *p = strstr(buf, "CSQ");
    if (p)
      sscanf(p, "%*[^:]:%d,", &csq);
  }
}
int GprsGetCsq()
{
  return csq;
}
void GprsInit()
{
  OpenLock(&m_AtportMutex);
  at_send_printf(NULL, 0, "ATE0\r\n");
}