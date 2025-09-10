<h1 align="center">ESP-12 Tracker</h1>

# 1 - Project Description
This is a GPS tracker using the **UBLOX NEO-6M** and **SIM800L**, controlled by an **ESP-12**.  
The user receives GPS coordinates via SMS from the tracker through a custom-made Android app that supports map navigation, routing, marker placement, and more.  

# 2 - Development Environment
I used the [ESP8266 FreeRTOS SDK](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html) with **Eclipse IDE** to develop this project.  

# 3 - Wiring
<img width="1448" height="1285" alt="image" src="https://github.com/user-attachments/assets/9c817e38-7827-449e-87d5-ab1f748e6cd6" />

# 4 - Code

[ESP-12 code](https://yassine-hattay.github.io/esp_tracker_12/index.html)  

[Android app repo](https://github.com/Yassine-Hattay/Esp-12-tracker-mobile-app)  

# 5 - How It Works

## a - ESP 12
**Note:** The user has access to two buttons as long as the ESP is not in deep sleep. Pressing the **OTA button** triggers an interrupt to update the firmware via Wi-Fi from a server running the `host_bin.py` Python script (included in this repo). The ESP can also be woken up from deep sleep using the **reset button**.  

A typical execution sequence of the tracker is as follows:  
1. When the ESP first wakes up, it powers the GPS module (UBLOX NEO-6M) by allowing current to flow from the collector to the emitter of a **2N2222 NPN transistor**, which is enabled by setting a GPIO to logic HIGH at the transistor's base.  
2. If the GPS module cannot acquire coordinates within a set time (`t`), the ESP cuts power to the module by setting the transistor’s base to logic LOW and enters **deep sleep**, retrying later.  
3. If coordinates are successfully obtained, the ESP shuts off power to the GPS module and powers the SIM800L using a **MOSFET**.  
4. The SIM800L then attempts to send an SMS containing the GPS coordinates and the current battery voltage.  
5. In case of a transmission error, the module retries sending the SMS a predefined number of times. Before entering deep sleep, the ESP resets the SIM800L to ensure proper operation on the next wake-up.  

For more details on the power management sequence, you can explore the **LTspice simulation** `tracker.asc` found in the **"ltspice sim"** folder.  

<img width="1359" height="854" alt="image" src="https://github.com/user-attachments/assets/6b420ab4-b320-4bd4-a721-e0b7fd75fad8" />

<div align="center">
Screenshot of the LTspice schematic
</div>

## b - android app
**Note:** This app that does not require internet connection to work only a sim signal for sms receiving and gps location for navigation .   
<div align="center">
<img width="155" height="156" alt="Capture d'écran 2025-09-10 123008_upscayl_3x_ultrasharp" src="https://github.com/user-attachments/assets/f5373d89-f3a2-4ab4-b191-2ab1548f544d" />  
</div>

After receiving the coordinates the user can click the bottom left filter button to select a date range the app then parses received sms messages to display the coordinates on the map .


<img src="https://github.com/user-attachments/assets/98da5526-0191-4b32-9724-2a5efbcebd61" width="250" />
<img src="https://github.com/user-attachments/assets/2bcf6832-cb78-4596-bfc0-2a1bca1ee96e" width="250" />
<img src="https://github.com/user-attachments/assets/3d23a379-592b-4f0c-856e-198934694b14" width="250" />


The app also supports marker placement , clicking the bottom button with the flag icon causes a red X to appear clicking the button again , places a marker at that X .


<img src="https://github.com/user-attachments/assets/214d2c62-e82f-4d14-af86-cbf6dfa9d86c" width="250" />
<img src="https://github.com/user-attachments/assets/23aa512c-60d8-4f45-8081-07b579c3b74c" width="250" />


Clicking the top right button will zoom in on your location represented as a red triangle updating it in real time .

<img src="https://github.com/user-attachments/assets/5f4b6cfd-7610-49a0-97b6-6c906a30c078" width="250" />


You can choose wich location to create a route between by clicking marker icon at the bottom , here in the example photo i selected my current location and the marker i placed earlier.
then clicking the arrow icon positioned bottom right will calcualte and show a route represented as a blue line between the selected locations .

<img src="https://github.com/user-attachments/assets/b95ff2dd-a3c7-428b-99a1-b019df4d7336" width="250" />
<img src="https://github.com/user-attachments/assets/16e690ff-13cb-4b5c-b86c-345d0b4c7076" width="250" />

**Note:** the routing is fully offline and is supported by hosting [grahopper](https://github.com/graphhopper/graphhopper) on termux 
