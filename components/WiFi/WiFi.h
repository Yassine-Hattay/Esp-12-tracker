#ifndef WIFI_H_
#define WIFI_H_

#include "esp_wifi.h"
#include "esp_http_client.h"
#include "esp_system.h"
#include <string.h>

#define ICACHE_FLASH_ATTR __attribute__((section(".irom0.text")))

#define WIFI_SSID "Orange-066C"
#define WIFI_PASS "GMA6ABLMG87"

void wifi_init_sta(void);


#endif /* COMPONENTS_DEBUGGING_MY_PRINT_H_ */
