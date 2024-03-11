/*jshint esversion: 9 */
const svgLogo = '<svg fill="brown" height=120px viewBox="0 0 24 24"><path d="M5 3C3.9 3 3 3.9 3 5S2.1 7 1 7V9C2.1 9 3 9.9 3 11S3.9 13 5 13H7V11H5V10C5 8.9 4.1 8 3 8C4.1 8 5 7.1 5 6V5H7V3M11 3C12.1 3 13 3.9 13 5S13.9 7 15 7V9C13.9 9 13 9.9 13 11S12.1 13 11 13H9V11H11V10C11 8.9 11.9 8 13 8C11.9 8 11 7.1 11 6V5H9V3H11M22 6V18C22 19.11 21.11 20 20 20H4C2.9 20 2 19.11 2 18V15H4V18H20V6H17.03V4H20C21.11 4 22 4.89 22 6Z" /></svg>'
const svgLock =  '<svg height="16pt" viewBox="0 0 24 24"><path d="M12,17A2,2 0 0,0 14,15C14,13.89 13.1,13 12,13A2,2 0 0,0 10,15A2,2 0 0,0 12,17M18,8A2,2 0 0,1 20,10V20A2,2 0 0,1 18,22H6A2,2 0 0,1 4,20V10C4,8.89 4.9,8 6,8H7V6A5,5 0 0,1 12,1A5,5 0 0,1 17,6V8H18M12,3A3,3 0 0,0 9,6V8H15V6A3,3 0 0,0 12,3Z" /></svg>';
const svgUnlock = '<svg height="16pt" viewBox="0 0 24 24"><path d="M18 1C15.24 1 13 3.24 13 6V8H4C2.9 8 2 8.89 2 10V20C2 21.11 2.9 22 4 22H16C17.11 22 18 21.11 18 20V10C18 8.9 17.11 8 16 8H15V6C15 4.34 16.34 3 18 3C19.66 3 21 4.34 21 6V8H23V6C23 3.24 20.76 1 18 1M10 13C11.1 13 12 13.89 12 15C12 16.11 11.11 17 10 17C8.9 17 8 16.11 8 15C8 13.9 8.9 13 10 13Z" /></svg>';
const svgScan = '<path d="M12 20L8.4 15.2C9.4 14.45 10.65 14 12 14S14.6 14.45 15.6 15.2L12 20M4.8 10.4L6.6 12.8C8.1 11.67 9.97 11 12 11S15.9 11.67 17.4 12.8L19.2 10.4C17.19 8.89 14.7 8 12 8S6.81 8.89 4.8 10.4M12 2C7.95 2 4.21 3.34 1.2 5.6L3 8C5.5 6.12 8.62 5 12 5S18.5 6.12 21 8L22.8 5.6C19.79 3.34 16.05 2 12 2M7 24H9V22H7V24M15 24H17V22H15V24M11 24H13V22H11V24Z" />';
const svgConnect = '<path d="M12,21L15.6,16.2C14.6,15.45 13.35,15 12,15C10.65,15 9.4,15.45 8.4,16.2L12,21M12,3C7.95,3 4.21,4.34 1.2,6.6L3,9C5.5,7.12 8.62,6 12,6C15.38,6 18.5,7.12 21,9L22.8,6.6C19.79,4.34 16.05,3 12,3M12,9C9.3,9 6.81,9.89 4.8,11.4L6.6,13.8C8.1,12.67 9.97,12 12,12C14.03,12 15.9,12.67 17.4,13.8L19.2,11.4C17.19,9.89 14.7,9 12,9Z" />';
const svgSave = '<path d="M15,9H5V5H15M12,19A3,3 0 0,1 9,16A3,3 0 0,1 12,13A3,3 0 0,1 15,16A3,3 0 0,1 12,19M17,3H5C3.89,3 3,3.9 3,5V19A2,2 0 0,0 5,21H19A2,2 0 0,0 21,19V7L17,3Z" />';
const svgRestart = '<path d="M12,4C14.1,4 16.1,4.8 17.6,6.3C20.7,9.4 20.7,14.5 17.6,17.6C15.8,19.5 13.3,20.2 10.9,19.9L11.4,17.9C13.1,18.1 14.9,17.5 16.2,16.2C18.5,13.9 18.5,10.1 16.2,7.7C15.1,6.6 13.5,6 12,6V10.6L7,5.6L12,0.6V4M6.3,17.6C3.7,15 3.3,11 5.1,7.9L6.6,9.4C5.5,11.6 5.9,14.4 7.8,16.2C8.3,16.7 8.9,17.1 9.6,17.4L9,19.4C8,19 7.1,18.4 6.3,17.6Z" />';
const svgEye = '<path d="M12,9A3,3 0 0,1 15,12A3,3 0 0,1 12,15A3,3 0 0,1 9,12A3,3 0 0,1 12,9M12,4.5C17,4.5 21.27,7.61 23,12C21.27,16.39 17,19.5 12,19.5C7,19.5 2.73,16.39 1,12C2.73,7.61 7,4.5 12,4.5M3.18,12C4.83,15.36 8.24,17.5 12,17.5C15.76,17.5 19.17,15.36 20.82,12C19.17,8.64 15.76,6.5 12,6.5C8.24,6.5 4.83,8.64 3.18,12Z" />';
const svgNoEye = '<path d="M0 0h24v24H0V0z" fill="none"/><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>';
const svgMenu = '<path d="M3,6H21V8H3V6M3,11H21V13H3V11M3,16H21V18H3V16Z"/>';

var closeCb = function(){};
var port = location.port || (window.location.protocol === 'https:' ? '443' : '80');
var esp = `${window.location.protocol}//${window.location.hostname}:${port}/`;
var options = {};
var configFile;
var lastBox;

// Element selector shorthands
var $ = function(el) {
	return document.getElementById(el);
};

function hide(id) {
  $(id).classList.add('hide');
}

function show(id) {
  $(id).classList.remove('hide');
}

function newEl(element, attribute) {
  var el = document.createElement(element);
  if (typeof(attribute) === 'object') {
    for (var key in attribute) {
      el.setAttribute(key, attribute[key]);
    }
  }
  return el;
}

function getParameters() {
  var logo;
  show('loader');
  // Fetch actual status and config info
  fetch(esp + "getStatus")
  .then(res => res.json())
  .then(data => {
    $('esp-mode').innerHTML = data.mode;
    $('esp-ip').innerHTML = `<a href="${esp}">${esp}</a>`;
    $('firmware').innerHTML = data.firmware;
    $('about').innerHTML = 'Created with ' + data.liburl;
    $('about').setAttribute('href', data.liburl);
    configFile = data.path;
    
    // Fetch 'config.json'
    fetch(esp + configFile)
    .then(response => response.json())
    .then(data => {
      for (const key in data){
        if(data.hasOwnProperty(key)){
          if (key === 'name-logo') {
            $('name-logo').innerHTML = data[key].replace( /(<([^>]+)>)/ig, '');
            document.title = data[key].replace( /(<([^>]+)>)/ig, '');
            delete data[key];
            continue;
          }
          if (key == 'img-logo') {
            logo = data[key];
            delete data[key];
            continue;
          }
        }
      }
      
      // Custom logo (base 64)
      if (logo){
        fetch(logo)
        .then((response) => response.text())
        .then(base64 => setLogoBase64(logo, base64));
      }
        
      options = data;
      createOptionsBox(options);
      hide('loader');      
    });
  });
}

function setLogoBase64(s, base64) {
  var size = s.replace(/[^\d_]/g, '').split('_');
  var img = newEl('img', {'class': 'logo', 'src': 'data:image/png;base64, '+ base64, 'style': `width:${size[0]}px;height:${size[1]}px`});
  $('img-logo').innerHTML = "";
  $('img-logo').append(img);
  $('img-logo').setAttribute('type', 'number');
  $('img-logo').setAttribute('title', '');
}

function addOptionsElement(opt) {
  const bools = Object.keys(opt)
  .filter(key => typeof(opt[key]) === "boolean")
  .reduce((obj, key) => {
    obj[key] = opt[key];
    return obj;
  }, {});
  
  if (Object.entries(bools).length !== 0) {
    var d  = newEl('div', {'class': 'row-wrapper'});
    Object.entries(bools).forEach(([key, value]) => {
      let lbl = newEl('label', {'class': 'input-label toggle'});
      let el = newEl('input', {'class': 't-check opt-input', 'type': 'checkbox', 'id': key});
      el.checked = value;
      let dv = newEl('div', {'class': 'toggle-switch'});
      let sp = newEl('span' , {'class': 'toggle-label'});
      sp.textContent = key;
      lbl.appendChild(el);
      lbl.appendChild(dv);
      lbl.appendChild(sp);
      addInputListener(el);
      d.appendChild(lbl);
    });
    lastBox.appendChild(d);
  }
  
  const others = Object.keys(opt)
  .filter(key => typeof(opt[key]) !== "boolean")
  .reduce((obj, key) => {
    obj[key] = opt[key];
    return obj;
  }, {});
  
  Object.entries(others).forEach(([key, value]) => {
    let lbl = newEl('label', {'class': 'input-label', 'label-for': key});
    lbl.textContent = key;
    let el = newEl('input',  {'class': 'opt-input', 'type': 'text', 'id': key});
    el.value = value;
    
    if (typeof(value) === "number")
      el.setAttribute('type', 'number');
    if (typeof(value) === "object" ) {
      // This is a select/option
      if (value.values) {
        el = newEl('select', {'id': key});
        value.values.forEach((a) => {
          var opt = newEl('option');
          opt.textContent = a;
          opt.value = a;
          el.appendChild(opt);
        });
        el.value = value.selected;
        lastBox.appendChild(el);
      }
      // This is a float value
      else {
        var num = Math.round(value.value  * (1/value.step)) / (1/value.step);
        el.setAttribute('type', 'number');
        el.setAttribute('step', value.step);
        el.setAttribute('min', value.min);
        el.setAttribute('max', value.max);
        el.value = Number(num).toFixed(3);
      }
    }
    addInputListener(el);
    var d = newEl('div', {'class': 'tf-wrapper'});
    d.appendChild(lbl);
    d.appendChild(el);
    lastBox.appendChild(d);

    if(key.endsWith('-hidden'))  {
      d.classList.add('hide');
    }
  });
  
}


function createNewBox(cont, lbl) {
  var box = newEl('div', {'class': 'ctn opt-box hide', 'id': 'option-box'+cont});
  var h = newEl('h2',  {'class': 'heading-2'});
  h.innerHTML = lbl;
  box.appendChild(h);
  $('main-box').appendChild(box);
  
  // Add new voice in menu and relatvie listener
  var lnk = newEl('a', {'class': 'a-link', 'id': 'set-opt'+cont, 'data-box': 'option-box'+cont});
  lnk.innerHTML = lbl;
  lnk.addEventListener('click', switchPage);
  $('nav-link').appendChild(lnk);
  return box;
}

async function createOptionsBox(raw) {
  var nest = {};
  var boxId = 'wifi-box';
  lastBox =  $(boxId);  
      
  Object.entries(raw).forEach(([key, value], index) => {
    if (boxId === 'wifi-box') {
      $('no-dhcp').checked = raw.dhcp;
      $('ip').value = raw.ip_address;
      $('gateway').value = raw.gateway;
      $('subnet').value = raw.subnet;
      if ($('no-dhcp').checked){
        show('conf-wifi');
        show('save-wifi');
      }
    }

    if (key.startsWith('param-box')) {
      addOptionsElement(nest);
      lastBox = createNewBox(index, value);
      nest = {};
      boxId = value;
    }
    else if (boxId != 'wifi-box') {
      var hidden = false;
      if (key.startsWith('img-logo') || key.startsWith('name-logo')) {
        hidden = true;
      }
      else if(key.startsWith('raw-css')) {
        var css = newEl("link", {'rel': 'stylesheet', 'href': value});
        document.head.appendChild(css);
        hidden = true;
      }
      // Inject runtime JS source file
      else if(key.startsWith('raw-javascript')) {
        var js = newEl("script", {'src': value});
        document.body.appendChild(js);
        hidden = true;
      }
      // Inject runtime HTML source file
      else if(key.startsWith('raw-html')) {
        var el = newEl('div', {'class': 'tf-wrapper raw-html', 'id': value, 'data-box': lastBox.id});
        lastBox.appendChild(el);
        fetch(value)
        .then((res) => res.text())
        .then((data) => $(value).innerHTML = data);
        hidden = true;
      }
      if (!hidden) {
        nest[key] = value;
      }
    }
  });
  
  // Add last items
  if (Object.entries(nest).length !== 0) {
    addOptionsElement(nest);
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

function insertKey(key,value,obj,pos){
  return Object.keys(obj).reduce((ac,a,i) => {
    if(i === pos) ac[key] = value;
    ac[a] = obj[a]; 
    return ac;
  },{});
}

function saveParameters() {
  // Backward compatibility
  if(Object.keys(options)[0].startsWith('param-box')) {
    if(Object.keys(options)[0] === 'param-box0') {
      options['param-box-0'] = options['wifi-box']; 
      options = {'dhcp': false, ...options};
    }
    else
      options = {'wifi-box': '', 'dhcp': false, ...options};
  }
  
  options.dhcp = $('no-dhcp').checked;
  if ($('no-dhcp').checked) {
    options = insertKey('ip_address', $('ip').value, options, 2);
    options = insertKey('gateway', $('gateway').value, options, 3);
    options = insertKey('subnet', $('subnet').value, options, 4);
    options["ip_address"] = $('ip').value;
    options["gateway"] = $('gateway').value;
    options["subnet"] = $('subnet').value;
  }
  
  var myblob = new Blob([JSON.stringify(options, null, 2)], {
    type: 'application/json'
  });
  var formData = new FormData();
  formData.append("data", myblob, '/' + configFile);

  // POST data using the Fetch API
  fetch('/edit', {
    method: 'POST',
    body: formData
  })

  // Handle the server response
  .then(response => response.text())
  .then(text => {
    openModal('Save options','<br><b>"/' + configFile +'"</b> saved successfully on flash memory!<br><br>');
  });
}


function showHidePassword() {
  var inp = $("password");
  if (inp.type === "password") {
    inp.type = "text";
    show('show-pass');
    hide('hide-pass');
  }
  else {
    inp.type = "password";
    hide('show-pass');
    show('hide-pass');
  }
}

function getWiFiList() {
  show('loader');
  fetch(esp + "scan")
  .then(response => response.json())
  .then(data => {
    listWifi(data);
    hide('loader');
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


function listWifi(obj) {
  if (obj.hasOwnProperty("reload"))
    setTimeout(getWiFiList, 2000); 
    
  obj.sort((a, b) => {
    return b.strength - a.strength;
  });
  
  const list = document.querySelector('#wifi-list');
  list.innerHTML = "";
	obj.forEach((net, i) => {
    // Create a single row with all columns
    var row = newEl('tr');
    var id = 'wifi-' + i;
    row.id = id;
    row.addEventListener('click', selectWifi);
	  row.innerHTML  = `<td><input type="radio" name="select" id="select-${id}"></td>`;
    row.innerHTML += `<td id="ssid-${id}">${net.ssid}</td>`;
    row.innerHTML += '<td class="hide-tiny">' + net.strength + ' dBm</td>';
    row.innerHTML += (net.security) ? '<td>' + svgLock + '</td>' : '<td>' + svgUnlock + '</td>';
    // Add row to list
    list.appendChild(row);
  });
  show('wifi-table');
}

function doConnection(e, f) {
  if ($('ssid').value === '' ||  $('password').value === ''){
    openModal('Connect to WiFi','Please insert a SSID and a Password');
    return;
  }
  var formdata = new FormData();
  formdata.append("ssid", $('ssid').value);
  formdata.append("password", $('password').value);
  formdata.append("persistent", $('persistent').checked);
  if (typeof f !== 'undefined')
    formdata.append("newSSID", true);
  if ($('no-dhcp').checked) {
    formdata.append("ip_address", $('ip').value);
    formdata.append("gateway", $('gateway').value);
    formdata.append("subnet", $('subnet').value);
  }
  var requestOptions = {
    method: 'POST',
    body: formdata,
    redirect: 'follow'
  };
  
  show('loader');
  var s;
  fetch('/connect', requestOptions)
  .then(function(res) {
    s = res.status;
    return res.text();
  })
  .then(function(data) {
    if (s === 200) {
      if (data.includes("already")) 
        openModal('Connect to WiFi', data, () => {doConnection(e, true)});
      else
        openModal('Connect to WiFi', data, restartESP);
    }
    else 
      openModal('Error!', data);
    
    hide('loader');
  })
  .catch((error) => {
    openModal('Connect to WiFi', error);
    hide('loader');
  });
}


function switchPage(el) {
  $('top-nav').classList.remove('resp');
  // Menu items
  document.querySelectorAll("a").forEach(item => {
    item.classList.remove('active');
  });
  el.target.classList.add('active');

  // Box items
  document.querySelectorAll(".opt-box").forEach(e => {
    e.classList.add('hide');
  });
  show(el.target.getAttribute("data-box"));

  if(el.target.id != 'set-wifi') {
    var fragment = document.createDocumentFragment();
    fragment.appendChild($('btn-hr'));
	  fragment.appendChild($('btn-box'));
	  const box = $(el.target.getAttribute("data-box"));
	  box.appendChild(fragment);
	  
	  document.querySelectorAll('.raw-html').forEach(function(el) {
	    if (el.getAttribute("data-box") === box.id)
        box.insertBefore(el,  $('btn-hr'));
    });
    
    show('btn-box');
    show('btn-hr');
  }
  else {
    hide('btn-box');
    hide('btn-hr');
  }
}


function showMenu() {
  $('top-nav').classList.add('resp');
}

function openModal(title, msg, fn, args) {
  $('message-title').innerHTML = title;
  $('message-body').innerHTML = msg;
  $('modal-message').open = true;
  $('main-box').style.filter = "blur(3px)";
  if (typeof fn != 'undefined') {
    closeCb = fn;
    show('ok-modal');
  }
  else
    hide('ok-modal');
}

function closeModal(do_cb) {
  $('modal-message').open = false;
  $('main-box').style.filter = "";
  if (typeof closeCb != 'undefined' && do_cb)
    closeCb();
}

function restartESP() {
  fetch(esp + "reset")
  .then(response => response.text())
  .then(data => {
    closeModal();
    openModal('Restart!', '<br>ESP restarted!');
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
  show('loader');
  show('progress-wrap');
  $('progress-wrap').classList.add('active');
  update.innerHTML = 'Update in progress';
  
  let formData = new FormData();
  formData.set('update', fileElement.files[0]);
  var req = new XMLHttpRequest();
  req.open('POST', '/update?size=' + fileElement.files[0].size);  
  req.onload = function(d) {
    hide('loader');
    $('progress-wrap').classList.remove('active');
    update.innerHTML = (req.status != 200)  ? `Error ${req.status}: ${req.statusText}` : req.response;
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
  let list = $('listing');
  for (let file of Array.from(e.target.files)) {
    let path = file.webkitRelativePath;
    let item = newEl('li');
    item.textContent = path;
    list.appendChild(item);
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
$('svg-save2').innerHTML = svgSave;
$('svg-restart').innerHTML = svgRestart;
$('img-logo').innerHTML = svgLogo;

$('hum-btn').addEventListener('click', showMenu);
$('scan-wifi').addEventListener('click', getWiFiList);
$('connect-wifi').addEventListener('click', doConnection);
$('save-params').addEventListener('click', saveParameters);
$('save-wifi').addEventListener('click', saveParameters);
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

$('no-dhcp').addEventListener('change', function() {
  if (this.checked) {
    show('conf-wifi');
    show('save-wifi');
  }
  else {
    hide('conf-wifi');
  }
});
window.addEventListener('load', getParameters);
