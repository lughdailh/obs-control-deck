#!/usr/bin/env python3
"""Inspect and extract installer payloads without executing installation actions."""
from pathlib import Path
import hashlib
import json
import plistlib
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
VERSION = json.loads((ROOT / 'buildspec.json').read_text())['version']
DIST = ROOT / 'dist'


def run(*args):
    return subprocess.run([str(a) for a in args], check=True, capture_output=True, text=True).stdout


def digest(path):
    return hashlib.sha256(path.read_bytes()).digest()


with tempfile.TemporaryDirectory(prefix='botonera-verify-') as tmp:
    work = Path(tmp)
    exe = DIST / f'Botonera-{VERSION}-Windows-x64-Setup.exe'
    pkg = DIST / f'Botonera-{VERSION}-macOS-universal.pkg'
    unpacked = work / 'windows'
    run('7zz', 'x', '-y', f'-o{unpacked}', exe)
    assert digest(unpacked / 'obs-control-deck.dll') == digest(DIST / 'windows/obs-plugins/64bit/obs-control-deck.dll')
    assert digest(unpacked / 'bin/64bit/obs-control-deck.dll') == digest(unpacked / 'obs-control-deck.dll')
    binary = (unpacked / 'obs-control-deck.dll').read_bytes()
    pe = int.from_bytes(binary[0x3c:0x40], 'little')
    assert binary[pe:pe+4] == b'PE\x00\x00'
    assert int.from_bytes(binary[pe+4:pe+6], 'little') == 0x8664, 'Plugin DLL must be x64'
    for locale in ('ca-ES.ini', 'en-US.ini'):
        assert digest(unpacked / 'data/locale' / locale) == digest(ROOT / 'data/locale' / locale)
    assert (unpacked / 'LICENSE.txt').stat().st_size > 1000
    assert (unpacked / 'LLEGEIX-ME.txt').read_text() == (DIST / 'Botonera-LLEGEIX-ME.txt').read_text()
    assert (unpacked / 'Desinstal·lar Botonera.exe').is_file()
    expanded = work / 'macos'
    run('pkgutil', '--expand-full', pkg, expanded)
    distribution = ET.parse(expanded / 'Distribution').getroot()
    domains = distribution.find('domains').attrib
    assert domains == {'enable_anywhere': 'false', 'enable_currentUserHome': 'true', 'enable_localSystem': 'false'}
    assert run('installer', '-dominfo', '-pkg', pkg).strip() == 'CurrentUserHomeDirectory'
    choices = plistlib.loads(run('installer', '-showChoicesXML', '-pkg', pkg, '-target', 'CurrentUserHomeDirectory').encode())
    assert choices[0]['childItems'][0]['choiceIsSelected'] == 1
    must_close = distribution.find('pkg-ref/must-close/app')
    assert must_close is not None and must_close.attrib['id'] == 'com.obsproject.obs-studio'
    payload = expanded / 'botonera-component.pkg/Payload/Library/Application Support/obs-studio/plugins/obs-control-deck.plugin'
    staged = DIST / 'macos/obs-control-deck.plugin'
    for source in staged.rglob('*'):
        if source.is_file():
            assert digest(source) == digest(payload / source.relative_to(staged)), source
    run('codesign', '--verify', '--verbose=2', payload)
    info = plistlib.loads((payload / 'Contents/Info.plist').read_bytes())
    assert info['CFBundleShortVersionString'] == VERSION
    print(f'Windows payload: x64 DLL, locales, guide, license and uninstaller verified.')
    print(f'macOS payload: byte-identical signed bundle, current-user-only domain and package choices verified.')
    print('No installer or Windows plugin was executed; native installation remains to be tested.')
