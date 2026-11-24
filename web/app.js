/* i_psb LittleFS companion — vanilla JS, no build step, no third-party code. */
(() => {
  "use strict";

  const state = {
    path: "/Music",
    token: localStorage.getItem("ipsb-token") || "dev-token",
    status: null,
    online: false,
  };

  const $ = (id) => document.getElementById(id);

  function toast(message) {
    const el = $("toast");
    el.textContent = message;
    el.classList.add("show");
    clearTimeout(toast.timer);
    toast.timer = setTimeout(() => el.classList.remove("show"), 2600);
  }

  function formatBytes(bytes) {
    if (!Number.isFinite(bytes) || bytes <= 0) return "0 B";
    const units = ["B", "KB", "MB", "GB"];
    let value = bytes;
    let unit = 0;
    while (value >= 1024 && unit < units.length - 1) {
      value /= 1024;
      unit += 1;
    }
    return `${value.toFixed(unit === 0 ? 0 : 1)} ${units[unit]}`;
  }

  function joinPath(dir, name) {
    if (!name) return dir;
    return `${dir.replace(/\/$/, "")}/${name}`;
  }

  function parentPath(path) {
    const parts = path.split("/").filter(Boolean);
    parts.pop();
    return parts.length ? `/${parts.join("/")}` : "/";
  }

  async function api(path, options = {}) {
    const headers = new Headers(options.headers || {});
    headers.set("X-IPSB-Token", state.token);
    const response = await fetch(path, { ...options, headers });
    const text = await response.text();
    let data = null;
    try {
      data = text ? JSON.parse(text) : null;
    } catch {
      data = null;
    }
    if (!response.ok) {
      const message = data && data.error ? data.error : `HTTP ${response.status}`;
      throw new Error(message);
    }
    return data;
  }

  async function refreshStatus() {
    try {
      const status = await fetch("/api/status").then((r) => {
        if (!r.ok) throw new Error(`HTTP ${r.status}`);
        return r.json();
      });
      state.status = status;
      state.online = true;
      $("status-dot").className = "status-dot online";
      const battery = status.battery && status.battery.valid
        ? `${status.battery.pct}%`
        : "battery --";
      const free = status.storage ? `${formatBytes(status.storage.free)} free` : "storage --";
      $("status-text").textContent = `${battery} · ${free} · fw ${status.fw || "?"}`;
    } catch {
      state.online = false;
      $("status-dot").className = "status-dot offline";
      $("status-text").textContent = "device offline";
    }
  }

  function rowButton(label, handler, danger = false) {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `btn${danger ? " danger" : ""}`;
    button.textContent = label;
    button.addEventListener("click", handler);
    return button;
  }

  function makeRow({ icon, name, meta, actions, extra }) {
    const li = document.createElement("li");
    li.className = "file-row";

    const iconEl = document.createElement("span");
    iconEl.className = `icon${icon === "dir" ? " dir" : ""}`;
    iconEl.textContent = icon === "dir" ? "DIR" : "AUD";
    iconEl.setAttribute("aria-hidden", "true");

    const nameEl = document.createElement("span");
    nameEl.className = "name";
    nameEl.textContent = name;
    nameEl.title = name;

    const metaEl = document.createElement("span");
    metaEl.className = "meta";
    metaEl.textContent = meta || "";

    const actionEl = document.createElement("span");
    actionEl.className = "row-actions";
    if (actions) actions.forEach((a) => actionEl.appendChild(a));

    li.append(iconEl, nameEl, metaEl, actionEl);
    if (extra) li.appendChild(extra);
    return li;
  }

  async function loadFiles() {
    const list = $("file-list");
    list.replaceChildren();
    $("current-path").textContent = state.path;
    $("upload-path").textContent = state.path;
    $("files-hint").textContent = "";

    try {
      const data = await api(
        `/api/files?path=${encodeURIComponent(state.path)}&limit=200`,
      );
      if (state.path !== "/") {
        list.appendChild(
          makeRow({
            icon: "dir",
            name: "..",
            meta: "up",
            actions: [
              rowButton("Open", () => {
                state.path = parentPath(state.path);
                loadFiles();
              }),
            ],
          }),
        );
      }

      const items = (data && data.items) || [];
      if (!items.length) {
        $("files-hint").textContent = "This folder is empty.";
      }

      items.forEach((item) => {
        const full = joinPath(state.path, item.name);
        if (item.is_dir) {
          list.appendChild(
            makeRow({
              icon: "dir",
              name: item.name,
              meta: "folder",
              actions: [
                rowButton("Open", () => {
                  state.path = full;
                  loadFiles();
                }),
              ],
            }),
          );
          return;
        }

        list.appendChild(
          makeRow({
            icon: "file",
            name: item.name,
            meta: formatBytes(item.size),
            actions: [
              rowButton("Play", async () => {
                try {
                  await api("/api/playback", {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({ action: "play", path: full }),
                  });
                  toast(`Playing ${item.name}`);
                } catch (error) {
                  toast(`Play failed: ${error.message}`);
                }
              }),
              rowButton(
                "Delete",
                async () => {
                  if (!confirm(`Delete ${item.name}?`)) return;
                  try {
                    await api(`/api/file?path=${encodeURIComponent(full)}`, {
                      method: "DELETE",
                    });
                    toast("Deleted");
                    loadFiles();
                  } catch (error) {
                    toast(`Delete failed: ${error.message}`);
                  }
                },
                true,
              ),
            ],
          }),
        );
      });
    } catch (error) {
      $("files-hint").textContent = `Cannot list files: ${error.message}`;
      toast("Device offline or token rejected");
    }
  }

  async function createFolder() {
    const name = prompt("Folder name");
    if (!name) return;
    try {
      await api("/api/mkdir", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ path: joinPath(state.path, name) }),
      });
      toast("Folder created");
      loadFiles();
    } catch (error) {
      toast(`Create failed: ${error.message}`);
    }
  }

  function uploadOne(file) {
    return new Promise((resolve) => {
      const li = document.createElement("li");
      li.className = "upload-row";
      li.dataset.name = file.name;

      const icon = document.createElement("span");
      icon.className = "icon";
      icon.textContent = "UP";
      const name = document.createElement("span");
      name.className = "name";
      name.textContent = file.name;
      const meta = document.createElement("span");
      meta.className = "meta";
      meta.textContent = formatBytes(file.size);
      const progress = document.createElement("span");
      progress.className = "progress";
      const bar = document.createElement("span");
      progress.appendChild(bar);
      li.append(icon, name, meta, progress);
      $("upload-list").appendChild(li);

      const xhr = new XMLHttpRequest();
      const url = `/api/upload?path=${encodeURIComponent(state.path)}&name=${encodeURIComponent(file.name)}`;
      xhr.open("POST", url);
      xhr.setRequestHeader("X-IPSB-Token", state.token);
      xhr.setRequestHeader("Content-Type", "application/octet-stream");
      xhr.upload.onprogress = (event) => {
        if (event.lengthComputable) {
          bar.style.width = `${Math.round((event.loaded / event.total) * 100)}%`;
        }
      };
      xhr.onload = () => {
        if (xhr.status >= 200 && xhr.status < 300) {
          bar.style.width = "100%";
          meta.textContent = "done";
          toast(`${file.name} uploaded`);
        } else {
          meta.textContent = `failed (${xhr.status})`;
          bar.style.background = "var(--danger)";
          toast(`${file.name}: upload failed`);
        }
        resolve();
      };
      xhr.onerror = () => {
        meta.textContent = "network error";
        bar.style.background = "var(--danger)";
        toast(`${file.name}: network error`);
        resolve();
      };
      xhr.send(file);
    });
  }

  async function uploadFiles(files) {
    const list = Array.from(files || []);
    for (const file of list) {
      await uploadOne(file);
    }
    loadFiles();
  }

  async function loadWifi() {
    try {
      const data = await api("/api/wifi");
      $("wifi-ssid").value = data.ssid || "";
      $("wifi-mode").value = data.mode || "sta";
      $("wifi-password").value = "";
      $("wifi-hint").textContent = data.rssi
        ? `Connected, RSSI ${data.rssi} dBm. Password is never returned.`
        : "Password is never returned by the device.";
    } catch (error) {
      $("wifi-hint").textContent = `Cannot read Wi-Fi config: ${error.message}`;
    }
  }

  async function saveWifi(event) {
    event.preventDefault();
    const payload = {
      ssid: $("wifi-ssid").value.trim(),
      password: $("wifi-password").value,
      mode: $("wifi-mode").value,
      apply: true,
    };
    try {
      await api("/api/wifi", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(payload),
      });
      toast("Wi-Fi settings saved");
      loadWifi();
    } catch (error) {
      toast(`Save failed: ${error.message}`);
    }
  }

  async function loadLogs() {
    const pre = $("logs");
    pre.textContent = "loading…";
    try {
      const data = await api("/api/logs?lines=200");
      pre.textContent = (data.lines || []).join("\n") || "no logs";
    } catch (error) {
      pre.textContent = `Cannot read logs: ${error.message}`;
    }
  }

  function switchTab(name) {
    document.querySelectorAll(".tab").forEach((tab) => {
      const active = tab.dataset.tab === name;
      tab.classList.toggle("active", active);
      tab.setAttribute("aria-selected", String(active));
    });
    document.querySelectorAll(".panel").forEach((panel) => {
      panel.classList.toggle("active", panel.id === `tab-${name}`);
    });
    if (name === "files") loadFiles();
    if (name === "wifi") loadWifi();
    if (name === "logs") loadLogs();
  }

  function bind() {
    $("token").value = state.token;
    $("save-token").addEventListener("click", () => {
      state.token = $("token").value.trim() || "dev-token";
      localStorage.setItem("ipsb-token", state.token);
      toast("Token saved");
      refreshStatus();
      loadFiles();
    });

    document.querySelectorAll(".tab").forEach((tab) => {
      tab.addEventListener("click", () => switchTab(tab.dataset.tab));
    });

    $("refresh-files").addEventListener("click", loadFiles);
    $("mkdir").addEventListener("click", createFolder);
    $("refresh-logs").addEventListener("click", loadLogs);
    $("wifi-form").addEventListener("submit", saveWifi);

    const dropzone = $("dropzone");
    const fileInput = $("file-input");
    dropzone.addEventListener("click", () => fileInput.click());
    dropzone.addEventListener("keydown", (event) => {
      if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        fileInput.click();
      }
    });
    fileInput.addEventListener("change", () => {
      uploadFiles(fileInput.files);
      fileInput.value = "";
    });

    ["dragenter", "dragover"].forEach((type) => {
      dropzone.addEventListener(type, (event) => {
        event.preventDefault();
        dropzone.classList.add("dragover");
      });
    });
    ["dragleave", "drop"].forEach((type) => {
      dropzone.addEventListener(type, (event) => {
        event.preventDefault();
        dropzone.classList.remove("dragover");
      });
    });
    dropzone.addEventListener("drop", (event) => {
      uploadFiles(event.dataTransfer.files);
    });
  }

  bind();
  refreshStatus();
  loadFiles();
  setInterval(refreshStatus, 15000);
})();
