const char thingsboard_htm[] PROGMEM = R"EOF(
<div>
  <br>If you don't have a valid <b>device token</b> press button "Device Provisioning" to start procedure in order to get a new token from ThingsBoard server.<br>
  <br>To perform <a href="https://thingsboard.io/docs/user-guide/device-provisioning/">device provisioning</a>, this functionality must be enabled in the ThingsBoard profile of your devices.
  <div class='btn-bar'>
    <a id=device-provisioning class='btn'>
        <div class=svg>
          <svg class='icon' viewBox='0 0 24 24'>
            <path fill='currentColor' d='M21.41 11.58L12.41 2.58C12.04 2.21 11.53 2 11 2H4C2.9 2 2 2.9 2 4V11C2 11.53 2.21 12.04 2.59 12.41L3 12.81C3.9 12.27 4.94 12 6 12C9.31 12 12 14.69 12 18C12 19.06 11.72 20.09 11.18 21L11.58 21.4C11.95 21.78 12.47 22 13 22S14.04 21.79 14.41 21.41L21.41 14.41C21.79 14.04 22 13.53 22 13S21.79 11.96 21.41 11.58M5.5 7C4.67 7 4 6.33 4 5.5S4.67 4 5.5 4 7 4.67 7 5.5 6.33 7 5.5 7M8.63 14.27L4.76 18.17L3.41 16.8L2 18.22L4.75 21L10.03 15.68L8.63 14.27' />
          </svg>
        </div>
        <span> Device provisioning</span>
    </a>
  </div>
</div>
)EOF";


const char thingsboard_script[] PROGMEM = R"EOF(
const TB_DEVICE_NAME = 'Device Name';
const TB_DEVICE_LAT = 'Device Latitude';
const TB_DEVICE_LON = 'Device Longitude';
const TB_SERVER = 'ThingsBoard server address';
const TB_PORT = 'ThingsBoard server port';
const TB_DEVICE_TOKEN = 'ThingsBoard device token';
const TB_DEVICE_KEY = 'Provisioning device key';
const TB_SECRET_KEY = 'Provisioning secret key';

function getUrl(host, port) {
  var url;
  if (port === '80')
    url = 'http://' + host + '/api/v1/';
  else if (port === '443')
    url = 'https://' + host + '/api/v1/'
  else
    url = 'http://' + host + ':'+ port + '/api/v1/'
  return url;
}

function deviceProvisioning() {
  console.log('Device provisioning');
  var server = $(TB_SERVER).value;
  var port = $(TB_PORT).value;
  var token = $(TB_DEVICE_TOKEN).value;
  if (token === '')
    token = 'xxxxx';
  const url = 'https://corsproxy.io/?' + encodeURIComponent(getUrl(server, port) + token + '/attributes');
  fetch(url, {
    method: "GET"
  })
  .then((response) => {
    if (response.ok) {
      openModalMessage('Device provisioning', 'Device already provisioned. Do you want update device client attributes?', setDeviceClientAttribute);
      return;
    }
    throw new Error('Device token not present. Provisioning new device');
  })
  .catch((error) => {
    openModalMessage('New device', 'A new device will be provisioned on ThingsBoard server', createNewDevice);
  });
}

function createNewDevice() {
  var server = $(TB_SERVER).value;
  var port = $(TB_PORT).value;
  var key = $(TB_DEVICE_KEY).value;
  var secret = $(TB_SECRET_KEY).value;
  var name = $(TB_DEVICE_NAME).value;
  const url = 'https://corsproxy.io/?' + encodeURIComponent(getUrl(server, port) + 'provision');
  var payload = {
    'deviceName': name,
    'provisionDeviceKey': key,
    'provisionDeviceSecret': secret
  };
  fetch(url, {
    headers: {
      'Accept': 'application/json',
      'Content-Type': 'application/json'
    },
    method: 'POST',
    body: JSON.stringify(payload)
  })
  .then(response => response.json())
  .then(obj => {
    var token = $(TB_DEVICE_TOKEN);
    token.focus();
    token.value = obj.credentialsValue;
    openModalMessage('Write device attributes', 'Device provisioned correctly.<br>Do you want set client attirbutes on ThingsBoard server?', setDeviceClientAttribute);
  });
}


function setDeviceClientAttribute(){
  var server = $(TB_SERVER).value;
  var port = $(TB_PORT).value;
  var token = $(TB_DEVICE_TOKEN).value;
  var name = $(TB_DEVICE_NAME).value;
  var latitude = $(TB_DEVICE_LAT).value;
  var longitude = $(TB_DEVICE_LON).value;
  const url = 'https://corsproxy.io/?' + encodeURIComponent(getUrl(server, port) + token + '/attributes');
  var payload = {
    'DeviceName': name,
    'latitude': latitude,
    'longitude': longitude,
  };
  fetch(url, {
    method: 'POST',
    body: JSON.stringify(payload),
  })
  .then(response => response.text())
  .then(() => {
    openModalMessage('Device properties', 'Device client properties updated!');
  });
}
$('device-provisioning').addEventListener('click', deviceProvisioning);
)EOF";