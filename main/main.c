#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>
#include "../components/UART/UART.h"
#include "../components/sim800L_driver/sim800L_driver.h"
#include "../components/OTA/OTA.h"

// GPIO used to trigger OTA (e.g., GPIO 0)
#define OTA_GPIO 5  // use GPIO5

// Semaphore to notify OTA task
static SemaphoreHandle_t ota_sem;

// ISR for OTA trigger
static void IRAM_ATTR ota_isr_handler(void *arg) {
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xSemaphoreGiveFromISR(ota_sem, &xHigherPriorityTaskWoken);
	if (xHigherPriorityTaskWoken)
		portYIELD_FROM_ISR();
}

// Task that waits for OTA trigger
static void ota_task(void *arg) {
	while (1) {
		if (xSemaphoreTake(ota_sem, portMAX_DELAY) == pdTRUE) {
			printf("OTA trigger received, starting update...\n");
			ota_update();  // This runs OTA safely
		}
	}
}

void init_esp() {
	uart_t uart0 = { 0, 3, 1, 1, 1, 9600 };
	my_uart_init(&uart0);

	uart_t uart1 = { 1, 3, 1, 1, 1, 9600 };
	my_uart_init(&uart1);

	printf("\n Version 10 \n");

	// Create OTA semaphore
	ota_sem = xSemaphoreCreateBinary();

	// Configure OTA GPIO interrupt
	gpio_config_t io_conf = {
			.intr_type = GPIO_INTR_NEGEDGE,  // Trigger on falling edge
			.mode = GPIO_MODE_INPUT, .pin_bit_mask = 1ULL << OTA_GPIO,
			.pull_up_en = GPIO_PULLUP_ENABLE, .pull_down_en =
					GPIO_PULLDOWN_DISABLE };
	gpio_config(&io_conf);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(OTA_GPIO, ota_isr_handler, NULL);
}


void app_main(void) {

	init_esp();

	// Start OTA task
	xTaskCreate(ota_task, "ota_task", 4096, NULL, 10, NULL);

	// Start SIM800 task
	xTaskCreate(sim800_task, "sim800_task", 4096, NULL, 5, NULL);
}
