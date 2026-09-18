# Architecture

## Native desktop

C++20 / Win32 / GDI+ draws the workspace. Standard buttons, edit controls and a device combo box retain keyboard focus and native input behavior. Natural Earth geometry and Apple presets load locally. Coordinates are WGS 84; the overview map is deliberately coarse.

The UI starts `engine/meridian-engine.exe` using CreateProcessW with redirected anonymous pipes, without a shell. Device identifiers never appear in arguments. A Windows Job Object owns the child lifetime. Normal close requests restoration first; a second close offers force-exit when necessary.

## Device bridge

`backend/bridge.py` owns USB discovery, random token-to-device mappings and one asynchronous operation. stdout carries anonymous JSON lines; third-party stdout/stderr are discarded. No HTTP server, network IPC, telemetry or position history.

Commands: `scan`, `enable_developer`, `apply`, `clear`, `stop`, `quit`. Device commands use an opaque `device` token; apply also takes finite `lat` / `lon`. Events: `ready`, `devices`, `status`, `error`, `done`. Every scan invalidates earlier tokens.

Location flow: check selected USB device → check iOS and Developer Mode → mount Personalized DDI → own UserspaceRsdTunnel context → DVT LocationSimulation → hold session → clear on stop → close contexts. USB presence is checked every two seconds. Setup, calls and cleanup have bounded waits. Success means protocol completion, not an independently verified GPS fix or third-party service feature activation.

DDI downloads use the upstream image repository/cache. Apple personalization may require device parameters. A network download can outlive its coroutine timeout in its worker thread; forced close terminates the child tree and is never reported as successful restoration.

## Build and privacy

WSL cross-compiles C++; Windows/Python 3.12 freezes the bridge. Release packaging uses an allowlist and audits private paths, credentials and pairing files. User environments, pairing data, DDI cache and build logs are excluded.

Unit tests use fake devices; native UI tests and frozen import tests validate the desktop and packaging. Actual iOS 27 behavior, developer-mode activation and regional service availability still need owner-operated hardware verification.
