# Draws the desktop-shortcut icon: a cubic bezier with its two handles on an
# orange rounded tile. Output: tablet.ico (multi-size).
from pathlib import Path
from PIL import Image, ImageDraw

SIZE = 256
image = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
draw = ImageDraw.Draw(image)
draw.rounded_rectangle((8, 8, SIZE - 8, SIZE - 8), radius=48, fill=(224, 112, 40, 255))

p0, p1, p2, p3 = (48, 200), (72, 40), (184, 216), (208, 56)
def bezier(t: float) -> tuple[float, float]:
    u = 1 - t
    return tuple(u**3 * a + 3 * u**2 * t * b + 3 * u * t**2 * c + t**3 * d for a, b, c, d in zip(p0, p1, p2, p3))
draw.line([p0, p1], fill=(255, 230, 200, 200), width=6)
draw.line([p3, p2], fill=(255, 230, 200, 200), width=6)
draw.line([bezier(i / 64) for i in range(65)], fill=(255, 255, 255, 255), width=16, joint="curve")
for point in (p0, p3):
    draw.rectangle((point[0] - 14, point[1] - 14, point[0] + 14, point[1] + 14), fill=(40, 30, 20, 255), outline=(255, 255, 255, 255), width=4)
for point in (p1, p2):
    draw.ellipse((point[0] - 12, point[1] - 12, point[0] + 12, point[1] + 12), fill=(255, 230, 200, 255))

out = Path(__file__).resolve().parent.parent / "tablet.ico"
image.save(out, sizes=[(256, 256), (64, 64), (48, 48), (32, 32), (16, 16)])
print(out)
