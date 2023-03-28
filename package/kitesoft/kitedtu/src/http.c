#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "curl/curl.h"
#include "cJSON.h"
#include "SysCall.h"
#include "typedef.h"
#define LOG_TAG "HTTP"
#include <elog.h>

static void HttpServerThread(void *arg);
void HttpInit()
{
  curl_global_init(CURL_GLOBAL_ALL);
  StartBackgroudTask(HttpServerThread, (void *)0, 30);
}
/*************************************
回调函数，处理post返回数据的地方
*************************************/
int curl_recv_post_data(void *buffer, size_t size, size_t nmemb, char *useless)
{
  cJSON *cjson = cJSON_Parse(buffer);//将JSON字符串转换成JSON结构体
  if (cjson == NULL)          //判断转换是否成功
    log_e("cjson error...");
  else {
    int code = cJSON_GetObjectItem(cjson, "code")->valueint;
    if (code == 200) {
      cJSON *psub = cJSON_GetObjectItem(cjson, "data"); //解析数组
      if (psub) {
        char *pobj = cJSON_GetObjectItem(psub, "password")->valuestring;
        if (pobj) {
          void SetMqttPwd(char *pwd);
          SetMqttPwd(pobj);
        }
      }
    }
    cJSON_Delete(cjson);//清除结构体
  }
  return size * nmemb;
}
/*************************************
模拟POST发送
*************************************/
int HttpPost(char *send_data, char *url)
{
  CURLcode res;
  CURL *curl;
  int iRet = 0;
  curl = curl_easy_init();
  curl_easy_setopt(curl, CURLOPT_URL, url); //url地址
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, send_data); //post参数
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_recv_post_data); //对返回的数据进行操作的函数地址
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, NULL); //这是write_data的第四个参数值
  curl_easy_setopt(curl, CURLOPT_POST, 1); //设置问非0表示本次操作为post
  //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1); //打印调试信息
  //curl_easy_setopt(g_curl, CURLOPT_HEADER, 1); //将响应头信息和相应体一起传给write_data
  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1); //设置为非0,响应头信息location
  //curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/Users/zhu/CProjects/curlposttest.cookie");
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla");
  //curl_easy_setopt(curl, CURLOPT_HTTPHEADER, "Content-Type: application/x-www-form-urlencoded");
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_FORBID_REUSE, 1);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
  res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    log_e("curl error: %d => ", res);
    switch (res) {
    case CURLE_UNSUPPORTED_PROTOCOL:
      log_e("Does not support agreement");
      break;
    case CURLE_COULDNT_CONNECT:
      log_e("not link remote server");
      break;
    case CURLE_HTTP_RETURNED_ERROR:
      log_e("http return failed");
      break;
    case CURLE_READ_ERROR:
      log_e("read local file failed");
      break;
    default:
      log_e("curl other error");
      break;
    }
    iRet = -1;
  } else
    iRet = 0;
  curl_easy_cleanup(curl);
  return iRet;
}

#include <signal.h>
#include "mongoose.h"

static int s_debug_level = MG_LL_INFO;
static const char *s_listening_address = "http://0.0.0.0:80";
static const char *s_enable_hexdump = "no";

static char *trim(char *str)
{
  char *p = str;
  while (p && *p) {
    if (*p == '\r' || *p == '\n') {
      *p = 0;
      break;
    }
    p++;
  }
  return str;
}
// Handle interrupts, like Ctrl-C
static int s_signo;
static void signal_handler(int signo)
{
  s_signo = signo;
}

static char m_wifissid[32];
static char m_wifipwd[32];
static char m_wifienc[32];
static char m_wifichg = 0;
static void ASyncCallShellCmd(void *arg)
{
  char cmd[128];
  char tmp[128];
  int type = arg;
  Sleep(2 * 1000);
  if (type == 0) {
    sprintf(cmd, "wifimode sta %s %s %s", m_wifissid, m_wifipwd, m_wifienc);
    execmd(cmd, tmp, sizeof(tmp));
    Sleep(10 * 1000);
    execmd("ifconfig | grep wlan0", tmp, sizeof(tmp));
    if (strstr(tmp, "wlan0") == NULL)
      execmd("wifi up", tmp, sizeof(tmp));
    m_wifichg = 0;
  } else if (type == 1)
    execmd("firstboot -y && reboot", tmp, sizeof(tmp));
  else if (type == 2)
    execmd("reboot", tmp, sizeof(tmp));
}
// Event handler for the listening connection.
static void cb(struct mg_connection *c, int ev, void *ev_data, void *fn_data)
{
  if (ev == MG_EV_HTTP_MSG) {
    struct mg_http_message *hm = ev_data;
    if (mg_http_match_uri(hm, "/api/config/set")) {
      char wifissid[32];
      char wifipwd[32];
      char wifienc[32];
      char mqttserver[128];
      char mqttport[32];
      char authurl[128];
      //char upgurl[128];
      memset(wifissid, 0, sizeof(wifissid));
      memset(wifipwd, 0, sizeof(wifipwd));
      memset(wifienc, 0, sizeof(wifienc));
      memset(mqttserver, 0, sizeof(mqttserver));
      memset(mqttport, 0, sizeof(mqttport));
      memset(authurl, 0, sizeof(authurl));
      //memset(upgurl, 0, sizeof(upgurl));
      mg_http_get_var(&hm->body, "wifissid", wifissid, sizeof(wifissid));
      mg_http_get_var(&hm->body, "wifipwd", wifipwd, sizeof(wifipwd));
      mg_http_get_var(&hm->body, "wifienc", wifienc, sizeof(wifienc));
      mg_http_get_var(&hm->body, "mqttserver", mqttserver, sizeof(mqttserver));
      mg_http_get_var(&hm->body, "mqttport", mqttport, sizeof(mqttport));
      mg_http_get_var(&hm->body, "authurl", authurl, sizeof(authurl));
      //mg_http_get_var(&hm->body, "upgurl", upgurl, sizeof(upgurl));
      if (strlen(mqttserver) > 0)
        strcpy(m_Parameter.MqttServer, mqttserver);
      if (strlen(authurl) > 0)
        strcpy(m_Parameter.AuthUrl, authurl);
      if (atoi(mqttport) > 0)
        m_Parameter.MqttPort = atoi(mqttport);
      //if (strlen(upgurl) > 0)
      //  strcpy(m_Parameter.UpgUrl, upgurl);
      if (strcmp(m_wifissid, wifissid) != 0 || strcmp(m_wifipwd, wifipwd) != 0 || strcmp(m_wifienc, wifienc) != 0) {
        strcpy(m_wifissid, wifissid);
        strcpy(m_wifipwd, wifipwd);
        strcpy(m_wifienc, wifienc);
        m_wifichg = 1;
        StartBackgroudTask(ASyncCallShellCmd, (void *)0, 10);
      }
      mg_http_reply(c, 200, NULL, "config set ok\n");
    } else if (mg_http_match_uri(hm, "/api/config/get")) {
      if (m_wifichg == 0) {
        memset(m_wifissid, 0, sizeof(m_wifissid));
        memset(m_wifipwd, 0, sizeof(m_wifipwd));
        memset(m_wifienc, 0, sizeof(m_wifienc));
        execmd("uci get wireless.sta0.ssid", m_wifissid, sizeof(m_wifissid));
        execmd("uci get wireless.sta0.key", m_wifipwd, sizeof(m_wifipwd));
        execmd("uci get wireless.sta0.encryption", m_wifienc, sizeof(m_wifienc));
        trim(m_wifissid);
        trim(m_wifipwd);
        trim(m_wifienc);
      }
      mg_http_reply(c, 200, NULL, "{%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q,%Q:%Q,%Q:%d,%Q:%Q,%Q:%Q}\n", "wifissid", m_wifissid, "wifipwd",
                    m_wifipwd, "wifienc", m_wifienc, "mqttserver", m_Parameter.MqttServer, "authurl", m_Parameter.AuthUrl,
                    "upgurl", m_Parameter.UpgUrl, "mqttport", m_Parameter.MqttPort, "MACID", m_Parameter.MACID, "CCID", m_Parameter.CCID);
      return;
    } else if (mg_http_match_uri(hm, "/api/config/reset")) {
      StartBackgroudTask(ASyncCallShellCmd, (void *)1, 10);
      mg_http_reply(c, 200, NULL, "rebooting\n");
    } else if (mg_http_match_uri(hm, "/api/reboot")) {
      StartBackgroudTask(ASyncCallShellCmd, (void *)2, 10);
      mg_http_reply(c, 200, NULL, "rebooting\n");
    } else {
      struct mg_http_serve_opts opts = {
        .root_dir = "/web_root",
        .fs = &mg_fs_packed,
      };
      mg_http_serve_dir(c, ev_data, &opts);
    }
  }
  (void) fn_data;
}

static void HttpServerThread(void *arg)
{
  char path[MG_PATH_MAX] = ".";
  struct mg_mgr mgr;
  struct mg_connection *c;
  // Root directory must not contain double dots. Make it absolute
  // Do the conversion only if the root dir spec does not contain overrides
  // Initialise stuff
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  mg_log_set(s_debug_level);
  mg_mgr_init(&mgr);
  if ((c = mg_http_listen(&mgr, s_listening_address, cb, &mgr)) == NULL) {
    MG_ERROR(("Cannot listen on %s. Use http://ADDR:PORT or :PORT",
              s_listening_address));
    exit(EXIT_FAILURE);
  }
  if (mg_casecmp(s_enable_hexdump, "yes") == 0) c->is_hexdumping = 1;
  // Start infinite event loop
  log_i(("Mongoose version : v%s", MG_VERSION));
  log_i(("Listening on     : %s", s_listening_address));
  while (s_signo == 0) mg_mgr_poll(&mgr, 1000);
  mg_mgr_free(&mgr);
  log_i(("Exiting on signal %d", s_signo));
}
