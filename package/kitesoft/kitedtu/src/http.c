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

void HttpInit()
{
  curl_global_init(CURL_GLOBAL_ALL);
}
/*************************************
回调函数，处理post返回数据的地方
*************************************/
int curl_recv_post_data(void *buffer, size_t size, size_t nmemb, char *useless)
{
  cJSON *cjson = cJSON_Parse(buffer);//将JSON字符串转换成JSON结构体
  if (cjson == NULL)          //判断转换是否成功
    printf("cjson error...\r\n");
  else {
    int code = cJSON_GetObjectItem(cjson, "code")->valueint;
    if (code == 200) {
      cJSON *psub = cJSON_GetObjectItem(cjson, "data"); //解析数组
      if (psub) {
        char *pobj = cJSON_GetObjectItem(psub, "password")->valuestring;
        if (pobj) {
          void SetMqttPwd(char *pwd);
          printf("%d %s\r\n", code, pobj);
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
    printf("curl error: %d => ", res);
    switch (res) {
    case CURLE_UNSUPPORTED_PROTOCOL:
      printf("Does not support agreement\n");
      break;
    case CURLE_COULDNT_CONNECT:
      printf("not link remote server\n");
      break;
    case CURLE_HTTP_RETURNED_ERROR:
      printf("http return failed\n");
      break;
    case CURLE_READ_ERROR:
      printf("read local file failed\n");
      break;
    default:
      printf("curl other error\n");
      break;
    }
    iRet = -1;
  } else
    iRet = 0;
  curl_easy_cleanup(curl);
  return iRet;
}