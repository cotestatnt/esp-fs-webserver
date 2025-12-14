const svgLogo = '<svg fill="brown" height="120" viewBox="0 0 24 24"><path d="M5 3C3.9 3 3 3.9 3 5S2.1 7 1 7v2c1.1 0 2 .9 2 2s.9 2 2 2h2v-2H5v-1c0-1.1-.9-2-2-2 1.1 0 2-.9 2-2V5h2V3M11 3c1.1 0 2 .9 2 2s.9 2 2 2v2c-1.1 0-2 .9-2 2s-.9 2-2 2H9v-2h2v-1c0-1.1.9-2 2-2-1.1 0-2-.9-2-2V5H9V3h2m11 3v12c0 1.11-.89 2-2 2H4a2 2 0 01-2-2v-3h2v3h16V6h-2.97V4H20c1.11 0 2 .89 2 2z"/></svg>';
const svgLock = '<svg height="16" viewBox="0 0 24 24"><path d="M12 17a2 2 0 002-2c0-1.11-.9-2-2-2a2 2 0 00-2 2 2 2 0 002 2m6-9a2 2 0 012 2v10a2 2 0 01-2 2H6a2 2 0 01-2-2V10c0-1.11.9-2 2-2h1V6a5 5 0 0110 0v2h1M12 3a3 3 0 00-3 3v2h6V6a3 3 0 00-3-3z"/></svg>';
const svgUnlock = '<svg height="16" viewBox="0 0 24 24"><path d="M18 1c-2.76 0-5 2.24-5 5v2H4c-1.1 0-2 .89-2 2v10c0 1.11.9 2 2 2h12c1.11 0 2-.89 2-2V10c0-1.1-.89-2-2-2h-1V6c0-1.66 1.34-3 3-3s3 1.34 3 3v2h2V6c0-2.76-2.24-5-5-5M10 13c1.1 0 2 .89 2 2 0 1.11-.9 2-2 2s-2-.89-2-2c0-1.1.9-2 2-2z"/></svg>';
const svgScan = '<path d="M12 20l-3.6-4.8c1 .75 2.25 1.2 3.6 1.2s2.6-.45 3.6-1.2L12 20M4.8 10.4l1.8 2.4C8.1 11.67 10 11 12 11s3.9.67 5.4 1.8l1.8-2.4C17.19 8.89 14.7 8 12 8s-5.19.89-8.2 2.4M12 2a10 10 0 00-10.8 3.6L3 8c2.5-1.88 5.62-3 9-3s6.5 1.12 9 3l1.8-2.4C19.79 3.34 16.05 2 12 2M7 24h2v-2H7v2m8 0h2v-2h-2v2m-4 0h2v-2h-2v2z"/>';
const svgConnect = '<path d="M12 21l3.6-4.8c-1-.75-2.25-1.2-3.6-1.2s-2.6.45-3.6 1.2L12 21M12 3C7.95 3 4.21 4.34 1.2 6.6L3 9c2.5-1.88 5.62-3 9-3s6.5 1.12 9 3l1.8-2.4C19.79 4.34 16.05 3 12 3m0 6c-2.7 0-5.19.89-7.2 2.4l1.8 2.4c1.5-1.13 3.37-1.8 5.4-1.8s3.9.67 5.4 1.8l1.8-2.4C17.19 9.89 14.7 9 12 9z"/>';
const svgSave = '<path d="M15 9H5V5h10m-3 10a3 3 0 01-3-3 3 3 0 013-3 3 3 0 013 3 3 3 0 01-3 3m8-16H5c-1.11 0-2 .9-2 2v14a2 2 0 002 2h14a2 2 0 002-2V7l-4-4z"/>';
const svgRestart = '<path d="M12 4a8 8 0 015.6 2.3C20.7 9.4 20.7 14.5 17.6 17.6a8 8 0 01-6.7 2.3l.5-2a6 6 0 004.8-1.7c2.3-2.3 2.3-6.1 0-8.5a6 6 0 00-4.2-1.7V10.6L7 5.6 12 .6V4M6.3 17.6C3.7 15 3.3 11 5.1 7.9l1.5 1.5A6 6 0 007.8 16.2c.5.5 1.1.9 1.8 1.2l-.6 2a8 8 0 01-2.7-1.8z"/>';
const svgEye = '<path d="M12 9a3 3 0 013 3 3 3 0 01-3 3 3 3 0 01-3-3 3 3 0 013-3m0-4.5c5 0 9.27 3.11 11 7.5-1.73 4.39-6 7.5-11 7.5s-9.27-3.11-11-7.5C2.73 7.61 7 4.5 12 4.5M3.18 12a9.821 9.821 0 0017.64 0A9.821 9.821 0 003.18 12z"/>';
const svgNoEye = '<path d="M0 0h24v24H0V0z" fill="none"/><path d="M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z"/>';
const svgMenu = '<path d="M3 6h18v2H3V6m0 5h18v2H3v-2m0 5h18v2H3v-2z"/>';

// Global Variables
let closeCb = function() {};
const port = location.port || (window.location.protocol === 'https:' ? '443' : '80');
const esp = `${window.location.protocol}//${window.location.hostname}:${port}/`;
let options = {};
let configFile;
let lastBox;

// Element selector shorthand
const $ = (el) => document.getElementById(el);

// Utility Functions
const hide = (id) => $(id).classList.add('hide');
const show = (id) => $(id).classList.remove('hide');

// Create element with attributes (readable + robust)
const newEl = (element, attribute) => {
  const el = document.createElement(element);
  if (attribute && typeof attribute === 'object') {
    for (const [key, val] of Object.entries(attribute)) el.setAttribute(key, val);
  }
  return el;
};

// Fetch and display parameters
const getParameters = () => {
  let logo;
  show('loader');
  fetch(`${esp}getStatus`)
    .then(res => res.json())
    .then(data => {
      $('esp-mode').innerHTML = data.mode;
      $('esp-ip').innerHTML = `<a href="http://${data.hostname}.local/">http://${data.hostname}.local</a><a href="${esp}"> (${data.ip})</a>`;
      $('firmware').innerHTML = data.firmware;
      $('about').innerHTML = 'Created with ' + data.liburl;
      $('about').setAttribute('href', data.liburl);
      configFile = data.path;

      fetch(`${esp}${configFile}`)
        .then(response => response.json())
        .then(data => {
          for (const key in data) {
            if (data.hasOwnProperty(key)) {
              if (key === 'name-logo') {
                $('name-logo').innerHTML = data[key].replace(/(<([^>]+)>)/ig, '');
                document.title = data[key].replace(/(<([^>]+)>)/ig, '');
                delete data[key];
                continue;
              }
              if (key === 'img-logo') {
                logo = data[key];
                delete data[key];
                continue;
              }
            }
          }

          if (logo) {
            fetch(logo)
              .then(response => response.text())
              .then(base64 => setLogoBase64(logo, base64));
          }

          options = data;
          createOptionsBox(options);
          hide('loader');
        });
    });
};

const setLogoBase64 = (s, base64) => {
  const size = s.replace(/[^\d_]/g, '').split('_');
  const img = newEl('img', { 'class': 'logo', 'src': `data:image/png;base64, ${base64}`, 'style': `width:${size[0]}px;height:${size[1]}px` });
  $('img-logo').innerHTML = "";
  $('img-logo').append(img);
  $('img-logo').setAttribute('type', 'number');
  $('img-logo').setAttribute('title', '');
};

// Add options element
const addOptionsElement = (opt) => {
  const bools = Object.keys(opt)
    .filter(key => typeof(opt[key]) === "boolean")
    .reduce((obj, key) => {
      obj[key] = opt[key];
      return obj;
    }, {});

  if (Object.entries(bools).length !== 0) {
    const d = newEl('div', { 'class': 'row-wrapper' });
    Object.entries(bools).forEach(([key, value]) => {
      const lbl = newEl('label', { 'class': 'input-label toggle' });
      const el = newEl('input', { 'class': 't-check opt-input', 'type': 'checkbox', 'id': key });
      el.checked = value;
      const dv = newEl('div', { 'class': 'toggle-switch' });
      const sp = newEl('span', { 'class': 'toggle-label' });
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
    const lbl = newEl('label', { 'class': 'input-label', 'label-for': key });
    lbl.textContent = key;
    let el = newEl('input', { 'class': 'opt-input', 'type': 'text', 'id': key });
    el.value = value;

    if (typeof(value) === "number") el.setAttribute('type', 'number');
    if (typeof(value) === "object") {
      // Dropdown list
      if (value.values) {
        el = newEl('select', { 'id': key });
        value.values.forEach((a) => {
          const opt = newEl('option');
          opt.textContent = a;
          opt.value = a;
          el.appendChild(opt);
        });
        el.value = value.selected;
        lastBox.appendChild(el);
      }
      // Slider (explicit discriminator)
      else if (value.type === 'slider' && typeof value.value === 'number' && 'min' in value && 'max' in value && 'step' in value) {
        const num = Math.round(value.value * (1 / value.step)) / (1 / value.step);
        const stepStr = String(value.step);
        const decimalPlaces = stepStr.includes('.') ? stepStr.split('.')[1].length : 0;

        // Create slider input
        const slider = newEl('input', { 'class': 'opt-input slider', 'type': 'range', 'id': key });
        slider.setAttribute('step', value.step);
        slider.setAttribute('min', value.min);
        slider.setAttribute('max', value.max);
        slider.value = Number(num).toFixed(decimalPlaces);

        // Create readout input for precise edits
        const readout = newEl('input', { 'class': 'opt-input slider-readout', 'type': 'number', 'id': `${key}-readout` });
        readout.setAttribute('step', value.step);
        readout.setAttribute('min', value.min);
        readout.setAttribute('max', value.max);
        readout.value = Number(num).toFixed(decimalPlaces);

        // Keep slider and readout in sync
        slider.addEventListener('input', (e) => {
          readout.value = e.target.value;
          const prev = (options[key] && typeof options[key] === 'object') ? options[key] : {};
          options[key] = {
            ...prev,
            value: Number(e.target.value),
            step: value.step,
            min: value.min,
            max: value.max,
            type: prev.type || 'slider'
          };
        });
        readout.addEventListener('change', (e) => {
          const v = Number(e.target.value);
          const bounded = Math.min(Math.max(v, value.min), value.max);
          const rounded = Math.round(bounded * (1 / value.step)) / (1 / value.step);
          const fixed = Number(rounded).toFixed(decimalPlaces);
          slider.value = fixed;
          readout.value = fixed;
          const prev = (options[key] && typeof options[key] === 'object') ? options[key] : {};
          options[key] = {
            ...prev,
            value: Number(fixed),
            step: value.step,
            min: value.min,
            max: value.max,
            type: prev.type || 'slider'
          };
        });

        // Wrap slider + readout in a container
        const container = newEl('div', { 'class': 'slider-wrapper' });
        container.appendChild(slider);
        container.appendChild(readout);
        el = container;
      }
      // Numeric object rendered as number input (explicit discriminator or fallback)
      else if ((value.type === 'number') && typeof value.value === 'number' && 'min' in value && 'max' in value && 'step' in value) {
        const num = Math.round(value.value * (1 / value.step)) / (1 / value.step);
        const stepStr = String(value.step);
        const decimalPlaces = stepStr.includes('.') ? stepStr.split('.')[1].length : 0;
        el.setAttribute('type', 'number');
        el.setAttribute('step', value.step);
        el.setAttribute('min', value.min);
        el.setAttribute('max', value.max);
        el.value = Number(num).toFixed(decimalPlaces);
      }
    }
    addInputListener(el);
    const d = newEl('div', { 'class': 'tf-wrapper' });
    d.appendChild(lbl);
    d.appendChild(el);
    lastBox.appendChild(d);

    if (key.endsWith('-hidden')) d.classList.add('hide');
  });
};

// Create new box
const createNewBox = (cont, lbl) => {
  const box = newEl('div', { 'class': 'ctn opt-box hide', 'id': `option-box${cont}` });
  const h = newEl('h2', { 'class': 'heading-2' });
  h.innerHTML = lbl;
  box.appendChild(h);
  $('main-box').appendChild(box);

  const lnk = newEl('a', { 'class': 'a-link', 'id': `set-opt${cont}`, 'data-box': `option-box${cont}` });
  lnk.innerHTML = lbl;
  lnk.addEventListener('click', switchPage);
  $('nav-link').appendChild(lnk);
  return box;
};

// Create options box
const createOptionsBox = async (raw) => {
  // Initialize WiFi settings
  const dhcp = !!raw.dhcp;
  $('no-dhcp').checked = dhcp;
  $('ip').value = raw.ip_address;
  $('gateway').value = raw.gateway;
  $('subnet').value = raw.subnet;
  if (dhcp) { show('conf-wifi'); show('save-wifi'); }

  let nest = {};
  let boxId = 'wifi-box';
  lastBox = $(boxId);

  Object.entries(raw).forEach(([key, value], index) => {
    if (key.startsWith('param-box')) {
      addOptionsElement(nest);
      lastBox = createNewBox(index, value);
      nest = {};
      boxId = value;
    } else if (boxId !== 'wifi-box') {
      let hidden = false;
      if (key.startsWith('img-logo') || key.startsWith('name-logo')) {
        hidden = true;
      } else if (key.startsWith('raw-css')) {
        const css = newEl("link", { 'rel': 'stylesheet', 'href': value });
        document.head.appendChild(css);
        hidden = true;
      } else if (key.startsWith('raw-javascript')) {
        const js = newEl("script", { 'src': value });
        document.body.appendChild(js);
        hidden = true;
      } else if (key.startsWith('raw-html')) {
        const el = newEl('div', { 'class': 'tf-wrapper raw-html', 'id': value, 'data-box': lastBox.id });
        lastBox.appendChild(el);
        fetch(value)
          .then((res) => res.text())
          .then((data) => $(value).innerHTML = data);
        hidden = true;
      }
      if (!hidden) nest[key] = value;
    }
  });

  if (Object.entries(nest).length !== 0) addOptionsElement(nest);
};

function addInputListener(item) {
  const onChange = (e) => {
    const t = e.target;
    switch (t.type) {
      case 'number': {
        const step = t.step ? Number(t.step) : 0;
        // Ignore readout changes handled above; treat plain number inputs
        if (t.id.endsWith('-readout')) return;
        if (step) {
          const prev = (options[t.id] && typeof options[t.id] === 'object') ? options[t.id] : {};
          options[t.id] = {
            ...prev,
            value: Math.round(Number(t.value) * (1/step)) / (1/step),
            step,
            min: Number(t.getAttribute('min')),
            max: Number(t.getAttribute('max')),
            type: (prev && prev.type) ? prev.type : 'number'
          };
        } else {
          options[t.id] = parseInt(t.value, 10);
        }
        break;
      }
      case 'text':
        options[t.id] = t.value;
        break;
      case 'checkbox':
        options[t.id] = t.checked;
        break;
      default:
        if (t.type === 'select-one') options[t.id].selected = t.value;
        break;
    }
  };
  item.addEventListener('change', onChange);
}

function insertKey(key, value, obj, pos) {
  const acc = {};
  const keys = Object.keys(obj);
  keys.forEach((k, i) => { if (i === pos) acc[key] = value; acc[k] = obj[k]; });
  return acc;
}

function saveParameters() {
  // Backward compatibility
  if (Object.keys(options)[0]?.startsWith('param-box')) {
    const isParamBox0 = Object.keys(options)[0] === 'param-box0';
    options = {
      ...(isParamBox0 
        ? { 'param-box-0': options['wifi-box'] }
        : { 'wifi-box': '' }
      ),
      'dhcp': false,
      ...options
    };
  }

  const noDhcp = $('no-dhcp').checked;
  options.dhcp = noDhcp;

  if (noDhcp) {
    const networkConfig = {
      ip_address: $('ip').value,
      gateway: $('gateway').value,
      subnet: $('subnet').value
    };

    Object.entries(networkConfig).forEach(([key, value], index) => {
      options = insertKey(key, value, options, index + 2);
      options[key] = value;
    });
  }

  const configData = new Blob([JSON.stringify(options, null, 2)], {
    type: 'application/json'
  });

  const formData = new FormData();
  formData.append("data", configData, '/' + configFile);

  fetch('/edit', { method: 'POST', body: formData })
    .then(r => r.text())
    .then(() => openModal('Save options', `<br><b>"/${configFile}"</b> saved successfully on flash memory!<br><br>`))
    .catch(err => openModal('Error!', `Failed to save: ${err}`));
}


function showHidePassword() {
  const inp = $("password");
  const isPassword = inp.type === "password";
  inp.type = isPassword ? "text" : "password";
  show(isPassword ? 'show-pass' : 'hide-pass');
  hide(isPassword ? 'hide-pass' : 'show-pass');
}

function getWiFiList() {
  show('loader');
  fetch(`${esp}scan`)
    .then(response => response.json())
    .then(data => {
      listWifi(data);
      hide('loader');
    })
    .catch(error => {
      console.error('Error fetching WiFi list:', error);
      hide('loader');
    });
}

function selectWifi(event) {
  const row = event.currentTarget;
  const id = row.id;
  const ssid = row.cells[1].textContent;

  try {
    $(`select-${id}`).checked = true;
  } catch (err) {
    $(id).checked = true;
  }

  $('ssid').value = ssid;
  $('ssid-name').textContent = ssid;
  $('password').focus();
  
  // Collapse wifi list and show chevron
  $('wifi-table').classList.add('hide');
  show('show-networks');
}

function listWifi(obj) {
  if (obj.reload) setTimeout(getWiFiList, 2000);

  obj.sort((a, b) => b.strength - a.strength);

  const list = document.querySelector('#wifi-list');
  list.innerHTML = "";
  const frag = document.createDocumentFragment();
  obj.forEach((net, i) => {
    const row = newEl('tr', { id: `wifi-${i}` });
    row.addEventListener('click', selectWifi);
    row.innerHTML = `
      <td><input type="radio" name="select" id="select-wifi-${i}"></td>
      <td id="ssid-wifi-${i}">${net.ssid}</td>
      <td class="hide-tiny">${net.strength} dBm</td>
      <td>${net.security ? svgLock : svgUnlock}</td>
    `;
    frag.appendChild(row);
  });
  list.appendChild(frag);
  show('wifi-table');
}

function doConnection(e, f) {
  const ssid = $('ssid').value;
  const password = $('password').value;

  if (!ssid || !password) {
    openModal('Connect to WiFi', 'Please insert a SSID and a Password');
    return;
  }

  const formdata = new FormData();
  formdata.append("ssid", ssid);
  formdata.append("password", password);
  formdata.append("persistent", $('persistent').checked);

  if (f) formdata.append("newSSID", true);

  if ($('no-dhcp').checked) {
    formdata.append("ip_address", $('ip').value);
    formdata.append("gateway", $('gateway').value);
    formdata.append("subnet", $('subnet').value);
  }

  const requestOptions = {
    method: 'POST',
    body: formdata,
    redirect: 'follow'
  };

  show('loader');

  fetch('/connect', requestOptions)
    .then(res => res.text().then(data => ({ status: res.status, data })))
    .then(({ status, data }) => {
      if (status === 200) {
        if (data.includes("already")) {
          openModal('Connect to WiFi', data, () => doConnection(e, true));
        } else {
          openModal('Connect to WiFi', data, restartESP);
        }
      } else {
        openModal('Error!', data);
      }
      hide('loader');
    })
    .catch(error => {
      openModal('Connect to WiFi', error);
      hide('loader');
    });
}

function switchPage(el) {
  const target = el.target;
  const boxId = target.getAttribute("data-box");
  
  $('top-nav').classList.remove('responsive');
  
  // Update active menu item
  document.querySelectorAll("a").forEach(item => item.classList.remove('active'));
  target.classList.add('active');

  // Hide all option boxes and show the selected one
  document.querySelectorAll(".opt-box").forEach(e => e.classList.add('hide'));
  show(boxId);

  const isWifiPage = target.id === 'set-wifi';
  if (!isWifiPage) {
    const box = $(boxId);
    const fragment = document.createDocumentFragment();
    fragment.appendChild($('btn-hr'));
    fragment.appendChild($('btn-box'));
    box.appendChild(fragment);

    document.querySelectorAll('.raw-html').forEach(elem => {
      if (elem.getAttribute("data-box") === box.id) {
        box.insertBefore(elem, $('btn-hr'));
      }
    });

    show('btn-box');
    show('btn-hr');
  } else {
    hide('btn-box');
    hide('btn-hr');
  }
}

function showMenu() {
  $('top-nav').classList.toggle('responsive');
}

function openModal(title, msg, fn) {
  const modal = $('modal-message');
  const mainBox = $('main-box');
  
  $('message-title').innerHTML = title;
  $('message-body').innerHTML = msg;
  modal.open = true;
  mainBox.style.filter = "blur(3px)";
  
  if (fn) {
    closeCb = fn;
    show('ok-modal');
  } else {
    hide('ok-modal');
  }
}

function closeModal(doCb) {
  $('modal-message').open = false;
  $('main-box').style.filter = "";
  if (closeCb && doCb) closeCb();
}

// Ask user confirmation before triggering restart
function confirmRestart() {
  openModal('Restart ESP', '<br>Do you want to restart now?', restartESP);
}

function restartESP() {
  fetch(`${esp}reset`)
    .then(() => {
      closeModal();
      openModal('Restart!', '<br>ESP restarted!');
    })
    .catch(error => openModal('Error!', `Failed to restart ESP: ${error}`));
}
function handleSubmit() {
  const fileElement = $('file-input');
  const files = fileElement.files;

  if (files.length === 0) {
    alert('Please choose a file');
    return;
  }

  const update = $('update-log');
  const progressWrap = $('progress-wrap');
  const progressAnim = $('progress-anim');

  show('loader');
  show('progress-wrap');
  progressWrap.classList.add('active');
  update.innerHTML = 'Update in progress';

  const formData = new FormData();
  formData.set('update', files[0]);

  const req = new XMLHttpRequest();
  req.open('POST', `/update?size=${files[0].size}`);

  req.onload = () => {
    hide('loader');
    progressWrap.classList.remove('active');
    update.innerHTML = req.status === 200 ? req.response : `Error ${req.status}: ${req.statusText}`;
  };

  req.upload.addEventListener('progress', (p) => {
    if (p.lengthComputable) {
      const width = `${Math.round((p.loaded / p.total) * 100)}%`;
      progressAnim.style.width = width;
      update.innerHTML = `Update in progress: ${width}`;
    }
  });

  req.send(formData);
}
async function uploadFolder(e) {
  const list = $('listing');
  const uploadFile = async (file, path) => {
    const item = newEl('li');
    item.textContent = path;
    list.appendChild(item);

    try {
      const formData = new FormData();
      formData.set("data", file, '/' + path);
      const response = await fetch('/edit', { method: 'POST', body: formData });
      if (!response.ok) throw new Error(`Upload failed`);
    } catch (err) {
      console.error(`Error uploading ${path}:`, err);
      item.style.color = 'red';
    }
  };

  for (const file of e.target.files) {
    const path = file.webkitRelativePath.replace(/^data\//, '');
    await uploadFile(file, path);
  }
}
// Initialize SVG icons
const svgIcons = {
  'svg-menu': svgMenu,
  'svg-eye': svgEye, 
  'svg-no-eye': svgNoEye,
  'svg-scan': svgScan,
  'svg-connect': svgConnect,
  'svg-save': svgSave,
  'svg-save2': svgSave,
  'svg-restart': svgRestart,
  'img-logo': svgLogo
};

// Set SVG icons
Object.entries(svgIcons).forEach(([id, svg]) => {
  $(id).innerHTML = svg;
});

// Add event listeners
const eventListeners = {
  'hum-btn': ['click', showMenu],
  'scan-wifi': ['click', getWiFiList],
  'connect-wifi': ['click', doConnection],
  'save-params': ['click', saveParameters],
  'save-wifi': ['click', saveParameters], 
  'show-hide-password': ['click', showHidePassword],
  'set-wifi': ['click', switchPage],
  'set-update': ['click', switchPage],
  'about': ['click', switchPage],
  'restart': ['click', confirmRestart],
  'picker': ['change', uploadFolder],
  'update-btn': ['click', handleSubmit]
};

Object.entries(eventListeners).forEach(([id, [event, handler]]) => {
  $(id).addEventListener(event, handler);
});

// File input change handler
$('file-input').addEventListener('change', () => {
  const file = $('file-input').files[0];
  $('fw-label').innerHTML = `${file.name} (${file.size} bytes)`;
  $('fw-label').style.background = 'brown';
});

// DHCP checkbox handler
$('no-dhcp').addEventListener('change', function() {
  const method = this.checked ? 'remove' : 'add';
  ['conf-wifi', 'save-wifi'].forEach(id => $(id).classList[method]('hide'));
});

// WiFi list chevron handler - toggle visibility
if ($('show-networks')) {
  $('show-networks').addEventListener('click', () => {
    $('wifi-table').classList.toggle('hide');
    $('show-networks').classList.toggle('hide');
  });
}

// Initialize on page load
window.addEventListener('load', getParameters);
