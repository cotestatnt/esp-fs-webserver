## esp-fs-webserver
This example is the most advanced since now. 

With the help of WebSocket technology, we can send message from server-to-clients or from client-to-server in a **full-duplex communication channels over a single TCP connection.**

We use the **WebSocket client** to populate asynchronously an _"area chart"_ with real time datas from ESP regarding the **total size of heap memory** and size of **max contiguous block of memory**.

Off course, on the ESP MCU we will run also a WebSocket server together to the web server.
I've used this library [arduinoWebSockets](https://github.com/Links2004/arduinoWebSockets), so it is needed to compile, but you can choose which you prefer.

>N.B. 
This example will run only if an internet connection is available because [Highcharts](https://www.highcharts.com/) Javascript resources is necessary 
(to run offline in AP mode, you can download all the used js files, put it in the flash memory (better if gzipped to speed-up page loading) and edit the [index.htm](https://github.com/cotestatnt/esp-fs-webserver/blob/main/examples/highcharts/data/index.htm))

In this example, web page CSS and JavaScript source file is stored in a separated file inside the folder **/app**

![image](https://user-images.githubusercontent.com/27758688/123048782-135e4b00-d3ff-11eb-84d5-45e2f164e0f7.png)
