#include "WiFi.h"

/* Minimal Wi-Fi handler */
esp_err_t ICACHE_FLASH_ATTR wifi_event_handler(void *ctx, system_event_t *event) {
	switch (event->event_id) {
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		printf("[WIFI] Got IP: %s\n",
				ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		break;
	default:
		break;
	}
	return ESP_OK;
}

/* Minimal Wi-Fi init */
void wifi_init_sta(void) {
	tcpip_adapter_init();
	esp_event_loop_init(wifi_event_handler, NULL);

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT()
	;
	esp_wifi_init(&cfg);
	esp_wifi_set_mode(WIFI_MODE_STA);

	wifi_config_t wifi_config;
	memset(&wifi_config, 0, sizeof(wifi_config));
	strncpy((char*) wifi_config.sta.ssid, WIFI_SSID,
			sizeof(wifi_config.sta.ssid));
	strncpy((char*) wifi_config.sta.password, WIFI_PASS,
			sizeof(wifi_config.sta.password));

	esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
	esp_wifi_start();

	tcpip_adapter_ip_info_t ip_info;
	printf("[WIFI] Waiting for IP...\n");
	while (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip_info) != ESP_OK
			|| ip_info.ip.addr == 0) {
		vTaskDelay(200 / portTICK_PERIOD_MS);
	}
}
