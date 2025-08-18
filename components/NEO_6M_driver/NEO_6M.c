#include "NEO_6M.h"

// Global variables to store coordinates
volatile double g_latitude = 0.0;
volatile double g_longitude = 0.0;
volatile int g_new_fix = 0;  // flag set to 1 when new fix is available

static double convert_to_decimal(const char *nmea_coord, const char hemi) {
    if (strlen(nmea_coord) < 4) return 0.0;
    double val = atof(nmea_coord);
    int deg = (int)(val / 100);
    double min = val - (deg * 100);
    double decimal = deg + min / 60.0;
    if (hemi == 'S' || hemi == 'W') decimal *= -1;
    return decimal;
}

static void parse_GPRMC(char *line) {
    char *pos = strchr(line, '\r');
    if (pos) *pos = '\0';
    pos = strchr(line, '\n');
    if (pos) *pos = '\0';

    char *token;
    char *rest = line;

    char lat[16] = {0}, lon[16] = {0};
    char lat_hemi = 'N', lon_hemi = 'E';
    char status = 'V';

    token = strtok_r(rest, ",", &rest);
    if (!token || strcmp(token, "$GPRMC") != 0) return;

    int field = 1;
    while ((token = strtok_r(rest, ",", &rest))) {
        field++;
        switch (field) {
            case 3: status = token[0]; break;
            case 4: strncpy(lat, token, sizeof(lat)-1); break;
            case 5: lat_hemi = token[0]; break;
            case 6: strncpy(lon, token, sizeof(lon)-1); break;
            case 7: lon_hemi = token[0]; break;
        }
    }

    if (status == 'A') {
        g_latitude = convert_to_decimal(lat, lat_hemi);
        g_longitude = convert_to_decimal(lon, lon_hemi);
        g_new_fix = 1;  // signal main task
    }
}

void gps_task(void *arg) {
    static char line_buf[BUF_SIZE];
    static int line_pos = 0;
    uint8_t data[128];

    while (1) {
        int len = uart_read_bytes(UART_GPS_RX, data, sizeof(data), 100 / portTICK_PERIOD_MS);
        for (int i = 0; i < len; i++) {
            char c = data[i];
            if (c == '\n') {
                if (line_pos > 0) {
                    line_buf[line_pos] = '\0';

                    if (strstr(line_buf, "$GPRMC")) {
                        parse_GPRMC(line_buf);
                    }
                    line_pos = 0;
                }
            } else if (c != '\r' && line_pos < BUF_SIZE - 1) {
                line_buf[line_pos++] = c;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}
