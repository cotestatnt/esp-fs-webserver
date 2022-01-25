## esp-fs-webserver
In this example a simple web page with a bounche of DOMs is hosted in the ESP filesystem.

A CSS styled checkbox can TOGGLE the built-in LED of more common ESP boards thanks to an AJAX (**A**synchronous **J**avaScript **A**nd **X**ML) call to page **http//yourhostname.local/led** (without params).

_Is possible also call the page passing the state wanted for the LED for example in this mamnner **http//yourhostname.local/led?val=1**_

The _led-state_ paragraph is updated asynchronously with the response of esp webserver.

![image](https://user-images.githubusercontent.com/27758688/151001855-ef9093e9-5402-41ad-942d-1d7bbdd7133e.png)
