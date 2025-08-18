#ifndef NEO_6M_CONFIG_H_
#define NEO_6M_CONFIG_H_

#include "../debugging/my_print.h"
#include "../web/web.h"
#include <driver/uart.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define UART_GPS_RX   UART_NUM_0
#define UART_GPS_TX   UART_NUM_1
#define BUF_SIZE      1024

extern volatile double g_latitude ;
extern volatile double g_longitude ;
extern volatile int g_new_fix ;

void gps_task(void *arg) ;

#endif
