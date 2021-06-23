## esp-fs-webserver
In this example a simple web page with a bounche of DOMs is hosted in the ESP filesystem.

A button can TOGGLE the built-in LED of more common boards thanks to an AJAX (**A**synchronous **J**avaScript **A**nd **X**ML) call to page **http//yourhostname.local/led** (without params).

_Is possible also call the page passing the state wanted for the LED for example in this mamnner **http//yourhostname.local/led?val=1**_

The _led-state_ div is updated asynchronously with the response of esp webserver.
