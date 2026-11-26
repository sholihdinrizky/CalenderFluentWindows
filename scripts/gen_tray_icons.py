# Generate minimalist Windows 11 Fluent tray icons.
# Design: rounded-square calendar outline, top divider, two binding hooks, 3x2 dots.
# Dark taskbar icon = white glyph; light taskbar icon = dark glyph.
import struct, os, math

SIZES = [16, 20, 24, 32]


def _dist_to_rounded_rect(px, py, x0, y0, x1, y1, r):
    # Signed distance approximation via nearest point on rounded-rect boundary.
    # Negative inside, positive outside.
    cx = min(max(px, x0 + r), x1 - r)
    cy = min(max(py, y0 + r), y1 - r)
    return math.hypot(px - cx, py - cy) - r


def _blend(px, color, alpha):
    r, g, b, a0 = px
    ar, ag, ab, _ = color
    a = max(0.0, min(1.0, alpha))
    na = a + a0 / 255.0 * (1.0 - a)
    if na <= 0:
        return (0, 0, 0, 0)
    nr = (ar * a + r * (a0 / 255.0) * (1.0 - a)) / na
    ng = (ag * a + g * (a0 / 255.0) * (1.0 - a)) / na
    nb = (ab * a + b * (a0 / 255.0) * (1.0 - a)) / na
    return (int(round(nr)), int(round(ng)), int(round(nb)), int(round(na * 255)))


def draw_rounded_rect_outline(canvas, s, x0, y0, x1, y1, r, stroke, color):
    for y in range(s):
        for x in range(s):
            d = _dist_to_rounded_rect(x + 0.5, y + 0.5, x0, y0, x1, y1, r)
            # Outline band: |d| <= stroke/2 with 1px AA feather
            ad = abs(d) - (stroke / 2.0)
            alpha = 1.0 - max(0.0, min(1.0, ad + 0.5))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_filled_rounded_rect(canvas, s, x0, y0, x1, y1, r, color):
    for y in range(max(0, int(y0) - 1), min(s, int(y1) + 2)):
        for x in range(max(0, int(x0) - 1), min(s, int(x1) + 2)):
            d = _dist_to_rounded_rect(x + 0.5, y + 0.5, x0, y0, x1, y1, r)
            alpha = 1.0 - max(0.0, min(1.0, d + 0.5))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_line(canvas, s, x0, y0, x1, y1, thickness, color):
    # Distance to segment, with AA.
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
        for x in range(minx, maxx + 1):
            px, py = x + 0.5, y + 0.5
            t = ((px - x0) * vx + (py - y0) * vy) / vv
            t = max(0.0, min(1.0, t))
            cx = x0 + t * vx
            cy = y0 + t * vy
            d = math.hypot(px - cx, py - cy) - r
            alpha = 1.0 - max(0.0, min(1.0, d + 0.5))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def draw_circle(canvas, s, cx, cy, radius, color):
    minx = int(max(0, math.floor(cx - radius - 1)))
    maxx = int(min(s - 1, math.ceil(cx + radius + 1)))
    miny = int(max(0, math.floor(cy - radius - 1)))
    maxy = int(min(s - 1, math.ceil(cy + radius + 1)))
    for y in range(miny, maxy + 1):
        for x in range(minx, maxx + 1):
            d = math.hypot(x + 0.5 - cx, y + 0.5 - cy) - radius
            alpha = 1.0 - max(0.0, min(1.0, d + 0.5))
            if alpha > 0:
                canvas[y][x] = _blend(canvas[y][x], color, alpha)


def render_icon(size, color):
    s = size
    canvas = [[(0, 0, 0, 0) for _ in range(s)] for _ in range(s)]
    stroke = 1.25 if s <= 16 else (1.5 if s <= 24 else 2.0)

    # Layout tuned for tray readability.
    margin = 1.2 if s <= 16 else 1.6
    top = 2.0 if s <= 16 else 2.6
    x0, y0 = margin, top
    x1, y1 = s - margin, s - margin
    w = x1 - x0
    h = y1 - y0
    r = max(2.0, min(w, h) * 0.24)  # 24% rounded corners

    draw_rounded_rect_outline(canvas, s, x0, y0, x1, y1, r, stroke, color)

    # Header divider slightly above vertical center.
    header_y = y0 + h * 0.38
    draw_line(canvas, s, x0 + stroke * 0.7, header_y, x1 - stroke * 0.7, header_y, stroke, color)

    # Two binding hooks protruding upward from top edge.
    hook_w = max(1.4, w * 0.10)
    hook_h = max(2.0, h * 0.18)
    hook_r = hook_w / 2.0
    hook_top = max(0.4, y0 - hook_h * 0.55)
    hook_bottom = y0 + stroke * 0.8
    for frac in (0.33, 0.67):
        cx = x0 + w * frac
        hx0 = cx - hook_w / 2.0
        hx1 = cx + hook_w / 2.0
        draw_filled_rounded_rect(canvas, s, hx0, hook_top, hx1, hook_bottom, hook_r, color)

    # 6 circular dots: 3 columns x 2 rows in lower area.
    lower_top = header_y + stroke * 0.8
    lower_bottom = y1 - stroke * 0.8
    lower_h = lower_bottom - lower_top
    cols = [x0 + w * f for f in (0.28, 0.50, 0.72)]
    rows = [lower_top + lower_h * f for f in (0.36, 0.72)]
    dot_r = max(0.75, min(w, h) * 0.055)
    if s <= 16:
        dot_r = 0.8
    elif s <= 20:
        dot_r = 1.0
    elif s <= 24:
        dot_r = 1.15
    else:
        dot_r = 1.5
    for cy in rows:
        for cx in cols:
            draw_circle(canvas, s, cx, cy, dot_r, color)

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
        mask = bytearray(mask_row * s)  # alpha used; mask can be zero
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
    write_ico('assets/tray_dark.ico', (255, 255, 255, 255))   # white glyph for dark taskbar
    write_ico('assets/tray_light.ico', (0, 0, 0, 255))         # dark glyph for light taskbar
