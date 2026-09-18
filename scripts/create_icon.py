"""Rebuild the original geometric app icon. Requires Pillow, only at design time."""
from pathlib import Path
from PIL import Image, ImageDraw

root = Path(__file__).resolve().parents[1]
image = Image.new("RGBA", (512, 512))
draw = ImageDraw.Draw(image)
draw.rounded_rectangle((0, 0, 511, 511), 110, fill="#25513e")
draw.ellipse((109, 109, 403, 403), outline="#f8f7f3", width=12)
draw.ellipse((190, 109, 322, 403), outline="#f8f7f3", width=10)
draw.line((109, 256, 403, 256), fill="#f8f7f3", width=10)
draw.ellipse((335, 132, 375, 172), fill="#d8e6ba")
image.save(root / "assets" / "meridian.ico", sizes=[(16,16),(24,24),(32,32),(48,48),(64,64),(128,128),(256,256)])
image.resize((256,256), Image.Resampling.LANCZOS).save(root / "assets" / "meridian.png")
