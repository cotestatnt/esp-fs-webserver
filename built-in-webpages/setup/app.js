// Setup page v2 - simplified and clean
const ICONS = {
  lock: "🔒", unlock: "🔓", scan: "📡", connect: "📶",
  save: "💾", restart: "↻", eye: "🐵", noEye: "🙈",
  menu: "☰", del: "🗑️", clear: "✖", upload: "✅"
};

const $ = id => document.getElementById(id);
const hide = id => { const el = $(id); if (el) el.classList.add("hide"); };
const show = id => { const el = $(id); if (el) el.classList.remove("hide"); };
const newEl = (tag, attrs = {}) => {
  const el = document.createElement(tag);
  for (const [k, v] of Object.entries(attrs)) el.setAttribute(k, v);
  return el;
};

const port = location.port || (location.protocol === "https:" ? "443" : "80");
let esp = `${location.protocol}//${location.hostname}:${port}/`;  // Will be updated with IP after getStatus

let config, configPath;
let wifiCredentials = [], selectedCredentialIndex = -1;
let modalCloseCb;
const loadedAssets = { css: new Set(), js: new Set(), html: new Set() };

// Modal
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

const confirmRestart = () => openModal("Restart", "Restart now?", restartESP);
const restartESP = () => fetch(`${esp}reset`).then(() => {
  closeModal(false);
  openModal("Restarted", "ESP restarted");
});

// Header responsive layout
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
  move(); window.onresize = move;
}

function applyIcons() {
  const map = {
    "svg-menu": ICONS.menu, "svg-eye": ICONS.eye, "svg-no-eye": ICONS.noEye,
    "svg-scan": ICONS.scan, "svg-connect": ICONS.connect, "svg-save": ICONS.save,
    "svg-restart": ICONS.restart, "svg-delete": ICONS.del,
    "svg-clear": ICONS.clear, "svg-update": ICONS.upload
  };
  for (const [id, ico] of Object.entries(map)) {
    const el = $(id);
    if (el) el.textContent = ico;
  }
}

// Apply status data from /getStatus
function applyStatusHeader(data) {
  if ($("esp-mode")) $("esp-mode").textContent = data.mode || "";
  if ($("firmware")) $("firmware").textContent = data.firmware || "";
  if ($("about")) {
    $("about").href = data.liburl || "#";
    $("about").textContent = data.liburl ? "Created with " + data.liburl : "";
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

// Apply config v2 metadata
function applyMetaFromConfig(cfg) {
  console.log(cfg)
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

// Load external assets (CSS, JS, HTML)
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

// Build a single option element
function buildElement(el, secIdx, elIdx, container) {
  const id = `opt-${secIdx}-${elIdx}`;
  let item, inp;

  if (el.type === "html") {
    item = newEl("div");
    if (el.value) fetch(el.value).then(r=>r.text()).then(t=>item.innerHTML=t).catch(()=>item.textContent="Error");
  } else if (el.type === "boolean") {
    // wrap label/switch inside dedicated .tbw so it can be styled independently
    const wrap = newEl("div", { class: "tbw" });
    item = newEl("label", { class: "lbl tgl", for: id });
    inp = newEl("input", { id, type: "checkbox", class: "tc in" });
    inp.checked = !!el.value;
    item.append(inp, newEl("div", { class: "tsw" }), Object.assign(newEl("span", { class: "tlbl" }), { textContent: el.label||"" }));
    wrap.appendChild(item);
    item = wrap;
  } else {
    item = newEl("div", { class: "tw" });
    item.appendChild(Object.assign(newEl("label", { class: "lbl", for: id }), { textContent: el.label||"" }));
    
    if (el.type === "select") {
      inp = newEl("select", { id, class: "in" });
      (el.options||[]).forEach(o => inp.add(new Option(o, o, false, String(o)===String(el.value))));
      item.appendChild(inp);
    } else if (el.type === "slider") {
      const g = newEl("div", { class: "slw" });
      inp = newEl("input", { id, type: "range", class: "in sl", min: el.min??0, max: el.max??100, step: el.step??1 });
      inp.value = el.value ?? el.min ?? 0;
      const ro = newEl("input", { id: id+"-readout", type: "number", class: "in slr", min: inp.min, max: inp.max, step: inp.step });
      ro.value = inp.value;
      ro.dataset.secIndex = secIdx; ro.dataset.elIndex = elIdx;
      g.append(inp, ro);
      item.appendChild(g);
    } else {
      const type = el.type === "number" ? "number" : "text";
      inp = newEl("input", { id, type, class: "in" });
      if(type==="number"){ if(el.min!=null)inp.min=el.min; if(el.max!=null)inp.max=el.max; if(el.step!=null)inp.step=el.step; }
      inp.value = el.value ?? "";
      item.appendChild(inp);
    }
  }

  if (inp) { inp.dataset.secIndex = secIdx; inp.dataset.elIndex = elIdx; }
  if (el.hidden) item.classList.add("hide");
  container.appendChild(item);
}

// Build all sections from config v2
function buildSectionsFromConfig() {
  clearDynamicContent();
  if (!config || !Array.isArray(config.sections)) return;

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

    // Optionally create a flex container for boolean elements when grouping is desired
    let toggleContainer = null;

    (section.elements || []).forEach((el, eIdx) => {
      // check element-specific group flag (default true for booleans)
      const shouldGroup = el.type === "boolean" && (el.group !== false);
      if (shouldGroup && toggleContainer === null) {
        toggleContainer = newEl("div", { class: "tglw" });
        box.appendChild(toggleContainer);
      }
      const container = shouldGroup ? toggleContainer : box;
      buildElement(el, sIdx, eIdx, container);
    });
  });
}

// Handle option value changes
function handleOptionChange(e) {
  const t = e.target;
  if (!t.classList.contains("in")) return;

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
  else if (el.type === "select") el.value = base.value;
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

// WiFi credentials management
async function loadCredentials() {
  try {
    const res = await fetch(`${esp}wifi/credentials?_=${Date.now()}`);
    if (!res.ok) return;
    
    wifiCredentials = await res.json();
    selectedCredentialIndex = wifiCredentials.length ? 0 : -1;

    if (wifiCredentials.length > 1) {
      if (typeof renderCredentialTabs === "undefined") {
        const script = document.createElement("script");
        script.src = "creds.js";
        script.onload = () => window.renderCredentialTabs && window.renderCredentialTabs();
        document.body.appendChild(script);
      } else {
        window.renderCredentialTabs();
      }
    } else {
      if ($("wifi-cred-tabs")) $("wifi-cred-tabs").innerHTML = "";
      ["delete-cred", "clear-creds", "cred-actions-inline"].forEach(hide);
    }

    if (selectedCredentialIndex >= 0 && wifiCredentials[0]) {
      window.applyCredential(wifiCredentials[0]);
    }
  } catch (e) {
    console.error(e);
  }
}

function hasStoredCredentialFor(ssid) {
  return wifiCredentials.some(c => c.ssid === ssid);
}

window.applyCredential = cred => {
  if (!cred) return;
  $("ssid").value = cred.ssid || "";
  $("ssid-name").textContent = cred.ssid || "SSID";

  const isStatic = cred.ip && cred.ip !== "0.0.0.0";
  $("dhcp").checked = !isStatic;

  if (isStatic) {
    show("conf-wifi");
    $("ip").value = cred.ip;
    $("gateway").value = cred.gateway;
    $("subnet").value = cred.subnet;
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
};

// WiFi scan
function getWiFiList() {
  const btn = $("scan-wifi");
  btn.classList.add("load");

  fetch(`${esp}scan`)
    .then(r => r.json())
    .then(data => {
      if (data.reload) setTimeout(getWiFiList, 2000);
      data.sort((a, b) => b.strength - a.strength);

      const list = $("w-lst");
      list.innerHTML = "";
      const frag = document.createDocumentFragment();

      data.forEach((net, i) => {
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
          try { $(`select-wifi-${i}`).checked = true; } catch (e) { }
          $("w-tbl").classList.add("hide");
          show("show-networks");
        };
        frag.appendChild(row);
      });
      
      list.appendChild(frag);
      show("w-tbl");
      btn.classList.remove("load");
    })
    .catch(() => hide("loader"));
}

// WiFi connect
function doConnection(e, skipConfirm) {
  const ssid = $("ssid").value;
  const pass = $("password").value;
  if (!ssid || (!pass && !hasStoredCredentialFor(ssid)))
    return openModal("WiFi", "Insert SSID & Password");

  const f = new FormData();
  f.append("ssid", ssid);
  f.append("password", pass);
  f.append("persistent", $("persistent").checked);
  
  const host = $("hostname") ? $("hostname").value.trim() : "";
  if (host) f.append("hostname", host);
  
  if (!$("dhcp").checked) {
    f.append("ip_address", $("ip").value);
    f.append("gateway", $("gateway").value);
    f.append("subnet", $("subnet").value);
    const dns1 = $("dns1").value.trim();
    const dns2 = $("dns2").value.trim();
    if (dns1) f.append("dns1", dns1);
    if (dns2) f.append("dns2", dns2);
  }
  
  if (skipConfirm) f.append("confirmed", "true");

  show("loader");
  fetch("/connect", { method: "POST", body: f })
    .then(r => r.text().then(msg => {
      hide("loader");
      if (r.status !== 200) {
        openModal("Error", msg);
      } else if (msg.includes("action-confirm-sta-change")) {
        openModal("WiFi", msg, () => doConnection(e, true));
      } else if (msg.includes("action-restart-required")) {
        openModal("WiFi", msg, restartESP);
      } else if (msg.includes("action-async-applying")) {
        openModal("WiFi", msg);
      }
    }));
}

// Firmware update
function handleUpdate() {
  const file = $("file-input").files[0];
  if (!file) return;

  show("loader");
  show("pgr-wrap");
  $("pgr-wrap").classList.add("active");
  $("update-log").textContent = "Updating...";

  const f = new FormData();
  f.append("update", file);
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
  
  xhr.upload.onprogress = p => {
    if (p.lengthComputable && $("pgr-anim"))
      $("pgr-anim").style.width = `${Math.round((p.loaded / p.total) * 100)}%`;
  };
  
  xhr.send(f);
}

// FS upload
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

  let ok = 0, fail = 0;

  for (const file of files) {
    const path = file.webkitRelativePath.replace(/^data\//, "");
    const li = newEl("li");
    li.textContent = path;
    if (list) list.appendChild(li);

    const f = new FormData();
    f.append("data", file, "/" + path);

    try {
      const res = await fetch("/edit", { method: "POST", body: f });
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

  const msg = `<br>Uploaded <b>${ok}</b> file${ok !== 1 ? "s" : ""}${fail ? `, <b>${fail}</b> failed` : " successfully"}.`;
  openModal("FS Upload", msg);
}

// Page navigation
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

// Save config
function saveParameters() {
  if (!config || !configPath) return;

  if ($("port") && config._meta) {
    const v = parseInt($("port").value, 10);
    if (!Number.isNaN(v)) config._meta.port = v;
  }

  const blob = new Blob([JSON.stringify(config, null, 2)], { type: "application/json" });
  const form = new FormData();
  form.append("data", blob, "/" + configPath);

  fetch("/edit", { method: "POST", body: form })
    .then(() => openModal("Save", `<br>Saved config to <b>/${configPath}</b>`))
    .catch(e => openModal("Error", `Save failed: ${e}`));
}

// Initialize on page load
window.addEventListener("load", () => {
  const main = $("main");
  if (main) {
    main.addEventListener("change", handleOptionChange);
    main.addEventListener("input", handleOptionInput);
  }

  // Button handlers
  if ($("hum-btn")) $("hum-btn").onclick = () => $("top-nav").classList.toggle("responsive");
  if ($("scan-wifi")) $("scan-wifi").onclick = getWiFiList;
  if ($("connect-wifi")) $("connect-wifi").onclick = e => doConnection(e, false);
  if ($("save-params")) $("save-params").onclick = saveParameters;
  if ($("set-wifi")) $("set-wifi").onclick = switchPage;
  if ($("set-update")) $("set-update").onclick = switchPage;
  if ($("restart")) $("restart").onclick = confirmRestart;
  if ($("update-btn")) $("update-btn").onclick = handleUpdate;
  if ($("ok-modal")) $("ok-modal").onclick = () => closeModal(true);
  if ($("close-modal")) $("close-modal").onclick = () => closeModal(false);
  if ($("picker")) $("picker").onchange = uploadFolder;

  if ($("show-networks")) {
    $("show-networks").onclick = () => {
      $("w-tbl").classList.toggle("hide");
      $("show-networks").classList.toggle("hide");
    };
  }

  if ($("show-hide-password")) {
    $("show-hide-password").onclick = () => {
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
    };
  }

  if ($("ssid") && $("ssid-name")) {
    $("ssid").addEventListener("input", () => {
      const v = $("ssid").value.trim();
      $("ssid-name").textContent = v || "SSID";
    });
  }

  if ($("dhcp")) {
    $("dhcp").onchange = function () {
      if (this.checked) hide("conf-wifi");
      else show("conf-wifi");
    };
  }

  // Store default text for labels
  const fwLabel = $("fw-lbl");
  if (fwLabel && !fwLabel.dataset.defaultText)
    fwLabel.dataset.defaultText = fwLabel.textContent;
  
  const pickLabel = $("pick-lbl");
  if (pickLabel && !pickLabel.dataset.defaultText)
    pickLabel.dataset.defaultText = pickLabel.textContent;

  if ($("update-btn")) $("update-btn").classList.add("load");
  
  if ($("file-input")) {
    $("file-input").onchange = function () {
      const lbl = $("fw-lbl");
      if (!this.files || !this.files.length || !lbl) return;
      const f = this.files[0];
      lbl.textContent = `${f.name} (${f.size} bytes)`;
      lbl.title = f.name;
      lbl.style.background = "brown";
      if ($("update-btn")) $("update-btn").classList.remove("load");
    };
  }

  applyIcons();
  setupHeaderLayout();

  // Load config
  show("loader");
  fetch(`${esp}getStatus`)
    .then(res => res.json())
    .then(data => {
      // Update esp with IP if available
      if (data.ip) {
        esp = `${location.protocol}//${data.ip}:${port}/`;
      }
      applyStatusHeader(data);
      if (!configPath) throw new Error("No config path");
      return fetch(`${esp}${configPath}`).then(r => r.json());
    })
    .then(cfg => {
      config = cfg;
      applyMetaFromConfig(cfg);
      loadAssets(cfg._assets || {});
      buildSectionsFromConfig();
      hide("loader");
      document.body.classList.remove("boot-loading");
      loadCredentials();
      window.dispatchEvent(new Event("setupPageReady"));
    })
    .catch(() => {
      hide("loader");
      document.body.classList.remove("boot-loading");
    });
});
