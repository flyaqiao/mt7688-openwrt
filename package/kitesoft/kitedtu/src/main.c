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
#include "gpio.h"
#include "SysCall.h"
#include "typedef.h"

static MACH_STATE m_RunData;
static uint8_t m_iComState = 0xFF;
static uint8_t m_iRedState = 0;
static uint8_t m_iYellowState = 0;
static uint8_t m_iGreenState = 0;
static uint8_t m_iCountState = 0;
static uint8_t m_iCloseState = 0;
static uint16_t m_iRunDelay = 0;

PARAMETER m_Parameter;
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
        gettimeofday(&data.timestamap, NULL);
        // 向队列里发送数据
        if (msgsnd(msg_id, (void *)&data, sizeof(data) - sizeof(data.mtype), 0) == -1)
          fprintf(stderr, "msgsnd failed\n");
      }
    }
  }
  msgctl(msg_id, IPC_RMID, NULL);
}
static RUN_STATE MachType0()
{
  RUN_STATE state;
  if (m_iRedState != m_iComState) {
    m_iRunDelay = 0;
    state = MS_ALERT;
  } else if (m_iYellowState != m_iComState) {
    m_iRunDelay = 0;
    state = MS_READY;
  } else if (m_iGreenState != m_iComState) {
    m_iRunDelay = 0;
    state = MS_RUNNING;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (state != MS_READY) {
      m_iRunDelay++;
      if (m_iRunDelay > m_Parameter.RunDelay)
        state = MS_READY;
    }
  }
  return state;
}
static RUN_STATE MachType1() // 注塑
{
  RUN_STATE state;
  if (m_iRedState != m_iComState) {
    m_RunData.RE = 0;
    m_iRunDelay = 0;
    state = MS_ALERT;
  } else if (m_iYellowState != m_iComState) {
    m_RunData.RE = 0;
    m_iRunDelay = 0;
    state = MS_READY;
  } else if (m_iGreenState != m_iComState) {
    state = MS_RUNNING;
    m_iRunDelay = 0;
    m_RunData.RE = 1;
    if (m_iCloseState == 0x00) // 合模
      state = MS_RUNNING;
    else if (m_iCountState == 0x00)   // 开模
      state = MS_READY;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (state != MS_READY || m_RunData.RE != 0) {
      m_iRunDelay++;
      if (m_iRunDelay > m_Parameter.RunDelay) {
        state = MS_READY;
        m_RunData.RE = 0;
      }
    }
  }
  return state;
}
static RUN_STATE MachType2() // 冲压(注塑无合模)
{
  RUN_STATE state;
  if (m_iRedState != m_iComState) {
    m_RunData.RE = 0;
    m_iRunDelay = 0;
    state = MS_ALERT;
  } else if (m_iYellowState != m_iComState) {
    m_RunData.RE = 0;
    m_iRunDelay = 0;
    state = MS_READY;
  } else if (m_iGreenState != m_iComState) {
    state = MS_RUNNING;
    m_iRunDelay = 0;
    m_RunData.RE = 1;
    if (m_iCountState == 0x00)   // 产量
      state = MS_READY;
  } else {
    state = (RUN_STATE)m_RunData.State;
    if (state != MS_READY || m_RunData.RE != 0) {
      m_iRunDelay++;
      if (m_iRunDelay > m_Parameter.RunDelay) {
        state = MS_READY;
        m_RunData.RE = 0;
      }
    }
  }
  return state;
}
static RUN_STATE MachType3()
{
  RUN_STATE state;
  state = MS_RUNNING;
  if (m_iCloseState == 0x00) { // 合模
    state = MS_RUNNING;
    m_RunData.RE = 1;
    m_iRunDelay = 0;
  } else if (m_iCountState == 0x00) {  // 开模
    state = MS_READY;
    m_RunData.RE = 1;
    m_iRunDelay = 0;
  } else {
    m_iRunDelay++;
    if (m_iRunDelay > m_Parameter.RunDelay) {
      state = MS_READY;
      m_RunData.RE = 0;
    }
  }
  return state;
}
int send_run_data(uint16_t run_time, uint16_t alert_time, uint16_t ready_time, time_t begin, time_t end, uint16_t state, uint16_t RE, uint16_t count);
void MqttThread(void *arg);
void CommThread(void *arg);
void InitCache();
//void AtPortThread(void* arg);
int main(int argc, char **argv)
{
  int msg_id = msgget(MSG_KEY, IPC_CREAT | 0666);
  InitCache();
  //StartBackgroudTask(AtPortThread, (void *)0, 64);
  StartBackgroudTask(GpioThreadIrq, (void *)0, 66);
  StartBackgroudTask(MqttThread, (void *)0, 65);
  if (msg_id == -1) {
    printf("创建消息队列失败\n");
    return -1;
  }
  while (1) {
    struct msg_data rb;
    int nread = msgrcv(msg_id, &rb, sizeof(rb) - sizeof(rb.mtype), MSG_TYPE, 0);
    if (nread == -1)
      continue;
    switch (rb.mindex) {
    case 0: // COM
      m_iComState = rb.mvalue;
      break;
    case 1: // RED
      m_iRedState = rb.mvalue;
      break;
    case 2: // GREEN
      m_iGreenState = rb.mvalue;
      break;
    case 3: // YELLOW
      m_iYellowState = rb.mvalue;
      break;
    case 4: // CLOSE
      m_iCloseState = rb.mvalue;
      if (m_iCloseState == 0 && m_Parameter.MachType == 1)  // 注塑
        m_RunData.Count++;
      break;
    case 5: // COUNT
      m_iCountState = rb.mvalue;
      if (m_iCountState == 0 && m_Parameter.MachType != 1)  // 非注塑
        m_RunData.Count++;
      break;
    }
    RUN_STATE state = MS_NONE;
    if (m_Parameter.MachType == 2)   // 冲压(注塑无合模)
      state = MachType2();
    else if (m_Parameter.MachType == 1)   // 注塑
      state = MachType1();
    else if (m_Parameter.MachType == 3)   // 注塑无信号灯
      state = MachType3();
    else    // 机加
      state = MachType0();
    printf("###%d %d\r\n", m_RunData.State, state);
    if (m_RunData.State != state || (m_Parameter.CountReport > 0 && m_RunData.Count >= m_Parameter.CountReport)) {
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
        rd.AlertTime = rd.EndTime - rd.BeginTime + 1;
        break;
      case MS_RUNNING:
        rd.RunTime = rd.EndTime - rd.BeginTime + 1;
        break;
      case MS_READY:
        rd.ReadyTime = rd.EndTime - rd.BeginTime + 1;
        break;
      }
      send_run_data(rd.RunTime, rd.AlertTime, rd.ReadyTime, rd.BeginTime, rd.EndTime, state, rd.RE, rd.Count);
    }
    //printf("读消息队列成功[%d]%d:%d,%d,%d\n", rb.timestamap.tv_usec, CLOCKS_PER_SEC, rb.mindex, rb.mvalue, (int)rb.timestamap.tv_sec);
  }
  msgctl(msg_id, IPC_RMID, NULL);
  return 0;
}
