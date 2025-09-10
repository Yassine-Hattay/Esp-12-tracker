/**
 * @file OTA.h
 * @author yassine hattay
 * @brief OTA update module for ESP12/ESP8266.
 *
 * This module provides functionality to perform OTA updates:
 * - Trigger OTA via a GPIO pin or manually.
 * - Download and install new firmware from a specified URL.
 * - Use FreeRTOS tasks and semaphores to manage OTA operations.
 *
 * @version 0.1
 * @date 2025-09-09
 */

#ifndef OTA_H_
#define OTA_H_

#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

/** @brief URL of the OTA firmware binary to download */
#define OTA_URL "http://192.168.1.106:8000/hello-world1.bin"

/** @brief GPIO used to trigger OTA update (e.g., button press) */
#define OTA_GPIO 5

void ota_update(void);
void ota_task(void *arg);
extern SemaphoreHandle_t ota_sem;

void IRAM_ATTR ota_isr_handler(void *arg);

#endif /* COMPONENTS_DEBUGGING_MY_PRINT_H_ */
