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
note : The user has access to two buttons as long as the esp is not in deep sleep mode pressing the OTA button will trigger an interupt to update it's firmware via wifi from a server that uses a python script host_bin.py (code in this repo) , the esp can be woken up from deep sleep using the rest button .

A possible sequence of excution of the tracker goes like this , when the esp first wakes up it provides the gps module (UBLOX NEO-6M) with power letting current flow from the collector to the emitter of a 2n2222 transistor by providing current through the base of the NPN transistor (by setting a gpio to a logic 1 at the base of 2n2222) , if the gps module can't find coordinates in "t" amount of time , the esp cuts power to it by setting the base of the 2n2222 transistor to a logic 0 then goes to deep sleep and retry again later , otherwise if the coordinates are found the esp then shuts power to the gps module and powers the sim800 with the help of a mosfet (for more details on how the power management is handled check out the ltspice simulation named tracker.asc that can be found in the "ltspice sim" folder) .

