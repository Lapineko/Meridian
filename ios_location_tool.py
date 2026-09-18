#!/usr/bin/env python3
"""Optional privacy-preserving CLI. Desktop users should run Meridian.exe."""
import argparse
import asyncio
import contextlib
import json
import logging
import os
from pathlib import Path
from backend.bridge import Bridge, UserError, coordinates


def validate_coordinates(lat, lon):
    try:
        a, b = coordinates(lat, lon)
        return True, "", a, b
    except UserError:
        return False, "经纬度格式或范围无效", 0.0, 0.0


async def run(args, output):
    failed = False
    def show(event):
        nonlocal failed
        if event["event"] in ("status", "error"):
            print(event["message"], file=output, flush=True)
            failed |= event["event"] == "error"
        elif event["event"] == "devices":
            for i, d in enumerate(event["devices"], 1):
                print(f"{i}. {d['label']}", file=output)
    bridge = Bridge(show)
    await bridge.execute({"action": "scan"})
    tokens = list(bridge.devices)
    if not tokens:
        print("未找到 USB 设备。请检查驱动、连接和信任状态。", file=output)
        return 1
    if len(tokens) > 1 and args.device_index is None:
        print("多台设备已连接，请用 --device-index 指定匿名序号。", file=output)
        return 1
    index = (args.device_index or 1) - 1
    if not 0 <= index < len(tokens):
        print("设备序号无效。", file=output)
        return 1
    if args.clear:
        command = {"action": "clear"}
    else:
        if args.preset:
            rows = json.loads((Path(__file__).parent / "data" / "presets.json").read_text(encoding="utf-8"))
            p = rows[int(args.preset) - 1]
            lat, lon = p["lat"], p["lon"]
        else:
            lat, lon = coordinates(args.lat, args.lon)
        command = {"action": "apply", "lat": lat, "lon": lon}
    command["device"] = tokens[index]
    print("按 Ctrl+C 结束会话；退出时将尝试恢复真实定位。", file=output)
    await bridge.execute(command)
    return int(failed)


def main():
    parser = argparse.ArgumentParser(description="Meridian 子午线 · iOS 定位 CLI")
    parser.add_argument("--preset", choices=[str(i) for i in range(1, 10)])
    parser.add_argument("--lat", type=float)
    parser.add_argument("--lon", type=float)
    parser.add_argument("--clear", action="store_true")
    parser.add_argument("--device-index", type=int)
    args = parser.parse_args()
    if not args.clear and not args.preset and (args.lat is None or args.lon is None):
        parser.error("请指定 --preset，或同时指定 --lat / --lon，或使用 --clear。")
    import sys
    output = sys.stdout
    logging.disable(logging.CRITICAL)
    with open(os.devnull, "w") as sink, contextlib.redirect_stdout(sink), contextlib.redirect_stderr(sink):
        try:
            return asyncio.run(run(args, output))
        except KeyboardInterrupt:
            return 0
        except UserError:
            print("经纬度无效。", file=output)
            return 1
        except Exception:
            print("操作未完成，请检查依赖、设备连接与开发者模式。", file=output)
            return 1


if __name__ == "__main__":
    raise SystemExit(main())
