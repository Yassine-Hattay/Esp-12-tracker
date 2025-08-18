#include "OTA.h"
#include "../WiFi/WiFi.h"

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
	const esp_partition_t *update_partition = esp_ota_get_next_update_partition(running);

	if (!update_partition) {
		printf("[OTA] No valid OTA partition found!\n");
		esp_http_client_cleanup(client);
		return;
	}

	printf("[OTA] Using partition: label=%s, offset=0x%X, size=0x%X\n",
			update_partition->label, update_partition->address, update_partition->size);

	esp_ota_handle_t ota_handle;
	esp_err_t ret = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
	if (ret != ESP_OK) {
		printf("[OTA] esp_ota_begin failed: 0x%X\n", ret);
		esp_http_client_cleanup(client);
		return;
	}

	int total_bytes = 0;
	int data_read;
	static char buffer[1024];

	while (1) {
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
			if (total_bytes >= content_len) break;
			vTaskDelay(10 / portTICK_PERIOD_MS);// small delay before retry
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
