"""Fail closed on private build paths and common credential/paired-device artifacts.

Only filenames and finding categories are printed, never matched secret values.
"""
import argparse
import os
import re
from pathlib import Path

DENIED_SUFFIXES = {".pem", ".key", ".p12", ".pfx", ".mobileprovision", ".log", ".dmp", ".pdb", ".pyc"}
DENIED_PARTS = {"__pycache__", ".git", ".venv", "venv", ".pymobiledevice3", "pair_records", "pairing_records"}
TEXT_EXTENSIONS = {".py", ".cpp", ".hpp", ".h", ".json", ".md", ".txt", ".sh", ".bat", ".ps1", ".yml", ".yaml", ".manifest", ".rc", ".spec", ".cmake"}


def private_needles():
    # Derive local values at audit time, never commit a user's identity to the scanner.
    paths = {str(Path.home()), str(Path(__file__).resolve().parents[1])}
    needles = []
    for path in paths:
        for spelling in {path, path.replace("\\", "/"), path.replace("\\", "\\\\")}:
            needles += [spelling.encode().lower(), spelling.encode("utf-16-le").lower()]
    user = os.environ.get("USERNAME", "")
    if len(user) > 4 and user.lower() not in {"runneradmin", "administrator"}:
        needles.extend([user.encode().lower(), user.encode("utf-16-le").lower()])
    return needles


def scan(root):
    findings = []
    needles = private_needles()
    count = 0
    for file in sorted(root.rglob("*")):
        if not file.is_file():
            continue
        relative = file.relative_to(root)
        count += 1
        public_ca = relative.parts[-2:] == ("certifi", "cacert.pem")
        if set(relative.parts) & DENIED_PARTS or (file.suffix.lower() in DENIED_SUFFIXES and not public_ca):
            findings.append((str(relative), "private/cache file type"))
        data = file.read_bytes()
        if any(needle in data.lower() for needle in needles):
            findings.append((str(relative), "personal build path or username"))
        # CA bundles and upstream sample code can contain public certificates. Private keys are never allowed.
        if re.search(rb"-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----", data) and file.name != "audit_release.py":
            findings.append((str(relative), "private key material"))
        if file.suffix.lower() in TEXT_EXTENSIONS:
            if re.search(rb"(?i)<key>(?:HostPrivateKey|DeviceCertificate|EscrowBag)</key>", data):
                findings.append((str(relative), "device pairing record"))
    return count, findings


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("root", type=Path)
    args = parser.parse_args()
    count, findings = scan(args.root)
    for path, category in findings:
        print(f"FAIL {category}: {path}")
    print(f"Privacy audit: {count} files; {len(findings)} findings.")
    raise SystemExit(bool(findings))
