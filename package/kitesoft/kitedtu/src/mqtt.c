#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include "mosquitto.h"
#include "cJSON.h"
#include "SysCall.h"
#include "typedef.h"
#include "version.h"
#define LOG_TAG "MQTT"
#include <elog.h>

#define KEEP_ALIVE 60

int HttpPost(char *send_data, char *url);
void PublishAck(int msgId);
// 定义运行标志决定是否需要结束
static int m_bConnected = 0;
static struct mosquitto *m_mosq;
static char szMqttUser[32];

int MqttPublish(int *msgId, char *topic, char *payload, int qos)
{
  char szTopic[128];
  if (m_bConnected != 1)
    return 0;
  sprintf(szTopic, "d/%s/%s", szMqttUser, topic);
  return mosquitto_publish(m_mosq, msgId, szTopic, strlen(payload), payload, qos, 0) == MOSQ_ERR_SUCCESS;
}
int MqttSubscribe(char *topic, int qos)
{
  char szTopic[128];
  sprintf(szTopic, "d/%s/%s", szMqttUser, topic);
  if (mosquitto_subscribe(m_mosq, NULL, szTopic, qos)) {
    log_e("Subscribe %s error!", topic);
    return 0;
  }
  return 1;
}
int MqttReconnect()
{
  m_bConnected = 2;
}
static void my_connect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  if (rc) {
    // 连接错误，退出程序
    log_e("on_connect error![%d]!(%s,%s,%s)", rc, m_Parameter.MACID, szMqttUser, m_Parameter.MqttPwd);
    m_bConnected = 0;
  } else {
    static int times = 0;
    char szData[256];
    time_t tt;
    m_bConnected = 1;
    // 订阅主题
    // 参数：句柄、id、订阅的主题、qos
    if (MqttSubscribe("cfg/set", 0) == 0)
      m_bConnected = 0;
    if (MqttSubscribe("upg", 0) == 0)
      m_bConnected = 0;
    if (MqttSubscribe("state/get", 0) == 0)
      m_bConnected = 0;
    if (MqttSubscribe("cmd", 0) == 0)
      m_bConnected = 0;
    time(&tt);
    sprintf(szData, "{\"date\":%ld,\"lat\":\"%s\",\"long\":\"%s\",\"ver\":%d,\"MachType\":%d,\"Times\":%d,\"hashver\":\"%s\"}",
            tt, m_Parameter.latitude, m_Parameter.longitude, SVNVERSION, m_Parameter.MachType, times++, GITVERSION);
    MqttPublish(NULL, "reg", szData, 1);
    log_i("Mqtt connected");
  }
}

static void my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
  log_e("Mqtt disconnect");
  m_bConnected = 0;
}

static void my_subscribe_callback(struct mosquitto *mosq, void *obj, int mid, int qos_count, const int *granted_qos)
{
  log_i("Subscribe successn");
}

static void my_publish_callback(struct mosquitto *mosq, void *obj, int msgId)
{
  PublishAck(msgId);
}
static void Config2Json(char *msg)
{
  sprintf(msg, "{\"MachType\":%d,\"ReportInterval\":%d,\"CountReport\":%d,\"RunDelay\":%d}",
          m_Parameter.MachType, m_Parameter.ReportInterval, m_Parameter.CountReport, m_Parameter.RunDelay);
}
static int Json2Config(char *msg)
{
  cJSON *cjson = cJSON_Parse(msg);//将JSON字符串转换成JSON结构体
  if (cjson != NULL) {  //判断转换是否成功
    void SaveParameter(void);
    PARAMETER para = m_Parameter;
    if (cJSON_GetObjectItem(cjson, "MachType") != NULL) {
      cJSON *obj = cJSON_GetObjectItem(cjson, "MachType");
      if (obj->type == cJSON_String)
        m_Parameter.MachType = atoi(obj->valuestring);
      else if (obj->type == cJSON_Number)
        m_Parameter.MachType = obj->valueint;
      if (m_Parameter.MachType > 2)
        m_Parameter.MachType = para.MachType;
    }
    if (cJSON_GetObjectItem(cjson, "ReportInterval") != NULL) {
      cJSON *obj = cJSON_GetObjectItem(cjson, "ReportInterval");
      if (obj->type == cJSON_String)
        m_Parameter.ReportInterval = atoi(obj->valuestring);
      else if (obj->type == cJSON_Number)
        m_Parameter.ReportInterval = obj->valueint;
      if (m_Parameter.ReportInterval > 300)
        m_Parameter.ReportInterval = 300;
    }
    if (cJSON_GetObjectItem(cjson, "CountReport") != NULL) {
      cJSON *obj = cJSON_GetObjectItem(cjson, "CountReport");
      if (obj->type == cJSON_String)
        m_Parameter.CountReport = atoi(obj->valuestring);
      else if (obj->type == cJSON_Number)
        m_Parameter.CountReport = obj->valueint;
    }
    if (cJSON_GetObjectItem(cjson, "RunDelay") != NULL) {
      cJSON *obj = cJSON_GetObjectItem(cjson, "RunDelay");
      if (obj->type == cJSON_String)
        m_Parameter.RunDelay = atoi(obj->valuestring);
      else if (obj->type == cJSON_Number)
        m_Parameter.RunDelay = obj->valueint;
      if (m_Parameter.RunDelay < 500 || m_Parameter.RunDelay > 30000)
        m_Parameter.RunDelay = 3000;
    }
    if (cJSON_GetObjectItem(cjson, "MqttPort") != NULL) {
      cJSON *obj = cJSON_GetObjectItem(cjson, "MqttPort");
      if (obj->type == cJSON_String)
        m_Parameter.MqttPort = atoi(obj->valuestring);
      else if (obj->type == cJSON_Number)
        m_Parameter.MqttPort = obj->valueint;
    }
    if (cJSON_GetObjectItem(cjson, "MqttServer") != NULL)
      strncpy(m_Parameter.MqttServer, cJSON_GetObjectItem(cjson, "MqttServer")->valuestring, 31);
    cJSON_Delete(cjson);//清除结构体
    if (m_Parameter.MqttPort != para.MqttPort || strcmp(m_Parameter.MqttServer, para.MqttServer) != 0)
      return 1;
  } else
    log_e("cfg/set error...(%s)", msg);
  return 0;
}
void report_state(void)
{
  char szPayload[128];
  void State2Json(char *msg);
  State2Json(szPayload);
  MqttPublish(NULL, "state/report", szPayload, 0);
}
static int asyncupgrade(char *url, int ver, char *script)
{
#ifndef WIN31
  char cmd[128];
  sprintf(cmd, "wget -q -O- %s%d/%s | sh &", url, ver, script);
  system(cmd);
#endif
}
static void my_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
  char szMachId[128];
  char szTopic[128];
  char szPayload[256];
  if (sscanf(msg->topic, "d/%[^/]/%s", szMachId, szTopic) == 2 && strcmp(szMachId, szMqttUser) == 0) {
    if (strcmp(szTopic, "upg") == 0) {
      char url[128] = { 0 };
      char script[128] = { 0 };
      int ver = 0;
      cJSON *cjson = cJSON_Parse(msg->payload);//将JSON字符串转换成JSON结构体
      if (cjson != NULL) {  //判断转换是否成功
        if (cJSON_GetObjectItem(cjson, "url") != NULL)
          strncpy(url, cJSON_GetObjectItem(cjson, "url")->valuestring, sizeof(url) - 1);
        if (cJSON_GetObjectItem(cjson, "ver") != NULL) {
          cJSON *obj = cJSON_GetObjectItem(cjson, "ver");
          if (obj->type == cJSON_String)
            ver = atoi(obj->valuestring);
          else if (obj->type == cJSON_Number)
            ver = obj->valueint;
        }
        if (cJSON_GetObjectItem(cjson, "cmd") != NULL)
          strncpy(script, cJSON_GetObjectItem(cjson, "cmd")->valuestring, sizeof(script) - 1);
        cJSON_Delete(cjson);//清除结构体
        if (strlen(url) == 0)
          strcpy(url, m_Parameter.UpgUrl);
        if (strlen(script) == 0)
          strcpy(script, "kiteupg.sh");
        asyncupgrade(url, ver, script);
      }
    } else if (strcmp(szTopic, "cfg/set") == 0) {
      if (Json2Config(msg->payload))
        MqttReconnect();
      Config2Json(szPayload);
      MqttPublish(NULL, "cfg/report", szPayload, 1);
    } else if (strcmp(szTopic, "state/get") == 0) {
      void set_state_report(int on);
      if (strstr(msg->payload, "on") != NULL)
        set_state_report(1);
      else
        set_state_report(0);
    } else if (strcmp(szTopic, "cmd") == 0) {
      char szCmdResult[4096];
      memset(szCmdResult, 0, sizeof(szCmdResult));
      execmd(msg->payload, szCmdResult, sizeof(szCmdResult));
      MqttPublish(NULL, "cmd/ack", szCmdResult, 0);
    }
  }
}
static void my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
  log_e("log level:%d str:%s\n", level, str);
}
void MqttThread(void *arg)
{
  int ret;
  // 初始化mosquitto库
  ret = mosquitto_lib_init();
  if (ret) {
    log_e("Init lib error!");
    return;
  }
  // 创建一个订阅端实例
  // 参数：id（不需要则为NULL）、clean_start、用户数据
  m_mosq = mosquitto_new(m_Parameter.MACID, true, NULL);
  if (m_mosq == NULL) {
    log_e("New sub_test error!");
    mosquitto_lib_cleanup();
    return;
  }
  // 开始通信：循环执行
  while (1) {
    if (m_bConnected == 0) {
      mosquitto_reinitialise(m_mosq, m_Parameter.MACID, true, NULL);
      // 设置回调函数
      // 参数：句柄、回调函数
      mosquitto_connect_callback_set(m_mosq, my_connect_callback);
      mosquitto_disconnect_callback_set(m_mosq, my_disconnect_callback);
      //mosquitto_subscribe_callback_set(m_mosq, my_subscribe_callback);
      mosquitto_publish_callback_set(m_mosq, my_publish_callback);
      mosquitto_message_callback_set(m_mosq, my_message_callback);
      //mosquitto_log_callback_set(m_mosq, my_log_callback);
      if (strlen(m_Parameter.CCID) > 4)
        strcpy(szMqttUser, m_Parameter.CCID + 4);
      else
        strcpy(szMqttUser, m_Parameter.MACID);
      if (strlen(m_Parameter.MqttPwd) == 0) {
        char szPostData[128];
        sprintf(szPostData, "clientid=%s&username=%s", m_Parameter.MACID, szMqttUser);
        HttpPost(szPostData, m_Parameter.AuthUrl);
      }
      // 连接至服务器
      // 参数：句柄、ip（host）、端口、心跳
      if (strlen(m_Parameter.MqttPwd))
        mosquitto_username_pw_set(m_mosq, szMqttUser, m_Parameter.MqttPwd);
      else
        mosquitto_username_pw_set(m_mosq, szMqttUser, szMqttUser);
      ret = mosquitto_connect(m_mosq, m_Parameter.MqttServer, m_Parameter.MqttPort, KEEP_ALIVE);
      if (ret) {
        log_e("Connect server error[%d]!(%s,%s,%s)", ret, m_Parameter.MACID, szMqttUser, m_Parameter.MqttPwd);
        Sleep(6000);
      }
      Sleep(1000);
    } else if (m_bConnected > 1) {
      mosquitto_disconnect(m_mosq);
      m_bConnected = 0;
    }
    mosquitto_loop(m_mosq, -1, 1);
  }
  // 结束后的清理工作
  mosquitto_destroy(m_mosq);
  mosquitto_lib_cleanup();
  return;
}