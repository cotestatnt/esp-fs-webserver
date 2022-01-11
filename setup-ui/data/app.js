
const svgLock =  '<svg height="16pt" viewBox="0 0 512 512"><path d="m336 512h-288c-26.453125 0-48-21.523438-48-48v-224c0-26.476562 21.546875-48 48-48h288c26.453125 0 48 21.523438 48 48v224c0 26.476562-21.546875 48-48 48zm-288-288c-8.8125 0-16 7.167969-16 16v224c0 8.832031 7.1875 16 16 16h288c8.8125 0 16-7.167969 16-16v-224c0-8.832031-7.1875-16-16-16zm0 0"/><path d="m304 224c-8.832031 0-16-7.167969-16-16v-80c0-52.929688-43.070312-96-96-96s-96 43.070312-96 96v80c0 8.832031-7.167969 16-16 16s-16-7.167969-16-16v-80c0-70.59375 57.40625-128 128-128s128 57.40625 128 128v80c0 8.832031-7.167969 16-16 16zm0 0"/></svg>'
const svgUnlock = '<svg height="16pt" viewBox="0 0 512 512"><path d="m336 512h-288c-26.453125 0-48-21.523438-48-48v-224c0-26.476562 21.546875-48 48-48h288c26.453125 0 48 21.523438 48 48v224c0 26.476562-21.546875 48-48 48zm-288-288c-8.8125 0-16 7.167969-16 16v224c0 8.832031 7.1875 16 16 16h288c8.8125 0 16-7.167969 16-16v-224c0-8.832031-7.1875-16-16-16zm0 0"/><path d="m80 224c-8.832031 0-16-7.167969-16-16v-80c0-70.59375 57.40625-128 128-128s128 57.40625 128 128c0 8.832031-7.167969 16-16 16s-16-7.167969-16-16c0-52.929688-43.070312-96-96-96s-96 43.070312-96 96v80c0 8.832031-7.167969 16-16 16zm0 0"/></svg>'

var options = {};
var connected = false;


// JQuery-like selector
var $ = function(el) {
	return document.getElementById(el);
};


/**
* Read some data from database
*/
function getWiFiList() {
 var url = new URL("http://" + `${window.location.hostname}` + "/scan");
  fetch(url)

  .then(response => response.json())
  .then(data => {
    console.log(data);
    listWifiNetworks(data);
  });
}


function selectWifi(row) {
  // console.log(row.target.parentNode.id);
  console.log(this.cells[1].innerHTML);
  $('select-' + row.target.parentNode.id).checked = true;
  $('ssid').value = this.cells[1].innerHTML;
  $('connect1').innerHTML = 'Connect to ' + this.cells[1].innerHTML;
  $('connect2').innerHTML = 'Connect to ' + this.cells[1].innerHTML;
  $('password').focus();
  
}


function listWifiNetworks(elems) {

  const list = document.querySelector('#wifi-list');
  list.innerHTML = "";
  var counter = 0;

	elems.forEach(elem => {
   
    // Create a single row with all columns
    var row = document.createElement('tr');
    var id = 'wifi-' + counter++;
    row.id = id;
    row.addEventListener('click', selectWifi); 
	  row.innerHTML  = `<td><input type="radio" name="select" id="select-${id}"></td>`;
    row.innerHTML += `<td id="ssid-${id}">${elem.ssid}</td>`;
    row.innerHTML += '<td>' + elem.strength + '</td>';
    if (elem.security)
      row.innerHTML += '<td>' + svgLock + '</td>';
    else
      row.innerHTML += '<td>' + svgUnlock + '</td>';
  
    // Add row to list
    list.appendChild(row);
    
  });
  
  $("wifi-box").classList.remove("hidden");
}


function getParameters() {
  var url = new URL("http://" + `${window.location.hostname}` + "/config.json");
  fetch(url)

  .then(response => response.json())
  .then(data => {
    options = data;
    listParameters(options);
  });
  
}

function listParameters( params) {
  var html, div;
  $('parameter-list').innerHTML = '';
  for (var key in params) {
    console.log(key, params[key]);
    
    if (typeof(params[key]) === "boolean"){
    
      html =  `<div class="field-body"><label class="label">${key}</label></div>`;
      html += `<div class="field-body"><div class="field checkbox" id="${key}"><input type="checkbox" checked=${params[key]}></div></div>`;
      div = document.createElement('div');
      div.className = 'field is-horizontal';
      div.innerHTML = html;
      $('parameter-list').appendChild(div);
    }
    else {
      var type = 'class="input" type="number"';
      var value = ' value=' + params[key];
      
      if (typeof(params[key]) === "string") {
        type = 'class="input" type="text"';
        value = ' value=' + params[key];
      }
      
      html = '<div class="field-body"><div class="field has-addons">';
      html += `<p class="control"><a class="button is-static">${key}</a></p>`;
      html += `<p class="control is-expanded" id="${key}"><input ${type} ${value}></p></div></div></div>`;
      
      div = document.createElement('div');
      div.className = 'field is-horizontal';
      div.innerHTML = html;
      $('parameter-list').appendChild(div);
    }
  }
  
  // Add event listener to option input box to get update options var
  document.querySelectorAll('input').forEach(item => {
  
    item.addEventListener('input', event => {
      if(item.type === "string") 
        options[item.parentElement.id] = item.value;
      
      if(item.type === "number") 
        options[item.parentElement.id] = parseInt(item.value);
      
      if(item.type === "checkbox") 
        options[item.parentElement.id] = item.checked;
    });
  });
 
  $("params-box").classList.remove("hidden");
  $("button-param").classList.remove("hidden");
}


function closeModalMessage() {
  $('modal-message').classList.remove('is-active');
}

function saveParameters() {
  console.log(options);
  
  var myblob = new Blob([JSON.stringify(options, null, 2)], {
    type: 'application/json'
  });
  var formData = new FormData();
  formData.append("data", myblob, '/config.json');

  // POST data using the Fetch API
  fetch('/edit', {
    method: 'POST',
    headers: {
                'Access-Control-Allow-Origin': '*',
                'Access-Control-Max-Age': '600',
                'Access-Control-Allow-Methods': 'PUT,POST,GET,OPTIONS',
                'Access-Control-Allow-Headers': '*'
             },
    body: formData
  })
  
  // Handle the server response
  .then(response => response.text())
  .then(text => {
    console.log(text);
    $('message-body').innerHTML = '<br><b>config.json</b> file saved successfully on flash memory!<br><br>';
    $('modal-message').classList.add('is-active');
    if(connected === false)
      $('connect-wifi2').classList.remove("hidden");
  });
}


function doConnection() {
  
  var httpCode;
  var formData = new FormData();
  formData.append('ssid',  $('ssid').value );
  formData.append('password',  $('password').value);
  formData.append('persistent', $('persistent').checked);
  
  var params = `ssid=${$('ssid').value}&password=${$('password').value}&persistent=${$('persistent').checked}`
  console.log(params);
  
  fetch('/connect', {
    method: 'POST',
    redirect: 'follow',
    headers: {
                "Content-Type": "application/x-www-form-urlencoded"
             },
    body: params
  })
  
  .then(function(response){
    httpCode = response.status;
    return response.text();
  })
  .then(function(text) {
    console.log(text);
    if (httpCode === 302) {
      connected = true;
      $('message-body').innerHTML = '<br>ESP connected to <b>' + $('ssid').value + '</b><br><br>';
      $('modal-message').classList.add('is-active');
    }
    
    if (httpCode === 500) {
      $('message-body').innerHTML = '<br>Error on connection: <b>' +  text + '</b><br><br>';
      $('modal-message').classList.add('is-active');
    }
  });
  
  
  
  // .then(response => {
  //   if( response.status === 302) {
  //     connected = true;
  //     $('message-body').innerHTML = '<br>ESP connected to <b>' + $('ssid').value + '</b><br><br>';
  //     $('modal-message').classList.add('is-active');
  //   }
    
  //   if( response.status === 500) {
  //     $('message-body').innerHTML = '<br>Error on connection: <b>' +  response + '</b><br><br>';
  //     $('modal-message').classList.add('is-active');
  //   }
    
  // })
  // .then(text => {
  //   if( connected === false) {
  //     $('message-body').innerHTML = '<br>Error on connection: <b>' +  text; + '</b><br><br>';
  //     $('modal-message').classList.add('is-active');
  //   }
  // });
}

/**
 * Initializes the app.
 */
 
 
window.addEventListener('load', getParameters);

$('scan-wifi').addEventListener('click', getWiFiList);

$('connect-wifi').addEventListener('click', doConnection);
$('connect-wifi2').addEventListener('click', doConnection);

$('save-params').addEventListener('click', saveParameters);

// Enable wifi-connect button only if password inserted
$('connect-wifi').disabled = true;
$('password').addEventListener('input', (event) => {
  if( $('password').value.length  === 0 ) {
    $('connect-wifi').disabled = true;
  }
  else {
    $('connect-wifi').disabled = false;
  }
});



