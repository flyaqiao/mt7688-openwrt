#ifndef __GPIO_H__
#define __GPIO_H__
 
/* GPIO value */
#define GPIO_HIGH     1
#define GPIO_LOW      0

/* GPIO direction code */
#define GPIO_INPUT       0
#define GPIO_OUTPUT      1
#define GPIO_OUTPUT_HIGH 2
#define GPIO_OUTPUT_LOW  3

/* GPIO intc edge */
#define GPIO_EDGE_NONE		0	// 非中断引脚
#define GPIO_EDGE_RISING	1	// 上升沿触发
#define GPIO_EDGE_FALLING	2	// 下降沿触发
#define GPIO_EDGE_BOTH		3	// 边沿触发

/* GPIO error code */
#define GPIO_SUCCESS            1
#define GPIO_INVALID_RESOURCE  -1
#define GPIO_ERROR_READ        -2
#define GPIO_ERROR_WRITE       -3
#define GPIO_ERROR_DIRECTION   -4
#define GPIO_ERROR_EDGE		   -5

int gpio_is_exported(int pin_number);
int gpio_export(int pin_number);
int gpio_unexport(int pin_number);
int gpio_open(int pin_number);
int gpio_read(int pin_number);
int gpio_write(int pin_number, int value);
int gpio_set_direction(int pin_number, int direction);
int gpio_get_direction(int pin_number);
int gpio_set_edge(int pin_number, int edge);

#endif // __GPIO_H__