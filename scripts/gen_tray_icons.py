# Generate minimal monochrome tray icons (dark taskbar=white glyph, light taskbar=dark glyph)
import struct, os

def make_icon(color_rgba, path):
    sizes = [16, 20, 24, 32]
    images = []
    for s in sizes:
        px = [[(0, 0, 0, 0) for _ in range(s)] for _ in range(s)]

        def setp(x, y, a=255):
            if 0 <= x < s and 0 <= y < s:
                r, g, b, _ = color_rgba
                px[y][x] = (r, g, b, a)

        def draw_rect(x0, y0, x1, y1, th=1):
            for t in range(th):
                for x in range(x0, x1 + 1):
                    setp(x, y0 + t)
                    setp(x, y1 - t)
                for y in range(y0, y1 + 1):
                    setp(x0 + t, y)
                    setp(x1 - t, y)

        def fill_rect(x0, y0, x1, y1, a=255):
            for y in range(y0, y1 + 1):
                for x in range(x0, x1 + 1):
                    setp(x, y, a)

        stroke = 1
        m = 1 if s <= 16 else 2
        x0, y0, x1, y1 = m, m + (1 if s >= 20 else 0), s - 1 - m, s - 1 - m
        # outer calendar outline
        draw_rect(x0, y0, x1, y1, stroke)

        # header separator
        header_y = y0 + max(3, round((y1 - y0 + 1) * 0.36))
        for x in range(x0 + stroke, x1 - stroke + 1):
            setp(x, header_y)

        # top binding tabs
        tab_w = max(1, round(s * 0.07))
        tab_h = max(2, round(s * 0.10))
        for frac in (0.32, 0.68):
            tx = x0 + round((x1 - x0 + 1) * frac) - tab_w // 2
            fill_rect(tx, max(0, y0 - tab_h // 2), tx + tab_w - 1, y0)

        # inner content: simple 2x1 blocks for clarity at small sizes
        ix0 = x0 + stroke + max(1, round(s * 0.18))
        ix1 = x1 - stroke - max(1, round(s * 0.18))
        iy0 = header_y + 1 + max(1, round(s * 0.14))
        iy1 = y1 - stroke - max(1, round(s * 0.16))
        if ix1 > ix0 and iy1 > iy0:
            gap = 1 if s <= 16 else max(1, round(s * 0.06))
            bw = (ix1 - ix0 + 1 - gap) // 2
            bh = iy1 - iy0 + 1
            if bw >= 1 and bh >= 1:
                fill_rect(ix0, iy0, ix0 + bw - 1, iy1)
                rx0 = ix0 + bw + gap
                fill_rect(rx0, iy0, min(ix1, rx0 + bw - 1), iy1)

        # 32-bit BGRA DIB
        pixel_data = bytearray()
        for y in range(s - 1, -1, -1):
            for x in range(s):
                r, g, b, a = px[y][x]
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

os.makedirs('assets', exist_ok=True)
make_icon((255, 255, 255, 255), 'assets/tray_dark.ico')
make_icon((0, 0, 0, 255), 'assets/tray_light.ico')
