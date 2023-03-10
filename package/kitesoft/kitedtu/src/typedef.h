
#ifndef __TYPE_DEF_H__
#define __TYPE_DEF_H__
#ifndef COUNTOF
#define COUNTOF(_Array) (sizeof(_Array) / sizeof(_Array[0]))
#endif
#define MSG_KEY  0x8384250
#define MSG_TYPE 8384

struct msg_data {
  long mtype;
  int  mindex;
  int  mvalue;
  struct timeval timestamap;
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
  char MqttServer[32];
  uint32_t MqttPort;
  char MqttUser[32];
  char MqttPwd[32];
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
  MS_NONE,
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

extern PARAMETER m_Parameter;
#endif // __TYPE_DEF_H__