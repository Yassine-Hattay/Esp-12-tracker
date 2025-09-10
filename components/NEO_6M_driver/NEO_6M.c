/**
 * @file NEO_6M.c
 * @author yassine hattay
 * @brief NEO-6M GPS driver for ESP12/ESP8266.
 *
 * This module provides functionality to interact with the NEO-6M GPS module:
 * - Parse NMEA sentences (GPRMC) to extract latitude and longitude.
 * - Prepare SMS message with current coordinates for SIM800L transmission.
 * - Monitor GPS fix and trigger deep sleep if no fix is obtained within timeout.
 * - Start SIM800L task automatically once a valid fix is acquired.
 *
 * @version 0.1
 * @date 2025-09-09
 */

#include "NEO_6M.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_sleep.h"

// Global variables to store coordinates
volatile double g_latitude = 0.0;
volatile double g_longitude = 0.0;
volatile int g_new_fix = 0;  // flag set to 1 when new fix is available

// Buffer for SMS message
char smsMessage[64] = { 0 };

// Handle for GPS task
TaskHandle_t gps_task_handle = NULL;

/**
 * @brief Convert NMEA coordinate format (DDMM.MMMM) to decimal degrees.
 *
 * @param nmea_coord The NMEA coordinate string (latitude or longitude).
 * @param hemi Hemisphere character ('N', 'S', 'E', 'W').
 * @return double Decimal degrees, negative for 'S' or 'W'.
 */

static double convert_to_decimal(const char *nmea_coord, const char hemi) {
	if (strlen(nmea_coord) < 4)
		return 0.0;
	double val = atof(nmea_coord);
	int deg = (int) (val / 100);
	double min = val - (deg * 100);
	double decimal = deg + min / 60.0;
	if (hemi == 'S' || hemi == 'W')
		decimal *= -1;
	return decimal;
}

/**
 * @brief Parse a GPRMC NMEA sentence to extract valid GPS coordinates.
 *
 * This function reads a GPRMC line, extracts latitude and longitude,
 * converts them to decimal degrees, updates global variables, and
 * prepares the SMS message. If a valid fix is found, it starts the
 * SIM800 task and deletes the GPS task.
 *
 * @param line The NMEA GPRMC sentence string to parse.
 */

static void parse_GPRMC(char *line) {
	char *pos = strchr(line, '\r');
	if (pos)
		*pos = '\0';
	pos = strchr(line, '\n');
	if (pos)
		*pos = '\0';

	char *token;
	char *rest = line;

	char lat[16] = { 0 }, lon[16] = { 0 };
	char lat_hemi = 'N', lon_hemi = 'E';
	char status = 'V';

	token = strtok_r(rest, ",", &rest);
	if (!token || strcmp(token, "$GPRMC") != 0)
		return;

	int field = 1;
	while ((token = strtok_r(rest, ",", &rest))) {
		field++;
		switch (field) {
		case 3:
			status = token[0];
			break;
		case 4:
			strncpy(lat, token, sizeof(lat) - 1);
			break;
		case 5:
			lat_hemi = token[0];
			break;
		case 6:
			strncpy(lon, token, sizeof(lon) - 1);
			break;
		case 7:
			lon_hemi = token[0];
			break;
		}
	}

	if (status == 'A') {
		g_latitude = convert_to_decimal(lat, lat_hemi);
		g_longitude = convert_to_decimal(lon, lon_hemi);
		g_new_fix = 1;

		snprintf(smsMessage, sizeof(smsMessage), "%.8f, %.8f", g_latitude,
				g_longitude);

		printf("GPRMC valid: lat=%.6f, lon=%.6f\n", g_latitude, g_longitude);
		printf("SMS Message prepared: %s\n", smsMessage);

		// Start SIM800 task and delete GPS task
		static bool sim_task_started = false;
		if (!sim_task_started) {
			sim_task_started = true;
			xTaskCreate(sim800_task, "SIM800", 4096, NULL, 5, NULL);
			gpio_set_level(GPS_gpio, 0);
			if (gps_task_handle != NULL) {
				vTaskDelete(gps_task_handle);
				gps_task_handle = NULL;
			}
		}
	}
}

/**
 * @brief GPS task to read NMEA sentences from the GPS module and extract coordinates.
 *
 * This FreeRTOS task continuously reads data from the GPS UART, buffers complete lines,
 * and checks for valid $GPRMC sentences. If a valid fix is obtained, it updates global
 * latitude and longitude, prepares the SMS message, and starts the SIM800 task.
 * If no fix is found within GPS_TIMEOUT_SEC, the ESP8266 enters deep sleep for GPS_RETRY_SLEEP_SEC.
 *
 * @param arg Task argument (unused).
 */

void gps_task(void *arg) {
	gps_task_handle = xTaskGetCurrentTaskHandle();
	static char line_buf[BUF_SIZE];
	static int line_pos = 0;
	uint8_t data[128];

	uint32_t start_time = xTaskGetTickCount(); // milliseconds

	gpio_set_level(GPS_gpio, 1);

	while (1) {
		int len = uart_read_bytes(UART_GPS_RX, data, sizeof(data),
				100 / portTICK_PERIOD_MS);
		for (int i = 0; i < len; i++) {
			char c = data[i];

			if (c == '\n') {
				if (line_pos > 0) {
					line_buf[line_pos] = '\0';

					printf("GPS line: %s\n", line_buf);

					if (strstr(line_buf, "$GPRMC")) {
						parse_GPRMC(line_buf);
					}
					line_pos = 0;
				}
			} else if (c != '\r' && line_pos < BUF_SIZE - 1) {
				line_buf[line_pos++] = c;
			}
		}

		if (g_new_fix) {
			start_time = xTaskGetTickCount();
		}

		uint32_t elapsed_sec = (xTaskGetTickCount() - start_time)
				/ configTICK_RATE_HZ;
		if (!g_new_fix && elapsed_sec >= GPS_TIMEOUT_SEC) {
			printf("No GPS fix after %d sec, deep sleeping for %d sec...\n",
			GPS_TIMEOUT_SEC, GPS_RETRY_SLEEP_SEC);
			gpio_set_level(GPS_gpio, 0);

			// ESP8266 deep sleep
			esp_deep_sleep(GPS_RETRY_SLEEP_SEC * 1000000ULL);
		}

		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}
