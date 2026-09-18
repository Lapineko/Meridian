#include <windows.h>
#include <objidl.h>
#include "json.hpp"
#include <algorithm>
#include <atomic>
#include <cmath>
#include <commctrl.h>
#include <dwmapi.h>
#include <filesystem>
#include <fstream>
#include <gdiplus.h>
#include <memory>
#include <shellapi.h>
#include <string>
#include <thread>
#include <vector>
#include <windowsx.h>

using namespace Gdiplus;
using json = nlohmann::json;
namespace fs = std::filesystem;
constexpr int WIDTH = 1280, HEIGHT = 870;
constexpr UINT WM_BRIDGE = WM_APP + 1;
const Color BG(248, 247, 243), INK(29, 48, 41), MUTED(111, 122, 112), GREEN(37, 81, 62), LINE(224, 226, 216),
    WHITE(255, 255, 252), SAGE(233, 238, 226);
enum class Lang { ZH_CN = 0, ZH_HK = 1, EN = 2 };
inline Lang detectUserLanguage() {
    LANGID langId = GetUserDefaultUILanguage();
    WORD primary = PRIMARYLANGID(langId);
    WORD sub = SUBLANGID(langId);
    if (primary == LANG_CHINESE) {
        if (sub == SUBLANG_CHINESE_TRADITIONAL ||
            sub == SUBLANG_CHINESE_HONGKONG ||
            sub == SUBLANG_CHINESE_MACAU) {
            return Lang::ZH_HK;
        }
        return Lang::ZH_CN;
    }
    return Lang::EN;
}
Lang currentLang = Lang::ZH_CN;

inline const wchar_t *tr(const wchar_t *zh_cn, const wchar_t *zh_hk, const wchar_t *en) {
    if (currentLang == Lang::ZH_HK) return zh_hk;
    if (currentLang == Lang::EN) return en;
    return zh_cn;
}

std::wstring trErrorCode(const std::string &code) {
    if (code == "driver")
        return tr(L"无法连接 Apple 设备服务。请安装 Apple Devices 或 iTunes，解锁并信任此电脑。",
                  L"無法連接 Apple 設備服務。請安裝 Apple Devices 或 iTunes，解鎖並信任此電腦。",
                  L"Cannot connect to Apple device service. Install Apple Devices or iTunes, unlock & trust PC.");
    if (code == "device")
        return tr(L"设备已断开或选取已过期，请重新扫描。",
                  L"設備已斷開或選取已過期，請重新掃描。",
                  L"Device disconnected or selection expired. Please re-scan.");
    if (code == "trust")
        return tr(L"请解锁 iPhone，并在手机上选择「信任此电脑」。",
                  L"請解鎖 iPhone，並在手機上選擇「信任此電腦」。",
                  L"Please unlock iPhone and tap 'Trust This Computer'.");
    if (code == "developer")
        return tr(L"请先开启开发者模式，再开始定位。",
                  L"請先開啟開發者模式，再開始定位。",
                  L"Please enable Developer Mode before starting location simulation.");
    if (code == "passcode")
        return tr(L"设备密码阻止了自动开启。请在手机设置中开启开发者模式，或临时关闭锁屏密码后重试。",
                  L"設備密碼阻止了自動開啟。請在手機設定中開啟開發者模式，或臨時關閉鎖屏密碼後重試。",
                  L"Passcode blocked automatic enable. Enable in Settings or temporarily disable passcode.");
    if (code == "version")
        return tr(L"此版本使用 iOS 17.4 及以上的 USB 通道；当前系统不适用。",
                  L"此版本使用 iOS 17.4 及以上的 USB 通道；當前系統不適用。",
                  L"Requires iOS 17.4+ USB tunnel; current system is not supported.");
    if (code == "coordinates")
        return tr(L"坐标无效：纬度 −90 至 90，经度 −180 至 180。",
                  L"座標無效：緯度 −90 至 90，經度 −180 至 180。",
                  L"Invalid coordinates: Latitude -90 to 90, Longitude -180 to 180.");
    if (code == "busy")
        return tr(L"请先结束当前操作或恢复真实定位。",
                  L"請先結束當前操作或恢復真實定位。",
                  L"Please finish the current operation or restore real location first.");
    if (code == "timeout")
        return tr(L"操作超时。请检查 USB 连接、信任提示和网络，然后重试。",
                  L"操作超時。請檢查 USB 連接、信任提示和網絡，然後重試。",
                  L"Operation timed out. Check USB connection, trust prompt and network.");
    if (code == "mount")
        return tr(L"开发者镜像未就绪。请确认开发者模式、网络连接及当前 iOS 的镜像支持。",
                  L"開發者鏡像未就緒。請確認開發者模式、網絡連接及當前 iOS 的鏡像支援。",
                  L"Developer image not ready. Check Developer Mode, internet and iOS support.");
    if (code == "restore")
        return tr(L"恢复定位未获设备确认。请重新连接后点击恢复；必要时重启 iPhone。",
                  L"恢復定位未獲設備確認。請重新連接後點擊恢復；必要時重啟 iPhone。",
                  L"Restoration unconfirmed. Reconnect and click restore, or reboot iPhone.");
    if (code == "protocol")
        return tr(L"请求格式不正确，请重新启动 Meridian。",
                  L"請求格式不正確，請重新啟動 Meridian。",
                  L"Invalid protocol request. Please restart Meridian.");
    return tr(L"设备操作未完成。请检查连接与开发者模式后重试。",
              L"設備操作未完成。請檢查連接與開發者模式後重試。",
              L"Device operation incomplete. Check connection and Developer Mode.");
}

std::wstring trState(const std::string &st) {
    if (st == "mounting")
        return tr(L"正在准备开发者镜像；首次使用可能需要联网下载…",
                  L"正在準備開發者鏡像；首次使用可能需要聯網下載…",
                  L"Preparing developer image (may download on first run)...");
    if (st == "connecting")
        return tr(L"正在建立设备定位通道…",
                  L"正在建立設備定位通道…",
                  L"Establishing location tunnel...");
    if (st == "active")
        return tr(L"定位会话进行中 · 请保持 USB 连接",
                  L"定位會話進行中 · 請保持 USB 連接",
                  L"Location session active · Keep USB connected");
    if (st == "restoring")
        return tr(L"正在恢复真实定位…",
                  L"正在恢復真實定位…",
                  L"Restoring real location...");
    if (st == "restored")
        return tr(L"已请求恢复真实定位，请在 iPhone 地图中确认。",
                  L"已請求恢復真實定位，請在 iPhone 地圖中確認。",
                  L"Real location restored. Please verify in iPhone Maps.");
    if (st == "scanning")
        return tr(L"正在寻找通过 USB 连接的设备…",
                  L"正在尋找透過 USB 連接的設備…",
                  L"Scanning for USB-connected devices...");
    if (st == "enabling")
        return tr(L"正在请求开启开发者模式…",
                  L"正在請求開啟開發者模式…",
                  L"Requesting Developer Mode...");
    if (st == "restart_required")
        return tr(L"请在 iPhone 重启后确认开启，再扫描设备。完成后可重新设置密码和 Face ID。",
                  L"請在 iPhone 重啟後確認開啟，再掃描設備。完成後可重新設置密碼和 Face ID。",
                  L"Confirm Developer Mode after reboot, then re-scan. Remember to re-enable Passcode & Face ID.");
    if (st == "enabled")
        return tr(L"开发者模式已开启，请重新扫描设备。",
                  L"開發者模式已開啟，請重新掃描設備。",
                  L"Developer Mode enabled. Please re-scan devices.");
    return {};
}

struct Place {
    std::wstring name, city, cityZhCn, cityZhHk, cityEn, region;
    double lat{}, lon{};
    std::wstring getCity() const {
        if (currentLang == Lang::ZH_HK) return cityZhHk.empty() ? city : cityZhHk;
        if (currentLang == Lang::EN) return cityEn.empty() ? city : cityEn;
        return cityZhCn.empty() ? city : cityZhCn;
    }
};
struct Control {
    int id;
    HWND window;
    RectF rect;
    std::wstring title, subtitle, tag;
    int page;
};
struct Device {
    std::string id;
    std::wstring label;
    bool developer{}, trusted{};
};
HWND windowHandle{}, latEdit{}, lonEdit{}, deviceCombo{};
HFONT editFont{};
HBRUSH editBrush{};
float scale = 1;
int page = 0, selection = 0, deviceIndex = -1;
bool busy = false, active = false, stopping = false, closing = false, workerReady = false, updating = false,
     restorationWarning = false;
std::wstring status = L"連接 iPhone，開啟屬於閣下的自由漫遊。", statusCode = L"idle";
std::vector<Place> places;
std::vector<std::vector<PointF>> lands;
std::vector<Control> controls;
std::vector<Device> devices;
double latitude = 37.334643, longitude = -122.008972;
fs::path appDir;
HANDLE processHandle{}, inputPipe{}, jobHandle{};
std::thread readerThread;
std::atomic_bool readerFinished{false};
ULONGLONG operationStart{};

std::wstring wide(const std::string &s) {
    if (s.empty())
        return {};
    int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), nullptr, 0);
    std::wstring out(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.data(), (int)s.size(), out.data(), n);
    return out;
}
std::wstring number(double d) {
    wchar_t b[48];
    swprintf(b, 48, L"%.6f", d);
    return b;
}
std::wstring textOf(HWND h) {
    int n = GetWindowTextLengthW(h);
    std::wstring s(n + 1, 0);
    GetWindowTextW(h, s.data(), n + 1);
    s.resize(n);
    return s;
}
void refresh() {
    InvalidateRect(windowHandle, nullptr, FALSE);
    for (auto &c : controls)
        InvalidateRect(c.window, nullptr, FALSE);
}
void rounded(Graphics &g, RectF r, float radius, Color color, Color border = Color(0, 0, 0, 0)) {
    GraphicsPath p;
    float d = radius * 2;
    p.AddArc(r.X, r.Y, d, d, 180, 90);
    p.AddArc(r.GetRight() - d, r.Y, d, d, 270, 90);
    p.AddArc(r.GetRight() - d, r.GetBottom() - d, d, d, 0, 90);
    p.AddArc(r.X, r.GetBottom() - d, d, d, 90, 90);
    p.CloseFigure();
    SolidBrush b(color);
    g.FillPath(&b, &p);
    if (border.GetA()) {
        Pen pen(border, 1);
        g.DrawPath(&pen, &p);
    }
}
void text(Graphics &g, const std::wstring &s, float x, float y, float w, float h, float size,
          Color color = INK, bool bold = false, const wchar_t *face = L"Microsoft YaHei UI",
          StringAlignment align = StringAlignmentNear) {
    Font font(face, size, bold ? FontStyleBold : FontStyleRegular, UnitPixel);
    SolidBrush brush(color);
    StringFormat format;
    format.SetAlignment(align);
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetTrimming(StringTrimmingEllipsisCharacter);
    g.DrawString(s.c_str(), (INT)s.size(), &font, RectF(x, y, w, h), &format, &brush);
}
void line(Graphics &g, float x1, float y1, float x2, float y2, Color col = LINE, float width = 1) {
    Pen pen(col, width);
    g.DrawLine(&pen, x1, y1, x2, y2);
}
void circle(Graphics &g, float x, float y, float radius, Color col) {
    SolidBrush b(col);
    g.FillEllipse(&b, x - radius, y - radius, 2 * radius, 2 * radius);
}
void globe(Graphics &g, float x, float y, float r, Color col) {
    Pen p(col, 1.6f);
    g.DrawEllipse(&p, x - r, y - r, r * 2, r * 2);
    g.DrawEllipse(&p, x - r * .46f, y - r, r * .92f, r * 2);
    g.DrawLine(&p, x - r, y, x + r, y);
}

PointF project(double lat, double lon) {
    return PointF(74.f + (float)(lon + 180) / 360 * 750.f, 266.f + (float)(85 - lat) / 150 * 232.f);
}
RectF mapRect(36, 218, 826, 330);
void drawMap(Graphics &g) {
    rounded(g, mapRect, 20, Color(236, 239, 230), LINE);
    text(g, L"THE WORLD, AT YOUR COORDINATES", 58, 233, 500, 22, 10, MUTED, true, L"Segoe UI");
    text(g, tr(L"点击画布选点", L"點選畫布選點", L"Click map to pick"), 652, 233, 188, 22, 11, MUTED, false, L"Microsoft YaHei UI",
         StringAlignmentFar);
    auto state = g.Save();
    g.SetClip(RectF(54, 262, 790, 238));
    for (int lon = -180; lon <= 180; lon += 30) {
        auto a = project(85, lon), b = project(-65, lon);
        line(g, a.X, a.Y, b.X, b.Y, Color(221, 226, 215));
    }
    for (int lat = -60; lat <= 80; lat += 30) {
        auto a = project(lat, -180), b = project(lat, 180);
        line(g, a.X, a.Y, b.X, b.Y, Color(221, 226, 215));
    }
    SolidBrush land(Color(205, 215, 198));
    Pen coast(Color(193, 206, 186), .65f);
    for (auto &polygon : lands) {
        std::vector<PointF> points;
        points.reserve(polygon.size());
        for (auto p : polygon)
            points.push_back(project(p.Y, p.X));
        if (points.size() > 2) {
            g.FillPolygon(&land, points.data(), (INT)points.size());
            g.DrawPolygon(&coast, points.data(), (INT)points.size());
        }
    }
    for (size_t i = 0; i < places.size(); ++i) {
        auto p = project(places[i].lat, places[i].lon);
        circle(g, p.X, p.Y, 3.5f, Color(116, 144, 104));
    }
    auto p = project(latitude, longitude);
    circle(g, p.X, p.Y, 18, Color(30, 37, 81, 62));
    circle(g, p.X, p.Y, 10, Color(45, 37, 81, 62));
    circle(g, p.X, p.Y, 5, GREEN);
    circle(g, p.X, p.Y, 2, WHITE);
    if (latitude >= -65 && latitude <= 85) {
        float bw = 140.f;
        std::wstring pinLabel = tr(L"阁下在此", L"閣下在此", L"You Are Here");
        if (selection >= 0 && selection < (int)places.size()) {
            pinLabel = tr(L"阁下在此 · ", L"閣下在此 · ", L"Here · ") + places[selection].name;
            bw = 180.f;
        }
        float bx = std::clamp(p.X - bw * 0.5f, 62.f, 826.f - bw - 20.f);
        float by = p.Y > 305 ? p.Y - 44 : p.Y + 21;
        rounded(g, {bx, by, bw, 29}, 9, GREEN);
        text(g, pinLabel, bx + 6, by, bw - 12, 29, 11, WHITE, true,
             L"Microsoft YaHei UI", StringAlignmentCenter);
    }
    g.Restore(state);
    text(g, L"WGS 84", 58, 506, 82, 22, 11, MUTED, true, L"Consolas");
    text(g, number(latitude) + L"°  /  " + number(longitude) + L"°", 151, 506, 445, 22, 12, INK, false,
         L"Consolas");
    text(g, tr(L"全球坐标网格 · WGS 84", L"全球座標網格 · WGS 84", L"Global Grid · WGS 84"), 624, 506, 216, 22, 10, MUTED, false, L"Microsoft YaHei UI",
         StringAlignmentFar);
}

void drawHome(Graphics &g) {
    if (currentLang == Lang::EN) {
        text(g, L"Your place, your pace.", 36, 110, 850, 65, 44, INK, true, L"Segoe UI");
    } else {
        text(g, tr(L"边界之外，自有方向。", L"邊界之外，自有方向。", L""), 36, 106, 850, 65, 36, INK, true);
        text(g, L"Your place Your pace", 39, 171, 710, 25, 14, MUTED, false, L"Segoe UI");
    }
    rounded(g, {1088, 133, 156, 30}, 15, SAGE);
    circle(g, 1104, 148, 3, GREEN);
    text(g, tr(L"自由开源 · 本地运行", L"自由開源 · 本地運行", L"Open Source · Local"), 1115, 133, 116, 30, 11, GREEN);
    drawMap(g);
    text(g, tr(L"精选目的地", L"精選目的地", L"Featured Destinations"), 36, 566, 300, 32, 19, INK, true);
    text(g, L"09  /  APPLE LANDMARKS", 530, 568, 329, 30, 10, MUTED, true, L"Consolas", StringAlignmentFar);
    rounded(g, {886, 218, 358, 592}, 20, WHITE, LINE);
    text(g, tr(L"阁下的设备", L"閣下的設備", L"Your Device"), 908, 239, 200, 27, 17, INK, true);
    circle(g, 920, 288, 4, active ? GREEN : Color(184, 151, 81));
    text(g,
         devices.empty() ? tr(L"等待 USB 连接", L"等待 USB 連接", L"Waiting for USB")
         : active        ? tr(L"定位漫游中", L"定位漫遊中", L"Simulating Location")
                         : tr(L"设备已连接", L"設備已連接", L"Device Connected"),
         934, 274, 278, 28, 13, INK);
    if (devices.empty())
        text(g, tr(L"解锁 iPhone，并信任此电脑", L"解鎖 iPhone，並信任此電腦", L"Unlock iPhone & trust computer"), 909, 310, 304, 25, 12, MUTED);
    text(g,
         deviceIndex >= 0 && devices[deviceIndex].developer ? tr(L"开发者模式  ·  已就绪", L"開發者模式  ·  已就緒", L"Developer Mode  ·  Ready")
                                                            : tr(L"开发者模式  ·  尚未确认", L"開發者模式  ·  尚未確認", L"Developer Mode  ·  Unconfirmed"),
         909, 351, 304, 25, 12, MUTED);
    line(g, 909, 427, 1221, 427);
    text(g, tr(L"目标位置", L"目標位置", L"Target Location"), 909, 442, 175, 22, 11, MUTED);
    text(g, selection >= 0 ? places[selection].name : tr(L"自订坐标", L"自訂座標", L"Custom Coordinates"), 909, 467, 313, 30, 18, INK, true,
         L"Segoe UI");
    text(g, selection >= 0 ? places[selection].getCity() : tr(L"点击画布或直接填入经纬度", L"點選畫布或直接填入經緯度", L"Click canvas or enter coordinates"), 909, 501, 313, 24, 12,
         MUTED);
    text(g, tr(L"纬度  LATITUDE", L"緯度  LATITUDE", L"LATITUDE"), 909, 540, 146, 22, 10, MUTED, true, L"Segoe UI");
    text(g, tr(L"经度  LONGITUDE", L"經度  LONGITUDE", L"LONGITUDE"), 1072, 540, 146, 22, 10, MUTED, true, L"Segoe UI");
    rounded(g, {907, 568, 150, 44}, 9, BG, LINE);
    rounded(g, {1070, 568, 152, 44}, 9, BG, LINE);
    Color sc = restorationWarning ? Color(161, 83, 37) : MUTED;
    text(g, status, 909, 721, 312, 67, 11, sc);
    text(g, tr(L"边界之外，自有方向。", L"邊界之外，自有方向。", L"Your place, your pace."), 37, 832, 250, 20, 11, MUTED);
    text(g, L"MERIDIAN  0.1   /   USB · iOS 17.4+", 859, 832, 384, 20, 10, MUTED, false, L"Consolas",
         StringAlignmentFar);
}

void drawGuide(Graphics &g) {
    text(g, tr(L"简单数步，开展自由之旅。", L"簡單數步，開展自由之旅。", L"A Few Simple Steps to Freedom."), 36, 110, 1100, 63, 36, INK, true);
    text(g, tr(L"连接阁下的 iPhone，探索无拘无束的数字世界。", L"連接閣下的 iPhone，探索無拘無束的數位世界。", L"Connect your iPhone and explore without boundaries."), 39, 173, 1120, 24, 14, MUTED);
    std::wstring titles[] = {
        tr(L"建立信任", L"建立信任", L"Establish Trust"),
        tr(L"开发者模式", L"開發者模式", L"Developer Mode"),
        tr(L"选取坐标，开展定位", L"選取座標，開展定位", L"Pick & Simulate")
    };
    std::wstring bodies[] = {
        tr(L"于 Windows 安装 Apple Devices 或 iTunes。\n使用支持数据传输的 USB 线连接。\n解锁 iPhone，点击「信任此电脑」。",
           L"於 Windows 安裝 Apple Devices 或 iTunes。\n使用支援數據傳輸的 USB 線連接。\n解鎖 iPhone，點選「信任此電腦」。",
           L"Install Apple Devices or iTunes on Windows.\nConnect via USB data cable.\nUnlock iPhone and tap 'Trust This Computer'."),
        tr(L"优先在「设置 → 隐私与安全性」中开启。\n若由本软件在 Windows 引导开启，需先临时关闭\n锁屏密码（Face ID 亦会随之停用）。",
           L"優先在「設定 → 私隱與保安」中開啟。\n若由本軟件在 Windows 引導開啟，需先臨時關閉\n鎖屏密碼（Face ID 亦會隨之停用）。",
           L"Enable in iPhone 'Settings → Privacy & Security'.\nIf initiating from Meridian on Windows, temporarily\nturn off passcode (Face ID is also disabled)."),
        tr(L"返回工作台，扫描并选取阁下的设备。\n点击地标或输入坐标，点击「开始定位」。\n结束时点击「恢复真实定位」。",
           L"返回工作台，掃描並選取閣下的設備。\n點選地標或輸入座標，點擊「開始定位」。\n結束時點擊「恢復真實定位」。",
           L"Return to workspace, scan & select your device.\nPick a landmark or enter coordinates, click Start.\nClick Restore when finished.")
    };
    for (int i = 0; i < 3; ++i) {
        float x = 36 + i * 412.f;
        rounded(g, {x, 238, 384, 257}, 18, WHITE, LINE);
        text(g, L"0" + std::to_wstring(i + 1), x + 24, 258, 150, 50, 35, GREEN, false, L"Georgia");
        text(g, titles[i], x + 24, 327, 335, 34, 21, INK, true);
        text(g, bodies[i], x + 24, 380, 336, 85, 13, MUTED);
    }
    rounded(g, {36, 520, 1208, 230}, 18, SAGE);
    text(g, tr(L"开启后，请重启密码保护。", L"開啟後，請重啟密碼保護。", L"Re-enable Passcode Protection Once Enabled."), 60, 542, 1128, 36, 22, GREEN, true);
    text(g,
         tr(L"软件发起开启后，iPhone 将自动重启；请在手机屏幕确认开启开发者模式，再重新扫描。随后请务必重新设置锁屏密码与 Face ID。",
            L"軟件發起開啟後，iPhone 將自動重啟；請在手機屏幕確認開啟開發者模式，再重新掃描。隨後請務必重新設置鎖屏密碼與 Face ID。",
            L"iPhone reboots automatically; confirm Developer Mode on screen, then re-scan in Meridian. Always re-enable your Passcode & Face ID immediately."),
         60, 590, 1128, 38, 14, INK);
    text(g,
         tr(L"密码只能由阁下于手机「设置」中手动关闭，本软件绝不会读取密码或移除 Face ID。已开启开发者模式的设备无需关闭密码。",
            L"密碼只能由閣下於手機「設定」中手動關閉，本軟件絕不會讀取密碼或移除 Face ID。已開啟開發者模式的設備無需關閉密碼。",
            L"Passcode can only be disabled manually by you in Settings. Meridian never reads passcodes or removes Face ID. Devices already in Developer Mode do not need passcode turned off."),
         60, 640, 1128, 36, 13, MUTED);
    text(g, tr(L"镜像与兼容性：首次连接可能联网下载开发者镜像；不同 iOS 版本需要相应镜像支持。",
               L"鏡像與兼容性：首次連接可能聯網下載開發者鏡像；不同 iOS 版本需要相應鏡像支援。",
               L"Images & Compatibility: First connection may download Developer Disk Images. Different iOS versions require matching images."), 60, 688, 1128, 25,
         12, MUTED);
}
void drawAbout(Graphics &g) {
    text(g, tr(L"轻盈纯粹，隐私至上。", L"輕盈純粹，私隱至上。", L"Pure, Light, Privacy-First."), 36, 110, 1120, 63, 36, INK, true);
    text(g, tr(L"Meridian 子午线 · 原生 iOS 位置模拟与开发者调试工作台。", L"Meridian 子午線 · 原生 iOS 位置模擬與開發者調試工作台。", L"Meridian · Native iOS Location Simulation & Dev Studio."), 39, 173, 1120, 24, 14, MUTED);
    rounded(g, {36, 238, 580, 475}, 18, WHITE, LINE);
    rounded(g, {640, 238, 604, 475}, 18, SAGE);
    text(g, tr(L"阁下的设备隐私，永留本机", L"閣下的設備私隱，永留本機", L"Your Device Privacy Stays Local"), 61, 263, 528, 40, 23, INK, true);
    text(g,
         tr(L"不记录个人设备名称、序列号、UDID 或位置历史。\n绝无遥测追踪，不设账户系统，无原始日志上传。\n界面仅显示匿名设备序号及 iOS 系统版本。",
            L"不記錄個人設備名稱、序號、UDID 或位置歷史。\n絕無遙測追蹤，不設帳戶系統，無原始日誌上載。\n介面僅顯示匿名設備序號及 iOS 系統版本。",
            L"Never logs personal device names, serials, UDID or history.\nZero telemetry, no accounts, no log uploads.\nUI shows only anonymous indices and iOS versions."),
         61, 328, 526, 110, 15, INK);
    text(g,
         tr(L"所有计算与设备通信均在阁下的本机电脑完成。\n代码全量开源透明，遵循 GPL-3.0 自由软件协议。\n无网络跟踪，阁下的设备隐私由阁下自主掌控。",
            L"所有計算與設備通信均在閣下的本機電腦完成。\n代碼全量開源透明，遵循 GPL-3.0 自由軟件協議。\n無網絡跟踪，閣下的設備私隱由閣下自主掌控。",
            L"All computation and device protocols run on your local PC.\n100% open-source under GNU General Public License v3.0.\nZero tracking. Your privacy stays in your hands."),
         61, 460, 526, 100, 13, MUTED);
    text(g, tr(L"100% 开源透明 · 本机运算 · 零数据收集", L"100% 開源透明 · 本機運算 · 零數據收集", L"100% Open Source · Local Compute · Zero Tracking"), 61, 614, 526, 40, 13, GREEN);
    text(g, tr(L"开发者调试与合规守则", L"開發者調試與合規守則", L"Developer Testing & Ethics Notice"), 665, 263, 552, 40, 23, GREEN, true);
    text(g,
         tr(L"Meridian 专为移动开发者与研究人员测试 LBS 场景而设计，\n基于 Apple 官方 DVT 协议模拟坐标。严禁用于任何\n商业欺诈、虚假考勤、游戏作弊或规避法律政策之行为。",
            L"Meridian 專為移動開發者與研究人員測試 LBS 場景而設計，\n基於 Apple 官方 DVT 協定模擬坐標。嚴禁用於任何\n商業欺詐、虛假考勤、遊戲作弊或規避法律政策之行為。",
            L"Meridian is designed for developers testing LBS scenarios\nusing Apple DVT protocols. Any use for commercial fraud, fake\nattendance, game cheating, or legal circumvention is prohibited."),
         665, 328, 552, 110, 15, INK);
    text(g,
         tr(L"功能可用性受操作系统、机型与服务提供商政策约束。\n坐标修改不改变硬件型号或服务器端账号权限。\n预设地标仅供开发者快捷调试代表性地理场景。",
            L"功能可用性受操作系統、機型與服務提供商政策約束。\n坐標修改不改變硬體型號或伺服器端帳號權限。\n預設地標僅供開發者快捷調試代表性地理場景。",
            L"Feature availability depends on OS, model, and service policies.\nSimulating coordinates does not alter hardware SKUs or account flags.\nPresets are provided solely for representative testing convenience."),
         665, 460, 552, 90, 13, MUTED);
    text(g, tr(L"C++ 原生窗口  /  pymobiledevice3 设备引擎\nGPLv3 开源 · 边界之外，自有方向 · v0.1.0",
               L"C++ 原生視窗  /  pymobiledevice3 設備引擎\nGPLv3 開源 · 邊界之外，自有方向 · v0.1.0",
               L"C++ Native Window  /  pymobiledevice3 Engine\nGPLv3 Open Source · Your place, your pace · v0.1.0"), 665, 614,
         552, 50, 13, GREEN);
}

void paint(Graphics &g) {
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    g.Clear(BG);
    globe(g, 55, 48, 18, GREEN);
    text(g, L"meridian", 88, 20, 186, 44, 29, INK, true, L"Segoe UI");
    text(g, tr(L"子午线", L"子午線", L"iOS Studio"), 246, 33, 108, 28, 12, MUTED);
    line(g, 36, 91, 1244, 91);
    if (page == 0)
        drawHome(g);
    else if (page == 1)
        drawGuide(g);
    else
        drawAbout(g);
}

Control *findControl(int id) {
    for (auto &c : controls)
        if (c.id == id)
            return &c;
    return nullptr;
}
void drawControl(Graphics &g, Control &c, bool hover, bool pressed, bool focused, bool enabled) {
    RectF r(0, 0, c.rect.Width, c.rect.Height);
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintClearTypeGridFit);
    SolidBrush base(c.id >= 200 ? WHITE : BG);
    g.FillRectangle(&base, r);
    bool selected = c.id >= 100 && c.id < 109 && selection == c.id - 100;
    if (c.id >= 100 && c.id < 109) {
        rounded(g, {1, 1, r.Width - 2, r.Height - 2}, 12,
                selected ? SAGE
                : hover  ? Color(242, 245, 236)
                         : WHITE,
                selected ? Color(130, 156, 115) : LINE);
        rounded(g, {13, 16, 34, 32}, 8, selected ? GREEN : BG);
        text(g, c.tag, 13, 16, 34, 32, 10, selected ? WHITE : MUTED, true, L"Consolas",
             StringAlignmentCenter);
        text(g, c.title, 57, 9, r.Width - 69, 25, 12, INK, true, L"Segoe UI");
        text(g, c.subtitle, 57, 34, r.Width - 69, 22, 10, MUTED);
    } else if (c.id < 10) {
        bool chosen = page == c.id - 1;
        bool isLang = c.id == 4;
        rounded(g, {0, 0, r.Width, r.Height}, 12, chosen ? SAGE : (isLang && hover ? SAGE : BG));
        text(g, c.title, 0, 0, r.Width, r.Height, 13, (chosen || isLang) ? GREEN : MUTED, chosen, L"Microsoft YaHei UI",
             StringAlignmentCenter);
    } else if (c.id == 206) {
        rounded(g, {1, 1, r.Width - 2, r.Height - 2}, 6, hover ? SAGE : WHITE, LINE);
        text(g, c.title, 0, 0, r.Width, r.Height, 10, GREEN, true, L"Microsoft YaHei UI", StringAlignmentCenter);
    } else {
        bool primary = c.id == 202;
        Color fill = primary ? (enabled ? (pressed ? Color(25, 60, 45) : GREEN) : Color(166, 182, 170))
                             : (hover ? SAGE : WHITE);
        rounded(g, {1, 1, r.Width - 2, r.Height - 2}, 10, fill, primary ? fill : LINE);
        std::wstring label = c.title;
        if (c.id == 202)
            label = active ? tr(L"定位进行中", L"定位進行中", L"Simulating...")
                   : busy  ? tr(L"正在准备…", L"正在準備…", L"Preparing...")
                           : tr(L"开始定位     →", L"開始定位     →", L"Start Simulation  →");
        if (c.id == 203)
            label = busy && !active ? tr(L"停止操作 / 恢复", L"停止操作 / 恢復", L"Stop & Restore")
                                    : tr(L"恢复真实定位 · 阁下在此", L"恢復真實定位 · 閣下在此", L"Restore Real Location");
        text(g, label, 8, 0, r.Width - 16, r.Height, 12,
             primary   ? WHITE
             : enabled ? INK
                       : Color(167, 174, 166),
             primary, L"Microsoft YaHei UI", StringAlignmentCenter);
    }
    if (focused) {
        Pen pen(GREEN, 1);
        pen.SetDashStyle(DashStyleDot);
        g.DrawRectangle(&pen, 4.f, 4.f, r.Width - 8, r.Height - 8);
    }
}

bool send(const json &command) {
    if (!inputPipe)
        return false;
    std::string data = command.dump() + "\n";
    DWORD written = 0;
    return WriteFile(inputPipe, data.data(), (DWORD)data.size(), &written, nullptr) && written == data.size();
}
void updateControls() {
    bool selected = deviceIndex >= 0 && deviceIndex < (int)devices.size();
    for (auto &c : controls) {
        bool enabled = true;
        if (c.id == 200 || c.id == 201)
            enabled = workerReady && !busy && (c.id == 200 || selected);
        if (c.id == 202)
            enabled = workerReady && !busy && selected && devices[deviceIndex].trusted &&
                      devices[deviceIndex].developer;
        if (c.id == 203)
            enabled = workerReady && ((busy && !stopping) || (!busy && selected));
        if (c.id == 206)
            enabled = !busy;
        if (c.id >= 100 && c.id < 109)
            enabled = !busy;
        EnableWindow(c.window, enabled);
    }
    EnableWindow(latEdit, !busy);
    EnableWindow(lonEdit, !busy);
    EnableWindow(deviceCombo, !busy);
    refresh();
}
void layout() {
    RECT r{};
    GetClientRect(windowHandle, &r);
    scale = std::min((float)r.right / WIDTH, (float)r.bottom / HEIGHT);
    for (auto &c : controls) {
        MoveWindow(c.window, (int)(c.rect.X * scale), (int)(c.rect.Y * scale), (int)(c.rect.Width * scale),
                   (int)(c.rect.Height * scale), TRUE);
        ShowWindow(c.window, c.page == -1 || c.page == page ? SW_SHOW : SW_HIDE);
    }
    auto move = [](HWND h, int x, int y, int w, int height) {
        MoveWindow(h, (int)(x * scale), (int)(y * scale), (int)(w * scale), (int)(height * scale), TRUE);
        ShowWindow(h, page == 0 ? SW_SHOW : SW_HIDE);
    };
    move(latEdit, 917, 579, 130, 24);
    move(lonEdit, 1080, 579, 133, 24);
    move(deviceCombo, 908, 310, 313, 220);
    ShowWindow(deviceCombo, page == 0 && !devices.empty() ? SW_SHOW : SW_HIDE);
    if (editFont)
        DeleteObject(editFont);
    editFont =
        CreateFontW(-(int)(13 * scale), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    for (HWND h : {latEdit, lonEdit, deviceCombo})
        SendMessageW(h, WM_SETFONT, (WPARAM)editFont, TRUE);
    refresh();
}
void setCoordinates(double lat, double lon, int index) {
    latitude = lat;
    longitude = lon;
    selection = index;
    updating = true;
    SetWindowTextW(latEdit, number(lat).c_str());
    SetWindowTextW(lonEdit, number(lon).c_str());
    updating = false;
    refresh();
}
bool parseCoordinates() {
    auto parse = [](HWND h, double &v) {
        auto s = textOf(h);
        if (s.empty())
            return false;
        wchar_t *end = nullptr;
        v = wcstod(s.c_str(), &end);
        while (end && *end == L' ')
            ++end;
        return end != s.c_str() && end && !*end && std::isfinite(v);
    };
    double a{}, b{};
    if (!parse(latEdit, a) || !parse(lonEdit, b) || std::abs(a) > 90 || std::abs(b) > 180)
        return false;
    latitude = a;
    longitude = b;
    return true;
}
void begin(json command) {
    if (busy)
        return;
    if (command["action"] != "scan") {
        if (deviceIndex < 0)
            return;
        command["device"] = devices[deviceIndex].id;
    }
    if (!send(command)) {
        status = tr(L"设备引擎尚未就绪，请重新启动应用。", L"設備引擎尚未就緒，請重新啟動應用程式。", L"Engine not ready. Please restart Meridian.");
        refresh();
        return;
    }
    busy = true;
    stopping = false;
    restorationWarning = false;
    operationStart = GetTickCount64();
    status = tr(L"正在处理，请稍候…", L"正在處理，請稍候…", L"Processing, please wait...");
    updateControls();
}
void updateLanguage();
void toggleLanguage();

void click(int id) {
    if (id >= 1 && id <= 3) {
        page = id - 1;
        layout();
        return;
    }
    if (id == 4) {
        toggleLanguage();
        return;
    }
    if (id >= 100 && id < 109 && !busy) {
        auto &p = places[id - 100];
        setCoordinates(p.lat, p.lon, id - 100);
        return;
    }
    if (id == 200) {
        begin({{"action", "scan"}});
        return;
    }
    if (id == 201) {
        int choice =
            MessageBoxW(windowHandle,
                        tr(L"【开启开发者模式引导】\n\n"
                           L"1. 优先推荐在 iPhone「设置 → 隐私与安全性 → 开发者模式」手动开启。\n\n"
                           L"2. 若由本软件在 Windows 环境下发起请求，因 iOS 安全机制限制，阁下需先于手机临时关闭锁屏密码（Face ID 亦随之停用）。\n"
                           L"   软件绝不会读取或记录阁下的密码。\n\n"
                           L"3. 发起后 iPhone 将自动重启；重启后请在手机屏幕确认开启开发者模式，随后请务必重新设置锁屏密码与 Face ID。\n\n"
                           L"是否现在向所选设备发送开启请求？",
                           L"【開啟開發者模式引導】\n\n"
                           L"1. 優先推薦在 iPhone「設定 → 私隱與保安 → 開發者模式」手動開啟。\n\n"
                           L"2. 若由本軟件在 Windows 環境下發起請求，因 iOS 安全機制限制，閣下需先於手機臨時關閉鎖屏密碼（Face ID 亦隨之停用）。\n"
                           L"   軟件絕不會讀取或記錄閣下的密碼。\n\n"
                           L"3. 發起後 iPhone 將自動重啟；重啟後請在手機屏幕確認開啟開發者模式，隨後請務必重新設置鎖屏密碼與 Face ID。\n\n"
                           L"是否現在向所選設備發送開啟請求？",
                           L"[Enable Developer Mode Guide]\n\n"
                           L"1. Recommended: Enable manually in iPhone 'Settings → Privacy & Security → Developer Mode'.\n\n"
                           L"2. If initiating via Meridian on Windows, iOS requires temporarily turning off your lock screen passcode (Face ID will also be disabled).\n"
                           L"   Meridian never reads or stores your passcode.\n\n"
                           L"3. iPhone reboots automatically. Confirm Developer Mode on screen after restart, then re-enable Passcode & Face ID.\n\n"
                           L"Send Developer Mode request now?"),
                        tr(L"开启开发者模式", L"開啟開發者模式", L"Enable Developer Mode"), MB_OKCANCEL | MB_ICONINFORMATION | MB_DEFBUTTON2);
        if (choice == IDOK)
            begin({{"action", "enable_developer"}});
        return;
    }
    if (id == 202) {
        if (!parseCoordinates()) {
            status = tr(L"坐标无效：纬度 −90 至 90，经度 −180 至 180。",
                        L"座標無效：緯度 −90 至 90，經度 −180 至 180。",
                        L"Invalid coordinates: Latitude -90 to 90, Longitude -180 to 180.");
            refresh();
            return;
        }
        begin({{"action", "apply"}, {"lat", latitude}, {"lon", longitude}});
        return;
    }
    if (id == 203) {
        if (busy) {
            send({{"action", "stop"}});
            stopping = true;
            status = tr(L"正在停止操作并尝试恢复定位…", L"正在停止操作並嘗試恢復定位…", L"Stopping operation and restoring location...");
            operationStart = GetTickCount64();
            updateControls();
        } else
            begin({{"action", "clear"}});
        return;
    }
    if (id == 204) {
        page = 1;
        layout();
    }
    if (id == 205)
        ShellExecuteW(windowHandle, L"open", L"https://support.apple.com/zh-cn/121115", nullptr, nullptr,
                      SW_SHOWNORMAL);
    if (id == 206) {
        if (!busy) {
            selection = -1;
            status = tr(L"正在定位：阁下在此…", L"正在定位：閣下在此…", L"Locating: You Are Here...");
            refresh();
            if (workerReady) {
                send({{"action", "locate"}});
            } else {
                status = tr(L"阁下在此 · 点击地图可自选坐标", L"閣下在此 · 點選地圖可自選座標", L"You Are Here · Click map to pick coordinates");
                refresh();
            }
        }
        return;
    }
}

void readEvent(const json &event) {
    std::string kind = event.value("event", "");
    if (kind == "ready") {
        workerReady = true;
        begin({{"action", "scan"}});
    }
    if (kind == "location") {
        if (event.value("ok", false)) {
            double lat = event.value("lat", latitude);
            double lon = event.value("lon", longitude);
            setCoordinates(lat, lon, -1);
            status = tr(L"定位成功 · 阁下在此", L"定位成功 · 閣下在此", L"Located · You Are Here");
        } else {
            status = tr(L"未能获取网络位置，阁下可直接在地图选点。", L"未能獲取網絡位置，閣下可直接在地圖選點。", L"Could not determine location; click map to pick.");
        }
        refresh();
    }
    if (kind == "devices") {
        devices.clear();
        SendMessageW(deviceCombo, CB_RESETCONTENT, 0, 0);
        for (auto &d : event.at("devices")) {
            devices.push_back(
                {d.at("id"), wide(d.at("label")), d.value("developer", false), d.value("trusted", false)});
            SendMessageW(deviceCombo, CB_ADDSTRING, 0, (LPARAM)devices.back().label.c_str());
        }
        deviceIndex = devices.empty() ? -1 : 0;
        SendMessageW(deviceCombo, CB_SETCURSEL, deviceIndex, 0);
        statusCode = devices.empty() ? L"no_devices" : L"found_devices";
        status = devices.empty() ? tr(L"未检测到设备。请使用 USB 数据线连接，解锁并信任此电脑。",
                                      L"未檢測到設備。請使用 USB 數據線連接，解鎖並信任此電腦。",
                                      L"No devices found. Connect via USB, unlock & trust computer.")
                                 : tr(L"已找到设备。请确认开发者模式，再选取目的地。",
                                      L"已找到設備。請確認開發者模式，再選取目的地。",
                                      L"Device found. Ensure Developer Mode, then pick destination.");
        layout();
    }
    if (kind == "status") {
        std::string st = event.value("state", "");
        statusCode = wide(st);
        std::wstring msg = trState(st);
        status = !msg.empty() ? msg : wide(event.value("message", ""));
        if (st == "active")
            active = true;
        if (st == "restored") {
            active = false;
            restorationWarning = false;
            status = tr(L"已恢复真实定位 · 阁下在此", L"已恢復真實定位 · 閣下在此", L"Real location restored · You are here");
        }
    }
    if (kind == "error") {
        std::string cd = event.value("code", "");
        statusCode = wide(cd);
        std::wstring msg = trErrorCode(cd);
        if (!restorationWarning)
            status = !msg.empty() ? msg : wide(event.value("message", "Operation incomplete."));
        if (cd == "restore")
            restorationWarning = true;
    }
    if (kind == "done") {
        busy = active = stopping = false;
        if (closing) {
            if (restorationWarning) {
                closing = false;
                MessageBoxW(windowHandle,
                            tr(L"恢复尚未获设备确认。请检查连接并重试恢复，或重启 iPhone。应用将保持开启。",
                               L"恢復尚未獲設備確認。請檢查連接並重試恢復，或重啟 iPhone。應用程式將保持開啟。",
                               L"Restoration unconfirmed by device. Reconnect and retry, or reboot iPhone. Meridian will stay open."),
                            tr(L"请确认真实定位", L"請確認真實定位", L"Confirm Location"), MB_OK | MB_ICONWARNING);
            } else
                DestroyWindow(windowHandle);
        }
    }
    if (kind == "exit") {
        workerReady = false;
        if (active || busy) {
            status = tr(L"设备引擎已退出。恢复尚未确认，请重启应用后恢复定位，必要时重启 iPhone。",
                        L"設備引擎已退出。恢復尚未確認，請重啟應用程式後恢復定位，必要時重啟 iPhone。",
                        L"Engine exited. Restoration unconfirmed; restart Meridian or reboot iPhone.");
            restorationWarning = true;
        } else
            status = tr(L"设备引擎已退出，请重新启动应用。",
                        L"設備引擎已退出，請重新啟動應用程式。",
                        L"Engine exited. Please restart Meridian.");
        active = busy = stopping = false;
        closing = false;
    }
    updateControls();
}

bool startBridge() {
    fs::path exe = appDir / L"engine" / L"meridian-engine.exe";
    if (!fs::exists(exe)) {
        status = tr(L"未找到设备引擎。请完整解压发布包，保留 engine 文件夹。",
                    L"未找到設備引擎。請完整解壓發布包，保留 engine 資料夾。",
                    L"Engine not found. Please unpack release completely.");
        return false;
    }
    SECURITY_ATTRIBUTES sa{sizeof(sa), nullptr, TRUE};
    HANDLE childIn{}, childOut{}, parentOut{}, nullError{};
    if (!CreatePipe(&childIn, &inputPipe, &sa, 0))
        return false;
    if (!CreatePipe(&parentOut, &childOut, &sa, 0)) {
        CloseHandle(childIn);
        CloseHandle(inputPipe);
        inputPipe = nullptr;
        return false;
    }
    SetHandleInformation(inputPipe, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(parentOut, HANDLE_FLAG_INHERIT, 0);
    nullError = CreateFileW(L"NUL", GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &sa, OPEN_EXISTING, 0,
                            nullptr);
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = childIn;
    si.hStdOutput = childOut;
    si.hStdError = nullError;
    PROCESS_INFORMATION pi{};
    std::wstring command = L"\"" + exe.wstring() + L"\"";
    BOOL ok = CreateProcessW(exe.c_str(), command.data(), nullptr, nullptr, TRUE,
                             CREATE_NO_WINDOW | CREATE_SUSPENDED, nullptr, appDir.c_str(), &si, &pi);
    CloseHandle(childIn);
    CloseHandle(childOut);
    CloseHandle(nullError);
    if (!ok) {
        CloseHandle(inputPipe);
        CloseHandle(parentOut);
        inputPipe = nullptr;
        status = tr(L"设备引擎启动失败，请完整解压发布包后重试。",
                    L"設備引擎啟動失敗，請完整解壓發布包後重試。",
                    L"Failed to launch engine. Please unpack completely and retry.");
        return false;
    }
    jobHandle = CreateJobObjectW(nullptr, nullptr);
    if (jobHandle) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION ji{};
        ji.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if (!SetInformationJobObject(jobHandle, JobObjectExtendedLimitInformation, &ji, sizeof(ji)) ||
            !AssignProcessToJobObject(jobHandle, pi.hProcess)) {
            CloseHandle(jobHandle);
            jobHandle = nullptr;
        }
    }
    processHandle = pi.hProcess;
    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    readerThread = std::thread([parentOut]() {
        char buf[4096];
        DWORD count;
        std::string pending;
        while (ReadFile(parentOut, buf, sizeof(buf), &count, nullptr) && count) {
            pending.append(buf, count);
            if (pending.size() > 65536)
                break;
            size_t n;
            while ((n = pending.find('\n')) != std::string::npos) {
                auto e = json::parse(pending.substr(0, n), nullptr, false);
                pending.erase(0, n + 1);
                if (!e.is_discarded() && e.is_object()) {
                    auto *p = new json(std::move(e));
                    if (!PostMessageW(windowHandle, WM_BRIDGE, 0, (LPARAM)p))
                        delete p;
                }
            }
        }
        CloseHandle(parentOut);
        readerFinished = true;
        auto *e = new json({{"event", "exit"}});
        if (!PostMessageW(windowHandle, WM_BRIDGE, 0, (LPARAM)e))
            delete e;
    });
    return true;
}

void addControl(int id, RectF r, std::wstring title, int onPage, std::wstring subtitle = L"",
                std::wstring tag = L"") {
    HWND h =
        CreateWindowExW(0, L"BUTTON", title.c_str(), WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW, 0, 0,
                        1, 1, windowHandle, (HMENU)(INT_PTR)id, GetModuleHandleW(nullptr), nullptr);
    controls.push_back({id, h, r, title, subtitle, tag, onPage});
}

void updateLanguage() {
    SetWindowTextW(windowHandle, tr(L"Meridian · 子午线", L"Meridian · 子午線", L"Meridian · iOS Studio"));
    auto setCtl = [](int id, const wchar_t *title) {
        Control *c = findControl(id);
        if (c) {
            c->title = title;
            SetWindowTextW(c->window, title);
        }
    };
    setCtl(1, tr(L"定位工作台", L"定位工作台", L"Workspace"));
    setCtl(2, tr(L"连接指南", L"連接指南", L"Guide"));
    setCtl(3, tr(L"关于与隐私", L"關於與私隱", L"About & Privacy"));
    setCtl(4, currentLang == Lang::ZH_CN ? L"简体中文" : (currentLang == Lang::ZH_HK ? L"繁體中文" : L"English"));
    setCtl(200, tr(L"扫描设备", L"掃描設備", L"Scan Devices"));
    setCtl(201, tr(L"开启开发者模式", L"開啟開發者模式", L"Enable Dev Mode"));
    setCtl(204, tr(L"阅读连接指南  →", L"閱讀連接指南  →", L"Read Guide  →"));
    setCtl(205, tr(L"Apple 官方说明  ↗", L"Apple 官方說明  ↗", L"Apple Docs  ↗"));
    setCtl(206, tr(L"阁下在此", L"閣下在此", L"You Are Here"));
    for (size_t i = 0; i < places.size(); ++i) {
        Control *c = findControl(100 + (int)i);
        if (c)
            c->subtitle = places[i].getCity();
    }
    if (statusCode == L"idle")
        status = tr(L"连接 iPhone，开启属于阁下的自由漫游。", L"連接 iPhone，開啟屬於閣下的自由漫遊。", L"Connect iPhone to begin your journey.");
    else if (statusCode == L"no_devices")
        status = tr(L"未检测到设备。请使用 USB 数据线连接，解锁并信任此电脑。", L"未檢測到設備。請使用 USB 數據線連接，解鎖並信任此電腦。", L"No devices found. Connect via USB, unlock & trust computer.");
    else if (statusCode == L"found_devices")
        status = tr(L"已找到设备。请确认开发者模式，再选取目的地。", L"已找到設備。請確認開發者模式，再選取目的地。", L"Device found. Ensure Developer Mode, then pick destination.");
    else if (!statusCode.empty()) {
        std::string sc(statusCode.begin(), statusCode.end());
        std::wstring msg = trState(sc);
        if (msg.empty())
            msg = trErrorCode(sc);
        if (!msg.empty())
            status = msg;
    }
    updateControls();
}
void toggleLanguage() {
    currentLang = (Lang)(((int)currentLang + 1) % 3);
    updateLanguage();
}

void createControls() {
    addControl(4, {604, 27, 140, 40}, currentLang == Lang::ZH_CN ? L"简体中文" : (currentLang == Lang::ZH_HK ? L"繁體中文" : L"English"), -1);
    addControl(1, {759, 27, 140, 40}, tr(L"定位工作台", L"定位工作台", L"Workspace"), -1);
    addControl(2, {914, 27, 140, 40}, tr(L"连接指南", L"連接指南", L"Guide"), -1);
    addControl(3, {1069, 27, 175, 40}, tr(L"关于与隐私", L"關於與私隱", L"About & Privacy"), -1);
    for (size_t i = 0; i < places.size(); ++i)
        addControl(100 + (int)i, {36 + (i % 3) * 280.f, 608 + (i / 3) * 70.f, 266, 62}, places[i].name, 0,
                   places[i].getCity(), places[i].region);
    addControl(200, {909, 389, 145, 31}, tr(L"扫描设备", L"掃描設備", L"Scan Devices"), 0);
    addControl(201, {1068, 389, 152, 31}, tr(L"开启开发者模式", L"開啟開發者模式", L"Enable Dev Mode"), 0);
    addControl(206, {1095, 437, 125, 26}, tr(L"阁下在此", L"閣下在此", L"You Are Here"), 0);
    addControl(202, {908, 630, 314, 45}, tr(L"开始定位", L"開始定位", L"Start Simulation"), 0);
    addControl(203, {908, 686, 314, 32}, tr(L"恢复真实定位 · 阁下在此", L"恢復真實定位 · 閣下在此", L"Restore Real Location"), 0);
    addControl(204, {1071, 763, 173, 43}, tr(L"阅读连接指南  →", L"閱讀連接指南  →", L"Read Guide  →"), 2);
    addControl(205, {869, 763, 184, 43}, tr(L"Apple 官方说明  ↗", L"Apple 官方說明  ↗", L"Apple Docs  ↗"), 2);
    deviceCombo = CreateWindowExW(0, L"COMBOBOX", tr(L"选取匿名设备", L"選取匿名設備", L"Select Anonymous Device"),
                                  WS_CHILD | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL, 0, 0, 1, 1,
                                  windowHandle, (HMENU)300, GetModuleHandleW(nullptr), nullptr);
    latEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 1,
                              1, windowHandle, (HMENU)301, GetModuleHandleW(nullptr), nullptr);
    lonEdit = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 1,
                              1, windowHandle, (HMENU)302, GetModuleHandleW(nullptr), nullptr);
    SendMessageW(latEdit, EM_SETLIMITTEXT, 24, 0);
    SendMessageW(lonEdit, EM_SETLIMITTEXT, 24, 0);
    setCoordinates(latitude, longitude, 0);
    status = tr(L"连接 iPhone，开启属于阁下的自由漫游。", L"連接 iPhone，開啟屬於閣下的自由漫遊。", L"Connect iPhone to begin your journey.");
    layout();
    updateControls();
}

bool loadAssets() {
    try {
        std::ifstream file(appDir / L"data" / L"presets.json");
        json data;
        file >> data;
        if (!data.is_array() || data.size() != 9)
            return false;
        for (auto &p : data) {
            double a = p.at("lat"), b = p.at("lon");
            if (!std::isfinite(a) || !std::isfinite(b) || std::abs(a) > 90 || std::abs(b) > 180)
                return false;
            std::string zc = p.value("city_zh_cn", p.value("city", ""));
            std::string zh = p.value("city_zh_hk", p.value("city", ""));
            std::string en = p.value("city_en", p.value("city", ""));
            places.push_back({wide(p.at("name")), wide(p.at("city")), wide(zc), wide(zh), wide(en),
                              wide(p.at("region")), a, b});
        }
        std::ifstream map(appDir / L"assets" / L"world.geojson");
        json geo;
        map >> geo;
        auto polygon = [](const json &poly) {
            for (auto &ring : poly) {
                std::vector<PointF> points;
                for (auto &p : ring)
                    points.emplace_back(p.at(0).get<float>(), p.at(1).get<float>());
                lands.push_back(std::move(points));
            }
        };
        for (auto &feature : geo["features"]) {
            auto &geometry = feature["geometry"];
            if (geometry["type"] == "Polygon")
                polygon(geometry["coordinates"]);
            else if (geometry["type"] == "MultiPolygon")
                for (auto &p : geometry["coordinates"])
                    polygon(p);
        }
        return true;
    } catch (...) {
        return false;
    }
}
int savePreview(const fs::path &path) {
    Bitmap bitmap(WIDTH, HEIGHT, PixelFormat32bppARGB);
    Graphics g(&bitmap);
    paint(g);
    for (auto &c : controls)
        if (c.page == -1 || c.page == page) {
            auto s = g.Save();
            g.TranslateTransform(c.rect.X, c.rect.Y);
            drawControl(g, c, false, false, false, IsWindowEnabled(c.window));
            g.Restore(s);
        }
    if (page == 0) {
        text(g, number(latitude), 917, 574, 135, 30, 13, INK, false, L"Segoe UI");
        text(g, number(longitude), 1080, 574, 137, 30, 13, INK, false, L"Segoe UI");
    }
    CLSID png{0x557cf406, 0x1a04, 0x11d3, {0x9a, 0x73, 0x00, 0x00, 0xf8, 0x1e, 0xf3, 0x2e}};
    return bitmap.Save(path.c_str(), &png, nullptr) == Ok ? 0 : 1;
}

LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        windowHandle = hwnd;
        createControls();
        SetTimer(hwnd, 1, 1000, nullptr);
        return 0;
    case WM_SIZE:
        if (latEdit)
            layout();
        return 0;
    case WM_GETMINMAXINFO: {
        auto *m = (MINMAXINFO *)lp;
        UINT dpi = GetDpiForWindow(hwnd);
        m->ptMinTrackSize = {MulDiv(1024, dpi, 96), MulDiv(750, dpi, 96)};
        return 0;
    }
    case WM_DPICHANGED: {
        auto r = (RECT *)lp;
        SetWindowPos(hwnd, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top,
                     SWP_NOZORDER | SWP_NOACTIVATE);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT r;
        GetClientRect(hwnd, &r);
        Bitmap buffer(std::max(1L, r.right), std::max(1L, r.bottom), PixelFormat32bppARGB);
        Graphics g(&buffer);
        g.Clear(BG);
        g.ScaleTransform(scale, scale);
        paint(g);
        Graphics out(dc);
        out.DrawImage(&buffer, 0, 0);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_DRAWITEM: {
        auto *d = (DRAWITEMSTRUCT *)lp;
        auto *c = findControl((int)d->CtlID);
        if (!c)
            break;
        Graphics g(d->hDC);
        g.ScaleTransform(scale, scale);
        POINT pos;
        GetCursorPos(&pos);
        ScreenToClient(c->window, &pos);
        RECT r;
        GetClientRect(c->window, &r);
        drawControl(g, *c, PtInRect(&r, pos), (d->itemState & ODS_SELECTED) != 0,
                    (d->itemState & ODS_FOCUS) != 0, IsWindowEnabled(c->window));
        return TRUE;
    }
    case WM_CTLCOLOREDIT: {
        SetTextColor((HDC)wp, RGB(29, 48, 41));
        SetBkColor((HDC)wp, RGB(248, 247, 243));
        return (LRESULT)editBrush;
    }
    case WM_COMMAND: {
        int id = LOWORD(wp), notification = HIWORD(wp);
        if ((id == 301 || id == 302) && notification == EN_CHANGE && !updating && !busy) {
            selection = -1;
            parseCoordinates();
            refresh();
        } else if (id == 300 && notification == CBN_SELCHANGE) {
            deviceIndex = (int)SendMessageW(deviceCombo, CB_GETCURSEL, 0, 0);
            updateControls();
        } else if (notification == BN_CLICKED)
            click(id);
        return 0;
    }
    case WM_LBUTTONUP: {
        if (page != 0 || busy)
            break;
        float x = GET_X_LPARAM(lp) / scale, y = GET_Y_LPARAM(lp) / scale;
        if (x >= 74 && x <= 824 && y >= 266 && y <= 498) {
            for (size_t i = 0; i < places.size(); ++i) {
                auto p = project(places[i].lat, places[i].lon);
                if (std::hypot(x - p.X, y - p.Y) < 8) {
                    setCoordinates(places[i].lat, places[i].lon, (int)i);
                    return 0;
                }
            }
            setCoordinates(85 - (y - 266) / 232 * 150, (x - 74) / 750 * 360 - 180, -1);
        }
        return 0;
    }
    case WM_BRIDGE: {
        std::unique_ptr<json> event((json *)lp);
        try {
            readEvent(*event);
        } catch (...) {
            status = tr(L"设备响应格式不正确，请重新启动应用。",
                        L"設備回應格式不正確，請重新啟動應用程式。",
                        L"Invalid response from engine. Please restart Meridian.");
            refresh();
        }
        return 0;
    }
    case WM_TIMER: {
        if (busy && GetTickCount64() - operationStart > (stopping ? 40000ULL : 240000ULL) && !active) {
            status = tr(L"操作等待时间较长。可结束应用后检查网络与 USB 连接再重试。",
                        L"操作等待時間較長。可結束應用程式後檢查網絡與 USB 連接再重試。",
                        L"Operation is taking longer than expected. You may close Meridian, verify network and USB, and retry.");
            refresh();
        }
        return 0;
    }
    case WM_CLOSE: {
        if (busy) {
            if (closing) {
                if (MessageBoxW(hwnd,
                                tr(L"后台尚未完成恢复。强制退出可能保留模拟定位；需要重新打开后恢复，或重启 "
                                   L"iPhone。\n\n仍然退出？",
                                   L"後台尚未完成恢復。強制結束可能保留模擬定位；需要重新打開後恢復，或重啟 "
                                   L"iPhone。\n\n仍然結束？",
                                   L"Background restoration in progress. Forcing exit may retain simulated location; "
                                   L"re-open Meridian or reboot iPhone to restore.\n\nExit anyway?"),
                                tr(L"恢复尚未确认", L"恢復尚未確認", L"Restoration Unconfirmed"), MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) == IDYES)
                    DestroyWindow(hwnd);
                return 0;
            }
            closing = true;
            stopping = true;
            status = tr(L"正在恢复真实定位，完成后关闭…", L"正在恢復真實定位，完成後關閉…", L"Restoring real location, closing when finished...");
            send({{"action", "stop"}});
            updateControls();
            return 0;
        }
        if (restorationWarning &&
            MessageBoxW(hwnd,
                        tr(L"真实定位尚未确认恢复。请重新连接恢复或重启 iPhone。仍然退出？",
                           L"真實定位尚未確認恢復。請重新連接恢復或重啟 iPhone。仍然結束？",
                           L"Real location restoration unconfirmed. Please reconnect and restore, or reboot iPhone. Exit anyway?"),
                        tr(L"请确认真实定位", L"請確認真實定位", L"Confirm Location"),
                        MB_YESNO | MB_DEFBUTTON2 | MB_ICONWARNING) != IDYES)
            return 0;
        DestroyWindow(hwnd);
        return 0;
    }
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// Deterministic desktop checks exercise real controls without connecting to hardware.
int uiSelfTest(const fs::path &output) {
    json checks = json::array();
    auto check = [&](const char *name, bool ok) { checks.push_back({{"name", name}, {"ok", ok}}); };
    check("landmarks_loaded", places.size() == 9 && !lands.empty());
    click(102);
    check("preset_updates_controls", selection == 2 && textOf(latEdit) == number(places[2].lat) &&
                                         textOf(lonEdit) == number(places[2].lon));
    SetWindowTextW(latEdit, L"nan");
    check("nonfinite_rejected", !parseCoordinates());
    SetWindowTextW(latEdit, L"91");
    check("out_of_range_rejected", !parseCoordinates());
    SetWindowTextW(latEdit, L"35.680176");
    SetWindowTextW(lonEdit, L"139.763855");
    check("custom_input", parseCoordinates() && selection == -1 && std::abs(latitude - 35.680176) < 0.000001);
    click(2);
    check("guide_navigation", page == 1);
    click(3);
    check("privacy_navigation", page == 2);
    click(1);
    currentLang = Lang::ZH_CN;
    updateLanguage();
    check("lang_default_zh_cn", currentLang == Lang::ZH_CN);
    click(4);
    check("lang_switch_zh_hk", currentLang == Lang::ZH_HK);
    click(4);
    check("lang_switch_en", currentLang == Lang::EN);
    click(4);
    check("lang_switch_zh_cn", currentLang == Lang::ZH_CN);
    check("lang_detect_valid", (int)detectUserLanguage() >= 0 && (int)detectUserLanguage() <= 2);
    click(206);
    check("you_are_here_clicked", selection == -1 && !status.empty());
    check("you_are_here_control_exists", findControl(206) != nullptr);
    workerReady = true;
    readEvent(
        {{"event", "devices"},
         {"devices",
          json::array(
              {{{"id", "test-only"}, {"label", "iPhone 1"}, {"developer", false}, {"trusted", true}}})}});
    check("developer_mode_gate", !IsWindowEnabled(findControl(202)->window));
    devices[0].developer = true;
    updateControls();
    check("ready_device_enables_apply", IsWindowEnabled(findControl(202)->window));
    busy = active = true;
    updateControls();
    check("session_locks_inputs", !IsWindowEnabled(latEdit) && !IsWindowEnabled(findControl(200)->window) &&
                                      IsWindowEnabled(findControl(203)->window));
    readEvent({{"event", "error"}, {"code", "restore"}, {"message", "Restoration unconfirmed"}});
    readEvent({{"event", "error"}, {"code", "device"}, {"message", "Disconnected"}});
    readEvent({{"event", "done"}, {"action", "apply"}});
    check("restoration_warning_preserved", restorationWarning && !status.empty());
    bool ok = std::all_of(checks.begin(), checks.end(), [](const json &c) { return c["ok"].get<bool>(); });
    std::ofstream file(output);
    file << json({{"ok", ok}, {"checks", checks}}).dump(2);
    return ok && file.good() ? 0 : 1;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int show) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    wchar_t path[32768];
    GetModuleFileNameW(nullptr, path, 32768);
    appDir = fs::path(path).parent_path();
    currentLang = detectUserLanguage();
    if (!loadAssets()) {
        MessageBoxW(nullptr, tr(L"无法读取地图或地标数据。请完整解压 Meridian 发布包。",
                                L"無法讀取地圖或地標數據。請完整解壓 Meridian 發布包。",
                                L"Cannot load map or landmark data. Please unpack completely."), L"Meridian",
                    MB_OK | MB_ICONERROR);
        return 1;
    }
    GdiplusStartupInput start;
    ULONG_PTR token;
    GdiplusStartup(&token, &start, nullptr);
    editBrush = CreateSolidBrush(RGB(248, 247, 243));
    WNDCLASSW wc{};
    wc.hInstance = instance;
    wc.lpszClassName = L"MeridianDesktop";
    wc.lpfnWndProc = procedure;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    RegisterClassW(&wc);
    UINT dpi = GetDpiForSystem();
    RECT r{0, 0, MulDiv(WIDTH, dpi, 96), MulDiv(HEIGHT, dpi, 96)};
    AdjustWindowRectExForDpi(&r, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    RECT work;
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &work, 0);
    int w = std::min(r.right - r.left, work.right - work.left - 30),
        h = std::min(r.bottom - r.top, work.bottom - work.top - 30);
    int x = (work.right - work.left > w) ? (work.left + (work.right - work.left - w) / 2) : work.left;
    int y = (work.bottom - work.top > h) ? (work.top + (work.bottom - work.top - h) / 2) : work.top;
    windowHandle = CreateWindowExW(0, wc.lpszClassName, tr(L"Meridian · 子午线", L"Meridian · 子午線", L"Meridian · iOS Studio"),
                                   WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, x, y, w, h, nullptr, nullptr, instance, nullptr);
    BOOL dark = FALSE;
    DwmSetWindowAttribute(windowHandle, 20, &dark, sizeof(dark));
    int argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool preview = argc >= 3 && std::wstring(argv[1]) == L"--render-preview";
    bool selfTest = argc >= 3 && std::wstring(argv[1]) == L"--self-test";
    if (preview || selfTest) {
        if (preview && argc >= 4)
            page = std::clamp(_wtoi(argv[3]), 0, 2);
        if (preview && argc >= 5) {
            currentLang = (Lang)std::clamp(_wtoi(argv[4]), 0, 2);
            updateLanguage();
        }
        int result = preview ? savePreview(argv[2]) : uiSelfTest(argv[2]);
        LocalFree(argv);
        DestroyWindow(windowHandle);
        DeleteObject(editFont);
        DeleteObject(editBrush);
        GdiplusShutdown(token);
        return result;
    }
    LocalFree(argv);
    int showCmd = (show == SW_HIDE || show == 0 || show == SW_SHOWMINIMIZED || show == SW_SHOWMINNOACTIVE) ? SW_SHOWNORMAL : show;
    ShowWindow(windowHandle, showCmd);
    UpdateWindow(windowHandle);
    SetForegroundWindow(windowHandle);
    startBridge();
    updateControls();
    MSG message;
    BOOL bRet;
    while ((bRet = GetMessageW(&message, nullptr, 0, 0)) != 0) {
        if (bRet == -1)
            break;
        if (!IsDialogMessageW(windowHandle, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }
    send({{"action", "quit"}});
    if (inputPipe)
        CloseHandle(inputPipe);
    inputPipe = nullptr;
    if (processHandle) {
        WaitForSingleObject(processHandle, 2000);
        if (jobHandle)
            CloseHandle(jobHandle);
        WaitForSingleObject(processHandle, 2000);
        CloseHandle(processHandle);
    }
    if (readerThread.joinable())
        readerThread.join();
    while (PeekMessageW(&message, nullptr, WM_BRIDGE, WM_BRIDGE, PM_REMOVE))
        delete (json *)message.lParam;
    DeleteObject(editFont);
    DeleteObject(editBrush);
    GdiplusShutdown(token);
    return 0;
}
