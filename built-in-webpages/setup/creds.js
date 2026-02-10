/**
 * Render WiFi credential tabs or dropdown based on screen size
 */
window.renderCredentialTabs = () => {
  const credTabs = $("wifi-cred-tabs");
  if (!credTabs) return;

  credTabs.innerHTML = "";
  const wifiBoxWidth = $("wifi-box").offsetWidth || Math.min(window.innerWidth, 840);
  const maxTabsPerRow = Math.floor((wifiBoxWidth - 60) / 140);

  // Mobile: Use dropdown select
  if (window.matchMedia("(max-width: 608px)").matches) {
    const select = newEl("select", { class: "in", style: "margin-bottom:10px" });
    wifiCredentials.forEach((cred, idx) => {
      const option = newEl("option", { value: idx });
      option.textContent = cred.ssid || "(empty SSID)";
      if (idx === selectedCredentialIndex) option.selected = true;
      select.appendChild(option);
    });
    select.addEventListener("change", (e) => {
      selectedCredentialIndex = parseInt(e.target.value);
      window.applyCredential(wifiCredentials[selectedCredentialIndex]);
    });
    credTabs.appendChild(select);
  }
  // Tablet: Use dropdown if too many credentials
  else if (wifiCredentials.length > maxTabsPerRow) {
    const select = newEl("select", { class: "in", style: "margin-bottom:10px" });
    wifiCredentials.forEach((cred, idx) => {
      const option = newEl("option", { value: idx });
      option.textContent = cred.ssid || "(empty SSID)";
      if (idx === selectedCredentialIndex) option.selected = true;
      select.appendChild(option);
    });
    select.addEventListener("change", (e) => {
      selectedCredentialIndex = parseInt(e.target.value);
      window.applyCredential(wifiCredentials[selectedCredentialIndex]);
    });
    credTabs.appendChild(select);
  }
  // Desktop: Use button tabs
  else {
    wifiCredentials.forEach((cred, idx) => {
      const tabBtn = newEl("button", { class: "tab" });
      tabBtn.textContent = cred.ssid || "(empty SSID)";
      if (idx === selectedCredentialIndex) tabBtn.classList.add("active");
      tabBtn.onclick = (e) => {
        credTabs.querySelectorAll(".tab").forEach(tab => tab.classList.remove("active"));
        e.target.classList.add("active");
        selectedCredentialIndex = idx;
        window.applyCredential(cred);
      };
      credTabs.appendChild(tabBtn);
    });
  }

  // Toggle visibility of credential action buttons
  const hasCredentials = wifiCredentials.length > 0;
  ["delete-cred", "clear-creds", "cred-actions-inline"].forEach(elemId => {
    const elem = $(elemId);
    if (elem) {
      if (hasCredentials) {
        elem.classList.remove("hide");
      } else {
        elem.classList.add("hide");
      }
    }
  });
};

/**
 * Re-render credential tabs on window resize
 */
window.addEventListener("resize", () => {
  if (wifiCredentials && wifiCredentials.length && typeof renderCredentialTabs === "function") {
    renderCredentialTabs();
  }
});

/**
 * Delete selected WiFi credential
 */
window.deleteSelectedCredential = () => {
  if (selectedCredentialIndex < 0 || selectedCredentialIndex >= wifiCredentials.length) {
    return;
  }

  const cred = wifiCredentials[selectedCredentialIndex];
  openModal(
    "Delete WiFi",
    `Delete <b>${cred.ssid}</b>?`,
    async () => {
      try {
        await fetch(`${esp}wifi/credentials?index=${selectedCredentialIndex}`, {
          method: "DELETE"
        });
        await loadCredentials();
        closeModal(false);
      } catch (err) {
        openModal("Error", "Failed to delete");
      }
    }
  );
};

/**
 * Delete all WiFi credentials
 */
window.clearAllCredentials = () => {
  if (wifiCredentials.length === 0) return;

  openModal(
    "Clear All",
    "Delete all WiFi credentials?",
    async () => {
      try {
        await fetch(`${esp}wifi/credentials`, {
          method: "DELETE"
        });
        await loadCredentials();
        closeModal(false);
      } catch (err) {
        openModal("Error", "Failed to clear");
      }
    }
  );
};

/**
 * Attach event handlers to credential action buttons
 * Called immediately when script loads (not waiting for DOMContentLoaded)
 */
(() => {
  const deleteBtn = $("delete-cred");
  const clearBtn = $("clear-creds");

  if (deleteBtn) {
    deleteBtn.onclick = window.deleteSelectedCredential;
  }
  if (clearBtn) {
    clearBtn.onclick = window.clearAllCredentials;
  }
})();
