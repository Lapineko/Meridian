"""Copy dependency notices and verified upstream copyleft source distributions."""
import argparse
import concurrent.futures
import hashlib
import importlib.metadata as metadata
import json
import re
import shutil
import sys
import urllib.request
from pathlib import Path

BUILD_ONLY = {"pip", "pyinstaller", "pyinstaller-hooks-contrib", "altgraph", "pefile", "pywin32-ctypes"}


def fetch_source(dist, destination):
    name, version = dist.metadata["Name"], dist.version
    info = json.load(urllib.request.urlopen(f"https://pypi.org/pypi/{name}/{version}/json", timeout=30))
    candidates = [f for f in info["urls"] if f["packagetype"] == "sdist"]
    if not candidates:
        raise RuntimeError(f"No source distribution for {name}; supply upstream source before release")
    item = candidates[0]
    target = destination / item["filename"]
    expected = item["digests"]["sha256"]
    if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest() != expected:
        payload = urllib.request.urlopen(item["url"], timeout=60).read()
        if hashlib.sha256(payload).hexdigest() != expected:
            raise RuntimeError(f"Source digest mismatch for {name}")
        target.write_bytes(payload)
    return {"name":name, "version":version, "file":target.name, "sha256":expected, "source":item["url"]}


def collect(output, sources):
    output.mkdir(parents=True, exist_ok=True)
    sources.mkdir(parents=True, exist_ok=True)
    inventory, copyleft = [], []
    for dist in sorted(metadata.distributions(), key=lambda d: d.metadata["Name"].lower()):
        name = dist.metadata["Name"]
        if name.lower() in BUILD_ONLY:
            continue
        license_text = dist.metadata.get("License-Expression") or dist.metadata.get("License") or ""
        classifiers = " ".join(dist.metadata.get_all("Classifier") or [])
        inventory.append({"name":name,"version":dist.version,"license":license_text.splitlines()[0] if license_text else classifiers})
        for entry in dist.files or []:
            parts = Path(entry).parts
            if any(re.match(r"(?i)^(licen[cs]e|copying|notice|copyright)(\b|[._-])", p) for p in parts):
                original = Path(dist.locate_file(entry))
                if original.is_file():
                    # Preserve nested license filenames without carrying install paths.
                    filename = "__".join(p for p in parts if p not in ("..", "."))
                    folder = output / name
                    folder.mkdir(exist_ok=True)
                    shutil.copyfile(original, folder / filename)
        combined = (license_text + classifiers).lower()
        if any(k in combined for k in ("gpl", "general public license", "mozilla", "mpl-")):
            copyleft.append(dist)
    (output / "inventory.json").write_text(json.dumps(inventory, ensure_ascii=False, indent=2), encoding="utf-8")
    for candidate in [Path(sys.base_prefix)/"LICENSE.txt",Path(sys.base_prefix)/"LICENSE"]:
        if candidate.exists():
            shutil.copyfile(candidate, output/"Python-LICENSE.txt")
            break
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
        source_inventory=list(pool.map(lambda d:fetch_source(d,sources), copyleft))
    (sources/"SOURCES.json").write_text(json.dumps(source_inventory,indent=2),encoding="utf-8")
    (sources/"README.txt").write_text("Unmodified upstream source distributions for bundled copyleft dependencies.\nSee SOURCES.json for exact versions, origins and verified SHA-256 digests.\nMeridian's source archive includes pinned dependency versions and all build scripts.\n",encoding="utf-8")
    print(f"Collected notices for {len(inventory)} distributions and {len(source_inventory)} verified source archives.")


if __name__ == "__main__":
    parser=argparse.ArgumentParser()
    parser.add_argument("--output",type=Path,required=True)
    parser.add_argument("--sources",type=Path,required=True)
    args=parser.parse_args()
    collect(args.output,args.sources)
