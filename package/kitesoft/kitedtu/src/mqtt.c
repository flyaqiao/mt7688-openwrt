#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "mosquitto.h"
#include "SysCall.h"
#include "typedef.h"

#define KEEP_ALIVE 60

// 定义运行标志决定是否需要结束
static int connected = 0;

void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  printf("Call the function: on_connect\n");

  if (rc) {
    // 连接错误，退出程序
    printf("on_connect error!\n");
  } else {
    connected = 1;
    // 订阅主题
    // 参数：句柄、id、订阅的主题、qos
    if (mosquitto_subscribe(mosq, NULL, "topic1", 2)) {
      printf("Set the topic error!\n");
      connected = 0;
    }
  }
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  printf("Call the function: my_disconnect_callback\n");
  connected = 0;
}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  printf("Call the function: on_subscribe\n");
}

void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  char buff[32];
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("Recieve %s : %s [ %ld ] %ld:%ld\n", (char *)msg->topic, (char *)msg->payload, clock(), tv.tv_sec, tv.tv_usec);
  /*发布消息*/
  sprintf(buff, "%ld", clock());
  mosquitto_publish(mosq, NULL, "topic2", strlen(buff), buff, 0, 0);
  if (0 == strcmp(msg->payload, "quit")) {
    mosquitto_disconnect(mosq);
  }
}

void MqttThread(void* arg)
{
  int ret;
  struct mosquitto *mosq;

  // 初始化mosquitto库
  ret = mosquitto_lib_init();
  if (ret) {
    printf("Init lib error!\n");
    return;
  }

  // 创建一个订阅端实例
  // 参数：id（不需要则为NULL）、clean_start、用户数据
  mosq = mosquitto_new("sub_test", true, NULL);
  if (mosq == NULL) {
    printf("New sub_test error!\n");
    mosquitto_lib_cleanup();
    return;
  }
  // 设置回调函数
  // 参数：句柄、回调函数
  mosquitto_connect_callback_set(mosq, my_connect_callback);
  mosquitto_disconnect_callback_set(mosq, my_disconnect_callback);
  mosquitto_subscribe_callback_set(mosq, my_subscribe_callback);
  mosquitto_message_callback_set(mosq, my_message_callback);

  // 开始通信：循环执行
  printf("Start!\n");
  while (1) {
    if (connected == 0) {
      // 连接至服务器
      // 参数：句柄、ip（host）、端口、心跳
      mosquitto_username_pw_set(mosq, m_Parameter.MqttUser, m_Parameter.MqttPwd);
      ret = mosquitto_connect(mosq, m_Parameter.MqttServer, m_Parameter.MqttPort, KEEP_ALIVE);
      if (ret) {
        printf("Connect server error!\n");
      }
      Sleep(1);
    }
    mosquitto_loop(mosq, -1, 1);
  }

  // 结束后的清理工作
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
  printf("End!\n");

  return;
}
