// UI Icons (Unicode)
const ICONS = { 
    lock: '🔒', unlock: '🔓', scan: '📡', connect: '📶',
    save: '💾', restart: '↻', eye: '🐵', noEye: '🙈',
    menu: '☰', del: '🗑️', clear: '✖', upload: '✅'
};

// Global Variables
let closeCb = () => { };
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

    $('main-box').addEventListener('change', handleInputChange);
    $('main-box').addEventListener('input', (e) => {
        if (e.target.type === 'range' && e.target.classList.contains('opt-input')) {
            const readout = $(`${e.target.id}-readout`);
            if (readout) readout.value = e.target.value;
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

    // Keep connect button label in sync with SSID field
    if ($('ssid') && $('ssid-name')) {
        $('ssid').addEventListener('input', () => {
            const v = $('ssid').value.trim();
            $('ssid-name').textContent = v || 'SSID';
        });
    }

    // DHCP toggle listener
    $('dhcp').onchange = function () {
        const action = this.checked ? 'add' : 'remove'; // Checked = Hide IP fields
        ['conf-wifi'].forEach(id => $(id)?.classList[action]('hide'));
    };

    $('picker').onchange = uploadFolder;

    // Store default label texts for reset after actions
    const fwLabel = $('fw-lbl');
    if (fwLabel && !fwLabel.dataset.defaultText) {
        fwLabel.dataset.defaultText = fwLabel.textContent;
    }
    const pickLabel = $('pick-lbl');
    if (pickLabel && !pickLabel.dataset.defaultText) {
        pickLabel.dataset.defaultText = pickLabel.textContent;
    }

    // Disable submit button until a firmware file is selected
    if ($('update-btn')) {
        $('update-btn').classList.add('load');
    }

    $('file-input').onchange = function () {
        const lbl = $('fw-lbl');
        if (!this.files || !this.files.length || !lbl) return;
        const f = this.files[0];
        const prettySize = `${f.size} bytes`;
        lbl.textContent = `${f.name} (${prettySize})`;
        lbl.title = f.name;
        lbl.style.background = 'brown';

        // Enable submit button now that a file is selected
        if ($('update-btn')) {
           $('update-btn').classList.remove('load');
        }
    };

    const map = {
        'svg-menu': ICONS.menu, 'svg-eye': ICONS.eye, 'svg-no-eye': ICONS.noEye,
        'svg-scan': ICONS.scan, 'svg-connect': ICONS.connect, 'svg-save': ICONS.save,
        'svg-save2': ICONS.save, 'svg-restart': ICONS.restart, 'svg-delete': ICONS.del,
        'svg-clear': ICONS.clear, 'svg-update': ICONS.upload
    };
    for (const [id, ico] of Object.entries(map)) if ($(id)) $(id).textContent = ico;

    // Dynamic header layout: place topnav inline after firmware span on large screens,
    // keep original stacked layout (with hamburger behavior) on small screens.
    let topNavOriginalParent = null;
    let topNavNextSibling = null;

    const updateHeaderLayout = () => {
        const header = document.querySelector('header.ctn');
        const title = header ? header.querySelector('.title') : null;
        const heading = title ? title.querySelector('.heading') : null;
        const topnav = $('top-nav');
        if (!header || !heading || !topnav) return;

        const isMobile = window.matchMedia('(max-width: 608px)').matches;

        if (isMobile) {
            // Restore original position below the title for small screens
            if (!topNavOriginalParent) {
                topNavOriginalParent = header;
                topNavNextSibling = title ? title.nextSibling : null;
            }
            if (topnav.parentElement !== topNavOriginalParent) {
                if (topNavNextSibling) {
                    topNavOriginalParent.insertBefore(topnav, topNavNextSibling);
                } else {
                    topNavOriginalParent.appendChild(topnav);
                }
            }
            topnav.classList.remove('inline-header');
        } else {
            // On larger screens, move nav inline into the heading (after firmware span)
            if (!topNavOriginalParent) {
                topNavOriginalParent = topnav.parentElement;
                topNavNextSibling = topnav.nextSibling;
            }
            if (topnav.parentElement !== heading) {
                heading.appendChild(topnav);
            }
            topnav.classList.add('inline-header');
        }
    };

    updateHeaderLayout();
    window.addEventListener('resize', updateHeaderLayout);
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

            // Populate port field with the actual server port (if the element exists)
            if ($('port') && typeof data.port !== 'undefined') {
                $('port').value = data.port;
            }

            // Cache current hostname for later hints (e.g. reconnect URL)
            window.espHostname = data.hostname || '';

            // Pre-fill hostname field in WiFi box with current mDNS name
            if ($('hostname')) {
                $('hostname').value = data.hostname || '';
            }

            // Apply logo and title immediately from getStatus (if available)
            const logoImg = $('img-logo')?.querySelector('img');
            if (data['img-logo'] && logoImg) {
                logoImg.src = data['img-logo'];
                $('img-logo')?.classList.remove('loading');
            }
            if (data['page-title']) {
                const safeName = data['page-title'].replace(/(<([^>]+)>)/ig, '');
                $('page-title').innerHTML = safeName;
                document.title = safeName;
            }

            // Custom logo, page title and optional port support from config.json (fallback if not in getStatus)
            fetch(`${esp}${configFile}`).then(r => r.json()).then(cfg => {
                // Only apply logo if not already set from getStatus
                if (!data['img-logo'] && logoImg) {
                    logoImg.src = cfg['img-logo'] || 'config/logo.svg';
                    $('img-logo')?.classList.remove('loading');
                }
                if (cfg['img-logo']) delete cfg['img-logo'];
                
                // Only apply title if not already set from getStatus
                if (!data['page-title'] && cfg['page-title']) {
                    const safeName = cfg['page-title'].replace(/(<([^>]+)>)/ig, '');
                    $('page-title').innerHTML = safeName;
                    document.title = safeName;
                }
                if (cfg['page-title']) delete cfg['page-title'];
                
                // If a "port" key is present in config.json, show and sync the input field;
                // otherwise hide the field entirely (advanced/optional feature).
                if ($('port')) {
                    if (Object.prototype.hasOwnProperty.call(cfg, 'port')) {
                        $('port-w').classList.remove('hide');
                        $('port').value = cfg['port'];
                    } else {
                        $('port-w').classList.add('hide');
                    }
                }

                options = cfg;
                createOptionsBox(options);
                hide('loader');
                // Initial boot loading complete: show header content
                document.body.classList.remove('boot-loading');
                // Load credentials after base config
                loadCredentials();
                
                // Emit event for custom scripts that need to wait for full page initialization
                window.dispatchEvent(new Event('setupPageReady'));
            })
            .catch(() => {
                hide('loader');
                document.body.classList.remove('boot-loading');
            });
        })
        .catch(() => {
            hide('loader');
            document.body.classList.remove('boot-loading');
        });
};

const addOptionsElement = (opt) => {
    const fragment = document.createDocumentFragment();
    const bools = Object.entries(opt).filter(([_, v]) => typeof v === "boolean");
    if (bools.length > 0) {
        const d = newEl('div', { class: 'row-w' });
        bools.forEach(([key, val]) => {
            const lbl = newEl('label', { class: 'input-lbl toggle' });
            const inp = newEl('input', { class: 't-check opt-input', type: 'checkbox', id: key });
            inp.checked = val;
            lbl.append(inp, newEl('div', { class: 'toggle-sw' }), newEl('span', { class: 'toggle-lbl' }));
            lbl.lastChild.textContent = key;
            d.appendChild(lbl);
        });
        fragment.appendChild(d);
    }

    Object.entries(opt).forEach(([key, val]) => {
        if (typeof val === "boolean") return;
        const wrapper = newEl('div', { class: 'tf-w' });
        const lbl = newEl('label', { class: 'input-lbl' });
        lbl.textContent = key;
        let inputEl;

        if (typeof val === 'object' && val.values) {
            inputEl = newEl('select', { id: key, class: 'opt-input' });
            val.values.forEach(v => {
                const opt = newEl('option', { value: v });
                opt.textContent = v;
                if (v === val.selected) opt.selected = true;
                inputEl.appendChild(opt);
            });
        } else if (typeof val === 'object' && val.type === 'slider') {
            const container = newEl('div', { class: 'slider-w' });
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
    // WiFi configuration is now handled by CredentialManager, not config.json
    // Remove any legacy WiFi keys if present
    delete raw.dhcp;
    delete raw.ip_address;
    delete raw.gateway;
    delete raw.subnet;
    delete raw.dns1;
    delete raw.dns2;

    let nest = {};
    let boxId = 'wifi-box';
    lastBox = $(boxId);

    Object.entries(raw).forEach(([key, value], idx) => {
        if (key.startsWith('param-box')) {
            if (Object.keys(nest).length) addOptionsElement(nest);
            lastBox = createNewBox(idx, value);
            nest = {};
            boxId = value;
        } else if (boxId !== 'wifi-box') {
            if (key.startsWith('raw-')) {
                const type = key.split('-')[1];
                if (type === 'html') {
                    const d = newEl('div', { class: 'tf-w raw-html', id: value, 'data-box': lastBox.id });
                    lastBox.appendChild(d);
                    fetch(value).then(r => r.text()).then(h => $(value).innerHTML = h);
                } else if (type === 'css') document.head.appendChild(newEl('link', { rel: 'stylesheet', href: value }));
                else if (type === 'javascript') document.body.appendChild(newEl('script', { src: value }));
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
                script.src = 'creds.js';
                script.onload = () => renderCredentialTabs();
                document.body.appendChild(script);
            } else {
                renderCredentialTabs();
            }
        } else {
            if ($('wifi-cred-tabs')) $('wifi-cred-tabs').innerHTML = '';
            ['delete-cred', 'clear-creds', 'cred-actions-inline'].forEach(hide);
        }
        if (selectedCredentialIndex >= 0) window.applyCredential(wifiCredentials[0]);
    } catch (e) { console.error(e); }
};

// Global function for credentials.js logic
window.applyCredential = (cred) => {
    $('ssid').value = cred.ssid || '';
    $('ssid-name').textContent = cred.ssid || 'SSID';

    // Check if credential has static IP (non-zero IP means static config)
    const isStatic = cred.ip && cred.ip !== '0.0.0.0';
    const isDhcp = !isStatic;

    $('dhcp').checked = isDhcp;

    if (isStatic) {
        show('conf-wifi');
        $('ip').value = cred.ip;
        $('gateway').value = cred.gateway;
        $('subnet').value = cred.subnet;
        $('dns1').value = cred.dns1 || '';
        $('dns2').value = cred.dns2 || '';
    } else {
        hide('conf-wifi');
        $('ip').value = '';
        $('gateway').value = '';
        $('subnet').value = '';
        $('dns1').value = '';
        $('dns2').value = '';
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
        delete options.dns1;
        delete options.dns2;
    }

    const blob = new Blob([JSON.stringify(options, null, 2)], { type: 'application/json' });
    const form = new FormData();
    form.append("data", blob, '/' + configFile);

    fetch('/edit', { method: 'POST', body: form })
        .then(() => openModal('Save', `<br>Saved config to <b>/${configFile}</b>`))
        .catch(e => openModal('Error', `Save failed: ${e}`));
}
function getWiFiList() {
    const btn = $('scan-wifi');
    btn.classList.add('load');

    fetch(`${esp}scan`).then(r => r.json()).then(data => {
        if (data.reload) setTimeout(getWiFiList, 2000);
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
                try { $(`select-wifi-${i}`).checked = true; } catch (e) { }
                $('wifi-table').classList.add('hide');
                show('show-networks');
            };
            frag.appendChild(row);
        });
        list.appendChild(frag);
        show('wifi-table');
        btn.classList.remove('load');
    }).catch(() => hide('loader'));
}

function doConnection(e, skipConfirm) {
    const ssid = $('ssid').value;
    const pass = $('password').value;
    if (!ssid || (!pass && !hasStoredCredentialFor(ssid))) return openModal('WiFi', 'Insert SSID & Password');

    const f = new FormData();
    f.append("ssid", ssid);
    f.append("password", pass);
    f.append("persistent", $('persistent').checked);
    const host = $('hostname') ? $('hostname').value.trim() : '';
    if (host) f.append("hostname", host);
    // Static IP configuration when DHCP is disabled
    if (!$('dhcp').checked) {
        f.append("ip_address", $('ip').value);
        f.append("gateway", $('gateway').value);
        f.append("subnet", $('subnet').value);
        const dns1 = $('dns1').value.trim();
        const dns2 = $('dns2').value.trim();
        if (dns1) f.append("dns1", dns1);
        if (dns2) f.append("dns2", dns2);
    }
    if (skipConfirm) f.append("confirmed", "true");

    show('loader');
    fetch('/connect', { method: 'POST', body: f })
        .then(r => r.text().then(msg => {
            hide('loader');
            if (r.status !== 200) {
                openModal('Error', msg);
            }
            else if (msg.includes('action-confirm-sta-change')) {
                // Server requires confirmation for STA mode change
                openModal('WiFi', msg, () => doConnection(e, true));
            }
            else if (msg.includes('action-restart-required')) {
                // Connection succeeded, restart required
                openModal('WiFi', msg, restartESP);
            }
            else if (msg.includes('action-async-applying')) {
                // Connection is being applied asynchronously
                openModal('WiFi', msg);
                // Don't reload credentials here - they haven't been saved yet
            }
        }))
}

function handleUpdate() {
    const file = $('file-input').files[0];
    if (!file) return; // Guard: ignore clicks when disabled / no file

    show('loader'); show('pgr-wrap');
    $('pgr-wrap').classList.add('active');
    $('update-log').textContent = 'Updating...';

    const f = new FormData(); f.append('update', file);
    const xhr = new XMLHttpRequest();
    xhr.open('POST', `/update?size=${file.size}`);
    xhr.onload = () => {
        hide('loader');
        $('pgr-wrap').classList.remove('active');
        $('update-log').textContent = xhr.status === 200 ? xhr.responseText : 'Error';

        // Reset UI to initial state after upload
        const input = $('file-input');
        const lbl = $('fw-lbl');
        if (input) input.value = '';
        if (lbl) {
            lbl.textContent = lbl.dataset.defaultText || 'Select firmware binary file';
            lbl.removeAttribute('title');
            lbl.style.background = '';
        }
        if ($('update-btn')) {
            $('update-btn').setAttribute('aria-disabled', 'true');
        }
        if ($('pgr-anim')) {
            $('pgr-anim').style.width = '0%';
        }
        hide('pgr-wrap');
    };
    xhr.upload.onprogress = (p) => {
        if (p.lengthComputable) $('pgr-anim').style.width = `${Math.round((p.loaded / p.total) * 100)}%`;
    };
    xhr.send(f);
}

async function uploadFolder(e) {
    const files = Array.from(e.target.files || []);
    if (!files.length) return;

    const list = $('listing');
    const label = $('pick-lbl');

    // Reset listing for a fresh upload session
    if (list) list.innerHTML = '';

    if (label) {
        label.setAttribute('aria-disabled', 'true');
        label.style.background = 'brown';
        const txt = label.dataset.defaultText || label.textContent;
        label.textContent = `${txt} (${files.length} file${files.length > 1 ? 's' : ''} selected)`;
    }

    let ok = 0;
    let fail = 0;

    for (const file of files) {
        const path = file.webkitRelativePath.replace(/^data\//, '');
        const li = newEl('li');
        li.textContent = path;
        if (list) list.appendChild(li);

        const f = new FormData();
        f.append('data', file, '/' + path);

        try {
            const res = await fetch('/edit', { method: 'POST', body: f });
            if (!res.ok) {
                fail++;
                li.style.color = 'red';
            } else {
                ok++;
            }
        } catch (_) {
            fail++;
            li.style.color = 'red';
        }
    }

    // Reset file input so the same folder can be re-selected
    if (e.target) e.target.value = '';

    if (label) {
        label.textContent = label.dataset.defaultText || 'Select the folder containing your files';
        label.style.background = '';
        label.removeAttribute('aria-disabled');
    }

    const title = 'FS Upload';
    let msg = `<br>Uploaded <b>${ok}</b> file${ok !== 1 ? 's' : ''}`;
    if (fail) {
        msg += `, <b>${fail}</b> failed.`;
    } else {
        msg += ' successfully.';
    }
    openModal(title, msg);
}

function switchPage(e) {
    const t = e.target;
    const boxId = t.getAttribute("data-box");
    $('top-nav').classList.remove('responsive');
    document.querySelectorAll('a').forEach(a => a.classList.remove('active'));
    t.classList.add('active');
    document.querySelectorAll('.opt-box').forEach(b => b.classList.add('hide'));
    show(boxId);

    if (t.id !== 'set-wifi') {
        const box = $(boxId);
        // Move shared buttons into the active box first, then insert
        // any raw-html blocks before the separator inside that box.
        box.appendChild($('btn-hr'));
        box.appendChild($('btn-box'));
        document.querySelectorAll('.raw-html').forEach(el => {
            if (el.dataset.box === boxId) {
                box.insertBefore(el, $('btn-hr'));
            }
        });
        show('btn-box'); show('btn-hr');
    } else {
        hide('btn-box'); hide('btn-hr');
    }
}

// Global Modal helpers for creds.js
window.openModal = (t, m, fn) => {
    $('message-title').innerHTML = t; $('message-body').innerHTML = m;
    $('modal-msg').open = true; $('main-box').style.filter = "blur(3px)";
    closeCb = fn;
    (fn ? show : hide)('ok-modal');
};

window.closeModal = (exec) => {
    $('modal-msg').open = false; $('main-box').style.filter = "";
    if (exec && closeCb) closeCb();
};

const confirmRestart = () => window.openModal('Restart', 'Restart now?', restartESP);
const restartESP = () => fetch(`${esp}reset`).then(() => { window.closeModal(); window.openModal('Restarted', 'ESP Restarted'); });