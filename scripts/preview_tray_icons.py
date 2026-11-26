from PIL import Image
import os

os.makedirs('build/icon_preview', exist_ok=True)
for name in ['assets/tray_dark.ico', 'assets/tray_light.ico']:
    base = os.path.splitext(os.path.basename(name))[0]
    for sz in [16, 24]:
        im = Image.open(name)
        try:
            im = im.ico.getimage((sz, sz))
        except Exception:
            im = im.convert('RGBA').resize((sz, sz), Image.NEAREST)
        im = im.convert('RGBA')
        # Save actual size
        im.save(f'build/icon_preview/{base}_{sz}.png')
        # Save enlarged nearest-neighbor preview for inspection
        im.resize((sz * 8, sz * 8), Image.NEAREST).save(f'build/icon_preview/{base}_{sz}_x8.png')
        print('wrote', base, sz)