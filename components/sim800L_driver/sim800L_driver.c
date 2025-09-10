/**
 * @file sim800L_driver.c
 * @author yassine hattay
 * @brief SIM800L driver for ESP12/ESP8266 handling SMS, network registration,
 * and deep sleep.
 *
 * This file provides functionality to interact with the SIM800L module:
 * - Sending SMS messages with automatic retries and delivery report handling.
 * - Checking and waiting for network registration.
 * - Performing a soft reset if network registration fails.
 * - Controlling deep sleep timings before and after sending messages.
 *
 * @version 0.1
 * @date 2025-09-09
 */

#include "sim800L_driver.h"
#include "driver/gpio.h"
#include "driver/uart.h"
#include "esp_sleep.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdbool.h>
#include <string.h>
#include "driver/adc.h"

/** @cond HIDDEN */
const char phoneNumber[] = "+21650713097";
/** @endcond */

extern char smsMessage[64];

/**
 * @brief Deep sleep duration (in seconds) if no SMS is sent (network fail).
 */
uint64_t deep_sleep_time_sec = 10;

/**
 * @brief Deep sleep duration (in seconds) after a successful SMS send.
 */
uint64_t deep_sleep_time_sec_after_send = 10;

TaskHandle_t sim_task_handle = NULL;


/**
 * @brief Flushes and clears any pending data from the SIM800 UART input buffer.
 *
 * This function continuously reads available bytes from the UART input buffer
 * until it is empty, discarding all data. It is typically called before sending
 * new AT commands to ensure old or partial responses do not interfere with
 * fresh communication.
 */
static void flush_uart_input(void) {
    uint8_t flush_buf[128];
    while (uart_read_bytes(UART_SIM800_NUM, flush_buf, sizeof(flush_buf),
                           10 / portTICK_PERIOD_MS) > 0) {
        vTaskDelay(1);
    }
}
/**
 * @brief Sends an AT command string to the SIM800 module over UART.
 *
 * This function writes the given AT command followed by carriage return
 * and newline characters (`\r\n`) to the SIM800 UART interface, then waits
 * for the specified delay to allow the module time to process the command.
 *
 * @param cmd       The AT command string to send (null-terminated).
 * @param delay_ms  Time in milliseconds to wait after sending the command.
 */
static void send_uart_command(const char *cmd, unsigned int delay_ms) {
    uart_write_bytes(UART_NUM_0, cmd, strlen(cmd));
    uart_write_bytes(UART_NUM_0, "\r\n", 2);
    vTaskDelay(delay_ms);
}


/**
 * @brief Reads data from the SIM800 UART into a buffer.
 *
 * This function continuously reads characters from the SIM800 UART until
 * either the buffer is full or the specified timeout expires.
 * It appends a null terminator (`'\0'`) at the end of the buffer to ensure
 * it can be safely used as a C-string.
 *
 * @param buffer       Pointer to the destination buffer to store received data.
 * @param bufsize      Size of the destination buffer in bytes.
 * @param timeout_ms   Maximum time to wait for data in milliseconds.
 *
 * @return Number of bytes actually read (excluding the null terminator).
 */
static int read_uart_response(char *buffer, int bufsize, uint32_t timeout_ms) {
    int pos = 0;
    uint32_t t_start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - t_start) < pdMS_TO_TICKS(timeout_ms)
            && pos < (bufsize - 1)) {
        uint8_t ch;
        int len = uart_read_bytes(UART_SIM800_NUM, &ch, 1,
                50 / portTICK_PERIOD_MS);
        if (len > 0) {
            buffer[pos++] = (char) ch;
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
    buffer[pos] = '\0';
    return pos;
}

/**
 * @brief Waits for SIM800 network registration.
 *
 * This function repeatedly queries the SIM800 module using the AT+CREG? command
 * until the module reports that it is registered to the network (status 1 or 5)
 * or until a 10-second timeout occurs.
 *
 * @return true if the SIM800 is registered to the network, false if timeout occurs.
 */
static bool wait_for_network() {
    uint32_t start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(10000)) {
        flush_uart_input();
        send_uart_command("AT+CREG?", 500);

        char response[256];
        int len = read_uart_response(response, sizeof(response), 1000);

        if (strstr(response, "+CREG: 0,1") || strstr(response, "+CREG: 0,5"))
            return true; // Registered
    }

    return false; // timeout
}

/**
 * @brief Performs a soft reset of the SIM800 module.
 *
 * Sends the AT+CFUN=1,1 command to the SIM800 to perform a full restart.
 * Waits 10 seconds for the module to reboot.
 */
static void soft_reset() {
    send_uart_command("AT+CFUN=1,1", 10000);
}

/**
 * @brief Sends an SMS message via SIM800 using UART0.
 *
 * This function:
 * - Flushes the UART input buffer.
 * - Configures the SIM800 for text mode and SMS settings.
 * - Sends the AT+CMGS command with the recipient number.
 * - Waits for the '>' prompt before sending the message text.
 * - Sends the message text followed by Ctrl+Z to submit.
 * - Waits for +CMGS confirmation or CMS ERROR response.
 *
 * @param number The recipient phone number (with country code, e.g., "+21650713097").
 * @param message The text message to send.
 * @return true if the SMS was submitted successfully, false otherwise.
 */
static bool send_sms(const char *number, const char *message) {
    flush_uart_input();

    send_uart_command("AT+CMGF=1", 1000);           // Text mode
    send_uart_command("AT+CSMP=49,167,0,0", 1000);  // SMS settings
    send_uart_command("AT+CNMI=2,1,0,0,0", 1000);   // Indications

    // Prepare CMGS command
    char cmgs_cmd[64];
    snprintf(cmgs_cmd, sizeof(cmgs_cmd), "AT+CMGS=\"%s\"\r\n", number);
    uart_write_bytes(UART_NUM_0, cmgs_cmd, strlen(cmgs_cmd));

    // Wait for '>' prompt
    char response[256] = {0};
    int len = read_uart_response(response, sizeof(response), 7000);
    if (len <= 0 || strchr(response, '>') == NULL) {
        return false;
    }

    // Send message text
    uart_write_bytes(UART_NUM_0, message, strlen(message));
    vTaskDelay(pdMS_TO_TICKS(200));

    // Send Ctrl+Z (end of SMS)
    const uint8_t ctrl_z = 0x1A;
    uart_write_bytes(UART_NUM_0, (const char*)&ctrl_z, 1);

    // Wait for +CMGS confirmation or ERROR
    memset(response, 0, sizeof(response));
    len = read_uart_response(response, sizeof(response), 15000);

    if (strstr(response, "+CMGS:")) {
        return true;
    } else if (strstr(response, "+CMS ERROR")) {
        return false;
    }
    return false;
}


/**
 * @brief Main task for SIM800 operation.
 *
 * This FreeRTOS task handles:
 * - Measuring battery voltage.
 * - Initializing and configuring the SIM800 module.
 * - Waiting for network registration with retries.
 * - Preparing and sending an SMS with coordinates and battery voltage.
 * - Performing soft reset and deep sleep on failure or after sending.
 *
 * @param arg Task argument (unused).
 */

void sim800_task(void *arg) {

	// --- Measure battery voltage ---
	uint32_t adc_sum = 0;
	const int samples = 100;

	for (int i = 0; i < samples; i++) {
		uint16_t adc_val;
		if (adc_read(&adc_val) == ESP_OK) {
			adc_sum += adc_val;
		}
		vTaskDelay(pdMS_TO_TICKS(5));
	}

	float adc_avg = adc_sum / (float) samples;
	float adc_ref_scale = 0.8f / 738.0f;
	float v_adc = adc_avg * adc_ref_scale;
	float divider_ratio = 0.258f;
	float v_bat = v_adc / divider_ratio;

	printf("Battery voltage: %.2f V\n", v_bat);
	if (v_bat <= 3.40f) {
		printf("Battery low!\n");
	}

	gpio_set_level(SIM_gpio, 0);

	vTaskDelay(pdMS_TO_TICKS(10000));
	sim_task_handle = xTaskGetCurrentTaskHandle();

	send_uart_command("AT", 1000);
	send_uart_command("ATE0", 1000);

	while (1) {
		if (!wait_for_network()) {
			soft_reset();
			if (!wait_for_network()) {
				gpio_set_level(SIM_gpio, 1);
				esp_deep_sleep(deep_sleep_time_sec * 1000000ULL);
			}
		}

		// --- Prepare SMS ---
		char sms_with_voltage[128];

		if (strlen(smsMessage) == 0) {
			snprintf(sms_with_voltage, sizeof(sms_with_voltage),
					"Coords: 36.38101236495415, 9.50555854663195\nBattery: %.2f V",
					v_bat);
		} else {
			snprintf(sms_with_voltage, sizeof(sms_with_voltage),
					"%s\nBattery: %.2f V", smsMessage, v_bat);
		}

		// --- Send SMS ---
		bool sms_sent = false;

		for (int attempt = 1; attempt <= SMS_MAX_RETRIES; attempt++) {
			if (send_sms(phoneNumber, sms_with_voltage)) {
				sms_sent = true;
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(2000));
		}

		if (!sms_sent) {
			soft_reset();
			continue;
		}

		gpio_set_level(SIM_gpio, 1);
		esp_deep_sleep(deep_sleep_time_sec_after_send * 1000000ULL);
	}
}
