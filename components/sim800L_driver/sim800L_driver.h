/**
 * @file sim800L_driver.h
 * @author yassine hattay
 * @brief SIM800L driver configuration for ESP12/ESP8266.
 *
 * This module provides configuration and function declarations for interacting
 * with the SIM800L module:
 * - Sending SMS with retries and delivery handling.
 * - Monitoring network registration.
 * - Triggering deep sleep before and after sending messages.
 *
 * @version 0.1
 * @date 2025-09-09
 */

#ifndef SIM800L_CONFIG_H_
#define SIM800L_CONFIG_H_

#include "../debugging/my_print.h"
#include "../web/web.h"
#include <driver/uart.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/adc.h"

/** @brief UART port used for SIM800 communication */
#define UART_SIM800_NUM      UART_NUM_0

/** @brief Baud rate for SIM800 UART communication */
#define UART_SIM800_BAUD     9600

/** @brief Size of UART buffer for reading/writing data */
#define BUF_SIZE             1024

/** @brief GPIO used to indicate SIM800 status (e.g., LED blink) */
#define SIM_gpio 14

/** @brief Maximum number of retries if SMS sending fails */
#define SMS_MAX_RETRIES 3

void sim800_task(void *arg);

#endif
