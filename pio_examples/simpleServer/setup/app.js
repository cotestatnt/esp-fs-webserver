// UI Icons (Unicode)
const ICONS = {
    lock: '🔒', unlock: '🔓', scan: '📡', connect: '📶', 
    save: '💾', restart: '↻', eye: '🐵', noEye: '🙈', 
    menu: '☰', del: '🗑️', clear: '✖', upload: '↥'
};

// Global Variables
let closeCb = () => {};
const port = location.port || (location.protocol === 'https:' ? '443' : '80');
const esp = `${location.protocol}//${location.hostname}:${port}/`;
let options = {};
let configFile;
let lastBox;
let wifiCredentials = [];
let selectedCredentialIndex = -1;

const $ = (id) => document.getElementById(id);
const hide = (id) => $(id)?.classList.add('hide');
const show = (id) => $(id)?.classList.remove('hide');

const newEl = (tag, attrs = {}) => {
    const el = document.createElement(tag);
    for (const [key, val] of Object.entries(attrs)) el.setAttribute(key, val);
    return el;
};

// Event Delegation
const handleInputChange = (e) => {
    const t = e.target;
    if (!t.classList.contains('opt-input')) return;

    const id = t.id;
    if (t.type === 'range') {
        const readout = $(`${id}-readout`);
        if (readout) readout.value = t.value;
        updateSliderOption(id, t.value);
    } else if (t.id.endsWith('-readout')) {
        const baseId = id.replace('-readout', '');
        const slider = $(baseId);
        if (slider) {
            slider.value = t.value; 
            updateSliderOption(baseId, t.value);
        }
    } else if (t.type === 'checkbox') {
        options[id] = t.checked;
    } else if (t.type === 'number') {
        const step = t.step ? Number(t.step) : 0;
        options[id] = step ? parseFloat(t.value) : parseInt(t.value, 10);
    } else {
        options[id] = t.value;
    }
};

const updateSliderOption = (key, val) => {
    const prev = (options[key] && typeof options[key] === 'object') ? options[key] : {};
    const numVal = Number(val);
    const step = prev.step || 1; 
    options[key] = { ...prev, value: Math.round(numVal * (1 / step)) / (1 / step), type: prev.type || 'slider' };
};

// Setup
window.addEventListener('load', () => {
    getParameters();
    // loadCredentials called inside getParameters
    
    const tpl = $('logo-tpl');
    if(tpl) $('img-logo').appendChild(tpl.content.cloneNode(true));
    
    $('main-box').addEventListener('change', handleInputChange);
    $('main-box').addEventListener('input', (e) => {
        if(e.target.type === 'range' && e.target.classList.contains('opt-input')) {
            const readout = $(`${e.target.id}-readout`);
            if(readout) readout.value = e.target.value;
        }
    });

    $('hum-btn').onclick = () => $('top-nav').classList.toggle('responsive');
    $('scan-wifi').onclick = getWiFiList;
    $('connect-wifi').onclick = (e) => doConnection(e, false);
    $('save-params').onclick = saveParameters;
    $('delete-cred').onclick = () => window.deleteSelectedCredential?.();
    $('clear-creds').onclick = () => window.clearAllCredentials?.();
    $('set-wifi').onclick = switchPage;
    $('set-update').onclick = switchPage;
    $('restart').onclick = confirmRestart;
    $('update-btn').onclick = handleUpdate;
    $('ok-modal').onclick = () => closeModal(true);
    $('close-modal').onclick = () => closeModal(false);
    
    $('show-networks').onclick = () => {
         $('wifi-table').classList.toggle('hide');
         $('show-networks').classList.toggle('hide');
    };
    
    $('show-hide-password').onclick = () => {
        const inp = $("password");
        const isPass = inp.type === "password";
        inp.type = isPass ? "text" : "password";
        isPass ? (show('show-pass'), hide('hide-pass')) : (hide('show-pass'), show('hide-pass'));
    };

    // UPDATE: Listener per #dhcp
    $('dhcp').onchange = function() {
        const action = this.checked ? 'add' : 'remove'; // Checked = Hide IP fields
        ['conf-wifi'].forEach(id => $(id)?.classList[action]('hide'));
    };
    
    $('picker').onchange = uploadFolder;
    $('file-input').onchange = function() {
        $('fw-label').innerHTML = `${this.files[0].name} (${this.files[0].size} bytes)`;
        $('fw-label').style.background = 'brown';
    };
    
    const map = { 
        'svg-menu': ICONS.menu, 'svg-eye': ICONS.eye, 'svg-no-eye': ICONS.noEye,
        'svg-scan': ICONS.scan, 'svg-connect': ICONS.connect, 'svg-save': ICONS.save,
        'svg-save2': ICONS.save, 'svg-restart': ICONS.restart, 'svg-delete': ICONS.del, 
        'svg-clear': ICONS.clear, 'svg-update': ICONS.upload
    };
    for (const [id, ico] of Object.entries(map)) if($(id)) $(id).textContent = ico;
});

const getParameters = () => {
  show('loader');
  fetch(`${esp}getStatus`)
    .then(res => res.json())
    .then(data => {
      $('esp-mode').innerHTML = data.mode;
      $('esp-ip').innerHTML = `<a href="http://${data.hostname}.local/">http://${data.hostname}.local</a><a href="${esp}"> (${data.ip})</a>`;
      $('firmware').innerHTML = data.firmware;
      $('about').href = data.liburl;
      $('about').innerHTML = 'Created with ' + data.liburl;
      configFile = data.path;

      fetch(`${esp}${configFile}`).then(r => r.json()).then(cfg => {
          if (cfg['img-logo']) {
              fetch(cfg['img-logo']).then(r => r.text()).then(b64 => {
                  const s = cfg['img-logo'].replace(/[^\d_]/g, '').split('_');
                  const img = newEl('img', { class: 'logo', src: `data:image/png;base64,${b64}`, style: `width:${s[0]}px;height:${s[1]}px`});
                  $('img-logo').innerHTML = ''; $('img-logo').appendChild(img);
              });
              delete cfg['img-logo'];
          }
          if (cfg['name-logo']) {
              const safeName = cfg['name-logo'].replace(/(<([^>]+)>)/ig, '');
              $('name-logo').innerHTML = safeName; document.title = safeName;
              delete cfg['name-logo'];
          }
          options = cfg;
          createOptionsBox(options);
          hide('loader');
          // UPDATE: Load credentials AFTER base config
          loadCredentials(); 
      });
    });
};

const addOptionsElement = (opt) => {
  const fragment = document.createDocumentFragment();
  const bools = Object.entries(opt).filter(([_, v]) => typeof v === "boolean");
  if (bools.length > 0) {
    const d = newEl('div', { class: 'row-wrapper' });
    bools.forEach(([key, val]) => {
      const lbl = newEl('label', { class: 'input-label toggle' });
      const inp = newEl('input', { class: 't-check opt-input', type: 'checkbox', id: key });
      inp.checked = val;
      lbl.append(inp, newEl('div', { class: 'toggle-switch' }), newEl('span', { class: 'toggle-label' }));
      lbl.lastChild.textContent = key;
      d.appendChild(lbl);
    });
    fragment.appendChild(d);
  }

  Object.entries(opt).forEach(([key, val]) => {
    if (typeof val === "boolean") return;
    const wrapper = newEl('div', { class: 'tf-wrapper' });
    const lbl = newEl('label', { class: 'input-label' });
    lbl.textContent = key;
    let inputEl;

    if (typeof val === 'object' && val.values) {
        inputEl = newEl('select', { id: key, class: 'opt-input' });
        val.values.forEach(v => {
            const opt = newEl('option', { value: v });
            opt.textContent = v;
            if(v === val.selected) opt.selected = true;
            inputEl.appendChild(opt);
        });
    } else if (typeof val === 'object' && val.type === 'slider') {
        const container = newEl('div', { class: 'slider-wrapper' });
        const slider = newEl('input', { class: 'opt-input slider', type: 'range', id: key, min: val.min, max: val.max, step: val.step });
        slider.value = val.value;
        const readout = newEl('input', { class: 'opt-input slider-readout', type: 'number', id: `${key}-readout`, min: val.min, max: val.max, step: val.step });
        readout.value = val.value;
        container.append(slider, readout);
        inputEl = container;
    } else if (typeof val === 'object' && val.type === 'number') {
        inputEl = newEl('input', { class: 'opt-input', type: 'number', id: key, min: val.min, max: val.max, step: val.step });
        inputEl.value = val.value;
    } else {
        inputEl = newEl('input', { class: 'opt-input', type: typeof val === 'number' ? 'number' : 'text', id: key });
        inputEl.value = val;
    }
    wrapper.append(lbl, inputEl);
    if (key.endsWith('-hidden')) wrapper.classList.add('hide');
    fragment.appendChild(wrapper);
  });
  lastBox.appendChild(fragment);
};

const createNewBox = (idx, lbl) => {
  const box = newEl('div', { class: 'ctn opt-box hide', id: `option-box${idx}` });
  const h = newEl('h2', { class: 'heading-2' });
  h.textContent = lbl;
  box.appendChild(h);
  $('main-box').appendChild(box);

  const lnk = newEl('a', { class: 'a-link', id: `set-opt${idx}`, 'data-box': `option-box${idx}` });
  lnk.textContent = lbl;
  lnk.onclick = switchPage;
  $('nav-link').appendChild(lnk);
  return box;
};

const createOptionsBox = (raw) => {
  // UPDATE: Logic ID dhcp
  const isDhcp = raw.hasOwnProperty('dhcp') ? raw.dhcp : true; 
  $('dhcp').checked = isDhcp;
  // If keys are missing (cleanup), they will be empty strings
  ['ip', 'gateway', 'subnet'].forEach(k => $(k).value = raw[k === 'ip' ? 'ip_address' : k] || '');
  
  // IsDhcp true -> Hide fields
  const action = isDhcp ? 'add' : 'remove';
  ['conf-wifi'].forEach(id => $(id)?.classList[action]('hide'));

  let nest = {};
  let boxId = 'wifi-box';
  lastBox = $(boxId);

  Object.entries(raw).forEach(([key, value], idx) => {
    if (key.startsWith('param-box')) {
      if(Object.keys(nest).length) addOptionsElement(nest);
      lastBox = createNewBox(idx, value);
      nest = {};
      boxId = value;
    } else if (boxId !== 'wifi-box') {
      if (key.startsWith('raw-')) {
          const type = key.split('-')[1];
          if(type === 'html') {
             const d = newEl('div', { class: 'tf-wrapper raw-html', id: value, 'data-box': lastBox.id });
             lastBox.appendChild(d);
             fetch(value).then(r => r.text()).then(h => $(value).innerHTML = h);
          } else if(type === 'css') document.head.appendChild(newEl('link', { rel: 'stylesheet', href: value }));
          else if(type === 'javascript') document.body.appendChild(newEl('script', { src: value }));
      } else if (!key.startsWith('img-') && !key.startsWith('name-')) {
          nest[key] = value;
      }
    }
  });
  if (Object.keys(nest).length) addOptionsElement(nest);
};

const loadCredentials = async () => {
  try {
    const res = await fetch(`${esp}wifi/credentials?_=${Date.now()}`); 
    if (!res.ok) return;
    wifiCredentials = await res.json();
    selectedCredentialIndex = wifiCredentials.length ? 0 : -1;

    if (wifiCredentials.length > 1) {
      if (typeof renderCredentialTabs === 'undefined') {
        const script = document.createElement('script');
        script.src = 'credentials.js';
        script.onload = () => renderCredentialTabs();
        document.body.appendChild(script);
      } else {
        renderCredentialTabs();
      }
    } else {
       if($('wifi-cred-tabs')) $('wifi-cred-tabs').innerHTML = '';
       ['delete-cred','clear-creds','cred-actions-inline'].forEach(hide);
    }
    if (selectedCredentialIndex >= 0) window.applyCredential(wifiCredentials[0]);
  } catch (e) { console.error(e); }
};

// UPDATE: Global Function for creds.js logic
window.applyCredential = (cred) => {
    $('ssid').value = cred.ssid || '';
    $('ssid-name').textContent = cred.ssid || 'SSID';
    
    // IP presente e != 0.0.0.0 -> Static IP -> NO DHCP
    const isStatic = cred.ip && cred.ip !== '0.0.0.0';
    const isDhcp = !isStatic;
    
    $('dhcp').checked = isDhcp;
    
    if (isStatic) {
        show('conf-wifi');
        $('ip').value = cred.ip;
        $('gateway').value = cred.gateway;
        $('subnet').value = cred.subnet;
    } else {
        hide('conf-wifi');
    }
    $('password').value = '';
};

const hasStoredCredentialFor = (ssid) => wifiCredentials.some(c => c.ssid === ssid);

function saveParameters() {
  const isWifi = $('wifi-box').classList.contains('active') || !$('wifi-box').classList.contains('hide');
  
  // WiFi params are handled by CredentialManager now, so remove them from JSON config if present
  if (isWifi) {
      delete options.dhcp;
      delete options.ip_address;
      delete options.gateway;
      delete options.subnet;
  }
  
  const blob = new Blob([JSON.stringify(options, null, 2)], { type: 'application/json' });
  const form = new FormData();
  form.append("data", blob, '/' + configFile);

  fetch('/edit', { method: 'POST', body: form })
    .then(() => openModal('Save', `<br>Saved config to <b>/${configFile}</b>`))
    .catch(e => openModal('Error', `Save failed: ${e}`));
}

function getWiFiList() {
  show('loader');
  fetch(`${esp}scan`).then(r => r.json()).then(data => {
      if(data.reload) setTimeout(getWiFiList, 2000);
      data.sort((a, b) => b.strength - a.strength);
      
      const list = $('wifi-list');
      list.innerHTML = "";
      const frag = document.createDocumentFragment();
      
      data.forEach((net, i) => {
        const row = newEl('tr', { id: `wifi-${i}` });
        row.innerHTML = `<td><input type="radio" name="select" id="select-wifi-${i}"></td><td>${net.ssid}</td><td class="hide-tiny">${net.strength} dBm</td><td>${net.security ? ICONS.lock : ICONS.unlock}</td>`;
        row.onclick = () => {
             $('ssid').value = net.ssid;
             $('ssid-name').textContent = net.ssid;
             $('password').focus();
             try{ $(`select-wifi-${i}`).checked=true; }catch(e){}
             $('wifi-table').classList.add('hide');
             show('show-networks');
        };
        frag.appendChild(row);
      });
      list.appendChild(frag);
      show('wifi-table');
      hide('loader');
  }).catch(() => hide('loader'));
}

function doConnection(e, isRetry) {
  const ssid = $('ssid').value;
  const pass = $('password').value;
  if (!ssid || (!pass && !hasStoredCredentialFor(ssid))) return openModal('WiFi', 'Insert SSID & Password');

  const f = new FormData();
  f.append("ssid", ssid);
  f.append("password", pass);
  f.append("persistent", $('persistent').checked);
  // UPDATE: Logic using ID dhcp
  if(!$('dhcp').checked) {
      f.append("ip_address", $('ip').value);
      f.append("gateway", $('gateway').value);
      f.append("subnet", $('subnet').value);
  }
  if (isRetry) f.append("newSSID", true);

  show('loader');
  fetch('/connect', { method: 'POST', body: f })
    .then(r => r.text().then(msg => {
        hide('loader');
        if(r.status !== 200) openModal('Error', msg);
        else if(msg.includes('already')) openModal('WiFi', msg, () => doConnection(e, true));
        else { openModal('WiFi', msg, restartESP); loadCredentials(); }
    }))
    .catch(() => { hide('loader'); openModal('WiFi', 'Connection failed'); });
}

function handleUpdate() {
  const file = $('file-input').files[0];
  if (!file) return alert('Select a file');
  
  show('loader'); show('progress-wrap');
  $('progress-wrap').classList.add('active');
  $('update-log').textContent = 'Updating...';
  
  const f = new FormData(); f.append('update', file);
  const xhr = new XMLHttpRequest();
  xhr.open('POST', `/update?size=${file.size}`);
  xhr.onload = () => {
      hide('loader'); $('progress-wrap').classList.remove('active');
      $('update-log').textContent = xhr.status === 200 ? xhr.responseText : 'Error';
  };
  xhr.upload.onprogress = (p) => {
      if(p.lengthComputable) $('progress-anim').style.width = `${Math.round((p.loaded/p.total)*100)}%`;
  };
  xhr.send(f);
}

async function uploadFolder(e) {
    const list = $('listing');
    for (const file of e.target.files) {
        const path = file.webkitRelativePath.replace(/^data\//, '');
        const li = newEl('li'); li.textContent = path; list.appendChild(li);
        const f = new FormData(); f.append("data", file, '/' + path);
        await fetch('/edit', { method: 'POST', body: f }).catch(() => li.style.color = 'red');
    }
}

function switchPage(e) {
  const t = e.target;
  const boxId = t.getAttribute("data-box");
  $('top-nav').classList.remove('responsive');
  document.querySelectorAll('a').forEach(a => a.classList.remove('active'));
  t.classList.add('active');
  document.querySelectorAll('.opt-box').forEach(b => b.classList.add('hide'));
  show(boxId);
  
  if(t.id !== 'set-wifi') {
      const box = $(boxId);
      document.querySelectorAll('.raw-html').forEach(el => {
          if(el.dataset.box === boxId) box.insertBefore(el, $('btn-hr'));
      });
      box.appendChild($('btn-hr')); box.appendChild($('btn-box'));
      show('btn-box'); show('btn-hr');
  } else {
      hide('btn-box'); hide('btn-hr');
  }
}

// Global Modal helpers for creds.js
window.openModal = (t, m, fn) => { 
    $('message-title').innerHTML = t; $('message-body').innerHTML = m; 
    $('modal-message').open = true; $('main-box').style.filter = "blur(3px)";
    closeCb = fn; 
    (fn ? show : hide)('ok-modal');
};

window.closeModal = (exec) => {
    $('modal-message').open = false; $('main-box').style.filter = "";
    if(exec && closeCb) closeCb();
};

const confirmRestart = () => window.openModal('Restart', 'Restart now?', restartESP);
const restartESP = () => fetch(`${esp}reset`).then(() => { window.closeModal(); window.openModal('Restarted', 'ESP Restarted'); });