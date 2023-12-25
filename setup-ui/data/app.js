const svgLogo = '<svg fill="brown" height=120px viewBox="0 0 24 24"><path d="M5 3C3.9 3 3 3.9 3 5S2.1 7 1 7V9C2.1 9 3 9.9 3 11S3.9 13 5 13H7V11H5V10C5 8.9 4.1 8 3 8C4.1 8 5 7.1 5 6V5H7V3M11 3C12.1 3 13 3.9 13 5S13.9 7 15 7V9C13.9 9 13 9.9 13 11S12.1 13 11 13H9V11H11V10C11 8.9 11.9 8 13 8C11.9 8 11 7.1 11 6V5H9V3H11M22 6V18C22 19.11 21.11 20 20 20H4C2.9 20 2 19.11 2 18V15H4V18H20V6H17.03V4H20C21.11 4 22 4.89 22 6Z" /></svg>'
const svgLock =  '<svg height="16pt" viewBox="0 0 24 24"><path d="M12,17A2,2 0 0,0 14,15C14,13.89 13.1,13 12,13A2,2 0 0,0 10,15A2,2 0 0,0 12,17M18,8A2,2 0 0,1 20,10V20A2,2 0 0,1 18,22H6A2,2 0 0,1 4,20V10C4,8.89 4.9,8 6,8H7V6A5,5 0 0,1 12,1A5,5 0 0,1 17,6V8H18M12,3A3,3 0 0,0 9,6V8H15V6A3,3 0 0,0 12,3Z" /></svg>';
const svgUnlock = '<svgheight="16pt" viewBox="0 0 24 24"><path d="M18 1C15.24 1 13 3.24 13 6V8H4C2.9 8 2 8.89 2 10V20C2 21.11 2.9 22 4 22H16C17.11 22 18 21.11 18 20V10C18 8.9 17.11 8 16 8H15V6C15 4.34 16.34 3 18 3C19.66 3 21 4.34 21 6V8H23V6C23 3.24 20.76 1 18 1M10 13C11.1 13 12 13.89 12 15C12 16.11 11.11 17 10 17C8.9 17 8 16.11 8 15C8 13.9 8.9 13 10 13Z" /></svg>';
const svgScan = '<path d="M12 20L8.4 15.2C9.4 14.45 10.65 14 12 14S14.6 14.45 15.6 15.2L12 20M4.8 10.4L6.6 12.8C8.1 11.67 9.97 11 12 11S15.9 11.67 17.4 12.8L19.2 10.4C17.19 8.89 14.7 8 12 8S6.81 8.89 4.8 10.4M12 2C7.95 2 4.21 3.34 1.2 5.6L3 8C5.5 6.12 8.62 5 12 5S18.5 6.12 21 8L22.8 5.6C19.79 3.34 16.05 2 12 2M7 24H9V22H7V24M15 24H17V22H15V24M11 24H13V22H11V24Z" /></svg>';
const svgConnect = '<path d="M12,6C8.6,6 5.5,7.1 3,9L1.2,6.6C4.2,4.3 8,3 12,3C16,3 19.8,4.3 22.8,6.6L21,9C18.5,7.1 15.4,6 12,6M13,19C13,17.7 13.4,16.4 14.2,15.4C13.5,15.2 12.8,15 12,15C10.7,15 9.4,15.5 8.4,16.2L12,21L13,19.6C13,19.4 13,19.2 13,19M16.8,13.4C17.1,13.3 17.5,13.2 17.9,13.1L19.2,11.4C17.2,9.9 14.7,9 12,9C9.3,9 6.8,9.9 4.8,11.4L6.6,13.8C8.1,12.7 10,12 12,12C13.8,12 15.4,12.5 16.8,13.4M16.5,22.6L17.2,19.8L15,17.9L17.9,17.7L19,15L20.1,17.6L23,17.8L20.8,19.7L21.5,22.5L19,21.1L16.5,22.6Z" /></svg>';
const svgSave = '<path d="M15,9H5V5H15M12,19A3,3 0 0,1 9,16A3,3 0 0,1 12,13A3,3 0 0,1 15,16A3,3 0 0,1 12,19M17,3H5C3.89,3 3,3.9 3,5V19A2,2 0 0,0 5,21H19A2,2 0 0,0 21,19V7L17,3Z" />';
const svgRestart = '<path d="M12,4C14.1,4 16.1,4.8 17.6,6.3C20.7,9.4 20.7,14.5 17.6,17.6C15.8,19.5 13.3,20.2 10.9,19.9L11.4,17.9C13.1,18.1 14.9,17.5 16.2,16.2C18.5,13.9 18.5,10.1 16.2,7.7C15.1,6.6 13.5,6 12,6V10.6L7,5.6L12,0.6V4M6.3,17.6C3.7,15 3.3,11 5.1,7.9L6.6,9.4C5.5,11.6 5.9,14.4 7.8,16.2C8.3,16.7 8.9,17.1 9.6,17.4L9,19.4C8,19 7.1,18.4 6.3,17.6Z" /></svg>';
const svgEye = '<path d="M12 6.5c2.76 0 5 2.24 5 5 0 .51-.1 1-.24 1.46l3.06 3.06c1.39-1.23 2.49-2.77 3.18-4.53C21.27 7.11 17 4 12 4c-1.27 0-2.49.2-3.64.57l2.17 2.17c.47-.14.96-.24 1.47-.24zM2.71 3.16c-.39.39-.39 1.02 0 1.41l1.97 1.97C3.06 7.83 1.77 9.53 1 11.5 2.73 15.89 7 19 12 19c1.52 0 2.97-.3 4.31-.82l2.72 2.72c.39.39 1.02.39 1.41 0 .39-.39.39-1.02 0-1.41L4.13 3.16c-.39-.39-1.03-.39-1.42 0zM12 16.5c-2.76 0-5-2.24-5-5 0-.77.18-1.5.49-2.14l1.57 1.57c-.03.18-.06.37-.06.57 0 1.66 1.34 3 3 3 .2 0 .38-.03.57-.07L14.14 16c-.65.32-1.37.5-2.14.5zm2.97-5.33c-.15-1.4-1.25-2.49-2.64-2.64l2.64 2.64z" />';
const svgNoEye = '<path d="M0 0h24v24H0V0z" fill="none"/><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>';
const svgMenu = '<path d="M3,6H21V8H3V6M3,11H21V13H3V11M3,16H21V18H3V16Z"/>';

var options = {};
var configFile;
var lastBox;

// Simple JQuery-like selector
var $ = function(el) {
	return document.getElementById(el);
};
var newEl = function(el) {
	return document.createElement(el);
};

function showHidePassword() {
  var inp = $("password");
  if (inp.type === "password") {
    inp.type = "text";
    $('show-pass').classList.remove("w--current");
    $('hide-pass').classList.add("w--current");
  }
  else {
    inp.type = "password";
    $('show-pass').classList.add("w--current");
    $('hide-pass').classList.remove("w--current");
  }
}

/**
* Read some data from database
*/
function getWiFiList() {
  $('loader').classList.remove('hide');
 var url = new URL("http://" + `${window.location.hostname}` + "/scan");
  fetch(url)
  .then(response => response.json())
  .then(data => {
    listWifiNetworks(data);
    $('loader').classList.add('hide');
  });
}


function selectWifi(row) {
  try {
    $('select-' + row.target.parentNode.id).checked = true;
  }
  catch(err) {
    $(row.target.id).checked = true;
  }
  $('ssid').value = this.cells[1].innerHTML;
  $('ssid-name').innerHTML = this.cells[1].innerHTML;
  $('password').focus();
}


function listWifiNetworks(elems) {
  const list = document.querySelector('#wifi-list');
  list.innerHTML = "";
	elems.forEach((elem, idx) => {
    // Create a single row with all columns
    var row = newEl('tr');
    var id = 'wifi-' + idx;
    row.id = id;
    row.addEventListener('click', selectWifi);
	  row.innerHTML  = `<td><input type="radio" name="select" id="select-${id}"></td>`;
    row.innerHTML += `<td id="ssid-${id}">${elem.ssid}</td>`;
    row.innerHTML += '<td class="hide-tiny">' + elem.strength + ' dBm</td>';
    if (elem.security)
      row.innerHTML += '<td>' + svgLock + '</td>';
    else
      row.innerHTML += '<td>' + svgUnlock + '</td>';

    // Add row to list
    list.appendChild(row);
  });

  $("wifi-table").classList.remove("hide");
}

function getEspStatus() {
  var url = new URL("http://" + `${window.location.hostname}` + "/wifistatus");
  fetch(url)
  .then(response => response.json())
  .then(data => {
    $('esp-mode').innerHTML = data.mode;
    $('esp-ip').innerHTML = (data.ip & 255) + '.' + (data.ip>>8 & 255) +'.' + (data.ip>>16 & 255) + '.' + (data.ip>>>24);
    $('firmware').innerHTML = data.firmware;
  });
}

async function fetchFromFile(f, m) {
  const response = await fetch(f, { method: m });
  const data = await response.text();
  return data;
}

function getParameters() {
  $('loader').classList.remove('hide');
  var url = new URL("http://" + `${window.location.hostname}` + '/get_config');
  fetch(url)
  .then(res => res.text())
  .then(config => {
    configFile = config;
    url = new URL("http://" + `${window.location.hostname}` + configFile);
    fetch(url)
    .then(response => response.json())
    .then(data => {
      for (const key in data){
        if(data.hasOwnProperty(key)){
          if (key === 'name-logo') {
            $('name-logo').innerHTML = data[key].replace( /(<([^>]+)>)/ig, '');
            delete data[key];
            continue;
          }
          if (key == 'img-logo') {
            fetch(data[key])
              .then((response) => response.text())
              .then((base64) => {
                var size = data[key].replace(/[^\d_]/g, '').split('_');
                var img = newEl('img');
                img.classList.add('logo');
                img.setAttribute('src', 'data:image/png;base64, ' + base64);
                img.setAttribute('style', `width:${size[0]}px;heigth:${size[1]}px`);
                $('img-logo').innerHTML = "";
                $('img-logo').append(img);
                $('img-logo').setAttribute('type', 'number');
                $('img-logo').setAttribute('title', '');
                delete data[key];
              })
            continue;
          }
        }
      }
      options = data;
      listParameters(options);
      $('loader').classList.add('hide');
    })
    .then( () => {
      getEspStatus();
    });
  });
}


function createNewBox(cont, lbl) {
  var box = newEl('div');
  box.setAttribute('id', 'option-box' + cont);
  box.classList.add('ctn', 'opt-box', 'hide');
  var h = newEl('h2');
  h.classList.add('heading-2');
  h.innerHTML = lbl;
  box.appendChild(h);
  $('main-box').appendChild(box);
  // Add new voice in menu and relatvie listener
  var lnk = newEl('a');
  lnk.setAttribute('id', 'set-opt' + cont);
  lnk.setAttribute('data-box', 'option-box' + cont);
  lnk.classList.add('a-link');
  lnk.innerHTML = lbl;
  lnk.addEventListener('click', switchPage);
  $('nav-link').appendChild(lnk);
  return box;
}

async function listParameters (params) {
  var el;
  if(!Object.keys(params)[0].startsWith('param-box')) {
    params = {'param-box1': 'Options', ...params};
    options = params;
  }

  // Iterate through the object
  var i = 0;
  for (const key in params) {
    i++;
    // Create a new box
    if(key.startsWith('param-box')) {
      lastBox = createNewBox(i, params[key]);
      continue;
    }
    // Inject runtime CSS source file          
    else  if(key.startsWith('raw-css')) {
        fetchFromFile(params[key], 'HEAD').then(() => {
        var css = newEl("link");
        css.setAttribute('rel', 'stylesheet');
        css.setAttribute('href', params[key]);
        document.head.appendChild(css);
        delete params[key];
      });
      continue;
    }
    // Inject runtime JS source file
    else if(key.startsWith('raw-javascript')) {
        fetchFromFile(params[key], 'HEAD').then(() => {
        var js = newEl("script");
        js.setAttribute('src', params[key]);
        document.body.appendChild(js);
        delete params[key];
      });
      continue;
    }
    // Inject runtime HTML source file
    else if(key.startsWith('raw-html')) {
      await fetchFromFile(params[key], 'GET').then((res) => {
        el = newEl('div');
        el.setAttribute('id', 'row' + i)
        el.style.width = '100%';
        el.innerHTML = res;
        lastBox.appendChild(el);
      });
      continue;
    }
    // Option variables
    else {
      let lbl = newEl('label');
      el = newEl('input');
      el.setAttribute('id', key);
      el.setAttribute('type', 'text');

      // Set input property (id, type and value). Check first if is boolean
      if (typeof(params[key]) === "boolean"){
        el.setAttribute('type', 'checkbox');
        el.classList.add('t-check', 'opt-input');
        el.checked = params[key];
        lbl.classList.add('input-label', 'toggle');
        let dv = newEl('div');
        dv.classList.add('toggle-switch');
        let sp = newEl('span');
        sp.classList.add('toggle-label');
        sp.textContent = key;
        lbl.appendChild(el);
        lbl.appendChild(dv);
        lbl.appendChild(sp);
        addInputListener(el);
        lastBox.appendChild(lbl);
      }
      else {
        el.value = params[key];
        el.classList.add('opt-input');
        lbl.setAttribute('label-for', key);
        lbl.classList.add('input-label');
        lbl.textContent = key;
        if (typeof(params[key]) === "number")
          el.setAttribute('type', 'number');

        if (typeof(params[key]) === "object" ) {
          // This is a select/option
          if (params[key].values) {
            el = newEl('select');
            el.setAttribute('id', key);
            params[key].values.forEach((a) => {
              var opt = newEl('option');
              opt.textContent = a;
              opt.value = a;
              el.appendChild(opt);
            })
            el.value = params[key].selected;
            lastBox.appendChild(el);
          }

          // This is a float value
          else {
            var num = Math.round(params[key].value  * (1/params[key].step)) / (1/params[key].step);
            el.setAttribute('type', 'number');
            el.setAttribute('step', params[key].step);
            el.setAttribute('min', params[key].min);
            el.setAttribute('max', params[key].max);
            el.value = Number(num).toFixed(3);
          }
        }
        addInputListener(el);
        var d  = newEl('div');
        d.classList.add('tf-wrapper');
        d.appendChild(lbl);
        d.appendChild(el);
        lastBox.appendChild(d);
      }
    }

    if(key.endsWith('-hidden'))  {
      el.classList.add('hide');
    }
  }
}

function addInputListener(item) {
  // Add event listener to option inputs
  if (item.type  === "number") {
    item.addEventListener('change', () => {
       if (item.getAttribute("step")) {
        var obj = {};
        obj.value = Math.round(item.value  * (1/item.step)) / (1/item.step);
        obj.step = item.getAttribute("step");
        obj.min = item.getAttribute("min");
        obj.max = item.getAttribute("max");
        options[item.id] = obj;
      }
      else
        options[item.id] = parseInt(item.value);
    });
    return;
  }

  if(item.type === "text") {
    item.addEventListener('change', () => {
      options[item.id] = item.value;
    });
    return;
  }

  if(item.type === "checkbox") {
    item.addEventListener('change', () => {
      options[item.id] = item.checked;
    });
    return;
  }

  if(item.type === 'select-one'){
    item.addEventListener('change', (e) => {
      options[e.target.id].selected = e.target.value;
    });
    return;
  }
}


function saveParameters() {
  var myblob = new Blob([JSON.stringify(options, null, 2)], {
    type: 'application/json'
  });
  var formData = new FormData();
  formData.append("data", myblob, configFile);

  // POST data using the Fetch API
  fetch('/edit', {
    method: 'POST',
    body: formData
  })

  // Handle the server response
  .then(response => response.text())
  .then(text => {
    openModalMessage('Save options', '<br><b>"' + configFile +'"</b> saved successfully on flash memory!<br><br>');
  });
}


function doConnection() {
  var formdata = new FormData();
  formdata.append("ssid", $('ssid').value);
  formdata.append("password", $('password').value);
  formdata.append("persistent", $('persistent').checked);
  var requestOptions = {
    method: 'POST',
    body: formdata,
    redirect: 'follow'
  };

  $('loader').classList.remove('hide');
  fetch('/connect', requestOptions)
  .then(function(response){
    httpCode = response.status;
    return response.text();
  })
  .then(function(text) {
    if (httpCode === 200) {
      openModalMessage('Connect to WiFi', '<br>' + text + '');
    }
    else {
      openModalMessage('Error!', '<br>Error on connection: <b>' +  text + '</b><br><br>');
    }
    $('loader').classList.add('hide');
  });
}


function switchPage(el) {
  $('top-nav').classList.remove('responsive');

  // Menu items
  document.querySelectorAll("a").forEach(item => {
    item.classList.remove('active');
  });
  el.target.classList.add('active');

  // Box items
  document.querySelectorAll(".opt-box").forEach(e => {
    e.classList.add('hide');
  });
  $(el.target.getAttribute("data-box")).classList.remove('hide');

  if(el.target.id != 'set-wifi') {
    var fragment = document.createDocumentFragment();
	  fragment.appendChild($('btn-box'));
	  $(el.target.getAttribute("data-box")).appendChild(fragment);
    $('btn-box').classList.remove('hide');
  }
  else
    $('btn-box').classList.add('hide');
}


function showMenu() {
  $('top-nav').classList.add('responsive');
}

var closeCallback = function(){;} ;

function openModalMessage(title, msg, fn) {
  $('message-title').innerHTML = title;
  $('message-body').innerHTML = msg;
  $('modal-message').open = true;
  $('main-box').style.filter = "blur(3px)";
  if (typeof fn != 'undefined') {
    closeCallback = fn;
    $('ok-modal').classList.remove('hide');
  }
  else
  $('ok-modal').classList.add('hide');
}

function closeModalMessage(do_cb) {
  $('modal-message').open = false;
  $('main-box').style.filter = "";
  if (typeof closeCallback != 'undefined' && do_cb)
    closeCallback();
}

function restartESP() {
  var url = new URL("http://" + `${window.location.hostname}` + "/reset");
  fetch(url)
  .then(response => response.text())
  .then(data => {
    closeModalMessage();
    openModalMessage('Restart!', '<br>ESP restarted. Please wait a little and then reload this page.<br>');
  });
}

function handleSubmit() {
  let fileElement = $('file-input');
  // check if user had selected a file
  if (fileElement.files.length === 0) {
    alert('please choose a file');
    return;
  }
  var update = $('update-log');
  var loader = $('loader');
  var prg = $('progress-wrap');
  loader.classList.remove('hide');
  prg.classList.add('active')
  update.innerHTML = 'Update in progress';
  
  let formData = new FormData();
  formData.set('update', fileElement.files[0]);
  var req = new XMLHttpRequest();
  req.open('POST', '/update?size=' + fileElement.files[0].size);  
  req.onload = function(d) {
    loader.classList.add('hide');
    prg.classList.remove('active');
    if (req.status != 200) 
      update.innerHTML = `Error ${req.status}: ${req.statusText}`; 
    else 
      update.innerHTML = req.response;
  };
  req.upload.addEventListener('progress', (p) => {
    let w = Math.round(p.loaded/p.total*100) + '%';
    if (p.lengthComputable) {
      $('progress-anim').style.width = w;
      update.innerHTML = 'Update in progress: ' + w;
    }
  });
  req.send(formData);
}

async function uploadFolder(e) {
  let listing = document.getElementById('listing');
  for (let file of Array.from(e.target.files)) {
    let path = file.webkitRelativePath;
    let item = document.createElement('li');
    item.textContent = path;
    listing.appendChild(item);
    
    // Save each file in the ESP flash
    var reader = new FileReader();
    reader.onload = function(event) {
      // Remove default "data" from path
      if (path.startsWith("data/"))
        path = path.replace("data/", "");
        
      var formData = new FormData();
      formData.set("data", file, "/" + path);
      
      // POST data using the Fetch API
      fetch('/edit', {method: 'POST', body: formData})
      .then(response => response.text())
    };
    reader.readAsText(file);
  };
}


// Initializes the app.
$('svg-menu').innerHTML = svgMenu;
$('svg-eye').innerHTML = svgEye;
$('svg-no-eye').innerHTML = svgNoEye;
$('svg-scan').innerHTML = svgScan;
$('svg-connect').innerHTML = svgConnect;
$('svg-save').innerHTML = svgSave;
$('svg-restart').innerHTML = svgRestart;
$('img-logo').innerHTML = svgLogo;

$('hum-btn').addEventListener('click', showMenu);
$('scan-wifi').addEventListener('click', getWiFiList);
$('connect-wifi').addEventListener('click', doConnection);
$('save-params').addEventListener('click', saveParameters);
$('show-hide-password').addEventListener('click', showHidePassword);
$('set-wifi').addEventListener('click', switchPage);
$('set-update').addEventListener('click', switchPage);
$('about').addEventListener('click', switchPage);
$('restart').addEventListener('click', restartESP);
$('picker').addEventListener('change', uploadFolder );
$('update-btn').addEventListener('click', handleSubmit);
$('file-input').addEventListener('change', () => {
  $('fw-label').innerHTML = $('file-input').files.item(0).name +' (' + $('file-input').files.item(0).size + ' bytes)' ;
  $('fw-label').style.background = 'brown';
});

window.addEventListener('load', getParameters);

// Enable wifi-connect btn only if password inserted
$('connect-wifi').disabled = true;
$('password').addEventListener('input', (event) => {
  if( $('password').value.length  === 0 )
    $('connect-wifi').disabled = true;
  else
    $('connect-wifi').disabled = false;
});
