"""Refresh public Apple Store coordinates from Apple's own JSON-LD (build time only)."""
import concurrent.futures
import json
import re
import urllib.request
from pathlib import Path

STORES = [
    ("visitor", "US", "Apple Park Visitor Center", "美国 · 库比蒂诺", "retail/appleparkvisitorcenter/"),
    ("ifc", "HK", "Apple ifc mall", "中国香港 · 中环", "hk/en/retail/ifcmall/"),
    ("tokyo", "JP", "Apple Marunouchi", "日本 · 东京丸之内", "jp/retail/marunouchi/"),
    ("london", "UK", "Apple Regent Street", "英国 · 伦敦", "uk/retail/regentstreet/"),
    ("berlin", "DE", "Apple Kurfürstendamm", "德国 · 柏林", "de/retail/kurfuerstendamm/"),
    ("paris", "FR", "Apple Champs-Élysées", "法国 · 巴黎", "fr/retail/champs-elysees/"),
    ("singapore", "SG", "Apple Marina Bay Sands", "新加坡 · 滨海湾", "sg/retail/marinabaysands/"),
    ("sydney", "AU", "Apple Sydney", "澳大利亚 · 悉尼", "au/retail/sydney/"),
]


def fetch(store):
    ident, region, name, city, page = store
    url = "https://www.apple.com/" + page
    html = urllib.request.urlopen(url, timeout=30).read().decode("utf-8")
    match = re.search(r'"geo"\s*:\s*\{[^}]*"latitude"\s*:\s*([\d.\-]+)\s*,\s*"longitude"\s*:\s*([\d.\-]+)', html)
    if not match:
        raise RuntimeError(f"No store coordinates: {ident}")
    return dict(id=ident, region=region, name=name, city=city, lat=float(match[1]), lon=float(match[2]), source=url, kind="store")


if __name__ == "__main__":
    rows = [dict(id="park", region="US", name="Apple Park", city="美国 · 库比蒂诺", lat=37.334643,
                 lon=-122.008972, source="https://www.apple.com/apple-park/", kind="campus",
                 note="Apple 总部园区的近似中心点，保留自原项目；零售店请选 Visitor Center。")]
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as pool:
        rows.extend(pool.map(fetch, STORES))
    path = Path(__file__).resolve().parents[1] / "data" / "presets.json"
    path.write_text(json.dumps(rows, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(rows)} public landmarks, including {len(STORES)} official store coordinates.")
