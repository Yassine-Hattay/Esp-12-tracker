#include "sim800L_driver.h"

// Add country code prefix to the phone number if needed (example: Tunisia +216)
const char phoneNumber[] = "50713097";
const char smsMessage[] = "35.370605615245296, 9.535919806657951";

// Flush UART input buffer (discard any data)
static void flush_uart_input(void) {
	uint8_t flush_buf[128];
	while (uart_read_bytes(UART_SIM800_NUM, flush_buf, sizeof(flush_buf),
			10 / portTICK_PERIOD_MS) > 0) {
		vTaskDelay(1);
	}
}


static void send_uart_command(const char *cmd, unsigned int delay_ms) {
	uart_write_bytes(UART_SIM800_NUM, cmd, strlen(cmd));
	uart_write_bytes(UART_SIM800_NUM, "\r\n", 2);
	vTaskDelay(delay_ms);
}

static bool wait_for_network() {
	uint32_t start = xTaskGetTickCount();

	while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(30000)) {
		flush_uart_input();
		send_uart_command("AT+CREG?", 500);

		char response[256];
		int pos = 0;
		memset(response, 0, sizeof(response));

		uint32_t t_start = xTaskGetTickCount();

		while ((xTaskGetTickCount() - t_start) < pdMS_TO_TICKS(1000)
				&& pos < (int) (sizeof(response) - 1)) {
			uint8_t ch;
			int len = uart_read_bytes(UART_SIM800_NUM, &ch, 1,
					20 / portTICK_PERIOD_MS);
			if (len > 0) {
				response[pos++] = (char) ch;
			} else {
				vTaskDelay(pdMS_TO_TICKS(10));
			}
		}
		response[pos] = '\0';


		if (strstr(response, "+CREG: 0,1") || strstr(response, "+CREG: 0,5")) {
			return true;
		}
	}
	return false;
}

static void soft_reset() {
	send_uart_command("AT+CFUN=1,1", 10000);
}

static void send_sms(const char *number, const char *message) {
	send_uart_command("AT+CMGF=1", 500);

	char cmgs_cmd[64];
	snprintf(cmgs_cmd, sizeof(cmgs_cmd), "AT+CMGS=\"%s\"", number);

	uart_write_bytes(UART_SIM800_NUM, cmgs_cmd, strlen(cmgs_cmd));
	uart_write_bytes(UART_SIM800_NUM, "\r\n", 2);
	vTaskDelay(500);
	uart_write_bytes(UART_SIM800_NUM, message, strlen(message));
	vTaskDelay(500);
	uint8_t ctrl_z = 26;
	uart_write_bytes(UART_SIM800_NUM, (const char*) &ctrl_z, 1);
	vTaskDelay(5000);

	my_print("SMS send confirmed\n");

}

static void sim800_task(void *arg) {
	my_print("SIM800 task started\n");

	uart_config_t uart_config = { .baud_rate = UART_SIM800_BAUD, .data_bits =
			UART_DATA_8_BITS, .parity = UART_PARITY_DISABLE, .stop_bits =
			UART_STOP_BITS_1, .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, };

	uart_driver_install(UART_SIM800_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
	uart_param_config(UART_SIM800_NUM, &uart_config);

	vTaskDelay(pdMS_TO_TICKS(1000));

	send_uart_command("AT", 1000);
	send_uart_command("ATE0", 1000);

	if (!wait_for_network()) {
		soft_reset();

		if (!wait_for_network()) {
			my_print(
					"Failed to register on GSM network after reset. SIM800 task exiting.\n");
			vTaskDelete(NULL);
			return;
		}
	}

	send_sms(phoneNumber, smsMessage);

	my_print("SIM800 task finished sending SMS.\n");
	vTaskDelete(NULL);
}
