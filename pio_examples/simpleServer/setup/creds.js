// Dynamic Credentials Management (loaded only if >1 credential)

// Helper for rendering the dynamic tabs or dropdown
window.renderCredentialTabs = () => {
  let container = $('wifi-cred-tabs');
  if (!container) return;
  container.innerHTML = '';

  // Dynamic check: calculate how many tabs fit
  const box = $('wifi-box');
  const totalWidth = box.offsetWidth || (Math.min(window.innerWidth, 840)); 
  const availableWidth = totalWidth - 100; 
  const maxTabs = Math.floor(availableWidth / 140);

  // If tabs limit exceeded, show a dropdown menu
  if (wifiCredentials.length > maxTabs) {
    const sel = newEl('select');
    wifiCredentials.forEach((cred, idx) => {
      const opt = newEl('option', { 'value': idx });
      opt.textContent = cred.ssid || '(empty SSID)';
      if (idx === selectedCredentialIndex) opt.selected = true;
      sel.appendChild(opt);
    });
    sel.addEventListener('change', () => {
      selectedCredentialIndex = parseInt(sel.value);
      applyCredentialToForm(wifiCredentials[selectedCredentialIndex]);
    });
    container.appendChild(sel);
  } else {
    wifiCredentials.forEach((cred, idx) => {
      const btn = newEl('button', { 'class': 'cred-tab', 'data-index': idx });
      btn.textContent = cred.ssid || '(empty SSID)';
      if (idx === selectedCredentialIndex) btn.classList.add('active');
      btn.addEventListener('click', () => {
        selectedCredentialIndex = idx;
        renderCredentialTabs();
        applyCredentialToForm(cred);
      });
      container.appendChild(btn);
    });
  }

  const showMgmt = wifiCredentials.length > 0;
  if (showMgmt) {
    show('delete-cred');
    show('clear-creds');
    show('cred-actions-inline');
  } else {
    hide('delete-cred');
    hide('clear-creds');
    hide('cred-actions-inline');
  }
};

window.deleteSelectedCredential = function() {
  if (selectedCredentialIndex < 0 || selectedCredentialIndex >= wifiCredentials.length) return;
  const cred = wifiCredentials[selectedCredentialIndex];

  openModal('Delete WiFi credential', `Delete saved credential for <b>${cred.ssid}</b>?`, async () => {
    try {
      await fetch(`${esp}wifi/credentials?index=${selectedCredentialIndex}`, { method: 'DELETE' });
      await loadCredentials();
      closeModal(false);
    } catch (err) {
      console.error('Error deleting credential:', err);
      openModal('Error!', 'Failed to delete credential');
    }
  });
};

window.clearAllCredentials = function() {
  if (!wifiCredentials.length) return;

  openModal('Clear WiFi credentials', 'Delete all saved WiFi credentials?', async () => {
    try {
      await fetch(`${esp}wifi/credentials`, { method: 'DELETE' });
      await loadCredentials();
      closeModal(false);
    } catch (err) {
      console.error('Error clearing credentials:', err);
      openModal('Error!', 'Failed to clear credentials');
    }
  });
};

// Handle resize only when this script is loaded
let resizeTimer;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    renderCredentialTabs();
  }, 200);
});
