#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <termios.h>
#include <poll.h>
#include "gpio.h"
#include "SysCall.h"
#include "mosquitto.h"

#ifndef COUNTOF
#define COUNTOF(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif
const unsigned int gpio_defs[] = { GPIO18, GPIO15, GPIO16, GPIO17, GPIO39, GPIO40, GPIO11 };
//开始捕获测试
static int gpio_irq_test(void)
{
  int i;
  struct pollfd fds[COUNTOF(gpio_defs)];
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    fds[i].fd = gpio_open_irq(gpio_defs[i], GPIO_EDGE_BOTH);
    fds[i].events = POLLPRI;
  }

  while (1) {
    if (poll(fds, COUNTOF(gpio_defs), 0) < 0) {
      perror("poll failed!\n");
      return -1;
    }

    for (i = 0; i < COUNTOF(gpio_defs); i++) {
      if (fds[i].revents & POLLPRI) {
        //能进这里就代表触发事件发生了,我这里是置了一个标志
        if (lseek(fds[i].fd, 0, SEEK_SET) == -1) {
          perror("lseek value failed!\n");
          return -1;
        }
        char buffer[16];
        int len;
        //一定要读取,不然会一直检测到紧急事件
        if ((len = read(fds[i].fd, buffer, sizeof(buffer))) == -1)  {
          perror("read value failed!\n");
          return -1;
        }
        buffer[len] = 0;
        printf("%d:%s", i, buffer);
      }
    }
  }

  return 0;
}
static void GpioThread(void* arg)
{
  int i;
  struct pollfd fds[COUNTOF(gpio_defs)];
  for (i = 0; i < COUNTOF(gpio_defs); i++) {
    fds[i].fd = gpio_open_irq(gpio_defs[i], GPIO_EDGE_BOTH);
    fds[i].events = POLLPRI;
  }

  while (1) {
    if (poll(fds, COUNTOF(gpio_defs), 0) < 0) {
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
        char buffer[16];
        int len;
        //一定要读取,不然会一直检测到紧急事件
        if ((len = read(fds[i].fd, buffer, sizeof(buffer))) == -1)  {
          perror("read value failed!\n");
          continue;
        }
        buffer[len] = 0;
        printf("%d:%s", i, buffer);
      }
    }
  }

  return 0;
}
void blink(int pin)
{
  printf("blink GPIO %d for 100 times\n", pin);
  fflush(stdout);

  if (GPIO_SUCCESS == gpio_export(pin)) {
    if (GPIO_SUCCESS == gpio_set_direction(pin, GPIO_OUTPUT)) {
      //blink
      int value = GPIO_HIGH;
      int i;
      for (i = 0; i < 100; i++) {
        printf("blink %d\n", i);
        value = (value == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
        gpio_write(pin, value);
        sleep(1);
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
    //printf("SNO: %d\r\n", sno);
  }
  //恢复配置
  if (-1 == tcsetattr(serial_fd, TCSANOW, &old_cfg)) {
    perror("tcgetattr");
    exit(-1);
  }

  close(serial_fd);
}

#define HOST "1.15.67.50"
#define PORT  1883
#define KEEP_ALIVE 60
#define MSG_MAX_SIZE  512

// 定义运行标志决定是否需要结束
static int running = 1;

void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  printf("Call the function: on_connect\n");

  if(rc){
    // 连接错误，退出程序
    printf("on_connect error!\n");
    exit(1);
  }else{
    // 订阅主题
    // 参数：句柄、id、订阅的主题、qos
    if(mosquitto_subscribe(mosq, NULL, "topic1", 2)){
      printf("Set the topic error!\n");
      exit(1);
    }
  }
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  printf("Call the function: my_disconnect_callback\n");
  running = 0;
}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  printf("Call the function: on_subscribe\n");
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  printf("Call the function: on_message\n");
  printf("Recieve a message of %s : %s\n", (char *)msg->topic, (char *)msg->payload);

  if(0 == strcmp(msg->payload, "quit")){
    mosquitto_disconnect(mosq);
  }
}

static int MqttThread(void* arg)
{
  int ret;
  struct mosquitto *mosq;

  // 初始化mosquitto库
  ret = mosquitto_lib_init();
  if(ret){
    printf("Init lib error!\n");
    return -1;
  }

  // 创建一个订阅端实例
  // 参数：id（不需要则为NULL）、clean_start、用户数据
  mosq = mosquitto_new("sub_test", true, NULL);
  if(mosq == NULL){
    printf("New sub_test error!\n");
    mosquitto_lib_cleanup();
    return -1;
  }
  mosquitto_username_pw_set(mosq, "mt7688", "12345678");
  // 设置回调函数
  // 参数：句柄、回调函数
  mosquitto_connect_callback_set(mosq, my_connect_callback);
  mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
  mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
  mosquitto_message_callback_set(mosq, my_message_callback);

  // 连接至服务器
  // 参数：句柄、ip（host）、端口、心跳
  ret = mosquitto_connect(mosq, HOST, PORT, KEEP_ALIVE);
  if(ret){
    printf("Connect server error!\n");
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return -1;
  }


  // 开始通信：循环执行、直到运行标志running被改变
  printf("Start!\n");
  while(running)
  {
    mosquitto_loop(mosq, -1, 1);
  }

  // 结束后的清理工作
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  printf("End!\n");

  return 0;
}

int main(int argc, char **argv)
{
  StartBackgroudTask(GpioThread, (void*)0);
  StartBackgroudTask(MqttThread, (void*)0);
  //StartBackgroudTask(CommThread, (void*)1);
  //StartBackgroudTask(CommThread, (void*)2);
  while (1) {
    DelayMs(1000);
  }
  printf("Exit\r\n");
  return 0;
}
