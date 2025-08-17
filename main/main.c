#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <driver/uart.h>
#include <stdio.h>

#include "../components/OTA/OTA.h"
#include "../components/WiFi/WiFi.h"
#include "../components/UART/UART.h"

void app_main(void) {
	uart_t uart0 = { 0, 3, 1, 1, 1, 115200 };
	my_uart_init(&uart0);
	printf("[APP] UART initialized V 3 \n");

	wifi_init_sta();
	printf("[APP] Wi-Fi connected, starting OTA...\n");

	ota_update();
}
