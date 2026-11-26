# Generate minimalist Windows 11 Fluent tray icons.
# Refined: thinner strokes, compact proportions, per-size tuning for crisp tray rendering.
# Dark taskbar icon = white glyph; light taskbar icon = dark glyph.
import struct
import os
import math

SIZES = [16, 20, 24, 32]


def _dist_to_rounded_rect(px, py, x0, y0, x1, y1, r):
    cx = min(max(px, x0 + r), x1 - r)
    cy = min(max(py, y0 + r), y1 - r)
    return math.hypot(px - cx, py - cy) - r


def _blend(px, color, alpha):
    r, g, b, a0 = px
    ar, ag, ab, _ = color
    a = max(0.0, min(1.0, alpha))
    if a <= 0:
        return px
    na = a + a0 / 255.0 * (1.0 - a)
    if na <= 0:
        return (0, 0, 0, 0)
    nr = (ar * a + r * (a0 / 255.0) * (1.0 - a)) / na
    ng = (ag * a + g * (a0 / 255.0) * (1.0 - a)) / na
    nb = (ab * a + b * (a0 / 255.0) * (1.0 - a)) / na
    return (int(round(nr)), int(round(ng)), int(round(nb)), int(round(na * 255)))


def draw_rounded_rect_outline(canvas, s, x0, y0, x1, y1, r, stroke, color, feather=0.5):
    half = stroke / 2.0
    for y in range(s):
        py = y + 0.5
        for x in range(s):
            px = x + 0.5
            d = _dist_to_rounded_rect(px, py, x0, y0, x1, y1, r)
            ad = abs(d) - half
            alpha = 1.0 - max(0.0, min(1.0, (ad + feather) / max(feather, 1e-6)))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_filled_rounded_rect(canvas, s, x0, y0, x1, y1, r, color, feather=0.5):
    miny = max(0, int(math.floor(y0 - 1)))
    maxy = min(s, int(math.ceil(y1 + 2)))
    minx = max(0, int(math.floor(x0 - 1)))
    maxx = min(s, int(math.ceil(x1 + 2)))
    for y in range(miny, maxy):
        py = y + 0.5
        for x in range(minx, maxx):
            px = x + 0.5
            d = _dist_to_rounded_rect(px, py, x0, y0, x1, y1, r)
            alpha = 1.0 - max(0.0, min(1.0, (d + feather) / max(feather, 1e-6)))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_line(canvas, s, x0, y0, x1, y1, thickness, color, feather=0.5):
    vx, vy = x1 - x0, y1 - y0
    vv = vx * vx + vy * vy
    if vv <= 0:
        return
    r = thickness / 2.0
    minx = int(max(0, math.floor(min(x0, x1) - r - 1)))
    maxx = int(min(s - 1, math.ceil(max(x0, x1) + r + 1)))
    miny = int(max(0, math.floor(min(y0, y1) - r - 1)))
    maxy = int(min(s - 1, math.ceil(max(y0, y1) + r + 1)))
    for y in range(miny, maxy + 1):
        py = y + 0.5
        for x in range(minx, maxx + 1):
            px = x + 0.5
            t = ((px - x0) * vx + (py - y0) * vy) / vv
            t = max(0.0, min(1.0, t))
            cx = x0 + t * vx
            cy = y0 + t * vy
            d = math.hypot(px - cx, py - cy) - r
            alpha = 1.0 - max(0.0, min(1.0, (d + feather) / max(feather, 1e-6)))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_circle(canvas, s, cx, cy, radius, color, feather=0.5):
    minx = int(max(0, math.floor(cx - radius - 1)))
    maxx = int(min(s - 1, math.ceil(cx + radius + 1)))
    miny = int(max(0, math.floor(cy - radius - 1)))
    maxy = int(min(s - 1, math.ceil(cy + radius + 1)))
    for y in range(miny, maxy + 1):
        py = y + 0.5
        for x in range(minx, maxx + 1):
            px = x + 0.5
            d = math.hypot(px - cx, py - cy) - radius
            alpha = 1.0 - max(0.0, min(1.0, (d + feather) / max(feather, 1e-6)))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


# Per-size tuning: prioritize clarity at actual tray size.
# stroke/hook/dot sizes are in pixels; positions are fractional where useful.
SPECS = {
    16: dict(margin=1.0, top=2.0, stroke=1.0, feather=0.45, radius_frac=0.24,
             header_frac=0.40, hook_w=1.6, hook_h=2.0, hook_top=0.4,
             dot_r=0.75, dot_cols=(0.28, 0.50, 0.72), dot_rows=(0.36, 0.70)),
    20: dict(margin=1.5, top=2.5, stroke=1.0, feather=0.50, radius_frac=0.24,
             header_frac=0.39, hook_w=1.8, hook_h=2.4, hook_top=0.8,
             dot_r=0.90, dot_cols=(0.29, 0.50, 0.71), dot_rows=(0.36, 0.70)),
    24: dict(margin=2.0, top=3.0, stroke=1.1, feather=0.55, radius_frac=0.24,
             header_frac=0.39, hook_w=2.0, hook_h=2.8, hook_top=1.2,
             dot_r=1.05, dot_cols=(0.29, 0.50, 0.71), dot_rows=(0.36, 0.70)),
    32: dict(margin=2.5, top=4.0, stroke=1.3, feather=0.60, radius_frac=0.24,
             header_frac=0.39, hook_w=2.6, hook_h=3.6, hook_top=1.8,
             dot_r=1.35, dot_cols=(0.29, 0.50, 0.71), dot_rows=(0.36, 0.70)),
}


def render_icon(size, color):
    s = size
    sp = SPECS[s]
    canvas = [[(0, 0, 0, 0) for _ in range(s)] for _ in range(s)]

    stroke = sp['stroke']
    feather = sp['feather']
    margin = sp['margin']
    top = sp['top']
    x0, y0 = margin, top
    x1, y1 = s - margin, s - margin
    w = x1 - x0
    h = y1 - y0
    r = max(2.0, min(w, h) * sp['radius_frac'])

    # Rounded calendar body
    draw_rounded_rect_outline(canvas, s, x0, y0, x1, y1, r, stroke, color, feather)

    # Header divider slightly above center
    header_y = y0 + h * sp['header_frac']
    draw_line(canvas, s, x0 + stroke * 0.8, header_y, x1 - stroke * 0.8, header_y, stroke, color, feather)

    # Two symmetrical binding hooks
    hook_w = sp['hook_w']
    hook_h = sp['hook_h']
    hook_r = hook_w / 2.0
    hook_top = sp['hook_top']
    hook_bottom = y0 + stroke * 0.9
    for frac in (0.33, 0.67):
        cx = x0 + w * frac
        hx0 = cx - hook_w / 2.0
        hx1 = cx + hook_w / 2.0
        draw_filled_rounded_rect(canvas, s, hx0, hook_top, hx1, hook_bottom, hook_r, color, feather)

    # 6 dots: 3 columns x 2 rows
    lower_top = header_y + stroke * 0.9
    lower_bottom = y1 - stroke * 1.0
    lower_h = lower_bottom - lower_top
    dot_r = sp['dot_r']
    for rf in sp['dot_rows']:
        cy = lower_top + lower_h * rf
        for cf in sp['dot_cols']:
            cx = x0 + w * cf
            draw_circle(canvas, s, cx, cy, dot_r, color, feather)

    return canvas


def write_ico(path, color):
    images = []
    for s in SIZES:
        canvas = render_icon(s, color)
        pixel_data = bytearray()
        for y in range(s - 1, -1, -1):
            for x in range(s):
                r, g, b, a = canvas[y][x]
                pixel_data += bytes([b, g, r, a])
        mask_row = ((s + 31) // 32) * 4
        mask = bytearray(mask_row * s)
        bih = struct.pack('<IIIHHIIIIII', 40, s, s * 2, 1, 32, 0, len(pixel_data) + len(mask), 0, 0, 0, 0)
        images.append((s, bih + bytes(pixel_data) + bytes(mask)))

    count = len(images)
    header = struct.pack('<HHH', 0, 1, count)
    offset = 6 + 16 * count
    entries = bytearray()
    blobs = bytearray()
    for s, data in images:
        w = s if s < 256 else 0
        h = s if s < 256 else 0
        entries += struct.pack('<BBBBHHII', w, h, 0, 0, 1, 32, len(data), offset)
        blobs += data
        offset += len(data)
    with open(path, 'wb') as f:
        f.write(header)
        f.write(entries)
        f.write(blobs)
    print('wrote', path, os.path.getsize(path))


if __name__ == '__main__':
    os.makedirs('assets', exist_ok=True)
    write_ico('assets/tray_dark.ico', (255, 255, 255, 255))
    write_ico('assets/tray_light.ico', (0, 0, 0, 255))
