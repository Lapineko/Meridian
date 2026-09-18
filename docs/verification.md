# Verification — 0.1.0

The release is built locally with Ubuntu 24.04 / WSL, MinGW-w64 GCC 13 and Windows Python 3.12. Tests do not claim hardware certification.

Verified before packaging:

- C++20 native x64 executable cross-compiles in WSL; GCC runtime dependencies are statically linked.
- Python unit tests cover coordinate boundaries/nonfinite values, anonymous discovery, stale device selection, overlapping requests, mount failure, normal restoration, failed restoration, unplug detection, EOF cleanup and developer-mode reboot messaging.
- Frozen engine imports the userspace tunnel, DVT location, image mounter and AMFI classes without relying on an installed Python.
- Native desktop self-test covers preset-to-input binding, custom coordinate validation, navigation, Developer Mode gating, busy controls and persistent restoration warnings.
- Three pages are rendered by the actual C++ drawing code and visually inspected. The app opens on Windows. The available Windows screenshot/input automation helper was incompatible with this host, so full pointer-driven UI automation is not claimed.
- Publication staging uses explicit file allowlists and a scan for personal build paths, credentials, pairing records and cache files.
- A fresh release extraction under a directory containing Chinese characters and spaces passes engine and native desktop self-tests with PATH limited to Windows system directories.

Not performed: modifying a real connected iPhone, changing its Developer Mode/passcode, verifying actual GPS behavior or claiming that specific regional features become available. Those actions need device-specific manual verification after the owner chooses to apply a location.

Before a stable public release, test on real hardware: trust prompt → scan → enable Developer Mode → apply → verify in Maps → clear → verify in Maps; repeat with USB unplug/reconnect, multiple devices and different supported iOS versions.
