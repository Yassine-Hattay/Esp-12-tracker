#include "sim800L_driver.h"
#include "esp_sleep.h"

// Add country code prefix to the phone number if needed (example: Tunisia +216)
const char phoneNumber[] = "50713097";
const char smsMessage[] = "35.370605615245296, 9.535919806657951";

// Global variable for deep sleep duration in seconds
uint64_t deep_sleep_time_sec = 10; // Default 10 seconds
uint64_t deep_sleep_time_sec_after_send = 10; // Default 10 seconds

// Flush UART input buffer (discard any data)
static void flush_uart_input(void) {
    uint8_t flush_buf[128];
    while (uart_read_bytes(UART_SIM800_NUM, flush_buf, sizeof(flush_buf),
            10 / portTICK_PERIOD_MS) > 0) {
        vTaskDelay(1);
    }
}

static void send_uart_command(const char *cmd, unsigned int delay_ms) {
    printf("SIM800: Sending command -> %s\n", cmd);
    uart_write_bytes(UART_NUM_1, cmd, strlen(cmd));
    uart_write_bytes(UART_NUM_1, "\r\n", 2);
    vTaskDelay(delay_ms);
}

static bool wait_for_network() {
    printf("SIM800: Waiting for network registration...\n");
    uint32_t start = xTaskGetTickCount();

    while ((xTaskGetTickCount() - start) < pdMS_TO_TICKS(5000)) {
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

        printf("SIM800: Network response -> %s\n", response);

        if (strstr(response, "+CREG: 0,1") || strstr(response, "+CREG: 0,5")) {
            printf("SIM800: Registered on network!\n");
            return true;
        }
    }

    printf("SIM800: Network registration timed out.\n");
    return false;
}

static void soft_reset() {
    printf("SIM800: Performing soft reset...\n");
    send_uart_command("AT+CFUN=1,1", 10000);
}

static void send_sms(const char *number, const char *message) {
    printf("SIM800: Sending SMS to %s\n", number);
    send_uart_command("AT+CMGF=1", 500);

    char cmgs_cmd[64];
    snprintf(cmgs_cmd, sizeof(cmgs_cmd), "AT+CMGS=\"%s\"", number);

    uart_write_bytes(UART_NUM_1, cmgs_cmd, strlen(cmgs_cmd));
    uart_write_bytes(UART_NUM_1, "\r\n", 2);
    vTaskDelay(500);

    uart_write_bytes(UART_NUM_1, message, strlen(message));
    vTaskDelay(500);

    uint8_t ctrl_z = 26;
    uart_write_bytes(UART_NUM_1, (const char*)&ctrl_z, 1);
    vTaskDelay(5000);

    printf("SIM800: SMS send confirmed\n");
}

void sim800_task(void *arg) {
    printf("SIM800 task started\n");

    vTaskDelay(pdMS_TO_TICKS(1000));

    send_uart_command("AT", 1000);
    send_uart_command("ATE0", 1000);

    while (1) { // Keep trying indefinitely
        if (!wait_for_network()) {
            soft_reset();

            if (!wait_for_network()) {
                printf("SIM800: Failed to register on GSM network after reset. Going to deep sleep for %llu seconds.\n", deep_sleep_time_sec);
                esp_deep_sleep(deep_sleep_time_sec * 1000000ULL); // Convert seconds to microseconds
            }
        }

        // Network is available, send SMS
        send_sms(phoneNumber, smsMessage);

        printf("SIM800 task finished sending SMS. Going to deep sleep for %llu seconds before next attempt.\n", deep_sleep_time_sec_after_send);
        esp_deep_sleep(deep_sleep_time_sec_after_send * 1000000ULL);
    }
}
