#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <sys/stat.h>
#include <sys/msg.h>
#include "gpio.h"
#include "SysCall.h"

#ifndef COUNTOF
#define COUNTOF(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif
#define MSG_KEY 0x1234
#define MSG_SIZE 512
#define MSG_TYPE 123

struct msg_data {
  long mtype;
  int  mindex;
  int  mvalue;
  time_t mtime;
};

typedef struct tagParameter {
  uint32_t Magic;
  uint32_t MachType; // 设备类型(0:机加工,1:注塑,2:HASS)
  uint32_t ReportInterval;  // 上报间隔(s)
  uint32_t CountReport; // 产量上报数量
  uint32_t LogReport; // 日志上报
  uint32_t RunDelay;    // 运行灯灭,判断待机的间隔(ms)
  uint32_t DebugMode;   // 测试模式
  char longitude[20]; // 经度
  char latitude[20];  // 纬度
#ifdef MQTT_SUPPORT
  char MqttServer[32];
  uint32_t MqttPort;
  char MqttPwd[32];
#endif
#ifdef FTP_SUPPORT
  char FtpServer[32];
  char FtpUser[32];
  char FtpPwd[32];
  uint32_t FtpPort;
#endif
  uint32_t Crc;
} PARAMETER;
typedef enum {
  MS_RUNNING = 0, // 生产
  MS_READY = 1,   // 待机
  MS_ALERT = 10,  // 故障
} RUN_STATE;
typedef struct {
  uint32_t ReadyTime : 8;
  uint32_t RunTime : 8;
  uint32_t AlertTime : 8;
  uint32_t State : 4;
  uint32_t RE : 4;
  uint32_t Count;
  uint32_t BeginTime;
  uint32_t EndTime;
} MACH_STATE;
static MACH_STATE m_RunData;
static uint8_t m_iComState = 0xFF;
static uint8_t m_iRedState = 0;
static uint8_t m_iYellowState = 0;
static uint8_t m_iGreenState = 0;
PARAMETER m_Parameter;
const uint32_t gpio_defs[] = { GPIO18, GPIO15, GPIO16, GPIO17, GPIO39, GPIO40 };
static void GpioThreadIrq(void* arg)
{
  int i;
  char buffer[16];
  int len;
  int value;
  struct msg_data data;
  struct pollfd fds[COUNTOF(gpio_defs)];
  data.mtype = MSG_TYPE; // 注意2
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    fds[i].fd = gpio_open_irq(gpio_defs[i], GPIO_EDGE_BOTH);
    fds[i].events = POLLPRI;
  }
  int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
  if (msg_id == -1) {
    printf("创建消息队列失败\n");
    return;
  }

  while (1) {
    if (poll(fds, COUNTOF(gpio_defs), -1) < 0) {
      perror("poll failed!\n");
      continue;
    }

    for (i = 0; i < COUNTOF(gpio_defs); i++) {
      if (fds[i].revents & POLLPRI) {
        //能进这里就代表触发事件发生了,我这里是置了一个标志
        if (lseek(fds[i].fd, 0, SEEK_SET) == -1) {
          perror("lseek value failed!\n");
          continue;
        }
        //一定要读取,不然会一直检测到紧急事件
        if ((len = read(fds[i].fd, buffer, sizeof(buffer))) == -1)  {
          perror("read value failed!\n");
          continue;
        }
        buffer[len] = 0;
        data.mindex = i;
        data.mvalue = atoi(buffer);
        data.mtime = time(NULL);
        //value = atoi(buffer);
        //printf("%d:%s", i, buffer);
        //sprintf(data.mtext, "%d:%d", i, value);
        // 向队列里发送数据
        if (msgsnd(msg_id, (void *)&data, sizeof(data) - sizeof(data.mtype), 0) == -1) {
          fprintf(stderr, "msgsnd failed\n");
        }
#if 0
        switch (i) {
        case 0: // COM
          m_iComState = value;
          break;
        case 1: // RED
          m_iRedState = value;
          break;
        case 2: // GREEN
          m_iGreenState = value;
          break;
        case 3: // YELLOW 
          m_iYellowState = value;
          break;
        case 4: // CLOSE
          if (m_Parameter.MachType == 1)  // 注塑
            m_RunData.Count++;
          break;
        case 5: // COUNT
          if (m_Parameter.MachType != 1)  // 非注塑
            m_RunData.Count++;
          break;
        }
#endif
      }
    }
  }
  msgctl(msg_id, IPC_RMID, NULL);
}
static void GpioThread(void* arg)
{
  int i;
  int fds[COUNTOF(gpio_defs)];
  uint8_t states[COUNTOF(gpio_defs)];
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    states[i] = 0;
    gpio_set_direction(gpio_defs[i], GPIO_INPUT);
    fds[i] = gpio_open(gpio_defs[i]);
  }

  while (1) {
    Sleep(1);
    for (i = 0; i < COUNTOF(gpio_defs); i++) {
      states[i] = (states[i] << 1) | (gpio_read(gpio_defs[i]) == GPIO_HIGH);
      if (states[i] == 0x80 || states[i] == 0x7F) {
        printf("Key Event %d => %d\n", i, states[i] == 0x80);
      }
    }
  }
}
static void CommThread(void* arg)
{
  struct termios old_cfg;
  struct termios new_cfg;
  char szComm[32];
  int sno = (int)arg;
  sprintf(szComm, "/dev/ttyS%d", sno);
  //1.打开串口设备文件
  int serial_fd = open(szComm, O_RDWR | O_NOCTTY);
  if (serial_fd == -1){
    perror("open fail");
    exit(-1);
  }
  //2.备份现有的串口配置
  if (-1 == tcgetattr(serial_fd, &old_cfg)) {
    perror("tcgetattr");
    exit(-1);
  }
  //3.原始模式
  new_cfg = old_cfg;
  cfmakeraw(&new_cfg);

  //4.配置波特率
  cfsetispeed(&new_cfg, B115200);
  cfsetospeed(&new_cfg, B115200);

  //5.设置控制标志
  new_cfg.c_cflag |= CREAD | CLOCAL;//使能数据接收和本地模式

  //6.设置帧结构
  new_cfg.c_cflag &= ~CSTOPB;//1位停止位
  new_cfg.c_cflag &= ~CSIZE;//去掉数据位屏蔽
  new_cfg.c_cflag |= CS8;//8位数据位
  new_cfg.c_cflag &= ~PARENB;//无校验	

  //7.设置阻塞模式
  tcflush(serial_fd, TCIOFLUSH);
  //收到1字节解除阻塞
  new_cfg.c_cc[VTIME] = 0;
  new_cfg.c_cc[VMIN] = 1;
  tcflush(serial_fd, TCIOFLUSH);

  //8.使能配置生效
  if (-1 == tcsetattr(serial_fd, TCSANOW, &new_cfg)) {
    perror("tcgetattr");
    exit(-1);
  }
  while (1) {
    if (sno == 1) {
      memset(szComm, 0, sizeof(szComm));
      int len = read(serial_fd, szComm, sizeof(szComm) - 1);
      if (len > 0)
        printf("RECV:%s", szComm); /* 打印从串口读出的字符串 */
    } else {
      int len = write(serial_fd, "Hello World\n", 12);
      printf("write data %d\n", len);
      DelayMs(100);
    }
    //printf("SNO: %d\n", sno);
  }
  //恢复配置
  if (-1 == tcsetattr(serial_fd, TCSANOW, &old_cfg)) {
    perror("tcgetattr");
    exit(-1);
  }

  close(serial_fd);
}

int MqttThread(void* arg);
int main(int argc, char **argv)
{
  int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
  StartBackgroudTask(GpioThreadIrq, (void*)0, 66);
  StartBackgroudTask(MqttThread, (void*)0, 65);
  if (msg_id == -1) {
    printf("创建消息队列失败\n");
    return -1;
  }
  while (1) {
    struct msg_data read_buf;
    int nread = msgrcv(msg_id, &read_buf, sizeof(read_buf) - sizeof(read_buf.mtype), MSG_TYPE, 0);
    if(nread == -1)
      printf("读消息队列失败\n");
    else
      printf("读消息队列成功:%d,%d,%d\n", read_buf.mindex, read_buf.mvalue, (int)read_buf.mtime);
  }
  msgctl(msg_id, IPC_RMID, NULL);
  return 0;
}
