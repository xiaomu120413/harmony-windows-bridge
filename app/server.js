const http = require('node:http');
const fs = require('node:fs');
const fsp = require('node:fs/promises');
const path = require('node:path');
const net = require('node:net');
const os = require('node:os');
const { spawn } = require('node:child_process');

const rootDir = path.resolve(__dirname, '..');
const publicDir = path.join(__dirname, 'public');
const configPath = path.join(rootDir, 'config.local.json');
const exampleConfigPath = path.join(rootDir, 'config.example.json');
const bundledFreeRdpPath = path.join(rootDir, 'tools', 'freerdp', 'wfreerdp.exe');
const nativeBridgePath = path.join(rootDir, 'native', 'freerdp-bridge', 'build', 'freerdp_bridge.exe');
const nativeBridgeReleasePath = path.join(rootDir, 'native', 'freerdp-bridge', 'build', 'Release', 'freerdp_bridge.exe');
const port = Number(process.env.PORT || 5173);

const contentTypes = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml; charset=utf-8'
};

function json(res, statusCode, payload) {
  const body = JSON.stringify(payload, null, 2);
  res.writeHead(statusCode, {
    'content-type': 'application/json; charset=utf-8',
    'cache-control': 'no-store'
  });
  res.end(body);
}

function text(res, statusCode, body) {
  res.writeHead(statusCode, { 'content-type': 'text/plain; charset=utf-8' });
  res.end(body);
}

async function readJsonIfExists(filePath) {
  try {
    const raw = await fsp.readFile(filePath, 'utf8');
    return JSON.parse(raw);
  } catch (error) {
    if (error.code === 'ENOENT') return null;
    throw error;
  }
}

async function readRequestJson(req) {
  const chunks = [];
  for await (const chunk of req) {
    chunks.push(chunk);
  }
  if (!chunks.length) return {};

  const body = Buffer.concat(chunks).toString('utf8');
  return JSON.parse(body);
}

function findExecutable(names) {
  const pathEnv = process.env.PATH || '';
  const extensions = process.platform === 'win32'
    ? (process.env.PATHEXT || '.EXE;.CMD;.BAT').split(';')
    : [''];

  for (const dir of pathEnv.split(path.delimiter)) {
    if (!dir) continue;
    for (const name of names) {
      const candidates = path.extname(name) ? [name] : extensions.map((ext) => `${name}${ext.toLowerCase()}`);
      for (const candidate of candidates) {
        const fullPath = path.join(dir, candidate);
        if (fs.existsSync(fullPath)) return fullPath;
      }
    }
  }

  return null;
}

function resolveFreeRdpPath(preferredPath) {
  if (preferredPath) {
    const resolved = path.resolve(preferredPath);
    if (fs.existsSync(resolved)) return resolved;
    throw new Error(`FreeRDP executable not found: ${preferredPath}`);
  }

  if (fs.existsSync(bundledFreeRdpPath)) return bundledFreeRdpPath;

  return findExecutable(['wfreerdp.exe', 'wfreerdp', 'xfreerdp.exe', 'xfreerdp']);
}

function resolveMstscPath() {
  if (process.platform !== 'win32') return null;
  const systemRoot = process.env.SystemRoot || 'C:\\Windows';
  const mstscPath = path.join(systemRoot, 'System32', 'mstsc.exe');
  if (fs.existsSync(mstscPath)) return mstscPath;
  return findExecutable(['mstsc.exe']);
}

function resolveNativeBridgePath(preferredPath) {
  const candidates = [
    preferredPath,
    process.env.FREERDP_BRIDGE_PATH,
    nativeBridgePath,
    nativeBridgeReleasePath,
    findExecutable(['freerdp_bridge.exe', 'freerdp_bridge'])
  ].filter(Boolean);

  for (const candidate of candidates) {
    const resolved = path.resolve(candidate);
    if (fs.existsSync(resolved)) return resolved;
  }

  return null;
}

function normalizeConnection(input) {
  const connection = {
    host: String(input.host || '').trim(),
    port: Number(input.port || 3389),
    user: String(input.user || '').trim(),
    domain: String(input.domain || '').trim(),
    certMode: String(input.certMode || 'tofu').trim(),
    size: String(input.size || '1400x900').trim(),
    fullscreen: Boolean(input.fullscreen),
    clipboard: input.clipboard !== false,
    sharePath: String(input.sharePath || '').trim(),
    engine: String(input.engine || 'library').trim(),
    freeRdpPath: String(input.freeRdpPath || '').trim(),
    bridgePath: String(input.bridgePath || '').trim()
  };

  if (!connection.host) throw new Error('Target host is required.');
  if (!Number.isInteger(connection.port) || connection.port < 1 || connection.port > 65535) {
    throw new Error('Port must be between 1 and 65535.');
  }
  if (!['ask', 'ignore', 'tofu'].includes(connection.certMode)) {
    throw new Error('certMode must be ask, ignore, or tofu.');
  }
  if (!['library', 'process', 'mstsc'].includes(connection.engine)) {
    throw new Error('engine must be library, process, or mstsc.');
  }
  if (connection.size && !/^\d{3,5}x\d{3,5}$/i.test(connection.size)) {
    throw new Error('Size must look like 1400x900.');
  }
  if (connection.sharePath) {
    const sharePath = path.resolve(connection.sharePath);
    if (!fs.existsSync(sharePath)) throw new Error(`Share path does not exist: ${connection.sharePath}`);
    connection.sharePath = sharePath;
  }

  return connection;
}

function buildBridgeArgs(connection) {
  const args = [
    '--connect',
    '--host', connection.host,
    '--port', String(connection.port),
    '--user', connection.user,
    '--cert-mode', connection.certMode
  ];

  if (connection.domain) args.push('--domain', connection.domain);
  if (connection.size && !connection.fullscreen) args.push('--size', connection.size);
  if (connection.fullscreen) args.push('--fullscreen');
  if (connection.clipboard) args.push('--clipboard');
  if (connection.sharePath) args.push('--share', connection.sharePath);

  return args;
}

async function createMstscFile(connection) {
  const target = connection.port === 3389 ? connection.host : `${connection.host}:${connection.port}`;
  const [width, height] = /^\d{3,5}x\d{3,5}$/i.test(connection.size)
    ? connection.size.toLowerCase().split('x').map(Number)
    : [1400, 900];

  const lines = [
    `full address:s:${target}`,
    `screen mode id:i:${connection.fullscreen ? 2 : 1}`,
    `desktopwidth:i:${width}`,
    `desktopheight:i:${height}`,
    `redirectclipboard:i:${connection.clipboard ? 1 : 0}`,
    'authentication level:i:2',
    'prompt for credentials:i:1'
  ];

  if (connection.user) {
    lines.push(`username:s:${connection.user}`);
  }

  const filePath = path.join(os.tmpdir(), `freerdp-control-demo-${Date.now()}.rdp`);
  await fsp.writeFile(filePath, `${lines.join('\r\n')}\r\n`, 'utf8');
  return filePath;
}

function buildFreeRdpArgs(connection) {
  const target = connection.port === 3389 ? connection.host : `${connection.host}:${connection.port}`;
  const args = [
    `/v:${target}`,
    `/cert:${connection.certMode}`,
    '/dynamic-resolution'
  ];

  if (connection.user) args.push(`/u:${connection.user}`);
  if (connection.domain) args.push(`/d:${connection.domain}`);
  if (connection.clipboard) args.push('+clipboard');
  if (connection.fullscreen) {
    args.push('/f');
  } else if (connection.size) {
    args.push(`/size:${connection.size}`);
  }
  if (connection.sharePath) args.push(`/drive:demo,${connection.sharePath}`);

  return args;
}

function testTcp(host, portNumber, timeoutMs = 3000) {
  return new Promise((resolve) => {
    const socket = new net.Socket();
    let settled = false;

    function finish(ok, detail) {
      if (settled) return;
      settled = true;
      socket.destroy();
      resolve({ ok, detail });
    }

    socket.setTimeout(timeoutMs);
    socket.once('connect', () => finish(true, `${host}:${portNumber} is reachable`));
    socket.once('timeout', () => finish(false, `${host}:${portNumber} timed out`));
    socket.once('error', (error) => finish(false, error.message));
    socket.connect(portNumber, host);
  });
}

function testRdpNegotiation(host, portNumber, timeoutMs = 5000) {
  return new Promise((resolve) => {
    const negotiationRequest = Buffer.from('030000130ee000000000000100080003000000', 'hex');
    const socket = new net.Socket();
    let settled = false;

    function finish(ok, detail, responseHex = '') {
      if (settled) return;
      settled = true;
      socket.destroy();
      resolve({ ok, detail, responseHex });
    }

    socket.setTimeout(timeoutMs);
    socket.once('connect', () => {
      socket.write(negotiationRequest);
    });
    socket.once('data', (data) => {
      finish(true, 'RDP negotiation response received', data.toString('hex'));
    });
    socket.once('timeout', () => {
      finish(false, 'TCP connected, but no RDP negotiation response was received');
    });
    socket.once('error', (error) => {
      finish(false, error.message);
    });
    socket.once('close', () => {
      finish(false, 'TCP connected, but the peer closed without an RDP negotiation response');
    });
    socket.connect(portNumber, host);
  });
}

async function getInitialConfig() {
  const localConfig = await readJsonIfExists(configPath);
  if (localConfig) return { config: localConfig, source: 'config.local.json' };

  const exampleConfig = await readJsonIfExists(exampleConfigPath);
  return { config: exampleConfig || {}, source: 'config.example.json' };
}

async function handleApi(req, res, pathname) {
  if (req.method === 'GET' && pathname === '/api/status') {
    const freeRdpPath = resolveFreeRdpPath('');
    const bridgePath = resolveNativeBridgePath('');
    const mstscPath = resolveMstscPath();
    const initial = await getInitialConfig();
    json(res, 200, {
      freeRdp: {
        found: Boolean(freeRdpPath),
        path: freeRdpPath,
        bundledPath: bundledFreeRdpPath
      },
      nativeBridge: {
        found: Boolean(bridgePath),
        path: bridgePath,
        defaultPath: nativeBridgePath,
        releasePath: nativeBridgeReleasePath
      },
      mstsc: {
        found: Boolean(mstscPath),
        path: mstscPath
      },
      config: initial.config,
      configSource: initial.source,
      platform: process.platform
    });
    return;
  }

  if (req.method === 'POST' && pathname === '/api/test') {
    const body = await readRequestJson(req);
    const host = String(body.host || '').trim();
    const portNumber = Number(body.port || 3389);
    if (!host) throw new Error('Target host is required.');
    const tcp = await testTcp(host, portNumber);
    const rdp = tcp.ok ? await testRdpNegotiation(host, portNumber) : null;
    json(res, 200, {
      ok: Boolean(tcp.ok && rdp && rdp.ok),
      detail: rdp ? `${tcp.detail}; ${rdp.detail}` : tcp.detail,
      tcp,
      rdp
    });
    return;
  }

  if (req.method === 'POST' && pathname === '/api/connect') {
    const body = await readRequestJson(req);
    const connection = normalizeConnection(body);

    if (connection.engine === 'library') {
      const bridgePath = resolveNativeBridgePath(connection.bridgePath);
      const args = buildBridgeArgs(connection);
      const command = bridgePath ? `${bridgePath} ${args.join(' ')}` : `${nativeBridgePath} ${args.join(' ')}`;

      if (body.dryRun) {
        json(res, 200, {
          launched: false,
          engine: 'library',
          command,
          available: Boolean(bridgePath)
        });
        return;
      }

      if (!bridgePath) {
        throw new Error('FreeRDP native bridge was not found. Build native/freerdp-bridge first or switch engine to wfreerdp process.');
      }

      const child = spawn(bridgePath, args, {
        detached: true,
        stdio: 'ignore'
      });
      child.unref();

      json(res, 200, {
        launched: true,
        engine: 'library',
        pid: child.pid,
        command
      });
      return;
    }

    if (connection.engine === 'mstsc') {
      const mstscPath = resolveMstscPath();
      if (!mstscPath) {
        throw new Error('mstsc.exe was not found on this Windows machine.');
      }

      if (body.dryRun) {
        const target = connection.port === 3389 ? connection.host : `${connection.host}:${connection.port}`;
        json(res, 200, {
          launched: false,
          engine: 'mstsc',
          command: `${mstscPath} /v:${target}`
        });
        return;
      }

      const rdpFile = await createMstscFile(connection);
      const child = spawn('cmd.exe', ['/c', 'start', 'FreeRDP Control Demo', mstscPath, rdpFile], {
        detached: true,
        windowsHide: false,
        stdio: 'ignore'
      });
      child.unref();

      json(res, 200, {
        launched: true,
        engine: 'mstsc',
        pid: child.pid,
        command: `cmd.exe /c start "FreeRDP Control Demo" "${mstscPath}" "${rdpFile}"`
      });
      return;
    }

    const freeRdpPath = resolveFreeRdpPath(connection.freeRdpPath);
    if (!freeRdpPath) {
      throw new Error('FreeRDP executable was not found. Set the wfreerdp path in Advanced options or use the library bridge.');
    }
    const args = buildFreeRdpArgs(connection);
    if (body.dryRun) {
      json(res, 200, {
        launched: false,
        engine: 'process',
        command: `${freeRdpPath} ${args.join(' ')}`
      });
      return;
    }

    const child = spawn(freeRdpPath, args, {
      detached: true,
      stdio: 'ignore'
    });
    child.unref();

    json(res, 200, {
      launched: true,
      engine: 'process',
      pid: child.pid,
      command: `${freeRdpPath} ${args.join(' ')}`
    });
    return;
  }

  text(res, 404, 'API route not found');
}

async function serveStatic(res, pathname) {
  const normalizedPath = pathname === '/' ? '/index.html' : pathname;
  const filePath = path.resolve(publicDir, `.${normalizedPath}`);

  if (!filePath.startsWith(publicDir)) {
    text(res, 403, 'Forbidden');
    return;
  }

  try {
    const data = await fsp.readFile(filePath);
    const ext = path.extname(filePath);
    res.writeHead(200, {
      'content-type': contentTypes[ext] || 'application/octet-stream',
      'cache-control': 'no-store'
    });
    res.end(data);
  } catch (error) {
    if (error.code === 'ENOENT') {
      text(res, 404, 'Not found');
      return;
    }
    throw error;
  }
}

const server = http.createServer(async (req, res) => {
  try {
    const requestUrl = new URL(req.url, `http://${req.headers.host}`);
    if (requestUrl.pathname.startsWith('/api/')) {
      await handleApi(req, res, requestUrl.pathname);
      return;
    }

    await serveStatic(res, requestUrl.pathname);
  } catch (error) {
    json(res, 400, { error: error.message });
  }
});

server.listen(port, '127.0.0.1', () => {
  console.log(`FreeRDP Control Demo is running at http://127.0.0.1:${port}`);
});
