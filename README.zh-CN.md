<p align="center">
  <a href="https://github.com/">
    <img src="assets/meridian.png" alt="Meridian Logo" width="100" height="100">
  </a>
</p>

<h1 align="center">Meridian · 子午线</h1>

<p align="center">
  <strong>边界之外，自有方向。</strong><br>
  <em>Your place Your pace</em>
</p>

<p align="center">
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-GPL--3.0--or--later-blue.svg?style=flat-square" alt="License"></a>
  <a href="#"><img src="https://img.shields.io/badge/Platform-Windows%2010%20%7C%2011%20x64-0078D6?style=flat-square&logo=windows" alt="Platform"></a>
  <a href="#"><img src="https://img.shields.io/badge/UI-C%2B%2B%20%7C%20Win32%20%2B%20GDI%2B-25513E?style=flat-square&logo=cplusplus" alt="C++ Win32"></a>
  <a href="#"><img src="https://img.shields.io/badge/iOS-17.4%20~%2027%2B-black?style=flat-square&logo=apple" alt="iOS"></a>
  <a href="#"><img src="https://img.shields.io/badge/Privacy-Local--First%20%7C%20Zero--Telemetry-brightgreen?style=flat-square" alt="Privacy"></a>
</p>

<p align="center">
  <a href="README.md">繁體中文</a> · <strong>简体中文</strong> · <a href="README.en.md">English</a>
</p>

---

<p align="center">
  <img src="docs/screenshot.png" alt="Meridian 主界面" width="850">
</p>

## 关于项目

**Meridian（子午线）** 是一款面向移动应用开发者与技术研究人员的 iOS 地理位置模拟与开发测试工具。基于 Apple 官方 DVT（Developer Tools）通信协议构建，旨在协助开发者在 Windows 桌面端便捷调试应用程序在不同地理区域下的 LBS 行为、地图展示与时区逻辑。

理论上 Apple Watch 等配对设备亦具备同步受益潜力，惟笔者目前未持有 Apple Watch 进行实机验证，亦未专门编写 Watch 相关指令，热心开发者阁下可参考源码自行编写适配代码。

本项目采用纯粹本地化的安全通信架构，摒弃了 Electron、Qt 及 WebView2 等庞大运行库，采用 **C++20 + Win32 + GDI+** 构建原生桌面，辅以暖白（`#F8F7F3`）与深林墨绿（`#25513E`）的克制美学，配备自绘矢量地图与全球 Apple 旗舰地标预设，带来轻盈、优雅、即开即用的使用体验。

<details>
  <summary><strong>目录（点击展开）</strong></summary>
  <ol>
    <li><a href="#界面一览">界面一览</a></li>
    <li><a href="#快速上手">快速上手</a></li>
    <li><a href="#windows-开发者模式配置">Windows 开发者模式配置</a></li>
    <li><a href="#精选全球地标">精选全球地标</a></li>
    <li><a href="#开发测试说明与硬件政策">开发测试说明与硬件政策</a></li>
    <li><a href="#隐私保护与网络透明度">隐私保护与网络透明度</a></li>
    <li><a href="#免责声明与禁止滥用守则">免责声明与禁止滥用守则</a></li>
    <li><a href="#从源码构建">从源码构建</a></li>
    <li><a href="#许可协议与致谢">许可协议与致谢</a></li>
  </ol>
</details>

---

## 界面一览

| 主控台定位 | 开发者模式引导 | 隐私守则与声明 |
| :---: | :---: | :---: |
| ![主控台](docs/screenshot.png) | ![开发者指南](docs/guide.png) | ![隐私守则](docs/privacy.png) |

---

## 快速上手

### 1. 环境准备
- **操作系统**：Windows 10 或 Windows 11（x64 架构）。
- **驱动环境**：请于 Windows 安装官方 [Apple Devices（Apple 设备）](https://apps.microsoft.com/detail/9np83lwlpz9k) 应用或最新版 iTunes，以获取原生通讯驱动。
- **连接线材**：使用支持数据传输的原厂或 MFi 认证 USB 数据线。

### 2. 操作步骤
1. 从本仓库 [Releases](../../releases) 页面下载最新版 `Meridian-0.1.0-windows-x64.zip`。
2. 将压缩包**完整解压**至任意目录（请保留 `Meridian.exe` 旁边的 `engine`、`data`、`assets` 等文件夹）。
3. 双击启动 `Meridian.exe`（软件会自动适配系统语言，亦可点击右上角语言按钮自由切换）。
4. 使用 USB 数据线将外版 iPhone 连接至电脑，解锁手机并在弹出提示中点选**“信任此电脑”**。
5. 确保设备已开启“开发者模式”（若未开启，请参考下方引导），点击软件中的**“扫描设备”**。
6. 从下拉菜单中选取预设地标、点击“阁下在此”或手动输入 WGS 84 经纬度，点击**“开始定位”**。
7. 打开手机内置地图验证当前位置。
8. 体验结束后，点击**“恢复真实定位 · 阁下在此”**，并在手机地图中确认已恢复。

---

## Windows 开发者模式配置

有别于 macOS 环境下通常通过 Mac/Xcode 直接配对启用，在 Windows 平台下，受限于 iOS 底层 AMFI 安全协议，启用方式如下：

- **方式一：手机直接开启（强烈推荐）**  
  前往 iPhone“设置 → 隐私与安全性 → 开发者模式”手动开启，并依照系统提示重启手机确认即可。
- **方式二：由本软件辅助引导开启**  
  若手机设置中未显示开发者模式入口，可通过软件发起开启请求。  
  > [!IMPORTANT]
  > 受 iOS 安全握手机制要求，在 Windows 发送启用请求前，**阁下需先在手机“设置 → 面容 ID 与密码”中暂时关闭锁屏密码**（Face ID 亦会随之暂时停用）。  
  > **本软件绝不会读取、记录、保存或破解阁下的密码。**  
  > 发送指令后 iPhone 将自动重启；重启后请在手机屏幕弹出的提示中确认**“开启开发者模式”**，再点击软件的“扫描设备”。完成后请务必立即在手机上重新设置锁屏密码与 Face ID。已开启开发者模式的设备无需关闭密码。

---

## 精选全球地标

| 地区 | 地标名称 | 纬度 (Latitude) | 经度 (Longitude) | 坐标来源 |
| :--- | :--- | :---: | :---: | :--- |
| **美国** | Apple Park 总部园区 | 37.334643 | -122.008972 | 园区近似地理中心点 |
| **美国** | Apple Park Visitor Center | 37.332890 | -122.005200 | 官方零售店 JSON-LD |
| **中国香港** | Apple ifc mall | 22.284630 | 114.159172 | 官方零售店 JSON-LD |
| **日本** | Apple Marunouchi | 35.680176 | 139.763855 | 官方零售店 JSON-LD |
| **英国** | Apple Regent Street | 51.514240 | -0.142140 | 官方零售店 JSON-LD |
| **德国** | Apple Kurfürstendamm | 52.503630 | 13.328650 | 官方零售店 JSON-LD |
| **法国** | Apple Champs-Élysées | 48.872217 | 2.301233 | 官方零售店 JSON-LD |
| **新加坡** | Apple Marina Bay Sands | 1.283286 | 103.857547 | 官方零售店 JSON-LD |
| **澳大利亚** | Apple Sydney | -33.868890 | 151.206780 | 官方零售店 JSON-LD |

> [!NOTE]
> 预设坐标仅供阁下快捷选取代表性位置，各坐标点在协议层面享有同等效果。精确选点请直接输入 WGS 84 数值。

---

## 开发测试说明与硬件政策

> [!NOTE]
> **官方硬件与服务政策说明**：  
> 依据 [Apple 官方技术政策文档](https://support.apple.com/zh-cn/121115)，中国大陆购买的 iPhone（硬件型号代码以 CH/A 结尾的机型）在出厂硬件与入网许可（SKU）层面对部分系统服务存在物理限制。  
> 本工具仅基于 Apple 官方开发者接口进行前端 GPS 坐标模拟调试，**不修改 Apple 账号所属地区、不篡改出厂硬件销售区域，亦无法突破系统级硬件或服务端鉴权**。开发者在进行跨区域应用测试时，请知悉相关官方硬件与服务政策限制。

1. **工作原理与局限**：本项目通过标准 DVT 协议修改设备上报之底层 GPS 模拟坐标，属于标准调试能力。
2. **区域服务可用性**：特定系统级功能（如 Apple Intelligence）的启用取决于设备芯片（如 A17 Pro、M 系列等）、操作系统版本、系统语言、Siri 语音及 Apple 官方服务授权等多重因素。**模拟 GPS 坐标不代表也不承诺可以解锁受限的系统服务**。
3. **系统版本适配**：核心通讯协议面向 **iOS 17.4 至 iOS 27+**，固定采用经过充分验证的 `pymobiledevice3 11.12.5` RSD / DVT 通讯架构。
4. **位置恢复机制**：正常关闭窗口或点击“恢复真实定位 · 阁下在此”时，程序会主动清除模拟状态。若遇异常断开、崩溃或程序被强制终止，请重新连接后点击恢复，或重启 iPhone 即可重置。

---

## 隐私保护与网络透明度

- **零遥测与本地优先**：不记录阁下设备名称、序列号、UDID、位置轨迹或异常崩溃日志；软件不设任何后台常驻、后门或隐式数据上报机制。
- **进程内存隔离**：设备通讯标识码仅在后台进程中临时使用，界面前端仅通过随机临时凭证交互，不落盘、不写入日志。
- **网络与通信透明度**：所有位置修改均在阁下电脑与本机手机之间的 USB 数据通道内完成。
  - 首次启用开发者镜像（DDI）时，可能需连接 Apple 官方服务器安全获取匹配之签名镜像。
  - 当点击“阁下在此”获取当前参考坐标时，程序会向公开 IP 地理库发起只读 HTTPS 查询以填充经纬度，不传输任何设备硬件标识，亦不记录任何定位日志。

---

## 免责声明与禁止滥用守则

> [!IMPORTANT]
> 本项目为开源的移动开发测试与技术研究工具，严格遵循技术中立原则。

1. **用途限制**：本项目仅供移动应用开发者进行 LBS 场景调试、跨区域地图业务测试、以及逆向工程与底层通信协议的学术研究。
2. **严禁非法与违规滥用**：使用者不得将本软件用于任何违反法律法规、侵犯第三方合法权益或破坏公平竞争的行为，包括但不限于：
   - 网约车、货运、即时配送等商业运营平台的虚假接单、抢单、刷单与调度欺诈；
   - 企事业单位各类移动考勤软件的虚假定位与考勤打卡欺诈；
   - 破坏网络游戏公平竞争环境的外挂辅助、作弊或多开刷取利益行为；
   - 故意规避监管政策、侵犯任何第三方商业合同或服务条款（TOS）的行为。
3. **责任豁免**：项目作者与贡献者不对任何个人或组织因直接或间接使用、传播本软件所导致的任何法律纠纷、行政处罚、账号封禁、数据损失或硬件故障承担任何民事或刑事连带责任。**下载、克隆、编译或运行本软件，即视为您已完全阅读、理解并同意遵守本守则。**

---

## 从源码构建

### 1. WSL 交叉编译 C++ 前端
推荐在 Ubuntu 24.04 (WSL 2) 下使用 MinGW-w64 交叉编译生成 Windows 原生二进制：

```bash
sudo apt-get update
sudo apt-get install -y g++-mingw-w64-x86-64-posix cmake ninja-build
cd /mnt/c/path/to/meridian
bash scripts/build-wsl.sh
```
编译产物将输出至 `.build/windows/Meridian.exe`，已静态链接标准库，无任何第三方 UI 运行环境依赖。

### 2. Windows 冻结 Python 引擎与打包便携包
在 Windows PowerShell (Python 3.12 x64) 环境下：

```powershell
py -3.12 -m venv .build/venv
.build/venv/Scripts/python.exe -m pip install -r requirements-lock.txt pyinstaller==6.19.0
.build/venv/Scripts/python.exe -m unittest discover -s tests -v
./scripts/package.ps1 -Python .build/venv/Scripts/python.exe
```

打包脚本会自动完成：
- 执行全量单元测试与原生 UI 自动化测试；
- 冻结后端引擎至 `release/Meridian/engine`；
- 执行敏感信息与隐私安全自动化审计；
- 生成便携版 ZIP、源代码 ZIP 及对应的 SHA256 校验清单。

---

## 许可协议与致谢

- 本项目采用 **[GPL-3.0-or-later](LICENSE)** 许可协议开源。
- 底层 iOS 通讯协议基于 [pymobiledevice3](https://github.com/doronz88/pymobiledevice3)（GPL-3.0）。
- 轻量 JSON 解析采用 [nlohmann/json](https://github.com/nlohmann/json)（MIT）。
- 本项目为独立开源研究工具，与 Apple Inc. 无任何官方关联或商业隶属。
