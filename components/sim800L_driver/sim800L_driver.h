#ifndef SIM800L_CONFIG_H_
#define SIM800L_CONFIG_H_

#include "../debugging/my_print.h"
#include "../web/web.h"
#include <driver/uart.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define UART_SIM800_NUM      UART_NUM_0
#define UART_SIM800_BAUD     9600
#define BUF_SIZE             1024

void sim800_task(void *arg);

#endif
