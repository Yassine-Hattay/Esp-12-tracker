/**
 * @file web.h
 * @author your name (you@domain.com)
 * @brief this file contains the header for web server functionality
 * @version 0.1
 * @date 2025-05-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#ifndef COMPONENTS_WEB_WEB_H_
#define COMPONENTS_WEB_WEB_H_

#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "event_groups.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdio.h>
#include "esp_log.h"
#include "../debugging/my_print.h"

void wifi_init_sta(void);

#define EXAMPLE_ESP_WIFI_SSID "Orange-066C"
#define EXAMPLE_ESP_WIFI_PASS "GMA6ABLMG87"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define HTML_BUF_SIZE 8192

#endif /* COMPONENTS_WEB_WEB_H_ */
