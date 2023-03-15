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
#include <sys/time.h>
#include <semaphore.h>
#include "gpio.h"
#include "SysCall.h"
#include "typedef.h"

static MACH_STATE m_RunData;
struct GpioState {
  uint8_t Com;
  uint8_t Red;
  uint8_t Yellow;
  uint8_t Green;
  uint8_t Count;
  uint8_t Close;
};
static struct GpioState m_GpioState;
//static uint8_t m_iComState = 0xFF;
//static uint8_t m_iRedState = 0;
//static uint8_t m_iYellowState = 0;
//static uint8_t m_iGreenState = 0;
//static uint8_t m_iCountState = 0;
//static uint8_t m_iCloseState = 0;
static time_t  m_iRunDelay = 0;
static uint8_t m_bReportState = 0;
static sem_t state_sem;
static struct GpioState m_GpioStateBak;

PARAMETER m_Parameter;
static void *m_pInputQueue = NULL;
const uint32_t gpio_defs[] = { GPIO18, GPIO15, GPIO16, GPIO17, GPIO39, GPIO40 };
static void GpioThreadIrq(void *arg)
{
  int i;
  char buffer[16];
  int len;
  struct msg_data data;
  struct pollfd fds[COUNTOF(gpio_defs)];
  data.mtype = MSG_TYPE; // 注意2
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    fds[i].fd = gpio_open_irq(gpio_defs[i], GPIO_EDGE_BOTH);
    fds[i].events = POLLPRI;
    read(fds[i].fd, buffer, sizeof(buffer));
    switch (i) {
    case 0: // COM
      m_GpioState.Com = atoi(buffer);
      break;
    case 1: // RED
      m_GpioState.Red = atoi(buffer);
      break;
    case 2: // GREEN
      m_GpioState.Green = atoi(buffer);
      break;
    case 3: // YELLOW
      m_GpioState.Yellow = atoi(buffer);
      break;
    case 4: // CLOSE
      m_GpioState.Close = atoi(buffer);
      break;
    case 5: // COUNT
      m_GpioState.Count = atoi(buffer);
      break;
    }
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
        gettimeofday(&data.timestamap, NULL);
        QueuePush(m_pInputQueue, &data);
      }
    }
  }
}
static void GpioThread(void *arg)
{
  int i;
  char buffer[16];
  int len;
  int value;
  struct msg_data data;
  int fds[COUNTOF(gpio_defs)];
  uint8_t state[COUNTOF(gpio_defs)];
  data.mtype = MSG_TYPE; // 注意2
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    gpio_set_direction(gpio_defs[i], GPIO_INPUT);
    fds[i] = gpio_open(gpio_defs[i]);
    lseek(fds[i], 0, SEEK_SET);
    read(fds[i], buffer, sizeof(buffer));
    value = !!atoi(buffer);
    if (value)
      state[i] = 0xFF;
    else
      state[i] = 0x00;
    switch (i) {
    case 0: // COM
      m_GpioState.Com = value;
      break;
    case 1: // RED
      m_GpioState.Red = value;
      break;
    case 2: // GREEN
      m_GpioState.Green = value;
      break;
    case 3: // YELLOW
      m_GpioState.Yellow = value;
      break;
    case 4: // CLOSE
      m_GpioState.Close = value;
      break;
    case 5: // COUNT
      m_GpioState.Count = value;
      break;
    }
  }
  while (1) {
    DelayMs(20);
    for (i = 0; i < COUNTOF(gpio_defs); i++) {
      lseek(fds[i], 0, SEEK_SET);
      read(fds[i], buffer, sizeof(buffer));
      value = !!atoi(buffer);
      state[i] = (state[i] << 1) | value;
      if (state[i] == 0x80 || state[i] == 0x7F) {
        data.mindex = i;
        data.mvalue = value;
        gettimeofday(&data.timestamap, NULL);
        QueuePush(m_pInputQueue, &data);
        sem_post(&state_sem);
      }
    }
  }
}
static RUN_STATE MachType0()
{
  RUN_STATE state;
  if (m_GpioState.Red != m_GpioState.Com) {
    m_iRunDelay = GetTickCount();
    state = MS_ALERT;
  } else if (m_GpioState.Green != m_GpioState.Com) {
    m_iRunDelay = GetTickCount();
    state = MS_RUNNING;
  } else if (m_GpioState.Yellow != m_GpioState.Com) {
    m_iRunDelay = GetTickCount();
    state = MS_READY;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (state != MS_RUNNING || (m_iRunDelay && (m_iRunDelay + m_Parameter.RunDelay) < GetTickCount())) {
      state = MS_READY;
      m_iRunDelay = 0;
    }
  }
  return state;
}
static RUN_STATE MachType1() // 注塑
{
  RUN_STATE state;
  if (m_GpioState.Red != m_GpioState.Com) {
    m_iRunDelay = GetTickCount();
    m_RunData.RE = 0;
    state = MS_ALERT;
  } else if (m_GpioState.Green != m_GpioState.Com) {
    state = MS_RUNNING;
    m_iRunDelay = GetTickCount();
    m_RunData.RE = 1;
    if (m_GpioState.Close == 0x00) // 合模
      state = MS_RUNNING;
    else if (m_GpioState.Count == 0x00)   // 开模
      state = MS_READY;
  } else if (m_GpioState.Yellow != m_GpioState.Com) {
    m_iRunDelay = GetTickCount();
    m_RunData.RE = 0;
    state = MS_READY;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (m_RunData.RE != 0 || (m_iRunDelay && (m_iRunDelay + m_Parameter.RunDelay) < GetTickCount())) {
      state = MS_READY;
      m_RunData.RE = 0;
      m_iRunDelay = 0;
    }
  }
  return state;
}
static RUN_STATE MachType2() // 冲压(注塑无合模)
{
  RUN_STATE state;
  if (m_GpioState.Red != m_GpioState.Com) {
    m_RunData.RE = 0;
    m_iRunDelay = GetTickCount();
    state = MS_ALERT;
  } else if (m_GpioState.Green != m_GpioState.Com) {
    state = MS_RUNNING;
    m_iRunDelay = GetTickCount();
    m_RunData.RE = 1;
    if (m_GpioState.Count == 0x00)   // 产量
      state = MS_READY;
  } else if (m_GpioState.Yellow != m_GpioState.Com) {
    m_RunData.RE = 0;
    m_iRunDelay = GetTickCount();
    state = MS_READY;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (m_RunData.RE != 0 || (m_iRunDelay && (m_iRunDelay + m_Parameter.RunDelay) < GetTickCount())) {
      state = MS_READY;
      m_RunData.RE = 0;
      m_iRunDelay = 0;
    }
  }
  return state;
}
static RUN_STATE MachType3()  // 注塑无信号灯
{
  RUN_STATE state;
  state = MS_RUNNING;
  if (m_GpioState.Close == 0x00) { // 合模
    state = MS_RUNNING;
    m_RunData.RE = 1;
    m_iRunDelay = GetTickCount();
  } else if (m_GpioState.Count == 0x00) {  // 开模
    state = MS_READY;
    m_RunData.RE = 1;
    m_iRunDelay = GetTickCount();
  } else {
    if (m_RunData.RE != 0 || (m_iRunDelay && (m_iRunDelay + m_Parameter.RunDelay) < GetTickCount())) {
      state = MS_READY;
      m_RunData.RE = 0;
      m_iRunDelay = 0;
    }
  }
  return state;
}
void State2Json(char *msg)
{
  sprintf(msg, "{\"COM\":%d,\"G\":%d,\"Y\":%d,\"R\":%d,\"Q\":%d,\"Q1\":%d,\"MachType\":%d,\"CSQ\":%d}",
          m_GpioState.Com, m_GpioState.Green, m_GpioState.Yellow, m_GpioState.Red, m_GpioState.Count, m_GpioState.Close, m_Parameter.MachType, 9/*get_4g_csq()*/);
}
void set_state_report(int on)
{
  m_bReportState = on;
  memset(&m_GpioStateBak, 0xFF, sizeof(m_GpioStateBak));
  sem_post(&state_sem);
}
int send_run_data(uint16_t run_time, uint16_t alert_time, uint16_t ready_time, time_t begin, time_t end, uint16_t state, uint16_t RE, uint16_t count);
static void InputThread(void *arg)
{
  uint32_t reportTime = 0;
  while (1) {
    struct msg_data rb;
    if (QueuePop(m_pInputQueue, &rb, 100) == 1) {
      switch (rb.mindex) {
      case 0: // COM
        m_GpioState.Com = rb.mvalue;
        break;
      case 1: // RED
        m_GpioState.Red = rb.mvalue;
        break;
      case 2: // GREEN
        m_GpioState.Green = rb.mvalue;
        break;
      case 3: // YELLOW
        m_GpioState.Yellow = rb.mvalue;
        break;
      case 4: // CLOSE
        m_GpioState.Close = rb.mvalue;
        if (m_GpioState.Close == 0 && m_Parameter.MachType == 1)  // 注塑
          m_RunData.Count++;
        break;
      case 5: // COUNT
        m_GpioState.Count = rb.mvalue;
        if (m_GpioState.Count == 0 && m_Parameter.MachType != 1)  // 非注塑
          m_RunData.Count++;
        break;
      }
      printf("INPUT: C:%d,G:%d,Y:%d,R:%d,COUNT:%d,CLOSE:%d\r\n", m_GpioState.Com, m_GpioState.Green, m_GpioState.Yellow, m_GpioState.Red, m_GpioState.Count, m_GpioState.Close);
    } else
      gettimeofday(&rb.timestamap, NULL);
    RUN_STATE state = MS_NONE;
    if (m_Parameter.MachType == 0)   // 机加
      state = MachType0();
    else if (m_Parameter.MachType == 1)   // 注塑
      state = MachType1();
    else if (m_Parameter.MachType == 2)   // 冲压(注塑无合模)
      state = MachType2();
    else if (m_Parameter.MachType == 3)   // 注塑无信号灯
      state = MachType3();
    if (m_RunData.State != state || reportTime < GetTickCount() || (m_Parameter.CountReport > 0 && m_RunData.Count >= m_Parameter.CountReport)) {
      MACH_STATE rd;
      if (state == MS_NONE)
        state = m_RunData.State;
      rd = m_RunData;
      memset(&m_RunData, 0, sizeof(m_RunData));
      m_RunData.State = state;
      m_RunData.BeginTime = rb.timestamap.tv_sec;
      rd.EndTime = m_RunData.BeginTime;
      if (m_Parameter.MachType == 0)
        rd.RE = 0;
      if (rd.BeginTime == 0 || rd.EndTime == 0)
        continue;
      switch (rd.State) {
      case MS_ALERT:
        rd.AlertTime = rd.EndTime - rd.BeginTime;
        break;
      case MS_RUNNING:
        rd.RunTime = rd.EndTime - rd.BeginTime;
        break;
      case MS_READY:
        rd.ReadyTime = rd.EndTime - rd.BeginTime;
        break;
      }
      send_run_data(rd.RunTime, rd.AlertTime, rd.ReadyTime, rd.BeginTime, rd.EndTime, state, rd.RE, rd.Count);
      reportTime = GetTickCount() + m_Parameter.ReportInterval * 1000;
      sem_post(&state_sem);
    }
  }
}
void MqttThread(void *arg);
void CommThread(void *arg);
void InitCache();
void report_state(void);
void AtPortThread(void* arg);
void get_location(void);
int main(int argc, char **argv)
{
  m_pInputQueue = QueueCreate(sizeof(struct msg_data), 32);
  sem_init(&state_sem, 0, 0);
  InitCache();
  StartBackgroudTask(AtPortThread, (void *)0, 63);
  StartBackgroudTask(GpioThread, (void *)0, 66);
  StartBackgroudTask(InputThread, (void *)0, 65);
  StartBackgroudTask(MqttThread, (void *)0, 64);
  get_location();
  while (1) {
    struct timeval now;
    struct timespec abstime;
    gettimeofday(&now, NULL);
    abstime.tv_sec = now.tv_sec + 60;
    abstime.tv_nsec = 0;
    sem_timedwait(&state_sem, &abstime);
    if (m_bReportState) {
      if (memcmp(&m_GpioStateBak, &m_GpioState, sizeof(m_GpioStateBak)) != 0) {
        report_state();
        memcpy(&m_GpioStateBak, &m_GpioState, sizeof(m_GpioStateBak));
      }
    }
  }
  return 0;
}
