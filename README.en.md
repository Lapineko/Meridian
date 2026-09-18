<p align="center">
  <a href="https://github.com/">
    <img src="assets/meridian.png" alt="Meridian Logo" width="100" height="100">
  </a>
</p>

<h1 align="center">Meridian</h1>

<p align="center">
  <strong>Your place Your pace</strong><br>
  <em>Boundaries fade when your compass is true.</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0--or--later-blue.svg?style=flat-square" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20x64-0078D6?style=flat-square&logo=windows" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/UI-C%2B%2B%20%7C%20Win32%20%2B%20GDI%2B-25513E?style=flat-square&logo=cplusplus" alt="C++ Win32"></a>
  <a href="#"><img src="https://img.shields.io/badge/iOS-17.4%20~%2027%2B-black?style=flat-square&logo=apple" alt="iOS"></a>
  <a href="#"><img src="https://img.shields.io/badge/Privacy-Local--First%20%7C%20Zero--Telemetry-brightgreen?style=flat-square" alt="Privacy"></a>
</p>

<p align="center">
  <a href="README.md">繁體中文</a> · <a href="README.zh-CN.md">简体中文</a> · <strong>English</strong>
</p>

---

<p align="center">
  <img src="docs/screenshot.png" alt="Meridian Desktop Screenshot" width="850">
</p>

## About The Project

**Meridian** is a lightweight, native iOS location simulation studio and development utility for Windows. Built on Apple's official DVT (Developer Tools) instrumentation protocols, Meridian helps mobile developers, QA engineers, and researchers simulate and validate location-based services (LBS), regional map behaviors, and timezone handling directly from their PC.

In theory, paired companion accessories such as the Apple Watch may also benefit synchronously. However, the author does not own an Apple Watch for hardware verification and has not specifically tailored watch-specific routines. Passionate community developers are warmly welcome to explore the codebase and contribute adaptations.

Meridian adopts a 100% local, privacy-first architecture. Steering clear of heavyweight runtimes like Electron, Qt, or WebView2, Meridian is handcrafted with pure **C++20 + Win32 + GDI+**, styled in an understated palette of warm white (`#F8F7F3`) and deep forest green (`#25513E`). Featuring a custom-rendered vector world map and curated Apple global flagship presets, it delivers an ultra-fast, elegant, and native desktop experience.

<details>
  <summary><strong>Table of Contents (click to expand)</strong></summary>
  <ol>
    <li><a href="#interface-preview">Interface Preview</a></li>
    <li><a href="#quick-start">Quick Start</a></li>
    <li><a href="#windows-developer-mode-setup">Windows Developer Mode Setup</a></li>
    <li><a href="#curated-global-landmarks">Curated Global Landmarks</a></li>
    <li><a href="#developer-testing-notes--official-hardware-policies">Developer Testing Notes & Official Hardware Policies</a></li>
    <li><a href="#privacy-security--network-transparency">Privacy, Security & Network Transparency</a></li>
    <li><a href="#disclaimer--acceptable-use-policy">Disclaimer & Acceptable Use Policy</a></li>
    <li><a href="#building-from-source">Building from Source</a></li>
    <li><a href="#license--acknowledgements">License & Acknowledgements</a></li>
  </ol>
</details>

---

## Interface Preview

| Main Console | Developer Mode Guide | Privacy & Ethics |
| :---: | :---: | :---: |
| ![Console](docs/screenshot.png) | ![Guide](docs/guide.png) | ![Privacy](docs/privacy.png) |

---

## Quick Start

### 1. Prerequisites
- **Operating System**: Windows 10 or Windows 11 (x64).
- **Apple Drivers**: Install official [Apple Devices](https://apps.microsoft.com/detail/9np83lwlpz9k) from Microsoft Store or the latest iTunes to ensure communication drivers are installed.
- **USB Cable**: A certified USB cable that supports reliable data transfer.

### 2. Steps
1. Download the latest `Meridian-0.1.0-windows-x64.zip` from [Releases](../../releases).
2. **Extract completely** to any local folder (keep `Meridian.exe` alongside `engine`, `data`, and `assets`).
3. Launch `Meridian.exe`. The application automatically matches your Windows display language (or click the language button in the upper-right to toggle).
4. Connect your non-mainland iPhone via USB, unlock the screen, and tap **"Trust This Computer"**.
5. Ensure Developer Mode is activated on your phone (see below), then click **"Scan Devices"**.
6. Choose a landmark from the dropdown, click "You Are Here", or enter custom WGS 84 coordinates, then click **"Start Location"**.
7. Open your iPhone's Maps application to verify the new coordinate.
8. When finished, click **"Restore Real Location · You Are Here"** and verify on your iPhone.

---

## Windows Developer Mode Setup

Unlike macOS environments where Developer Mode is enabled directly through Xcode pairing, Windows uses iOS low-level AMFI protocols:

- **Method 1: Direct on Device (Recommended)**  
  Go to iPhone **Settings → Privacy & Security → Developer Mode**, toggle it on, and restart your device when prompted.
- **Method 2: Guided by Meridian**  
  If the Developer Mode menu does not appear in your iPhone settings, Meridian can send an enablement request.  
  > [!IMPORTANT]
  > Under iOS security specifications on Windows, **you must temporarily turn off your lock screen passcode** in **Settings → Face ID & Passcode** (Face ID will also be paused).  
  > **Meridian never reads, logs, stores, or cracks your passcode.**  
  > Once triggered, your iPhone will reboot automatically. After rebooting, confirm **"Turn On Developer Mode"** on your iPhone screen, then click "Scan Devices" in Meridian. Once activated, immediately re-enable your Passcode and Face ID. Devices with Developer Mode already turned on do not need to disable passcodes.

---

## Curated Global Landmarks

| Region | Landmark | Latitude | Longitude | Source |
| :--- | :--- | :---: | :---: | :--- |
| **USA** | Apple Park HQ | 37.334643 | -122.008972 | Approximate campus center |
| **USA** | Apple Park Visitor Center | 37.332890 | -122.005200 | Official Retail JSON-LD |
| **Hong Kong** | Apple ifc mall | 22.284630 | 114.159172 | Official Retail JSON-LD |
| **Japan** | Apple Marunouchi | 35.680176 | 139.763855 | Official Retail JSON-LD |
| **UK** | Apple Regent Street | 51.514240 | -0.142140 | Official Retail JSON-LD |
| **Germany** | Apple Kurfürstendamm | 52.503630 | 13.328650 | Official Retail JSON-LD |
| **France** | Apple Champs-Élysées | 48.872217 | 2.301233 | Official Retail JSON-LD |
| **Singapore** | Apple Marina Bay Sands | 1.283286 | 103.857547 | Official Retail JSON-LD |
| **Australia** | Apple Sydney | -33.868890 | 151.206780 | Official Retail JSON-LD |

> [!NOTE]
> Presets provide quick shortcuts to representative spots; all coordinates operate equivalently on the protocol layer. For custom locations, simply enter exact WGS 84 values.

---

## Developer Testing Notes & Official Hardware Policies

> [!NOTE]
> **Official Hardware & Service Policy Notice**:  
> According to [Apple Official Technical Documentation](https://support.apple.com/en-us/121115), iPhone devices purchased in mainland China (hardware model numbers ending in CH/A) have factory-level hardware and regulatory SKU restrictions for specific system features.  
> Meridian interfaces exclusively with Apple's standard developer simulation APIs to simulate GPS coordinates. It **does not alter Apple ID account regions, does not tamper with factory hardware SKU identifiers, and cannot bypass hardware- or server-side authorization gates**. Developers conducting multi-region testing should be aware of these official policy boundaries.

1. **Mechanism & Scope**: Meridian modifies the simulated GPS coordinates reported by device location services via standard DVT instruments protocol.
2. **Feature Availability**: System-level feature availability (such as Apple Intelligence) depends on device silicon (A17 Pro, M-series, etc.), iOS version, system and Siri language configuration, and Apple server-side rollout policies. **Simulating GPS coordinates does not guarantee or intend to unlock region-gated system features**. Refer to [Apple Support Documentation](https://support.apple.com/en-us/121115).
3. **OS Version Coverage**: Communication targets **iOS 17.4 through iOS 27+**, utilizing the tested `pymobiledevice3 11.12.5` RSD / DVT architecture.
4. **Restoration**: Normal window closure or clicking "Restore Real Location · You Are Here" clears the simulation. In the event of an unexpected disconnect, crash, or forced termination, reconnect and click restore, or simply reboot your iPhone to return to true GPS.

---

## Privacy, Security & Network Transparency

- **Zero Telemetry & Local-First**: We do not record or upload device names, serial numbers, UDIDs, location histories, or crash dumps. There are no background daemons, analytics trackers, or remote backdoors.
- **Process Memory Isolation**: Real hardware identifiers are kept exclusively inside background process memory. The UI communicates solely via short-lived randomized session tokens.
- **Network Transparency**: All location simulations operate over your local USB connection.
  - When mounting Developer Disk Images (DDI) for the first time, a secure connection to official Apple servers may be established to obtain matching signatures.
  - Clicking "You Are Here" initiates a read-only HTTPS query to public geolocation APIs solely to populate initial reference coordinates, transmitting no hardware identifiers and storing no logs.

---

## Disclaimer & Acceptable Use Policy

> [!IMPORTANT]
> Meridian is an open-source development testing and protocol research tool, adhering strictly to the principle of technological neutrality.

1. **Intended Use**: This tool is provided solely for mobile software development testing, LBS feature verification, and academic study of USB device protocols.
2. **Prohibited Activities**: Users may not utilize this software for any fraudulent, malicious, or unlawful purposes, including but not limited to:
   - Commercial dispatch or logistics fraud (e.g., fake ride-hailing orders, courier dispatch spoofing);
   - Falsification of enterprise attendance, workplace clock-ins, or telecommuting verifications;
   - Unfair advantage, botting, or cheating in location-based mobile games;
   - Circumvention of applicable laws, regulations, cybersecurity policies, or third-party Terms of Service (TOS).
3. **Waiver of Liability**: The authors and contributors shall not be liable for any claims, damages, account suspensions, administrative penalties, or hardware/data issues arising directly or indirectly from the use or misuse of this software. **By downloading, cloning, building, or using this software, you acknowledge and agree to this policy in full.**

---

## Building from Source

### 1. Cross-Compile C++ UI in WSL
On Ubuntu 24.04 (WSL 2) using MinGW-w64:

```bash
sudo apt-get update
sudo apt-get install -y g++-mingw-w64-x86-64-posix cmake ninja-build
cd /mnt/c/path/to/meridian
bash scripts/build-wsl.sh
```
Build output: `.build/windows/Meridian.exe` (statically linked, zero external UI framework dependencies).

### 2. Freeze Python Engine & Package on Windows
In PowerShell on Windows with Python 3.12 x64:

```powershell
py -3.12 -m venv .build/venv
.build/venv/Scripts/python.exe -m pip install -r requirements-lock.txt pyinstaller==6.19.0
.build/venv/Scripts/python.exe -m unittest discover -s tests -v
./scripts/package.ps1 -Python .build/venv/Scripts/python.exe
```

The packaging script will:
- Run all unit tests and native desktop UI self-tests;
- Freeze the backend engine to `release/Meridian/engine`;
- Run automated security, credential, and privacy leak audits;
- Produce portable release ZIPs, sanitized source ZIPs, and SHA256 checksums.

---

## License & Acknowledgements

- Licensed under **[GPL-3.0-or-later](LICENSE)**.
- iOS protocol layer powered by [pymobiledevice3](https://github.com/doronz88/pymobiledevice3) (GPL-3.0).
- Lightweight JSON parsing by [nlohmann/json](https://github.com/nlohmann/json) (MIT).
- Meridian is an independent open-source project and is not affiliated with or endorsed by Apple Inc.
