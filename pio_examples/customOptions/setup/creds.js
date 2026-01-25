// creds.js - Gestione dinamica delle credenziali WiFi
// Richiede che app.js sia caricato prima

window.renderCredentialTabs = () => {
  const container = $('wifi-cred-tabs');
  if (!container) return;
  container.innerHTML = '';

  // Calcolo dinamico dello spazio disponibile (per layout desktop)
  const box = $('wifi-box');
  const totalWidth = box.offsetWidth || Math.min(window.innerWidth, 840);
  const maxTabs = Math.floor((totalWidth - 60) / 140); // 140px stimati per tab

  // Viewport piccolo/medio: usa sempre un menu a tendina
  const isMobile = window.matchMedia('(max-width: 608px)').matches;

  if (isMobile) {
    const sel = newEl('select', { class: 'opt-input', style: 'margin-bottom:10px' });
    wifiCredentials.forEach((cred, idx) => {
      const opt = newEl('option', { value: idx });
      opt.textContent = cred.ssid || '(empty SSID)';
      if (idx === selectedCredentialIndex) opt.selected = true;
      sel.appendChild(opt);
    });
    sel.addEventListener('change', (e) => {
      selectedCredentialIndex = parseInt(e.target.value);
      window.applyCredential(wifiCredentials[selectedCredentialIndex]);
    });
    container.appendChild(sel);
  } else if (wifiCredentials.length > maxTabs) {
    // Desktop: troppe credenziali, usa comunque una select
    const sel = newEl('select', { class: 'opt-input', style: 'margin-bottom:10px' });
    wifiCredentials.forEach((cred, idx) => {
      const opt = newEl('option', { value: idx });
      opt.textContent = cred.ssid || '(empty SSID)';
      if (idx === selectedCredentialIndex) opt.selected = true;
      sel.appendChild(opt);
    });
    sel.addEventListener('change', (e) => {
      selectedCredentialIndex = parseInt(e.target.value);
      window.applyCredential(wifiCredentials[selectedCredentialIndex]);
    });
    container.appendChild(sel);
  } else {
    // Desktop: spazio sufficiente, mostra i tab come prima
    wifiCredentials.forEach((cred, idx) => {
      const btn = newEl('button', { class: 'cred-tab' });
      btn.textContent = cred.ssid || '(empty SSID)';

      if (idx === selectedCredentialIndex) btn.classList.add('active');

      btn.onclick = (e) => {
        const siblings = container.querySelectorAll('.cred-tab');
        siblings.forEach(s => s.classList.remove('active'));
        e.target.classList.add('active');

        selectedCredentialIndex = idx;
        window.applyCredential(cred);
      };
      container.appendChild(btn);
    });
  }

  // Mostra/Nascondi pulsanti di gestione
  const hasCreds = wifiCredentials.length > 0;
  const actions = ['delete-cred', 'clear-creds', 'cred-actions-inline'];
  actions.forEach(id => {
    const el = $(id);
    if (el) hasCreds ? el.classList.remove('hide') : el.classList.add('hide');
  });
};

// Rende la vista tab/select responsive a cambi di dimensione/orientamento
window.addEventListener('resize', () => {
  if (!wifiCredentials || !wifiCredentials.length) return;
  if (typeof renderCredentialTabs === 'function') renderCredentialTabs();
});

window.deleteSelectedCredential = () => {
  if (selectedCredentialIndex < 0 || selectedCredentialIndex >= wifiCredentials.length) return;
  const cred = wifiCredentials[selectedCredentialIndex];

  // Usa la funzione openModal globale di app.js
  openModal('Delete WiFi', `Delete <b>${cred.ssid}</b>?`, async () => {
    try {
      await fetch(`${esp}wifi/credentials?index=${selectedCredentialIndex}`, { method: 'DELETE' });
      // Ricarica le credenziali usando la funzione globale
      await loadCredentials(); 
      closeModal(false);
    } catch (err) {
      openModal('Error', 'Failed to delete');
    }
  });
};

window.clearAllCredentials = () => {
  if (!wifiCredentials.length) return;
  openModal('Clear All', 'Delete all WiFi credentials?', async () => {
    try {
      await fetch(`${esp}wifi/credentials`, { method: 'DELETE' });
      await loadCredentials();
      closeModal(false);
    } catch (err) {
      openModal('Error', 'Failed to clear');
    }
  });
};