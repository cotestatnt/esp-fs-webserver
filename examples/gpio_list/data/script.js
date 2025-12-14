const svgLightOn = '<svg style="width:24px;height:24px" viewBox="0 0 24 24"><path fill="currentColor" d="M20,11H23V13H20V11M1,11H4V13H1V11M13,1V4H11V1H13M4.92,3.5L7.05,5.64L5.63,7.05L3.5,4.93L4.92,3.5M16.95,5.63L19.07,3.5L20.5,4.93L18.37,7.05L16.95,5.63M12,6A6,6 0 0,1 18,12C18,14.22 16.79,16.16 15,17.2V19A1,1 0 0,1 14,20H10A1,1 0 0,1 9,19V17.2C7.21,16.16 6,14.22 6,12A6,6 0 0,1 12,6M14,21V22A1,1 0 0,1 13,23H11A1,1 0 0,1 10,22V21H14M11,18H13V15.87C14.73,15.43 16,13.86 16,12A4,4 0 0,0 12,8A4,4 0 0,0 8,12C8,13.86 9.27,15.43 11,15.87V18Z" /></svg>' ;
const svgLightOff = '<svg style="width:24px;height:24px" viewBox="0 0 24 24"><path fill="currentColor" d="M12,2A7,7 0 0,0 5,9C5,11.38 6.19,13.47 8,14.74V17A1,1 0 0,0 9,18H15A1,1 0 0,0 16,17V14.74C17.81,13.47 19,11.38 19,9A7,7 0 0,0 12,2M9,21A1,1 0 0,0 10,22H14A1,1 0 0,0 15,21V20H9V21Z" /></svg>';

/**
* Custom selector "JQuery style", but in plain "Vanilla JS"
*/
var $ = function(el) {
	return document.getElementById(el);
};

/**
* Start a websocket client and set event callback functions
*/
function ws_connect() {
  var ws = new WebSocket('ws://'+document.location.host+'/ws',['arduino']);
  ws.onopen = function() {
    ws.send('Connected - ' + new Date());
    getGpioList();
  };
  ws.onmessage = function(e) {
    parseMessage(e.data);
  };
  ws.onclose = function(e) {
    setTimeout(function() {
    ws_connect();
    }, 1000);
  };
  ws.onerror = function(err) {
    ws.close();
  };
  return ws;
}

/**
* Send data "cmds" to ESP
*/
function sendCommand(cmd, pin, level) {
  var data = {
    cmd: cmd,
    pin: parseInt(pin),
    level: level
  };
  console.log(data);
  connection.send(JSON.stringify(data));
}

/**
* Parse messages receveid via websocket
*/
function parseMessage(msg) {
  const obj = JSON.parse(msg);
  if (typeof obj === 'object' && obj !== null) {
    if (obj.esptime !== null) {
      var date = new Date(0); // The 0 sets the date to epoch
      if( date.setUTCSeconds(obj.esptime))
        document.getElementById("esp-time").innerHTML = date;
    }
    updateGpiosList(obj);
  }
}

/**
* Read GPIO list status
*/
function getGpioList() {
  fetch('/getGpioList')                 // Do the request
  .then(response => response.json())    // Parse the response
  .then(obj => {                        // DO something with response
    updateGpiosList(obj);
  });
}


/**
* Iterate to the gpio list passed as parameter anc create DOMs dinamically
*/
function updateGpiosList(elems) {

  // Get reference to gpio-list element and clear content
  const list = document.querySelector('#gpio-list');
  list.innerHTML = "";

  // Draw all input rows
  const inputs = Object.entries(elems).filter((item) => item[1].type === 'input');

  inputs.forEach(el => {
    const obj = el[1];
    var lbl = obj.level ? `${svgLightOn} HIGH` : `${svgLightOff}  LOW`;
    // Create a single row with all columns
    var row = document.createElement('tr');
	  row.innerHTML  = '<td>' + obj.label + '</td>';
    row.innerHTML += '<td>' + obj.pin + '</td>';
    row.innerHTML += '<td style="text-align:center;">' + obj.type + '</td>';
    row.innerHTML += `<td style="text-align:center;"><label id="pin-${obj}" for="">${lbl}</label></td>` ;
    // Append this row to list
    list.appendChild(row);
  });

  // Draw all output rows
  const outputs = Object.entries(elems).filter((item) => item[1].type === 'output');

  outputs.forEach(el => {
    const obj = el[1];
    var lbl = obj.level ? ` checked>Turn OFF` : `>Turn ON`;
    // Create a single row with all columns
    var row = document.createElement('tr');
	  row.innerHTML  = '<td>' + obj.label + '</td>';
    row.innerHTML += '<td>' + obj.pin + '</td>';
    row.innerHTML += '<td style="text-align:center;">' + obj.type + '</td>';
    row.innerHTML += `<td><label id="lbl-${obj.pin}" for=""><input type="checkbox" id="pin-${obj.pin}" role="switch" data-pin=${obj.pin}${lbl}</label></td>`;
    // Append this row to list
    list.appendChild(row);
  });

  // Add event listener to each out switch
  const outSwitches = document.querySelectorAll('input[type="checkbox"]');
  outSwitches.forEach( (outSw) => {
    outSw.addEventListener('change', function() {
      sendCommand('writeOut', this.dataset.pin, this.checked);
    });
  });
}

var connection = ws_connect();
window.addEventListener('load', getGpioList);
