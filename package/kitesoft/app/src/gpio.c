#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include <poll.h>
#include "gpio.h"

#define FILENAME_SIZE  128

#define GPIO_PATH "/sys/class/gpio"

/**
* Return 1 if specified GPIO pin is exported
*
* @param pin_number specified the pin
*
* @return 1 if GPIO pin is exported.
*  return 0 if GPIO pin is not exported
*/
int gpio_is_exported(int pin_number)
{
  char  buf[FILENAME_SIZE];
  struct stat dir;

  //check if the gpioXX directory exists
  snprintf(buf, sizeof(buf), GPIO_PATH "/gpio%d/", pin_number);
  if (stat(buf, &dir) == 0 && S_ISDIR(dir.st_mode)) {
    return GPIO_SUCCESS;
  } else {
    return 0;
  }
}
//导出io
int gpio_export(int pin_number)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;

	if (gpio_is_exported(pin_number)) { return GPIO_SUCCESS; }

	//write to export file
	fp = open(GPIO_PATH "/export", O_WRONLY);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	length = snprintf(buf, sizeof(buf), "%d", pin_number);
	if (write(fp, buf, length * sizeof(char)) == -1) {
		close(fp);
		return GPIO_INVALID_RESOURCE;
	}

	close(fp);
	return gpio_is_exported(pin_number);
}
/**
* unexport specified GPIO pin
*
* @param pin_number specified the pin
*
* @return 1 if success.
*  return negative integer error code if fail.
*/
int gpio_unexport(int pin_number)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;

	//write to unexport file
	fp = open(GPIO_PATH "/unexport", O_WRONLY);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	length = snprintf(buf, sizeof(buf), "%d", pin_number);
	if (write(fp, buf, length * sizeof(char)) == -1) {
		close(fp);
		return GPIO_INVALID_RESOURCE;
	}

	close(fp);
	return GPIO_SUCCESS;
}

int gpio_open(int pin_number)
{
	char buf[FILENAME_SIZE];
	int  fp;

	if (!gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number))
			return GPIO_INVALID_RESOURCE;
	}

	// open value file
	snprintf(buf, sizeof(buf), "%s/gpio%d/value", GPIO_PATH, pin_number);
	fp = open(buf, O_RDWR);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if (lseek(fp, 0, SEEK_SET) < 0) {
		close(fp);
	}

	return fp;
}
/**
* Read specified GPIO pin
*
* @param pin_number specified the pin
*
* @return integer value of GPIO if success.
*  return negative integer error code if fail.
*/
int gpio_read(int pin_number)
{
	char buf[FILENAME_SIZE];
	int  fp;

	if (!gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number))
			return GPIO_INVALID_RESOURCE;
	}

	//read value file
	snprintf(buf, sizeof(buf), "%s/gpio%d/value", GPIO_PATH, pin_number);
	fp = open(buf, O_RDWR);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if (lseek(fp, 0, SEEK_SET) < 0) {
		close(fp);
	}

	if (read(fp, buf, 2 * sizeof(char)) != 2) {
		close(fp);
		return GPIO_ERROR_READ;
	}

	close(fp);

	return (int) strtol(buf, NULL, 10);
}

/**
* Write specified GPIO pin
*
* @param pin_number specified the pin
*
* @param value  could be GPIO_HIGH or GPIO_LOW
*
* @return 1 if success.
*  return negative integer error code if fail.
*/
int gpio_write(int pin_number, int value)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;


	if (GPIO_SUCCESS != gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number) )
			return GPIO_INVALID_RESOURCE;
	}

	//write to value file
	snprintf(buf, sizeof(buf), "%s/gpio%d/value", GPIO_PATH, pin_number);
	fp = open(buf, O_RDWR);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if ( lseek(fp, 0, SEEK_SET) < 0 ) {
		close(fp);
	}

	length = snprintf(buf, sizeof(buf), "%d", value);
	if (write(fp, buf, length * sizeof(char)) == -1) {
		close(fp);
		return GPIO_ERROR_WRITE;
	}

	close(fp);

	return GPIO_SUCCESS;
}
/**
* set direction of specified GPIO pin
*
* @param pin_number specified the pin
*
* @param direction could be GPIO_INPUT or GPIO_OUTPUT, GPIO_OUTPUT_HIGH, GPIO_OUTPUT_LOW
*
* @return 1 if success.
*  return negative integer error code if fail.
*/
int gpio_set_direction(int pin_number, int direction)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;

	if (GPIO_SUCCESS != gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number))
			return GPIO_INVALID_RESOURCE;
	}

	//write direction file
	snprintf(buf, sizeof(buf), "%s/gpio%d/direction", GPIO_PATH, pin_number);
	fp = open(buf, O_RDWR);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if (lseek(fp, 0, SEEK_SET) < 0) {
		close(fp);
		return GPIO_INVALID_RESOURCE;
	}

	switch (direction) {
	case GPIO_OUTPUT:
		length = snprintf(buf, sizeof(buf), "out");
		break;
	case GPIO_INPUT:
		length = snprintf(buf, sizeof(buf), "in");
		break;
	case GPIO_OUTPUT_HIGH:
		length = snprintf(buf, sizeof(buf), "high");
		break;
	case GPIO_OUTPUT_LOW:
		length = snprintf(buf, sizeof(buf), "low");
		break;
	default:
		close(fp);
		return GPIO_ERROR_DIRECTION;
	}

	if (write(fp, buf, length * sizeof(char)) == -1) {
		close(fp);
		return GPIO_ERROR_WRITE;
	}

	close(fp);

	return GPIO_SUCCESS;
}

/**
* get direction of specified GPIO pin
*
* @param pin_number specified the pin
*
* @return positive integer direction code if success.
*  return negative integer error code if fail.
*/
int gpio_get_direction(int pin_number)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;
	int  result;

	if (GPIO_SUCCESS != gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number))
			return GPIO_INVALID_RESOURCE;
	}

	//read direction file
	snprintf(buf, sizeof(buf), "%s/gpio%d/direction", GPIO_PATH, pin_number);
	fp = open(buf, O_RDONLY);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if (lseek(fp, 0, SEEK_SET) < 0 ) {
		close(fp);
		return GPIO_INVALID_RESOURCE;
	}

	memset(buf, '\0', sizeof(buf));
	length = read(fp, buf, sizeof(buf));
	close(fp);

	if (length <= 0) {
		return GPIO_INVALID_RESOURCE;
	}

	if (strncmp(buf, "out", 3) == 0) {
		result = GPIO_OUTPUT;
	} else if (strncmp(buf, "in", 2) == 0) {
		result = GPIO_INPUT;
	} else if (strncmp(buf, "high", 4) == 0) {
		result = GPIO_OUTPUT_HIGH;
	} else if (strncmp(buf, "low", 3) == 0) {
		result = GPIO_OUTPUT_LOW;
	} else {
		result = GPIO_ERROR_DIRECTION;
	}

	return result;
}

int gpio_set_edge(int pin_number, int edge)
{
	char buf[FILENAME_SIZE];
	int  fp;
	int  length;

	if (GPIO_SUCCESS != gpio_is_exported(pin_number)) {
		if (GPIO_SUCCESS != gpio_export(pin_number))
			return GPIO_INVALID_RESOURCE;
	}

	//write edge file
	snprintf(buf, sizeof(buf), "%s/gpio%d/edge", GPIO_PATH, pin_number);
	fp = open(buf, O_RDWR);
	if (fp == -1) {
		return GPIO_INVALID_RESOURCE;
	}

	if (lseek(fp, 0, SEEK_SET) < 0) {
		close(fp);
		return GPIO_INVALID_RESOURCE;
	}

	switch (edge) {
	case GPIO_EDGE_NONE:
		length = snprintf(buf, sizeof(buf), "none");
		break;
	case GPIO_EDGE_RISING:
		length = snprintf(buf, sizeof(buf), "rising");
		break;
	case GPIO_EDGE_FALLING:
		length = snprintf(buf, sizeof(buf), "falling");
		break;
	case GPIO_EDGE_BOTH:
		length = snprintf(buf, sizeof(buf), "both");
		break;
	default:
		close(fp);
		return GPIO_ERROR_EDGE;
	}

	if (write(fp, buf, length * sizeof(char)) == -1) {
		close(fp);
		return GPIO_ERROR_WRITE;
	}

	close(fp);

	return GPIO_SUCCESS;
}

