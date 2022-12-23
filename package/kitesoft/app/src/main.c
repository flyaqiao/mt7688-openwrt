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

#define TST_GPIO 491
//开始捕获测试
int gpio_irq_test(void)
{
	int fd = -1;

	//导出io
	if (GPIO_SUCCESS != gpio_export(TST_GPIO)) {
		perror("gpio_export failed!\n");
	}

	//配置成输入
	if (gpio_set_direction(TST_GPIO, GPIO_INPUT) != GPIO_SUCCESS) {
		perror("open direction failed!\n");
		return GPIO_INVALID_RESOURCE;
	}
	printf("set direction ok!\n");

	//边沿触发
	if (GPIO_SUCCESS != gpio_set_edge(TST_GPIO, GPIO_EDGE_BOTH)) {
		perror("open edge failed!\n");
		return GPIO_INVALID_RESOURCE;
	}
	printf("set edge ok!\n");

	//开始捕获
	fd = gpio_open(TST_GPIO);
	if (fd < 0) {
		perror("open failed!\n");
		return -1;
	}
	printf("open Ok!\n");

	struct pollfd fds[1];
	fds[0].fd = fd;
	fds[0].events = POLLPRI;

	while (1) {
		if (poll(fds, 1, 0) == -1) {
			perror("poll failed!\n");
			return -1;
		}

		if (fds[0].revents & POLLPRI) {
			//能进这里就代表触发事件发生了,我这里是置了一个标志
			if (lseek(fd, 0, SEEK_SET) == -1) {
				perror("lseek value failed!\n");
				return -1;
			}
			char buffer[16];
			int len;
			//一定要读取,不然会一直检测到紧急事件
			if ((len = read(fd,buffer,sizeof(buffer))) == -1)  {
				perror("read value failed!\n");
				return -1;
			}
			buffer[len] = 0;
			printf("%s", buffer);
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

int main(int argc, char **argv)
{
	//gpio_irq_test();
	StartBackgroudTask(CommThread, (void*)1);
	StartBackgroudTask(CommThread, (void*)2);
	while (1) {
		printf("Main\r\n");
		DelayMs(1000);
	}
	printf("Exit\r\n");
	return 0;
}
