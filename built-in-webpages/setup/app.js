const ICONS = {
  lock: "🔒", unlock: "🔓", scan: "📡", connect: "📶",
  save: "💾", restart: "↻", eye: "🐵", noEye: "🙈",
  menu: "☰", del: "🗑️", clear: "✖", upload: "✅"
};

const $ = id => document.getElementById(id);
const hide = id => { const el = $(id); if (el) el.classList.add("hide"); };
const show = id => { const el = $(id); if (el) el.classList.remove("hide"); };
const bindIfPresent = (id, eventName, handler) => {
  const el = $(id);
  if (el) el.addEventListener(eventName, handler);
  return el;
};
const newEl = (tag, attrs = {}) => {
  const el = document.createElement(tag);
  for (const [key, value] of Object.entries(attrs)) el.setAttribute(key, value);
  return el;
};

const port = Number(location.port || (location.protocol === "https:" ? "443" : "80"));
let esp = `${location.protocol}//${location.hostname}:${port}/`;

let config;
let configPath;
let wifiCredentials = [];
let selectedCredentialIndex = -1;
let modalCloseCb;
let setupSocket;
let setupSocketConnectPromise;
let reconnectTimer = 0;
let requestSeq = 0;

const pendingRequests = new Map();
const loadedAssets = { css: new Set(), js: new Set() };

function openModal(title, body, onOk) {
  $("message-title").innerHTML = title;
  $("message-body").innerHTML = body;
  $("modal").open = true;
  $("main").style.filter = "blur(3px)";
  modalCloseCb = typeof onOk === "function" ? onOk : null;
  if (onOk) show("ok-modal"); else hide("ok-modal");
}

function closeModal(exec) {
  $("modal").open = false;
  $("main").style.filter = "";
  if (exec && modalCloseCb) modalCloseCb();
  modalCloseCb = null;
}

window.openModal = openModal;
window.closeModal = closeModal;

function getCookieValue(name) {
  const prefix = `${name}=`;
  return document.cookie
    .split(";")
    .map(chunk => chunk.trim())
    .find(chunk => chunk.startsWith(prefix))
    ?.slice(prefix.length) || "";
}

function getSetupWsUrls() {
  const proto = location.protocol === "https:" ? "wss:" : "ws:";
  const sameOriginHost = location.host || `${location.hostname}:${port}`;
  const dedicatedPortHost = `${location.hostname}:${port + 2}`;
  const wsMode = getCookieValue("esp_fs_ws_mode");

  // The backend tells us whether setup websocket traffic shares the HTTP
  // server or lives on the dedicated websocket port.
  if (wsMode === "dedicated") {
    return [`${proto}//${dedicatedPortHost}/`];
  }

  return [`${proto}//${sameOriginHost}/setup-ws`];
}

function rejectAllPending(error) {
  pendingRequests.forEach(entry => {
    clearTimeout(entry.timer);
    entry.reject(error);
  });
  pendingRequests.clear();
}

function connectSetupSocket() {
  if (setupSocket) {
    if (setupSocket.readyState === WebSocket.OPEN) {
      return Promise.resolve();
    }
    if (setupSocket.readyState === WebSocket.CONNECTING && setupSocketConnectPromise) {
      return setupSocketConnectPromise;
    }
  }

  setupSocketConnectPromise = new Promise((resolve, reject) => {
    const urls = getSetupWsUrls();

    // Try the configured endpoint list in order and keep one live socket for
    // all setup commands/events.
    const tryConnect = (urls, index) => {
      const ws = new WebSocket(urls[index]);
      let opened = false;
      setupSocket = ws;

      const connectFailed = () => {
        if (opened) {
          return;
        }
        if (setupSocket === ws) {
          setupSocket = null;
        }
        if (index + 1 < urls.length) {
          tryConnect(urls, index + 1);
        } else {
          setupSocketConnectPromise = null;
          reject(new Error("WebSocket connection failed"));
        }
      };

      ws.onopen = () => {
        opened = true;
        if (reconnectTimer) {
          clearTimeout(reconnectTimer);
          reconnectTimer = 0;
        }
        resolve();
      };

      ws.onmessage = event => {
        try {
          const msg = JSON.parse(event.data);
          handleSetupSocketMessage(msg);
        } catch (error) {
          console.error("Invalid WS message", error);
        }
      };

      ws.onerror = () => {
        if (!opened) {
          connectFailed();
        }
      };

      ws.onclose = () => {
        if (!opened) {
          connectFailed();
          return;
        }

        const reconnectNeeded = !document.hidden;
        if (setupSocket === ws) {
          setupSocket = null;
          setupSocketConnectPromise = null;
          rejectAllPending(new Error("WebSocket disconnected"));
        }
        if (reconnectNeeded) {
          reconnectTimer = window.setTimeout(() => {
            connectSetupSocket().catch(() => {});
          }, 1500);
        }
      };
    };

    tryConnect(urls, 0);
  });

  return setupSocketConnectPromise;
}

async function ensureSetupSocket() {
  let lastError;

  for (let attempt = 0; attempt < 8; attempt++) {
    try {
      await connectSetupSocket();
      if (setupSocket && setupSocket.readyState === WebSocket.OPEN) {
        return;
      }
      throw new Error("WebSocket not ready");
    } catch (error) {
      lastError = error;
      if (attempt < 7) {
        await new Promise(resolve => {
          window.setTimeout(resolve, 350 * (attempt + 1));
        });
      }
    }
  }

  throw lastError || new Error("WebSocket connection failed");
}

async function sendWsCommand(name, payload = {}, timeout = 15000) {
  await ensureSetupSocket();

  if (!setupSocket || setupSocket.readyState !== WebSocket.OPEN) {
    throw new Error("WebSocket not ready");
  }

  const reqId = `${Date.now()}-${++requestSeq}`;
  const envelope = { type: "cmd", reqId, name, payload };

  return new Promise((resolve, reject) => {
    const timer = window.setTimeout(() => {
      pendingRequests.delete(reqId);
      reject(new Error(`Timeout for command ${name}`));
    }, timeout);

    pendingRequests.set(reqId, { resolve, reject, timer, name });
    setupSocket.send(JSON.stringify(envelope));
  });
}

function handleSetupSocketMessage(message) {
  if (!message || typeof message !== "object") return;

  if (message.type === "res" && message.reqId) {
    const pending = pendingRequests.get(message.reqId);
    if (!pending) return;
    clearTimeout(pending.timer);
    pendingRequests.delete(message.reqId);
    if (message.ok) pending.resolve(message.payload || {});
    else pending.reject(new Error(message.error || `Command failed: ${pending.name}`));
    return;
  }

  if (message.type === "evt") {
    handleSetupSocketEvent(message.name, message.payload || {});
  }
}

function applyRuntimeNetworkPayload(payload) {
  if (payload.ip) {
    esp = `${location.protocol}//${payload.ip}:${port}/`;
  }
  if (payload.hostname && $("hostname")) {
    $("hostname").value = payload.hostname;
  }
}

const setupSocketEventHandlers = {
  "wifi.connect.started": payload => {
    show("loader");
    openModal("WiFi", `Connecting to <b>${payload.ssid || $("ssid").value || "WiFi"}</b>...`);
  },
  "wifi.connect.success": payload => {
    hide("loader");
    applyRuntimeNetworkPayload(payload);
    if (payload.body && payload.body.includes("action-restart-required")) {
      openModal("WiFi", payload.body, restartESP);
    } else {
      openModal("WiFi", payload.body || "Connected");
    }
    refreshRuntimeStatus();
    loadCredentials();
  },
  "wifi.connect.error": payload => {
    hide("loader");
    openModal("WiFi", payload.body || "Connection failed");
    refreshRuntimeStatus();
  }
};

function handleSetupSocketEvent(name, payload) {
  const handler = setupSocketEventHandlers[name];
  if (handler) {
    handler(payload);
  }
}

const confirmRestart = () => openModal("Restart", "Restart now?", restartESP);

async function restartESP() {
  try {
    await sendWsCommand("device.restart", {}, 5000);
  } catch (_) {
  }
  closeModal(false);
  openModal("Restarted", "ESP restarted");
}

function setupHeaderLayout() {
  const move = () => {
    const h = document.querySelector("header.ctn"), t = h?.querySelector(".title"), hg = t?.querySelector(".hd"), n = $("top-nav");
    if (!h || !hg || !n) return;
    if (window.innerWidth <= 608) {
      if (n.parentNode !== h) h.insertBefore(n, t.nextSibling);
      n.classList.remove("inline-header");
    } else {
      if (n.parentNode !== hg) hg.appendChild(n);
      n.classList.add("inline-header");
    }
  };
  move();
  window.addEventListener("resize", move);
}

function applyIcons() {
  const map = {
    "svg-menu": ICONS.menu, "svg-eye": ICONS.eye, "svg-no-eye": ICONS.noEye,
    "svg-scan": ICONS.scan, "svg-connect": ICONS.connect, "svg-save": ICONS.save,
    "svg-restart": ICONS.restart, "svg-delete": ICONS.del,
    "svg-clear": ICONS.clear, "svg-update": ICONS.upload
  };
  for (const [id, icon] of Object.entries(map)) {
    const el = $(id);
    if (el) el.textContent = icon;
  }
}

function applyStatusHeader(data) {
  if (!data) return;
  if ($("esp-mode")) $("esp-mode").textContent = data.mode || "";
  if ($("firmware")) $("firmware").textContent = data.firmware || "";
  if ($("about")) {
    $("about").href = data.liburl || "#";
    $("about").textContent = data.liburl ? `Created with ${data.liburl}` : "";
  }
  if ($("esp-ip")) {
    const host = data.hostname || location.hostname;
    $("esp-ip").innerHTML =
      `<a href="http://${host}.local/">http://${host}.local</a>` +
      `<a href="${esp}"> (${data.ip || ""})</a>`;
  }

  configPath = data.path;
  if ($("hostname")) $("hostname").value = data.hostname || "";
  if ($("ssid-name")) $("ssid-name").textContent = "SSID";
}

function applyMetaFromConfig(cfg) {
  if (!cfg || typeof cfg !== "object") return;
  const meta = cfg._meta || {};

  const logoImg = $("img-logo")?.querySelector("img");
  if (logoImg && !logoImg.src) {
    logoImg.src = meta.logo || "config/logo.svg";
    $("img-logo").classList.remove("loading");
  }

  if (meta.app_title) {
    const safe = meta.app_title.replace(/(<([^>]+)>)/ig, "");
    $("page-title").textContent = safe;
    document.title = safe;
  }

  if ($("port") && typeof meta.port === "number") {
    $("port").value = meta.port;
    show("port-w");
  }
}

function loadAssets(assets) {
  if (!assets || typeof assets !== "object") return;

  (assets.css || []).forEach(href => {
    if (loadedAssets.css.has(href)) return;
    loadedAssets.css.add(href);
    document.head.appendChild(newEl("link", { rel: "stylesheet", href }));
  });

  (assets.js || []).forEach(src => {
    if (loadedAssets.js.has(src)) return;
    loadedAssets.js.add(src);
    document.body.appendChild(newEl("script", { src }));
  });
}

function clearDynamicContent() {
  document.querySelectorAll(".box.dyn").forEach(el => el.remove());
  document.querySelectorAll("#nav-link a[data-dynamic='1']").forEach(el => el.remove());
}

function slugify(str) {
  return String(str || "")
    .toLowerCase()
    .trim()
    .replace(/[^a-z0-9]+/g, "-")
    .replace(/^-+|-+$/g, "");
}

function buildElement(el, secIdx, elIdx, container) {
  const base = slugify(el.label);
  const id = $(base) ? `${base}-${elIdx}` : base;
  let item;
  let inp;

  if (el.type === "html") {
    item = newEl("div");
    if (el.value) fetch(el.value).then(r => r.text()).then(t => { item.innerHTML = t; }).catch(() => { item.textContent = "Error"; });
  } else if (el.type === "boolean") {
    const wrap = newEl("div", { class: "tbw" });
    item = newEl("label", { class: "lbl tgl", for: id });
    inp = newEl("input", { id, type: "checkbox", class: "tc in" });
    inp.checked = !!el.value;
    item.append(inp, newEl("div", { class: "tsw" }), Object.assign(newEl("span", { class: "tlbl" }), { textContent: el.label || "" }));
    wrap.appendChild(item);
    item = wrap;
  } else {
    item = newEl("div", { class: "tw" });
    item.appendChild(Object.assign(newEl("label", { class: "lbl", for: id }), { textContent: el.label || "" }));

    if (el.type === "select") {
      inp = newEl("select", { id, class: "in" });
      (el.options || []).forEach(option => inp.add(new Option(option, option, false, String(option) === String(el.value))));
      item.appendChild(inp);
    } else if (el.type === "slider") {
      const g = newEl("div", { class: "slw" });
      inp = newEl("input", { id, type: "range", class: "in sl", min: el.min ?? 0, max: el.max ?? 100, step: el.step ?? 1 });
      inp.value = el.value ?? el.min ?? 0;
      const ro = newEl("input", { id: `${id}-readout`, type: "number", class: "in slr", min: inp.min, max: inp.max, step: inp.step });
      ro.value = inp.value;
      ro.dataset.secIndex = secIdx;
      ro.dataset.elIndex = elIdx;
      g.append(inp, ro);
      item.appendChild(g);
    } else {
      const type = el.type === "number" ? "number" : "text";
      inp = newEl("input", { id, type, class: "in" });
      if (type === "number") {
        if (el.min != null) inp.min = el.min;
        if (el.max != null) inp.max = el.max;
        if (el.step != null) inp.step = el.step;
      }
      inp.value = el.value ?? "";
      item.appendChild(inp);
    }
  }

  if (inp) {
    inp.dataset.secIndex = secIdx;
    inp.dataset.elIndex = elIdx;
  }
  if (el.hidden) item.classList.add("hide");
  if (el.comment) {
    if (el.type === "boolean") {
      const span = newEl("span", { class: "cmt inline" });
      span.textContent = el.comment;
      item.appendChild(span);
    } else {
      const cdiv = newEl("div", { class: "cmt" });
      cdiv.textContent = el.comment;
      item.appendChild(cdiv);
    }
  }
  container.appendChild(item);
}

function buildSectionsFromConfig() {
  clearDynamicContent();
  if (!config || !Array.isArray(config.sections)) return;

  // Custom options are described by config.json and rendered entirely on the
  // client so the same setup page can serve different projects.
  const main = $("main");
  const before = $("btn-hr");

  config.sections.forEach((section, sIdx) => {
    const boxId = `option-box-${sIdx}`;
    const box = newEl("div", { class: "ctn box dyn hide", id: boxId });
    const h2 = newEl("h2");
    h2.textContent = section.title || `Options ${sIdx + 1}`;
    box.appendChild(h2);
    main.insertBefore(box, before);

    const navLink = newEl("a", {
      class: "a-link",
      id: `set-opt-${sIdx}`,
      "data-box": boxId,
      "data-dynamic": "1"
    });
    navLink.textContent = section.title || `Options ${sIdx + 1}`;
    navLink.onclick = switchPage;
    $("nav-link").appendChild(navLink);

    let toggleContainer = null;
    (section.elements || []).forEach((el, eIdx) => {
      const shouldGroup = el.type === "boolean" && (el.group !== false);
      if (shouldGroup && toggleContainer === null) {
        toggleContainer = newEl("div", { class: "tglw" });
        box.appendChild(toggleContainer);
      }
      buildElement(el, sIdx, eIdx, shouldGroup ? toggleContainer : box);
    });
  });
}

function handleOptionChange(e) {
  const t = e.target;
  if (!t.classList.contains("in")) return;

  // Slider readouts mirror the real input, so map them back to the slider
  // before updating the in-memory config model.
  let base = t;
  if (t.id && t.id.endsWith("-readout")) {
    const slider = $(t.id.replace(/-readout$/, ""));
    if (slider) base = slider;
  }

  const sIdx = base.dataset.secIndex;
  const eIdx = base.dataset.elIndex;
  if (sIdx == null || eIdx == null || !config || !Array.isArray(config.sections)) return;

  const sec = config.sections[Number(sIdx)] || {};
  const el = (sec.elements || [])[Number(eIdx)];
  if (!el) return;

  if (el.type === "boolean") el.value = !!base.checked;
  else if (el.type === "slider" || el.type === "number") el.value = Number(base.value);
  else el.value = base.value;
}

function handleOptionInput(e) {
  const t = e.target;
  if (t.type === "range" && t.classList.contains("in")) {
    const ro = $(`${t.id}-readout`);
    if (ro) ro.value = t.value;
  }
  handleOptionChange(e);
}

function hasStoredCredentialFor(ssid) {
  return wifiCredentials.some(c => c.ssid === ssid);
}

function applyCredential(cred) {
  if (!cred) return;
  $("ssid").value = cred.ssid || "";
  $("ssid-name").textContent = cred.ssid || "SSID";

  const isStatic = cred.ip && cred.ip !== "0.0.0.0";
  $("dhcp").checked = !isStatic;

  if (isStatic) {
    show("conf-wifi");
    $("ip").value = cred.ip || "";
    $("gateway").value = cred.gateway || "";
    $("subnet").value = cred.subnet || "";
    $("dns1").value = cred.dns1 || "";
    $("dns2").value = cred.dns2 || "";
  } else {
    hide("conf-wifi");
    $("ip").value = "";
    $("gateway").value = "";
    $("subnet").value = "";
    $("dns1").value = "";
    $("dns2").value = "";
  }

  $("password").value = "";
}

function updateCredentialActionVisibility() {
  const hasCredentials = wifiCredentials.length > 0;
  ["delete-cred", "clear-creds", "cred-actions-inline"].forEach(id => {
    const elem = $(id);
    if (!elem) return;
    elem.classList.toggle("hide", !hasCredentials);
  });
}

function applySelectedCredential() {
  if (selectedCredentialIndex < 0 || selectedCredentialIndex >= wifiCredentials.length) return;
  applyCredential(wifiCredentials[selectedCredentialIndex]);
}

function syncCredentials(credentials, preferredIndex = selectedCredentialIndex) {
  wifiCredentials = credentials || [];
  selectedCredentialIndex = wifiCredentials.length
    ? Math.min(preferredIndex >= 0 ? preferredIndex : 0, wifiCredentials.length - 1)
    : -1;
  renderCredentialTabs();
  applySelectedCredential();
}

function renderCredentialTabs() {
  const credTabs = $("wifi-cred-tabs");
  if (!credTabs) return;

  credTabs.innerHTML = "";
  const wifiBoxWidth = $("wifi-box").offsetWidth || Math.min(window.innerWidth, 840);
  const maxTabsPerRow = Math.floor((wifiBoxWidth - 60) / 140);

  if (window.matchMedia("(max-width: 608px)").matches || wifiCredentials.length > maxTabsPerRow) {
    const select = newEl("select", { class: "in", style: "margin-bottom:10px" });
    wifiCredentials.forEach((cred, idx) => {
      const option = newEl("option", { value: idx });
      option.textContent = cred.ssid || "(empty SSID)";
      if (idx === selectedCredentialIndex) option.selected = true;
      select.appendChild(option);
    });
    select.addEventListener("change", event => {
      selectedCredentialIndex = parseInt(event.target.value, 10);
      applySelectedCredential();
    });
    credTabs.appendChild(select);
  } else {
    wifiCredentials.forEach((cred, idx) => {
      const tabBtn = newEl("button", { class: "tab" });
      tabBtn.textContent = cred.ssid || "(empty SSID)";
      if (idx === selectedCredentialIndex) tabBtn.classList.add("active");
      tabBtn.onclick = event => {
        credTabs.querySelectorAll(".tab").forEach(tab => tab.classList.remove("active"));
        event.target.classList.add("active");
        selectedCredentialIndex = idx;
        applyCredential(cred);
      };
      credTabs.appendChild(tabBtn);
    });
  }

  updateCredentialActionVisibility();
}

window.addEventListener("resize", () => {
  if (wifiCredentials.length) renderCredentialTabs();
});

async function loadCredentials() {
  try {
    const payload = await sendWsCommand("credentials.get");
    syncCredentials(payload.credentials);
  } catch (error) {
    console.error(error);
  }
}

function deleteSelectedCredential() {
  if (selectedCredentialIndex < 0 || selectedCredentialIndex >= wifiCredentials.length) return;
  const cred = wifiCredentials[selectedCredentialIndex];
  openModal("Delete WiFi", `Delete <b>${cred.ssid}</b>?`, async () => {
    try {
      const payload = await sendWsCommand("credentials.delete", { index: selectedCredentialIndex });
      syncCredentials(payload.credentials, 0);
      closeModal(false);
    } catch (_) {
      openModal("Error", "Failed to delete");
    }
  });
}

function clearAllCredentials() {
  if (!wifiCredentials.length) return;
  openModal("Clear All", "Delete all WiFi credentials?", async () => {
    try {
      const payload = await sendWsCommand("credentials.clear");
      syncCredentials(payload.credentials, -1);
      closeModal(false);
    } catch (_) {
      openModal("Error", "Failed to clear");
    }
  });
}

function renderWiFiNetworks(networks) {
  const list = $("w-lst");
  list.innerHTML = "";
  const frag = document.createDocumentFragment();

  networks.sort((a, b) => b.strength - a.strength);
  networks.forEach((net, i) => {
    const row = newEl("tr", { id: `wifi-${i}` });
    row.innerHTML =
      `<td><input type="radio" name="select" id="select-wifi-${i}"></td>` +
      `<td>${net.ssid}</td>` +
      `<td class="hide-tiny">${net.strength} dBm</td>` +
      `<td>${net.security ? ICONS.lock : ICONS.unlock}</td>`;
    row.onclick = () => {
      $("ssid").value = net.ssid;
      $("ssid-name").textContent = net.ssid;
      $("password").focus();
      try { $(`select-wifi-${i}`).checked = true; } catch (_) {}
      $("w-tbl").classList.add("hide");
      show("show-networks");
    };
    frag.appendChild(row);
  });

  list.appendChild(frag);
  show("w-tbl");
}

async function getWiFiList() {
  const btn = $("scan-wifi");
  btn.classList.add("load");
  try {
    const payload = await sendWsCommand("wifi.scan", {}, 10000);
    if (payload.reload) {
      window.setTimeout(getWiFiList, 1500);
      return;
    }
    renderWiFiNetworks(payload.networks || []);
  } catch (error) {
    console.error(error);
  } finally {
    btn.classList.remove("load");
    hide("loader");
  }
}

function buildConnectPayload(confirmed) {
  // Mirror the setup websocket command envelope expected by the firmware.
  const payload = {
    ssid: $("ssid").value.trim(),
    password: $("password").value,
    persistent: $("persistent").checked,
    dhcp: $("dhcp").checked,
    confirmed: !!confirmed
  };

  const host = $("hostname") ? $("hostname").value.trim() : "";
  if (host) payload.hostname = host;

  if (!payload.dhcp) {
    payload.ip_address = $("ip").value.trim();
    payload.gateway = $("gateway").value.trim();
    payload.subnet = $("subnet").value.trim();
    const dns1 = $("dns1").value.trim();
    const dns2 = $("dns2").value.trim();
    if (dns1) payload.dns1 = dns1;
    if (dns2) payload.dns2 = dns2;
  }

  return payload;
}

async function doConnection(skipConfirm) {
  const ssid = $("ssid").value.trim();
  const pass = $("password").value;
  if (!ssid || (!pass && !hasStoredCredentialFor(ssid))) {
    openModal("WiFi", "Insert SSID & Password");
    return;
  }

  show("loader");
  try {
    // The firmware may ask for an explicit confirmation when a live station
    // connection is about to be replaced.
    const payload = await sendWsCommand("wifi.connect", buildConnectPayload(skipConfirm), 10000);
    if (payload.confirmRequired) {
      hide("loader");
      openModal(
        "WiFi",
        `You are about to apply new WiFi configuration for <b>${payload.ssid || ssid}</b>.<br><br><i>This page may lose connection during reconfiguration.</i><br><br>Do you want to proceed?`,
        () => doConnection(true)
      );
      return;
    }
    if (payload.accepted) {
      openModal("WiFi", `Applying configuration for <b>${ssid}</b>...`);
    }
  } catch (error) {
    hide("loader");
    openModal("Error", error.message || "Connection failed");
  }
}

function handleUpdate() {
  const file = $("file-input").files[0];
  if (!file) return;

  show("loader");
  show("pgr-wrap");
  $("pgr-wrap").classList.add("active");
  $("update-log").textContent = "Updating...";

  const form = new FormData();
  form.append("update", file);
  const xhr = new XMLHttpRequest();
  xhr.open("POST", `/update?size=${file.size}`);

  xhr.onload = () => {
    hide("loader");
    $("pgr-wrap").classList.remove("active");
    $("update-log").textContent = xhr.status === 200 ? xhr.responseText : "Error";

    const input = $("file-input");
    const lbl = $("fw-lbl");
    if (input) input.value = "";
    if (lbl) {
      lbl.textContent = lbl.dataset.defaultText || "Select firmware binary file";
      lbl.removeAttribute("title");
      lbl.style.background = "";
    }
    if ($("update-btn")) $("update-btn").setAttribute("aria-disabled", "true");
    if ($("pgr-anim")) $("pgr-anim").style.width = "0%";
    hide("pgr-wrap");
  };

  xhr.upload.onprogress = progress => {
    if (progress.lengthComputable && $("pgr-anim")) {
      $("pgr-anim").style.width = `${Math.round((progress.loaded / progress.total) * 100)}%`;
    }
  };

  xhr.send(form);
}

async function uploadFolder(e) {
  const files = Array.from(e.target.files || []);
  if (!files.length) return;

  const list = $("listing");
  const label = $("pick-lbl");
  if (list) list.innerHTML = "";

  if (label) {
    label.setAttribute("aria-disabled", "true");
    label.style.background = "brown";
    const txt = label.dataset.defaultText || label.textContent;
    label.textContent = `${txt} (${files.length} file${files.length > 1 ? "s" : ""} selected)`;
  }

  let ok = 0;
  let fail = 0;

  for (const file of files) {
    const path = file.webkitRelativePath.replace(/^data\//, "");
    const li = newEl("li");
    li.textContent = path;
    if (list) list.appendChild(li);

    const form = new FormData();
    form.append("data", file, `/${path}`);

    try {
      const res = await fetch("/edit", { method: "POST", body: form });
      if (!res.ok) {
        fail++;
        li.style.color = "red";
      } else {
        ok++;
      }
    } catch (_) {
      fail++;
      li.style.color = "red";
    }
  }

  if (e.target) e.target.value = "";
  if (label) {
    label.textContent = label.dataset.defaultText || "Select the folder containing your files";
    label.style.background = "";
    label.removeAttribute("aria-disabled");
  }

  openModal("FS Upload", `<br>Uploaded <b>${ok}</b> file${ok !== 1 ? "s" : ""}${fail ? `, <b>${fail}</b> failed` : " successfully"}.`);
}

function switchPage(e) {
  const t = e.target;
  const boxId = t.getAttribute("data-box");

  $("top-nav").classList.remove("responsive");
  document.querySelectorAll("a").forEach(a => a.classList.remove("active"));
  t.classList.add("active");
  document.querySelectorAll(".box").forEach(b => b.classList.add("hide"));
  show(boxId);

  if (t.id !== "set-wifi") {
    const box = $(boxId);
    box.appendChild($("btn-hr"));
    box.appendChild($("btn-box"));
    show("btn-box");
    show("btn-hr");
  } else {
    hide("btn-box");
    hide("btn-hr");
  }
}

async function saveParameters() {
  if (!config) return;

  if ($("port") && config._meta) {
    const v = parseInt($("port").value, 10);
    if (!Number.isNaN(v)) config._meta.port = v;
  }

  try {
    const payload = await sendWsCommand("config.save", { config }, 10000);
    if (payload.config) {
      config = payload.config;
    }
    openModal("Save", `<br>Saved config to <b>/${configPath}</b>`);
  } catch (error) {
    openModal("Error", `Save failed: ${error.message || error}`);
  }
}

async function refreshRuntimeStatus() {
  try {
    const payload = await sendWsCommand("status.get");
    applyRuntimeNetworkPayload(payload.status || {});
    applyStatusHeader(payload.status || {});
  } catch (error) {
    console.error(error);
  }
}

async function bootstrapSetupPage() {
  show("loader");
  try {
    // Load runtime status first, then config-driven UI, then stored WiFi
    // credentials so the page renders with the current device state.
    await ensureSetupSocket();
    const statusPayload = await sendWsCommand("status.get");
    applyRuntimeNetworkPayload(statusPayload.status || {});
    applyStatusHeader(statusPayload.status || {});

    const configPayload = await sendWsCommand("config.get");
    config = configPayload.config || {};
    applyMetaFromConfig(config);
    loadAssets(config._assets || {});
    buildSectionsFromConfig();
    await loadCredentials();
  } finally {
    hide("loader");
    document.body.classList.remove("boot-loading");
  }
}

function toggleNetworkList() {
  $("w-tbl").classList.toggle("hide");
  $("show-networks").classList.toggle("hide");
}

function togglePasswordVisibility() {
  const inp = $("password");
  const isPass = inp.type === "password";
  inp.type = isPass ? "text" : "password";
  if (isPass) {
    show("show-pass");
    hide("hide-pass");
  } else {
    hide("show-pass");
    show("hide-pass");
  }
}

function syncSsidLabel() {
  const v = $("ssid").value.trim();
  $("ssid-name").textContent = v || "SSID";
}

function toggleDhcpConfig() {
  if ($("dhcp").checked) hide("conf-wifi");
  else show("conf-wifi");
}

function primeFileInputLabel() {
  const lbl = $("fw-lbl");
  const input = $("file-input");
  if (!input || !lbl) return;
  if (!input.files || !input.files.length) return;
  const f = input.files[0];
  lbl.textContent = `${f.name} (${f.size} bytes)`;
  lbl.title = f.name;
  lbl.style.background = "brown";
  if ($("update-btn")) $("update-btn").classList.remove("load");
}

function initializeDefaultLabelText(id) {
  const el = $(id);
  if (el && !el.dataset.defaultText) el.dataset.defaultText = el.textContent;
}

function bindUiActions() {
  // Keep DOM lookups local to startup and bind only controls that exist in the
  // current setup variant.
  [
    ["hum-btn", () => $("top-nav").classList.toggle("responsive")],
    ["scan-wifi", getWiFiList],
    ["connect-wifi", () => doConnection(false)],
    ["save-params", saveParameters],
    ["set-wifi", switchPage],
    ["set-update", switchPage],
    ["restart", confirmRestart],
    ["update-btn", handleUpdate],
    ["ok-modal", () => closeModal(true)],
    ["close-modal", () => closeModal(false)],
    ["delete-cred", deleteSelectedCredential],
    ["clear-creds", clearAllCredentials],
    ["show-networks", toggleNetworkList],
    ["show-hide-password", togglePasswordVisibility]
  ].forEach(([id, handler]) => bindIfPresent(id, "click", handler));

  bindIfPresent("picker", "change", uploadFolder);
  bindIfPresent("ssid", "input", syncSsidLabel);
  bindIfPresent("dhcp", "change", toggleDhcpConfig);
  bindIfPresent("file-input", "change", primeFileInputLabel);
}

window.addEventListener("load", () => {
  const main = $("main");
  if (main) {
    // Dynamic config controls are created after bootstrap, so delegate change
    // handling from the main container.
    main.addEventListener("change", handleOptionChange);
    main.addEventListener("input", handleOptionInput);
  }

  bindUiActions();

  initializeDefaultLabelText("fw-lbl");
  initializeDefaultLabelText("pick-lbl");

  if ($("update-btn")) $("update-btn").classList.add("load");

  applyIcons();
  setupHeaderLayout();
  bootstrapSetupPage().catch(error => {
    console.error(error);
    hide("loader");
    document.body.classList.remove("boot-loading");
    openModal("Setup", "Unable to initialize setup websocket.");
  });
});