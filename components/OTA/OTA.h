#ifndef OTA_H_
#define OTA_H_

#include "esp_ota_ops.h"

#define OTA_URL "http://192.168.1.106:8000/hello-world1.bin"


void ota_update(void);

#endif /* COMPONENTS_DEBUGGING_MY_PRINT_H_ */
