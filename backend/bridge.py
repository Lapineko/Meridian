"""JSON-lines bridge. Device identifiers never cross the process boundary.

Commands arrive on stdin; stdout is exclusively the documented event stream.
No shell, TCP listener, telemetry, raw exception output, or application log files.
"""
from __future__ import annotations

import asyncio
import contextlib
import json
import logging
import math
import re
import sys
import threading
import uuid
from contextlib import asynccontextmanager


class UserError(Exception):
    def __init__(self, code: str):
        self.code = code


MESSAGES = {
    "driver": "無法連接 Apple 設備服務。請安裝 Apple Devices 或 iTunes，解鎖並信任此電腦。",
    "device": "設備已斷開或選取已過期，請重新掃描。",
    "trust": "請解鎖 iPhone，並在手機上選擇「信任此電腦」。",
    "developer": "請先開啟開發者模式，再開始定位。",
    "passcode": "設備密碼阻止了自動開啟。請在手機設定中開啟開發者模式，或臨時關閉鎖屏密碼後重試。",
    "version": "此版本使用 iOS 17.4 及以上的 USB 通道；當前系統不適用。",
    "coordinates": "經緯度無效。緯度須在 −90 至 90，經度須在 −180 至 180。",
    "busy": "請先結束當前操作或恢復真實定位。",
    "timeout": "操作超時。請檢查 USB 連接、信任提示和網絡，然後重試。",
    "mount": "開發者鏡像未就緒。請確認開發者模式、網絡連接及當前 iOS 的鏡像支援。",
    "restore": "恢復定位未獲設備確認。請重新連接後點擊恢復；必要時重啟 iPhone。",
    "protocol": "請求格式不正確，請重新啟動 Meridian。",
    "unexpected": "設備操作未完成。請檢查連接與開發者模式後重試。",
}


def coordinates(lat, lon):
    if isinstance(lat, bool) or isinstance(lon, bool):
        raise UserError("coordinates")
    try:
        lat, lon = float(lat), float(lon)
    except (TypeError, ValueError, OverflowError):
        raise UserError("coordinates") from None
    if not (math.isfinite(lat) and math.isfinite(lon) and -90 <= lat <= 90 and -180 <= lon <= 180):
        raise UserError("coordinates")
    return lat, lon


def error_code(error):
    if isinstance(error, UserError):
        return error.code
    if isinstance(error, (TimeoutError, asyncio.TimeoutError)):
        return "timeout"
    # Class-name allowlist deliberately excludes exception text, which can contain UDIDs.
    name = type(error).__name__
    return {
        "DeviceHasPasscodeSetError": "passcode",
        "DeveloperModeIsNotEnabledError": "developer",
        "NotPairedError": "trust", "PairingDialogResponsePendingError": "trust",
        "UserDeniedPairingError": "trust", "PasswordRequiredError": "trust",
        "NoDeviceConnectedError": "device", "DeviceNotFoundError": "device",
        "ConnectionTerminatedError": "device", "ConnectionRefusedError": "driver",
        "ConnectionFailedError": "driver", "MuxException": "driver",
        "ConnectionResetError": "device", "BrokenPipeError": "device",
    }.get(name, "unexpected")


class Hardware:
    async def list(self):
        from pymobiledevice3.usbmux import list_devices
        return [d for d in await list_devices() if d.connection_type == "USB"]

    @asynccontextmanager
    async def connect(self, serial):
        from pymobiledevice3.lockdown import create_using_usbmux
        client = await create_using_usbmux(serial=serial, connection_type="USB", pair_timeout=10)
        try:
            yield client
        finally:
            await client.close()

    async def enable(self, serial):
        from pymobiledevice3.services.amfi import AmfiService
        async with self.connect(serial) as client:
            if await client.get_developer_mode_status():
                return "already_enabled"
            # This request reboots the phone. Final consent stays on the phone.
            await AmfiService(client).enable_developer_mode(enable_post_restart=False)
        return "restart_required"

    async def mount(self, client):
        from pymobiledevice3.exceptions import AlreadyMountedError
        from pymobiledevice3.services.mobile_image_mounter import PersonalizedImageMounter, fetch_personalized_ddi
        async with PersonalizedImageMounter(client) as mounter:
            if await mounter.is_image_mounted("Personalized"):
                return
            try:
                paths = await asyncio.to_thread(fetch_personalized_ddi)
                await mounter.mount(*paths)
            except AlreadyMountedError:
                return
            except Exception as exc:
                if error_code(exc) != "unexpected":
                    raise
                raise UserError("mount") from None

    @asynccontextmanager
    async def location(self, serial, progress):
        from pymobiledevice3.remote.userspace_tunnel import UserspaceRsdTunnel
        from pymobiledevice3.services.dvt.instruments.dvt_provider import DvtProvider
        from pymobiledevice3.services.dvt.instruments.location_simulation import LocationSimulation
        async with self.connect(serial) as client:
            version = tuple(int(n) for n in client.product_version.split(".")[:2])
            if version < (17, 4):
                raise UserError("version")
            if not await client.get_developer_mode_status():
                raise UserError("developer")
            progress("mounting", "正在準備開發者鏡像；首次使用可能需要聯網下載…")
            await self.mount(client)
        progress("connecting", "正在建立設備定位通道…")
        async with UserspaceRsdTunnel(serial=serial, remotepairing_fallback=False) as rsd:
            async with DvtProvider(rsd) as dvt:
                async with LocationSimulation(dvt) as simulation:
                    yield simulation


class Bridge:
    def __init__(self, emit, hardware=None):
        self.emit = emit
        self.hardware = hardware or Hardware()
        self.devices = {}
        self.task = None
        self.stop = asyncio.Event()

    def progress(self, state, message):
        self.emit({"event": "status", "state": state, "message": message})

    def fail(self, error):
        code = error_code(error)
        self.emit({"event": "error", "code": code, "message": MESSAGES[code]})

    def serial(self, command):
        token = command.get("device")
        if not isinstance(token, str) or token not in self.devices:
            raise UserError("device")
        return self.devices[token]

    async def scan(self):
        self.devices.clear()
        found = []
        for index, device in enumerate(await self.hardware.list()):
            token = uuid.uuid4().hex
            self.devices[token] = device.serial
            label, developer, trusted = f"iPhone / iPad {index + 1}", False, False
            try:
                async with self.hardware.connect(device.serial) as client:
                    # No personal device name, serial, ECID, account or network address is sent.
                    product = "iPad" if client.product_type.startswith("iPad") else "iPhone"
                    version = client.product_version
                    if not re.fullmatch(r"\d+(?:\.\d+){0,3}", version):
                        version = "?"
                    label = f"{product} {index + 1} · iOS {version}"
                    developer = await client.get_developer_mode_status()
                    trusted = True
            except Exception:
                pass
            found.append({"id": token, "label": label, "developer": developer, "trusted": trusted})
        self.emit({"event": "devices", "devices": found})

    async def locate(self):
        def _fetch():
            import urllib.request
            urls = [
                ("https://ipapi.co/json/", lambda d: (float(d["latitude"]), float(d["longitude"]), f"{d.get('country_name', '')} · {d.get('city', '')}".strip(" ·"))),
                ("https://ipwho.is/", lambda d: (float(d["latitude"]), float(d["longitude"]), f"{d.get('country', '')} · {d.get('city', '')}".strip(" ·")) if d.get("success") else None)
            ]
            for url, parser in urls:
                try:
                    req = urllib.request.Request(url, headers={"User-Agent": "Meridian/0.1.0"})
                    with urllib.request.urlopen(req, timeout=3.0) as resp:
                        parsed = parser(json.loads(resp.read().decode()))
                        if parsed:
                            return {"ok": True, "lat": parsed[0], "lon": parsed[1], "city": parsed[2]}
                except Exception:
                    continue
            return {"ok": False}
        return await asyncio.to_thread(_fetch)

    async def session(self, command, clear_only=False):
        serial = self.serial(command)
        if not clear_only:
            lat, lon = coordinates(command.get("lat"), command.get("lon"))
        simulated, restored = False, False
        async with asyncio.timeout(180):
            manager = self.hardware.location(serial, self.progress)
            simulation = await manager.__aenter__()
        try:
            if clear_only:
                try:
                    await asyncio.wait_for(simulation.clear(), 15)
                except Exception:
                    raise UserError("restore") from None
                self.progress("restored", "已請求恢復真實定位，請在 iPhone 地圖中確認。")
                return
            if self.stop.is_set():
                return
            # Treat an uncertain set() failure as potentially applied and attempt to clear it.
            simulated = True
            await asyncio.wait_for(simulation.set(lat, lon), 15)
            self.progress("active", "定位會話進行中 · 請保持 USB 連接")
            while not self.stop.is_set():
                try:
                    await asyncio.wait_for(self.stop.wait(), 2)
                except asyncio.TimeoutError:
                    devices = await asyncio.wait_for(self.hardware.list(), 8)
                    if serial not in {d.serial for d in devices}:
                        raise UserError("device")
            self.progress("restoring", "正在恢復真實定位…")
        finally:
            if simulated:
                try:
                    await asyncio.wait_for(simulation.clear(), 15)
                    restored = True
                except Exception:
                    self.fail(UserError("restore"))
            with contextlib.suppress(Exception):
                await asyncio.wait_for(manager.__aexit__(None, None, None), 10)
        if restored:
            self.progress("restored", "已請求恢復真實定位，請在 iPhone 地圖中確認。")

    async def execute(self, command):
        action = command.get("action")
        try:
            if action == "scan":
                self.progress("scanning", "正在尋找透過 USB 連接的設備…")
                await asyncio.wait_for(self.scan(), 35)
            elif action == "enable_developer":
                self.progress("enabling", "正在請求開啟開發者模式…")
                state = await asyncio.wait_for(self.hardware.enable(self.serial(command)), 25)
                self.progress(state, "請在 iPhone 重啟後確認開啟，再掃描設備。完成後可重新設置密碼和 Face ID。" if state == "restart_required" else "開發者模式已開啟，請重新掃描設備。")
            elif action == "locate":
                loc = await self.locate()
                self.emit({"event": "location", **loc})
            elif action in ("apply", "clear"):
                await self.session(command, clear_only=action == "clear")
            else:
                raise UserError("protocol")
        except asyncio.CancelledError:
            self.fail(UserError("timeout"))
        except Exception as exc:
            self.fail(exc)
        finally:
            self.emit({"event": "done", "action": action})

    async def dispatch(self, command):
        if not isinstance(command, dict) or not isinstance(command.get("action"), str):
            self.fail(UserError("protocol"))
            return
        action = command["action"]
        if action in ("stop", "quit"):
            self.stop.set()
            if self.task and not self.task.done():
                if action == "quit":
                    try:
                        await asyncio.wait_for(asyncio.shield(self.task), 30)
                    except asyncio.TimeoutError:
                        self.task.cancel()
                        with contextlib.suppress(asyncio.CancelledError):
                            await self.task
            return
        if self.task and not self.task.done():
            self.fail(UserError("busy"))
            return
        self.stop.clear()
        self.task = asyncio.create_task(self.execute(command))


async def serve(emit):
    bridge = Bridge(emit)
    queue = asyncio.Queue()
    loop = asyncio.get_running_loop()

    def reader():
        try:
            while True:
                raw = sys.stdin.buffer.readline(65537)
                if not raw:
                    break
                if len(raw) > 65536:
                    loop.call_soon_threadsafe(queue.put_nowait, {})
                    break
                try:
                    command = json.loads(raw)
                except (ValueError, UnicodeError):
                    command = {}
                loop.call_soon_threadsafe(queue.put_nowait, command)
        finally:
            with contextlib.suppress(RuntimeError):
                loop.call_soon_threadsafe(queue.put_nowait, {"action": "quit"})

    threading.Thread(target=reader, daemon=True).start()
    emit({"event": "ready", "protocol": 1})
    while True:
        command = await queue.get()
        await bridge.dispatch(command)
        if isinstance(command, dict) and command.get("action") == "quit":
            break


def main():
    # Third-party progress / tracebacks must not leak personal device information.
    wire = sys.stdout
    wire.reconfigure(encoding="utf-8", errors="replace")
    import os
    with open(os.devnull, "w", encoding="utf-8") as sink:
        sys.stdout = sys.stderr = sink
        logging.disable(logging.CRITICAL)

        def emit(event):
            wire.write(json.dumps(event, ensure_ascii=False, allow_nan=False) + "\n")
            wire.flush()

        try:
            if "--self-test" in sys.argv:
                from pymobiledevice3.remote.userspace_tunnel import UserspaceRsdTunnel
                from pymobiledevice3.services.dvt.instruments.location_simulation import LocationSimulation
                from pymobiledevice3.services.mobile_image_mounter import PersonalizedImageMounter
                from pymobiledevice3.services.amfi import AmfiService
                coordinates(37.334643, -122.008972)
                emit({"event": "self_test", "ok": True, "protocol": 1})
            else:
                asyncio.run(serve(emit))
        except Exception as exc:
            event = {"event": "error", "code": "unexpected", "message": MESSAGES["unexpected"]}
            if "--self-test" in sys.argv:
                # Import-only diagnostics never connect to a device or expose exception text.
                event["exception_type"] = type(exc).__name__
                if isinstance(exc, ModuleNotFoundError) and re.fullmatch(r"[A-Za-z0-9_.]+", exc.name or ""):
                    event["missing_module"] = exc.name
            emit(event)
            return 1
        finally:
            sys.stdout, sys.stderr = wire, sys.__stderr__
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
