<h1 align="center">ESP-12 Tracker</h1>

# 1 - Project description:
This is a GPS tracker using UBLOX NEO-6M and SIM800L controlled by ESP-12.  
The user receives GPS coordinates via SMS from the tracker on a custom-made Android app that supports map navigation, routing, marker placement, etc.

# 2 - Development environment 
I used the [ESP8266 FreeRTOS SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html) with Eclipse IDE to develop this project.

# 3 - Wiring 

# 4 - Code 
[ESP-12 code](https://yassine-hattay.github.io/esp_tracker_12/index.html)  

[Android app repo](https://github.com/Yassine-Hattay/Esp-12-tracker-mobile-app)



# 5 - How it works:
Initially, when the ESP-12 wakes up, the user has access to two buttons.  
If they press the OTA update button, the ESP will try to update it's code by downloading new firmware over wifi.
