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
static struct mosquitto *m_mosq;

int MqttPublish(int *msgId, char *topic, char *payload, int qos)
{
  char szTopic[128];
  sprintf(szTopic, "d/%s/%s", m_Parameter.machid, topic);
  return mosquitto_publish(m_mosq, msgId, szTopic, strlen(payload), payload, 0, 0) == MOSQ_ERR_SUCCESS;
}
int MqttSubscribe(char *topic, int qos)
{
  char szTopic[128];
  sprintf(szTopic, "d/%s/%s", m_Parameter.machid, topic);
  if (mosquitto_subscribe(m_mosq, NULL, szTopic, qos)) {
    printf("Subscribe %s error!\n", topic);
    return 0;
  }
  return 1;
}
void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  if (rc) {
    // 连接错误，退出程序
    printf("on_connect error![ %d ]\n", rc);
  } else {
    static int times = 0;
    char szData[256];
    time_t tt;
    connected = 1;
    // 订阅主题
    // 参数：句柄、id、订阅的主题、qos
    if (MqttSubscribe("cfg/set", 0) == 0)
      connected = 0;
    if (MqttSubscribe("upg", 0) == 0)
      connected = 0;
    if (MqttSubscribe("state/get", 0) == 0)
      connected = 0;
    time(&tt);
    sprintf(szData, "{\"date\":%d,\"lat\":\"%s\",\"long\":\"%s\",\"ver\":%d,\"MachType\":%d,\"Times\":%d}",
            tt, m_Parameter.latitude, m_Parameter.longitude, 1, m_Parameter.MachType, times++);
    MqttPublish(NULL, "reg", szData, 0);
    //report_now();
  }
}

void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  //printf("Call the function: my_disconnect_callback\n");
  connected = 0;
}

void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  printf("Subscribe success\r\n");
}

void my_publish_callback(struct mosquitto *mosq, void *obj, int msgId)
{
  void FreeCacheData(int msgId);
  FreeCacheData(msgId);
}
void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  char buff[32];
  struct timeval tv;
  gettimeofday(&tv, NULL);
  printf("Recieve %s : %s [ %ld ] %ld:%ld\n", (char *)msg->topic, (char *)msg->payload, clock(), tv.tv_sec, tv.tv_usec);
}

void MqttThread(void *arg)
{
  int ret;
  // 初始化mosquitto库
  ret = mosquitto_lib_init();
  if (ret) {
    printf("Init lib error!\n");
    return;
  }
  // 创建一个订阅端实例
  // 参数：id（不需要则为NULL）、clean_start、用户数据
  m_mosq = mosquitto_new(m_Parameter.machid, true, NULL);
  if (m_mosq == NULL) {
    printf("New sub_test error!\n");
    mosquitto_lib_cleanup();
    return;
  }
  // 设置回调函数
  // 参数：句柄、回调函数
  mosquitto_connect_callback_set(m_mosq, my_connect_callback);
  mosquitto_disconnect_callback_set(m_mosq, my_disconnect_callback);
  //mosquitto_subscribe_callback_set(m_mosq, my_subscribe_callback);
  mosquitto_publish_callback_set(m_mosq, my_publish_callback);
  mosquitto_message_callback_set(m_mosq, my_message_callback);
  // 开始通信：循环执行
  printf("Mqtt Start!\n");
  while (1) {
    if (connected == 0) {
      // 连接至服务器
      // 参数：句柄、ip（host）、端口、心跳
      mosquitto_username_pw_set(m_mosq, m_Parameter.MqttUser, m_Parameter.MqttPwd);
      ret = mosquitto_connect(m_mosq, m_Parameter.MqttServer, m_Parameter.MqttPort, KEEP_ALIVE);
      if (ret)
        printf("Connect server error!\n");
      Sleep(1);
    }
    mosquitto_loop(m_mosq, -1, 1);
  }
  // 结束后的清理工作
  mosquitto_destroy(m_mosq);
  mosquitto_lib_cleanup();
  printf("End!\n");
  return;
}
