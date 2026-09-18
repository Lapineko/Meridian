# Build on Windows: python -m PyInstaller --noconfirm scripts/engine.spec
from PyInstaller.utils.hooks import collect_data_files, collect_submodules, collect_dynamic_libs, copy_metadata
from pathlib import Path

root = Path(SPECPATH).parent
hidden = []
for module in ("pymobiledevice3.remote", "pymobiledevice3.services.dvt", "pmd_pytcp", "pmd_net_proto", "pmd_net_addr"):
    hidden += collect_submodules(module)
datas = collect_data_files("pymobiledevice3") + collect_data_files("developer_disk_image") + collect_data_files("pytun_pmd3")
datas += copy_metadata("pymobiledevice3", recursive=True)
a = Analysis([str(root / "backend" / "bridge.py")], pathex=[str(root)], binaries=collect_dynamic_libs("pytun_pmd3"), datas=datas,
             hiddenimports=hidden, hookspath=[], runtime_hooks=[],
             excludes=["IPython", "matplotlib", "tkinter", "pytest", "xonsh", "av"], noarchive=False)
# Some wheel license directories contain .py files that pip byte-compiles with local paths.
# Preserve the original notices, never distribute those generated caches.
a.datas = [entry for entry in a.datas if "__pycache__" not in Path(entry[0]).parts
           and not entry[0].endswith((".pyc", ".pyo"))]
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, [], exclude_binaries=True, name="meridian-engine", debug=False,
          bootloader_ignore_signals=False, strip=False, upx=False, console=True)
coll = COLLECT(exe, a.binaries, a.datas, strip=False, upx=False, name="engine")
