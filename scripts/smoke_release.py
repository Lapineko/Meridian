"""Run packaged executables from a Unicode/space path without development PATH entries."""
import json
import os
from pathlib import Path
import subprocess
from zipfile import ZipFile

root = Path(__file__).resolve().parents[1]
destination = root / ".build" / "portable-smoke" / "Meridian 便携测试"
destination.mkdir(parents=True, exist_ok=True)
with ZipFile(root / "release" / "Meridian-0.1.0-windows-x64.zip") as archive:
    for name in archive.namelist():
        if not (destination / name).resolve().is_relative_to(destination.resolve()):
            raise SystemExit("Invalid archive member")
    archive.extractall(destination)
environment = os.environ.copy()
system_root = os.environ["SYSTEMROOT"]
environment["PATH"] = os.pathsep.join([str(Path(system_root) / "System32"), system_root])
environment.pop("PYTHONHOME", None)
environment.pop("PYTHONPATH", None)
engine = subprocess.run([str(destination / "engine" / "meridian-engine.exe"), "--self-test"],
                        env=environment, cwd=destination, capture_output=True, text=True, encoding="utf-8", timeout=30)
assert engine.returncode == 0, "Portable engine failed"
assert json.loads(engine.stdout)["ok"], "Portable engine self-test failed"
report = root / ".build" / "portable-smoke" / "ui-checks.json"
desktop = subprocess.run([str(destination / "Meridian.exe"), "--self-test", str(report)],
                         env=environment, cwd=destination, timeout=30)
assert desktop.returncode == 0 and json.loads(report.read_text())["ok"], "Portable UI failed"
print("Portable smoke passed: Unicode + spaces, isolated PATH, frozen engine and 11 native desktop checks.")
