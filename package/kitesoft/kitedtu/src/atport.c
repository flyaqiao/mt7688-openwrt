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

#define DEFAULT_TIMEOUT 3000
static int serial_fd = -1;
int at_send_printf(const char *fmt, ...)
{
  char line[512];                                    //发送行
  va_list args;
  va_start(args, fmt);
  vsnprintf(line, sizeof(line), fmt, args);
  write(serial_fd, line, strlen(line));
  va_end(args);
#if 0
  memset(line, 0, sizeof(line));
  tcflush(serial_fd, TCIFLUSH);
  int len = read(serial_fd, line, sizeof(line) - 1);
  if (len > 0) {
    printf("%s", line); /* 打印从串口读出的字符串 */
    if (strstr(line, "OK") != NULL || strstr(line, "ERROR") != NULL)
      return 0;
  }
#endif
  return -1;
}
void get_location(void)
{
  at_send_printf("AT+CGDCONT?\r\n");
  Sleep(1000);
  at_send_printf("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n");
  Sleep(1000);
  at_send_printf("AT+SAPBR=3,1,\"APN\",\"\"\r\n");
  Sleep(1000);
  at_send_printf("AT+SAPBR=1,1\r\n");
  Sleep(1000);
  at_send_printf("AT+SAPBR=2,1\r\n");
  Sleep(1000);
  at_send_printf("AT+CIPGSMLOC=1,1\r\n");
  Sleep(1000);
}
void AtPortThread(void *arg)
{
  struct termios old_cfg;
  struct termios new_cfg;
  char szComm[32];
  sprintf(szComm, "/dev/ttyUSB1");
  //1.打开串口设备文件
  serial_fd = open(szComm, O_RDWR | O_NOCTTY);
  if (serial_fd == -1) {
    printf("open fail");
    return;
  }
  //2.备份现有的串口配置
  if (-1 == tcgetattr(serial_fd, &old_cfg)) {
    printf("tcgetattr");
    return;
  }
  //3.原始模式
  new_cfg = old_cfg;
  cfmakeraw(&new_cfg);
  //4.配置波特率
  cfsetispeed(&new_cfg, B115200);
  cfsetospeed(&new_cfg, B115200);
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
  tcflush(serial_fd, TCIOFLUSH);
  //收到1字节解除阻塞
  new_cfg.c_cc[VTIME] = 100;
  new_cfg.c_cc[VMIN] = 1;
  tcflush(serial_fd, TCIFLUSH); // 刷清输入缓存区
  //8.使能配置生效
  if (-1 == tcsetattr(serial_fd, TCSANOW, &new_cfg)) {
    printf("tcgetattr");
    return;
  }
  while (1) {
    memset(szComm, 0, sizeof(szComm));
    tcflush(serial_fd, TCIFLUSH);
    int len = read(serial_fd, szComm, sizeof(szComm) - 1);
    if (len > 0)
      printf("%s", szComm); /* 打印从串口读出的字符串 */
    Sleep(1);
  }
  //恢复配置
  if (-1 == tcsetattr(serial_fd, TCSANOW, &old_cfg)) {
    printf("tcgetattr");
    return;
  }
  close(serial_fd);
}