/**
 * @file NEO_6M.h
 * @author yassine hattay
 * @brief NEO-6M GPS driver for ESP12/ESP8266.
 *
 * This module provides functionality to interact with the NEO-6M GPS module:
 * - Parse NMEA sentences (GPRMC) to extract latitude and longitude.
 * - Prepare SMS messages with current coordinates for SIM800L transmission.
 * - Monitor GPS fix and trigger deep sleep if no fix is obtained within timeout.
 * - Start the SIM800L task automatically once a valid fix is acquired.
 *
 * @version 0.1
 * @date 2025-09-09
 */

#ifndef NEO_6M_CONFIG_H_
#define NEO_6M_CONFIG_H_

#include "../debugging/my_print.h"
#include "../web/web.h"
#include <driver/uart.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "../sim800L_driver/sim800L_driver.h"
/** @brief UART port for GPS RX */
#define UART_GPS_RX   UART_NUM_0

/** @brief UART port for GPS TX */
#define UART_GPS_TX   UART_NUM_0

/** @brief Size of the buffer for reading GPS data */
#define BUF_SIZE      1024

/** @brief GPIO used for GPS status indication (e.g., LED blink) */
#define GPS_gpio 4

/** @brief Maximum time to wait for a GPS fix before entering deep sleep (seconds) */
#define GPS_TIMEOUT_SEC 1000

/** @brief Duration to sleep between GPS retry attempts (seconds) */
#define GPS_RETRY_SLEEP_SEC 300

extern volatile double g_latitude;
extern volatile double g_longitude;
extern volatile int g_new_fix;

void gps_task(void *arg);

#endif
