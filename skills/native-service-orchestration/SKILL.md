---
name: native-service-orchestration
description: Start, stop, configure, and diagnose native daemon-style services from an application or packaged runtime. Use when Codex needs to integrate services such as xrdp, nginx, sshd, frp, databases, local proxies, media servers, or custom native daemons with app lifecycle, generated config, ports, certificates, logs, health checks, and runtime directories.
---

# Native Service Orchestration

## Workflow

Treat the service as an app-managed runtime component.

1. Identify the service binary, config files, writable directories, ports, and required permissions.
2. Generate runtime material before startup: config, certificates, state directories, socket directories, and logs.
3. Start the service through a narrow controller API.
4. Poll or subscribe to health state: process, port, log marker, pid file, or control socket.
5. Surface diagnostics to the app without leaking sensitive material.
6. Stop the service cleanly on app teardown, permission loss, or configuration changes.

## Controller Contract

Expose a small app-facing contract:

- `ensureStarted(reason)`
- `stop(reason)`
- `refreshDiagnostics()`
- `getStatus()`
- `getLastError()`

Keep service-specific command construction inside the controller, not scattered through UI code.

Use this shape when writing a formal interface:

```text
ServiceController {
  ensureStarted(reason) -> ServiceStatus
  stop(reason) -> ServiceStatus
  restart(reason) -> ServiceStatus
  refreshDiagnostics() -> Diagnostics
}
```

The controller owns config generation, process launch, health checks, and redacted diagnostics.

## Runtime Material

Generate or validate:

- Config files with absolute runtime paths.
- TLS certificates or keys when required.
- Writable `run`, `var`, `log`, and `tmp` directories.
- Port or socket selection.
- Environment variables and library paths.

Avoid writing generated files into immutable bundled asset locations.

## Diagnostics

Collect enough information to debug field failures:

- Command line with secrets redacted.
- Process state and exit code.
- Port listening state.
- Recent logs.
- Config path and runtime directory path.
- Version and feature probe output when available.

## Validation

Test startup, duplicate startup, stop, restart, config regeneration, missing permission, missing runtime file, occupied port, and app relaunch.
