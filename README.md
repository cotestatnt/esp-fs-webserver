If you like this work, please consider [sponsoring this project!](https://paypal.me/cotesta)


# esp-fs-webserver
From **FSBrowser.ino** example to **esp-fs-webserver** Arduino library

When you need a webserver running on ESP32 or ESP8266 device, [FSBrowser.ino - ESP8266](https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266WebServer/examples/FSBrowser) or [FSBrowser.ino - ESP32](https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer/examples/FSBrowser) are the best examples as start point because you can simply put your webserver HTML/CSS/JavaScript source files in the flash memory of device (for example with filesystem-type dedicated plugins like [Arduino ESP8266 LittleFS Filesystem Uploader](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin) or [Arduino ESP32 filesystem uploader](https://github.com/lorol/arduino-esp32fs-plugin)) or in an external SD.

Unfortunately are a little complex examples for a novice due to a lot of handling functions "under the cover". 
This library makes it easier by incorporating this functions into a specific class and more, it adds some convenience functionality like WiFi / Custom Options manager.

**Note**:
Starting from version 2.0.0 ESP32 core for Aruino introduced the LittlsFS library like ESP8266. The examples in this library is written to work with this for both platform by default. Change according to your needs if you prefer other filesystems.

## ACE web broswer editor
Thanks to the built-in **/edit** page (enabled by default), is possible upload, delete and edit the HTML/CSS/JavaScript source files directly from browser and immediately displaying the changes introduced at runtime without having to recompile the device firmware.

![editor](https://user-images.githubusercontent.com/27758688/122570105-b6a01080-d04b-11eb-832c-f60c0a886efd.png)



## WiFi manager
I've added also another built-in page **/setup**, also enabled by default, with which it is possible to set the WiFi credentials and other freely configurable parameters.

![image](https://user-images.githubusercontent.com/27758688/218732999-e22fe092-cbc9-40d6-a34b-38282fbd60e2.png)

This web page can be injected also with custom HTML and Javascript code in order to create very smart and powerful web application.

In the image below, for example, the HTML and Javascript code to provision the devices in the well-known [ThingsBoard IoT platform](https://thingsboard.io/) has been added at runtime starting from the Arduino sketch (check example customHTML.ino).

![image](https://user-images.githubusercontent.com/27758688/218733394-9cd7af3e-257e-4798-98b0-b8d426e07848.png)



