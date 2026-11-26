from PIL import Image, ImageDraw, ImageFont
import os
import struct

os.makedirs('build/icon_preview', exist_ok=True)
os.makedirs('build/icon_compare', exist_ok=True)

def load_ico_size(path, sz):
    im = Image.open(path)
    try:
        im = im.ico.getimage((sz, sz))
    except Exception:
        im = im.convert('RGBA').resize((sz, sz), Image.NEAREST)
    return im.convert('RGBA')

def ico_info(path):
    b = open(path, 'rb').read()
    _, _, count = struct.unpack_from('<HHH', b, 0)
    off = 6
    infos = []
    for _ in range(count):
        w, h, cc, rs, planes, bit, bsz, ofs = struct.unpack_from('<BBBBHHII', b, off)
        off += 16
        blob = b[ofs:ofs+bsz]
        px = blob[40:40 + (w or 256)*(h or 256)*4]
        has_alpha = any(px[i+3] != 0 for i in range(0, len(px), 4))
        has_trans = any(px[i+3] != 255 for i in range(0, len(px), 4))
        infos.append((w or 256, h or 256, bit, has_alpha, has_trans))
    return infos

for name in ['assets/tray_dark.ico', 'assets/tray_light.ico', 'build/icon_compare/old_tray_dark.ico', 'build/icon_compare/old_tray_light.ico']:
    if not os.path.exists(name):
        continue
    base = os.path.splitext(os.path.basename(name))[0]
    print(name, ico_info(name))
    for sz in [16, 20, 24, 32]:
        im = load_ico_size(name, sz)
        im.save(f'build/icon_preview/{base}_{sz}.png')
        im.resize((sz * 8, sz * 8), Image.NEAREST).save(f'build/icon_preview/{base}_{sz}_x8.png')

# side-by-side compare old vs refined for dark and light
for variant in ['dark', 'light']:
    old = f'build/icon_compare/old_tray_{variant}.ico'
    new = f'assets/tray_{variant}.ico'
    if not (os.path.exists(old) and os.path.exists(new)):
        continue
    sizes = [16, 20, 24, 32]
    pad = 16
    cell = 96
    label_h = 20
    width = pad + len(sizes) * (cell + pad)
    height = pad + label_h + 2 * (cell + label_h + pad)
    img = Image.new('RGBA', (width, height), (32, 32, 32, 255) if variant == 'dark' else (240, 240, 240, 255))
    draw = ImageDraw.Draw(img)
    try:
        font = ImageFont.load_default()
    except Exception:
        font = None
    text_color = (255,255,255,255) if variant=='dark' else (0,0,0,255)
    draw.text((pad, 4), f'{variant} tray icon: old (top) vs refined (bottom)', fill=text_color, font=font)
    for i, sz in enumerate(sizes):
        x = pad + i * (cell + pad)
        for row, (label, path) in enumerate([('old', old), ('refined', new)]):
            y = pad + label_h + row * (cell + label_h + pad)
            icon = load_ico_size(path, sz).resize((sz * 3, sz * 3), Image.NEAREST)
            ix = x + (cell - icon.size[0]) // 2
            iy = y + (cell - icon.size[1]) // 2
            # checkerboard behind for transparency visibility
            for yy in range(y, y+cell, 8):
                for xx in range(x, x+cell, 8):
                    c = (70,70,70,255) if ((xx//8+yy//8)%2==0) else (50,50,50,255)
                    if variant=='light':
                        c = (220,220,220,255) if ((xx//8+yy//8)%2==0) else (200,200,200,255)
                    draw.rectangle([xx,yy,min(x+cell,xx+8),min(y+cell,yy+8)], fill=c)
            img.alpha_composite(icon, (ix, iy))
            draw.rectangle([x, y, x+cell, y+cell], outline=(120,120,120,255))
            draw.text((x, y+cell+2), f'{label} {sz}px', fill=text_color, font=font)
    out = f'build/icon_compare/compare_{variant}.png'
    img.save(out)
    print('wrote', out)
