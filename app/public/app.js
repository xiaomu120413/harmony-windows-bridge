const form = document.querySelector('#connectForm');
const runtimeStatus = document.querySelector('#runtimeStatus');
const freerdpState = document.querySelector('#freerdpState');
const configSource = document.querySelector('#configSource');
const tcpState = document.querySelector('#tcpState');
const logBox = document.querySelector('#logBox');
const reloadStatus = document.querySelector('#reloadStatus');
const testConnection = document.querySelector('#testConnection');
const previewCommand = document.querySelector('#previewCommand');
const nativeBridgeState = document.querySelector('#nativeBridgeState');
const mstscState = document.querySelector('#mstscState');

const fields = {
  host: document.querySelector('#host'),
  port: document.querySelector('#port'),
  user: document.querySelector('#user'),
  domain: document.querySelector('#domain'),
  certMode: document.querySelector('#certMode'),
  size: document.querySelector('#size'),
  sharePath: document.querySelector('#sharePath'),
  engine: document.querySelector('#engine'),
  freeRdpPath: document.querySelector('#freeRdpPath'),
  bridgePath: document.querySelector('#bridgePath'),
  clipboard: document.querySelector('#clipboard'),
  fullscreen: document.querySelector('#fullscreen')
};

function log(message, type = 'info') {
  const time = new Date().toLocaleTimeString();
  logBox.textContent += `[${time}] ${message}\n`;
  logBox.scrollTop = logBox.scrollHeight;

  if (type === 'error') console.error(message);
}

function setRuntimeState(text, className) {
  runtimeStatus.textContent = text;
  runtimeStatus.className = `status-pill ${className || ''}`.trim();
}

function applyConfig(config = {}) {
  fields.host.value = config.host || '';
  fields.port.value = config.port || 3389;
  fields.user.value = config.user || '';
  fields.domain.value = config.domain || '';
  fields.certMode.value = config.certMode || 'tofu';
  fields.size.value = config.size || '1400x900';
  fields.sharePath.value = config.sharePath || '';
  fields.engine.value = config.engine || 'library';
  fields.bridgePath.value = config.bridgePath || '';
  fields.freeRdpPath.value = config.freeRdpPath || '';
  fields.clipboard.checked = config.clipboard !== false;
  fields.fullscreen.checked = Boolean(config.fullscreen);
}

function collectConnection() {
  return {
    host: fields.host.value.trim(),
    port: Number(fields.port.value || 3389),
    user: fields.user.value.trim(),
    domain: fields.domain.value.trim(),
    certMode: fields.certMode.value,
    size: fields.size.value.trim(),
    sharePath: fields.sharePath.value.trim(),
    engine: fields.engine.value,
    freeRdpPath: fields.freeRdpPath.value.trim(),
    bridgePath: fields.bridgePath.value.trim(),
    clipboard: fields.clipboard.checked,
    fullscreen: fields.fullscreen.checked
  };
}

async function requestJson(url, options = {}) {
  const response = await fetch(url, {
    headers: { 'content-type': 'application/json' },
    ...options
  });
  const data = await response.json();
  if (!response.ok) throw new Error(data.error || 'Request failed');
  return data;
}

async function loadStatus() {
  setRuntimeState('检测中', '');
  const data = await requestJson('/api/status');
  applyConfig(data.config);
  configSource.textContent = data.configSource;

  if (data.freeRdp.found) {
    freerdpState.textContent = data.freeRdp.path;
    freerdpState.className = 'ok-text';
    log(`检测到 FreeRDP: ${data.freeRdp.path}`);
  } else {
    freerdpState.textContent = '未找到';
    freerdpState.className = 'warn-text';
    log('没有在 PATH 中找到 FreeRDP。可以在高级选项里填写 wfreerdp.exe 路径。');
  }

  if (data.nativeBridge.found) {
    nativeBridgeState.textContent = data.nativeBridge.path;
    nativeBridgeState.className = 'ok-text';
    setRuntimeState('可连接', 'ok');
    log(`检测到 FreeRDP native bridge: ${data.nativeBridge.path}`);
  } else {
    nativeBridgeState.textContent = '未构建';
    nativeBridgeState.className = 'warn-text';
    setRuntimeState(data.freeRdp.found ? '兼容模式可用' : '需配置', 'warn');
    log(`未找到 native bridge。默认构建路径: ${data.nativeBridge.defaultPath}`);
    if (data.freeRdp.found && fields.engine.value === 'library') {
      fields.engine.value = 'process';
      log('已自动切换到 wfreerdp executable / 兼容模式，用于当前桌面 demo 直接连接。');
    }
  }

  if (data.mstsc.found) {
    mstscState.textContent = data.mstsc.path;
    mstscState.className = 'ok-text';
  } else {
    mstscState.textContent = '未找到';
    mstscState.className = 'warn-text';
  }
}

async function runPortTest() {
  const connection = collectConnection();
  tcpState.textContent = '测试中';
  tcpState.className = '';
  const result = await requestJson('/api/test', {
    method: 'POST',
    body: JSON.stringify({
      host: connection.host,
      port: connection.port
    })
  });

  tcpState.textContent = result.ok ? '可访问' : '不可访问';
  tcpState.className = result.ok ? 'ok-text' : 'error-text';
  log(result.detail);
}

async function runConnect(dryRun) {
  const payload = {
    ...collectConnection(),
    dryRun
  };
  const result = await requestJson('/api/connect', {
    method: 'POST',
    body: JSON.stringify(payload)
  });

  if (dryRun) {
    log(`命令预览:\n${result.command}`);
    if (result.engine === 'library' && result.available === false) {
      log('当前选择的是 library 引擎，但 native bridge 还没有构建。');
    }
    return;
  }

  const engineName = result.engine === 'library'
    ? 'FreeRDP native bridge'
    : result.engine === 'mstsc'
      ? 'Windows mstsc'
      : 'FreeRDP';
  log(`已启动 ${engineName}，进程 PID: ${result.pid}`);
  log(result.command);
}

reloadStatus.addEventListener('click', () => {
  loadStatus().catch((error) => log(error.message, 'error'));
});

testConnection.addEventListener('click', () => {
  runPortTest().catch((error) => {
    tcpState.textContent = '测试失败';
    tcpState.className = 'error-text';
    log(error.message, 'error');
  });
});

previewCommand.addEventListener('click', () => {
  runConnect(true).catch((error) => log(error.message, 'error'));
});

form.addEventListener('submit', (event) => {
  event.preventDefault();
  runConnect(false).catch((error) => log(error.message, 'error'));
});

loadStatus().catch((error) => {
  setRuntimeState('异常', 'warn');
  log(error.message, 'error');
});
