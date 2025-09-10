/**
 * @file OTA.c
 * @author yassine hattay (hattayyassine519@gmail.com)
 * @brief OTA update module for ESP12/ESP8266, including GPIO-triggered updates
 * and HTTP download.
 *
 * This file implements over-the-air (OTA) update functionality for the ESP12
 * system, allowing firmware updates to be triggered via a GPIO interrupt. It
 * includes:
 *
 * 1. **OTA Semaphore:** A binary semaphore (`ota_sem`) used for signaling the
 * OTA task from the interrupt service routine (ISR).
 * 2. **OTA GPIO Interrupt:** The ISR (`ota_isr_handler`) is attached to the
 * OTA_GPIO pin and triggers on a negative edge to safely notify the OTA task.
 * 3. **OTA Task:** The `ota_task` waits for the semaphore and calls
 * `ota_update()` when triggered, ensuring safe execution in task context.
 * 4. **OTA Update Function:** `ota_update()` performs the following:
 *    - Connects to Wi-Fi using `wifi_init_sta()`.
 *    - Opens an HTTP connection to the defined `OTA_URL`.
 *    - Determines the current running partition and selects the next update
 * partition.
 *    - Downloads the new firmware in chunks, writing to flash using
 * `esp_ota_write()`.
 *    - Completes the OTA process and sets the boot partition to the new
 * firmware.
 *    - Restarts the ESP to apply the update.
 *
 * @version 0.1
 * @date 2025-09-09
 * @copyright Copyright (c) 2025
 */

#include "OTA.h"
#include "../WiFi/WiFi.h"

SemaphoreHandle_t ota_sem;

/**
 * @brief GPIO interrupt service routine for OTA triggering.
 *
 * This ISR is attached to the OTA_GPIO pin and triggers on a negative edge.
 * It gives the `ota_sem` semaphore to notify the OTA task that an update should
 * start. The ISR uses `portYIELD_FROM_ISR()` to request a context switch if a
 * higher-priority task was woken.
 *
 * @param arg Optional argument passed to the ISR (not used here).
 */
void IRAM_ATTR ota_isr_handler(void *arg) {
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xSemaphoreGiveFromISR(ota_sem, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken)
    portYIELD_FROM_ISR();
}
/**
 * @brief FreeRTOS task handling OTA updates.
 *
 * This task waits for the `ota_sem` semaphore, which is given by the OTA GPIO
 * interrupt service routine (`ota_isr_handler`). When the semaphore is
 * received, the task safely calls `ota_update()` to perform the firmware update
 * in task context, ensuring that the potentially long-running OTA process does
 * not execute inside the ISR.
 *
 * @param arg Optional argument passed to the task (not used here).
 */
void ota_task(void *arg) {
  while (1) {
    if (xSemaphoreTake(ota_sem, portMAX_DELAY) == pdTRUE) {
      printf("OTA trigger received, starting update...\n");
      ota_update(); // run OTA safely
    }
  }
}

/**
 * @brief Performs an over-the-air (OTA) firmware update.
 *
 * This function handles the complete OTA update process for the ESP12/ESP8266
 * system:
 * 1. Initializes Wi-Fi in station mode using `wifi_init_sta()`.
 * 2. Opens an HTTP connection to the firmware URL defined by `OTA_URL`.
 * 3. Determines the current running partition and selects the next OTA
 * partition.
 * 4. Downloads the new firmware in chunks and writes it to flash using
 * `esp_ota_write()`.
 * 5. Completes the OTA process with `esp_ota_end()` and sets the boot partition
 * to the new firmware.
 * 6. Restarts the ESP to apply the update.
 *
 * If any step fails, the function prints an error message and safely cleans up
 * resources.
 *
 * @note This function should be called from a FreeRTOS task context (e.g.,
 * `ota_task`) and not directly from an ISR.
 */

void ota_update(void) {

  wifi_init_sta();

  printf("[OTA] Starting OTA...\n");

  esp_http_client_config_t config = {.url = OTA_URL, .timeout_ms = 5000};
  esp_http_client_handle_t client = esp_http_client_init(&config);

  if (esp_http_client_open(client, 0) != ESP_OK) {
    printf("[OTA] Failed to open HTTP connection\n");
    esp_http_client_cleanup(client);
    return;
  }

  const esp_partition_t *running = esp_ota_get_running_partition();
  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(running);

  if (!update_partition) {
    printf("[OTA] No valid OTA partition found!\n");
    esp_http_client_cleanup(client);
    return;
  }

  printf("[OTA] Using partition: label=%s, offset=0x%X, size=0x%X\n",
         update_partition->label, update_partition->address,
         update_partition->size);

  esp_ota_handle_t ota_handle;
  esp_err_t ret =
      esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
  if (ret != ESP_OK) {
    printf("[OTA] esp_ota_begin failed: 0x%X\n", ret);
    esp_http_client_cleanup(client);
    return;
  }

  int total_bytes = 0;
  int data_read;
  static char buffer[1024];

  while (1) {
    vTaskDelay(1);

    data_read = esp_http_client_read(client, buffer, sizeof(buffer));
    if (data_read > 0) {
      ret = esp_ota_write(ota_handle, buffer, data_read);
      if (ret != ESP_OK) {
        printf("[OTA] esp_ota_write failed: 0x%X\n", ret);
        esp_http_client_cleanup(client);
        return;
      }
      total_bytes += data_read;
    } else if (data_read == 0) {
      // Check if we reached the end of content
      int content_len = esp_http_client_fetch_headers(client);
      if (total_bytes >= content_len)
        break;
      vTaskDelay(10 / portTICK_PERIOD_MS); // small delay before retry
    } else {
      printf("[OTA] esp_http_client_read error\n");
      esp_http_client_cleanup(client);
      return;
    }
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  ret = esp_ota_end(ota_handle);
  if (ret != ESP_OK) {
    printf("[OTA] esp_ota_end failed: 0x%X\n", ret);
    return;
  }

  ret = esp_ota_set_boot_partition(update_partition);
  if (ret != ESP_OK) {
    printf("[OTA] esp_ota_set_boot_partition failed: 0x%X\n", ret);
    return;
  }

  printf("[OTA] OTA successful, restarting...\n");
  esp_restart();
}
