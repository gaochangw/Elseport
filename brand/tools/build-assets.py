#!/usr/bin/env python3
# $Id$
# Elseport - build vector brand variants and application icon resources.
# SPDX-License-Identifier: GPL-3.0-only

from pathlib import Path
import re
import shutil
import subprocess

BRAND = Path(__file__).resolve().parent.parent
ROOT = BRAND.parent
MASTER = (BRAND / 'elseport-master.svg').read_text()
MARK = re.search(r'<g id="symbol">(.*?)</g>', MASTER, re.S).group(1).strip()
MARK_PATH = re.search(r'<path d="([^"]+)"', MARK).group(1)


def svg(body, width=128, height=128, title='Elseport'):
    return (f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
            f'viewBox="0 0 {width} {height}" role="img" aria-label="{title}">'
            f'<title>{title}</title>{body}</svg>\n')


def write(name, body):
    (BRAND / name).write_text(body, encoding='utf-8')


def raster(source, target, size):
    subprocess.run(['rsvg-convert', '-w', str(size), '-h', str(size), '-o', str(target), str(source)], check=True)


for color, name in [('#6659D8', 'mark'), ('#15152B', 'mark-mono'), ('#FFFFFF', 'mark-reversed')]:
    write(name + '.svg', svg(f'<g fill="{color}">{MARK}</g>'))
write('menu-template.svg', svg(f'<g transform="translate(-.4,-.4) scale(.2)" fill="#000000">{MARK}</g>', 24, 24, 'Elseport menu template'))

wordmark = (BRAND / 'wordmark.svg').read_text()
word_width = int(re.search(r'viewBox="0 0 (\d+)', wordmark).group(1))
word_path = re.search(r'<path d="([^"]+)"', wordmark).group(1)
for name, fill, mark_fill in [('logo-light', '#1B1C30', '#6659D8'), ('logo-dark', '#F5F4FF', '#B5ACFF'),
                               ('logo-mono', '#000000', '#000000'), ('logo-reversed', '#FFFFFF', '#FFFFFF')]:
    body = (f'<g fill="{mark_fill}" transform="translate(0,8)">{MARK}</g>'
            f'<g fill="{fill}" transform="translate(145,0)"><path d="{word_path}"/></g>')
    write(name + '.svg', svg(body, word_width + 151, 144, 'Elseport logo'))

app_body = ('<defs><linearGradient id="surface" x1="0" y1="0" x2="1" y2="1">'
            '<stop stop-color="#8572EE"/><stop offset=".6" stop-color="#6557D8"/>'
            '<stop offset="1" stop-color="#454BAF"/></linearGradient></defs>'
            '<rect x="12" y="12" width="1000" height="1000" rx="228" fill="url(#surface)"/>'
            '<rect x="20" y="20" width="984" height="984" rx="220" fill="none" stroke="#FFFFFF" stroke-opacity=".14" stroke-width="2"/>'
            '<g transform="translate(166,148) scale(5.4)" fill="#FFFFFF">'
            f'<path d="{MARK_PATH}"/><circle cx="98" cy="64" r="9" fill="#C4F0E1"/></g>')
write('app-icon.svg', svg(app_body, 1024, 1024, 'Elseport application icon'))
raster(BRAND / 'app-icon.svg', BRAND / 'app-icon-1024.png', 1024)
iconset = BRAND / 'Elseport.iconset'
iconset.mkdir(exist_ok=True)
for size in [16, 32, 128, 256, 512]:
    raster(BRAND / 'app-icon.svg', iconset / f'icon_{size}x{size}.png', size)
    raster(BRAND / 'app-icon.svg', iconset / f'icon_{size}x{size}@2x.png', size * 2)
subprocess.run(['/usr/bin/iconutil', '-c', 'icns', str(iconset), '-o', str(BRAND / 'Elseport.icns')], check=True)
raster(BRAND / 'menu-template.svg', BRAND / 'menu-template.png', 24)
raster(BRAND / 'menu-template.svg', BRAND / 'menu-template@2x.png', 48)

assets = ROOT / 'doc/design/prototype/assets'
assets.mkdir(parents=True, exist_ok=True)
for name in ['mark.svg', 'logo-light.svg', 'logo-dark.svg', 'app-icon.svg']:
    shutil.copy2(BRAND / name, assets / name)
shutil.copy2(BRAND / 'type/Manrope.ttf', assets / 'Manrope.ttf')
shutil.copy2(BRAND / 'type/OFL.txt', assets / 'Manrope-OFL.txt')
shutil.copy2(ROOT / 'LICENSE.txt', assets / 'GPL-3.0.txt')
print('Built SVG variants, 1024 px icon, ICNS and menu resources')
