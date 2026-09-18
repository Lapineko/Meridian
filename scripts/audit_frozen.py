"""Inspect compressed Python code filenames/constants for local build identities."""
import argparse
import marshal
import types
from pathlib import Path
from zipfile import ZipFile
from PyInstaller.archive.readers import CArchiveReader
from audit_release import private_needles


def inspect(code, needles):
    if not isinstance(code, types.CodeType):
        return 0
    findings = 0
    for item in (code.co_filename,) + code.co_consts:
        if isinstance(item, types.CodeType):
            findings += inspect(item, needles)
        elif isinstance(item, (str, bytes)):
            data = item.encode("utf-8", errors="replace") if isinstance(item, str) else item
            findings += int(any(n in data.lower() for n in needles))
    return findings


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("engine",type=Path)
    args = parser.parse_args()
    archive = CArchiveReader(str(args.engine))
    needles = private_needles()
    findings, checked = 0, 0
    for name, item in archive.toc.items():
        if item[-1] in ("m", "s"):
            findings += inspect(marshal.loads(archive.extract(name)), needles)
            checked += 1
        elif item[-1] == "z":
            modules = archive.open_embedded_archive(name)
            for module in modules.toc:
                findings += inspect(modules.extract(module), needles)
                checked += 1
    with ZipFile(args.engine.parent/"_internal"/"base_library.zip") as stdlib:
        for name in stdlib.namelist():
            if name.endswith(".pyc"):
                findings += inspect(marshal.loads(stdlib.read(name)[16:]),needles)
                checked += 1
    print(f"Frozen code audit: {checked} modules; {findings} personal path/name findings.")
    raise SystemExit(bool(findings))
