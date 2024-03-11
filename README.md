If you like this work, please consider [sponsoring this project!](https://paypal.me/cotesta)


# esp-fs-webserver
From **FSBrowser.ino** example to **esp-fs-webserver** Arduino library

When you need a webserver running on ESP32 or ESP8266 device, [FSBrowser.ino - ESP8266](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/examples/FSBrowser) or [FSBrowser.ino - ESP32](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/examples/FSBrowser) are the best examples as start point because you can simply put your webserver HTML/CSS/JavaScript source files in the flash memory of device (for example with filesystem-type dedicated plugins like [Arduino ESP8266 LittleFS Filesystem Uploader](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin) or [Arduino ESP32 filesystem uploader](https://github.com/lorol/arduino-esp32fs-plugin)) or in an external SD.

Unfortunately are a little complex examples for a novice due to a lot of handling functions "under the cover". 
This library makes it easier by incorporating this functions into a specific class and more, it adds some convenience functionality like WiFi / Custom Options manager.

**Note**:
Starting from version 2.0.0 ESP32 core for Aruino introduced the LittlsFS library like ESP8266. The examples in this library is written to work with this for both platform by default. Change according to your needs if you prefer other filesystems.

## WiFi, OTA firmware update and Options manager
Thanks to the built-in page **/setup** (about 8Kb of program space) it is possible to scan and set the WiFi credentials and other freely configurable parameters.

With **/setup** webpage it is also possible to perform remote firmware update (OTA-update). 

![image](https://github.com/cotestatnt/async-esp-fs-webserver/assets/27758688/81a1f6db-a4bd-4f1d-b263-7bebe79cae7d)


This web page can be injected also with custom HTML and Javascript code in order to create very smart and powerful web application.

In the image below, for example, the HTML and Javascript code to provision the devices in the well-known [ThingsBoard IoT platform](https://thingsboard.io/) has been added at runtime starting from the Arduino sketch (check example [customHTML.ino](https://github.com/cotestatnt/async-esp-fs-webserver/tree/main/examples/customHTML)).

![image](https://github.com/cotestatnt/async-esp-fs-webserver/assets/27758688/d728c315-7271-454d-8c34-fb9db0b7a333)

## Web server file upload

In addition to built-in firmware update functionality, you can also upload your web server content all at once (typically the files are placed inside the folder `data` of your sketch).

![image](https://github.com/cotestatnt/async-esp-fs-webserver/assets/27758688/7c261216-3acd-4463-9105-d11e0be3a59a)



## ACE web file editor/browser
Thanks to the built-in **/edit** page, it is possible to upload, delete and edit the HTML/CSS/JavaScript source files directly from browser and immediately display the changes introduced at runtime without having to recompile the device firmware.
The page can be enabled at runtime using the method `enableFsCodeEditor()` and it occupies about 6.7Kb of program space.

![image](https://github.com/cotestatnt/async-esp-fs-webserver/assets/27758688/668c0899-a060-4aed-956b-51311bf3fe13)

