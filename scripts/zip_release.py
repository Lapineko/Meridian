from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

root = Path(__file__).resolve().parents[1]
for source, filename in [
    (root / "release" / "Meridian", "Meridian-0.1.0-windows-x64.zip"),
    (root / "release" / "Meridian-source", "Meridian-0.1.0-source.zip"),
    (root / ".build" / "upstream-source", "Meridian-0.1.0-upstream-source.zip"),
]:
    with ZipFile(root / "release" / filename, "w", ZIP_DEFLATED, compresslevel=6) as archive:
        for path in sorted(source.rglob("*")):
            if path.is_file():
                archive.write(path, path.relative_to(source))
    print(filename)
