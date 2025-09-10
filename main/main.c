/**
 * @file main.c
 * @author yassine hattay (hattayyassine519@gmail.com)
 * @brief Main application file for ESP12 integrating SIM800L, NEO-6M GPS,
 * and OTA update functionality.
 * @version 0.1
 * @date 2025-05-28
 * @copyright Copyright (c) 2025
 *
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <stdio.h>
#include <string.h>
#include "../components/UART/UART.h"
#include "../components/sim800L_driver/sim800L_driver.h"
#include "../components/NEO_6M_driver/NEO_6M.h"
#include "../components/OTA/OTA.h"

/**
 * @brief Initializes essential ESP peripherals including UART, OTA, GPIO, and ADC.
 *
 * This function performs the initial setup of core ESP microcontroller peripherals
 * required for the application. It configures UART communication, sets up OTA
 * interrupt handling, and initializes the ADC for analog readings.
 *
 * The initialization steps include:
 * 1. **UART Initialization:**  
 *    - Creates a `uart_t` structure with default parameters (UART0, baud rate 9600, etc.).  
 *    - Calls `my_uart_init()` to configure the UART peripheral.
 * 2. **OTA Semaphore:**  
 *    - Creates a binary semaphore `ota_sem` for OTA update handling.
 * 3. **OTA GPIO Interrupt Configuration:**  
 *    - Configures the `OTA_GPIO` pin as input with pull-up enabled.  
 *    - Sets interrupt type to negative edge (`GPIO_INTR_NEGEDGE`).  
 *    - Installs ISR service and attaches `ota_isr_handler` to the GPIO.
 * 4. **ADC Initialization:**  
 *    - Configures ADC with `ADC_READ_TOUT_MODE` for reading A0 pin.  
 *    - Sets the sample clock divider (`clk_div`) to 8.  
 *    - Calls `adc_init()` and checks for successful initialization.
 *
 * @note Prints status messages to indicate successful initialization or errors.
 */
void init_esp() {
	// UART init
	uart_t uart0 = { 0, 3, 1, 1, 1, 9600 };
	my_uart_init(&uart0);

	printf("Initialization done!\n");

	// OTA semaphore
	ota_sem = xSemaphoreCreateBinary();

	// Configure OTA GPIO interrupt
	gpio_config_t io_conf = { .intr_type = GPIO_INTR_NEGEDGE, .mode =
			GPIO_MODE_INPUT, .pin_bit_mask = 1ULL << OTA_GPIO, .pull_up_en =
			GPIO_PULLUP_ENABLE, .pull_down_en = GPIO_PULLDOWN_DISABLE };
	gpio_config(&io_conf);
	gpio_install_isr_service(0);
	gpio_isr_handler_add(OTA_GPIO, ota_isr_handler, NULL);

	// Initialize ADC
	adc_config_t adc_cfg = { .mode = ADC_READ_TOUT_MODE,  // read A0 pin
			.clk_div = 8                 // sample clock divider (8–32)
			};
	if (adc_init(&adc_cfg) != ESP_OK) {
		printf("ADC initialization failed!\n");
	} else {
		printf("ADC initialized successfully.\n");
	}
}

/**
 * @brief Main application entry point for ESP8266.
 *
 * This function initializes the ESP peripherals, configures GPIOs for status
 * indicators, starts FreeRTOS tasks, and continuously monitors the battery voltage
 * using the ADC.
 *
 * Initialization and setup steps:
 * 1. **ESP Peripheral Initialization:**  
 *    - Calls `init_esp()` to initialize UART, ADC, and OTA-related GPIO and semaphore.
 * 2. **GPIO Configuration for LEDs/Status Pins:**  
 *    - Configures `GPS_gpio` and `SIM_gpio` as output pins without pull-up/pull-down.  
 *    - Sets initial levels: GPS low (0), SIM high (1).
 * 3. **Task Creation:**  
 *    - Creates `ota_task` with priority 11 and `gps_task` with priority 10.
 * 4. **Battery Monitoring Loop:**  
 *    - Samples the ADC 100 times every 5 seconds.  
 *    - Computes the average ADC value and scales it using the measured ADC reference.  
 *    - Calculates battery voltage using the divider ratio (based on the resistor network).  
 *    - Prints ADC average, ADC voltage, and battery voltage to console.  
 *    - Warns if battery voltage falls below 3.4 V.
 *
 * @note The function runs an infinite loop and never returns.
 */

void app_main(void) {
	init_esp();
	// Configure GPIO4 for blinking
	gpio_config_t io_conf_out = { .pin_bit_mask = 1ULL << GPS_gpio, .mode =
			GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };
	gpio_config(&io_conf_out);

	// Configure GPIO4 for blinking
	gpio_config_t io_conf_out1 = { .pin_bit_mask = 1ULL << SIM_gpio, .mode =
			GPIO_MODE_OUTPUT, .pull_up_en = GPIO_PULLUP_DISABLE, .pull_down_en =
			GPIO_PULLDOWN_DISABLE, .intr_type = GPIO_INTR_DISABLE };

	gpio_config(&io_conf_out1);

	gpio_set_level(GPS_gpio, 0);
	gpio_set_level(SIM_gpio, 1);

	vTaskDelay(3000);
	// Start OTA task
	xTaskCreate(ota_task, "ota_task", 4096, NULL, 11, NULL);
	xTaskCreate(gps_task, "sim800_task", 4096, NULL, 10, NULL);

}
