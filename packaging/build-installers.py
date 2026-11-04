#!/usr/bin/env python3
"""Build installers from existing, tested release binaries; never installs them."""
from pathlib import Path
import hashlib
import json
import plistlib
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as ET
from html import escape

ROOT = Path(__file__).resolve().parents[1]
DIST = ROOT / 'dist'
VERSION = json.loads((ROOT / 'buildspec.json').read_text())['version']


def run(*args):
    subprocess.run([str(arg) for arg in args], check=True, cwd=ROOT)


def main():
    for tool in ('makensis', 'pkgbuild', 'productbuild', 'ditto'):
        if not shutil.which(tool):
            raise SystemExit(f'Missing tool: {tool}')
    dll = DIST / 'windows/obs-plugins/64bit/obs-control-deck.dll'
    bundle = DIST / 'macos/obs-control-deck.plugin'
    if not dll.is_file() or not bundle.is_dir():
        raise SystemExit('Stage the release binaries in dist/windows and dist/macos first.')
    run('codesign', '--verify', '--verbose=2', bundle)
    guide = (ROOT / 'packaging/LLEGEIX-ME.txt').read_text().replace('@VERSION@', VERSION)
    (DIST / 'Botonera-LLEGEIX-ME.txt').write_text(guide)
    with tempfile.TemporaryDirectory(prefix='botonera-installers-') as tmp:
        work = Path(tmp)
        windows = work / 'windows'
        (windows / 'data/locale').mkdir(parents=True)
        shutil.copy2(dll, windows / dll.name)
        for locale in ('ca-ES.ini', 'en-US.ini'):
            shutil.copy2(ROOT / 'data/locale' / locale, windows / 'data/locale' / locale)
        shutil.copy2(ROOT / 'LICENSE', windows / 'LICENSE.txt')
        (windows / 'LLEGEIX-ME.txt').write_text(guide)
        size_kb = sum(p.stat().st_size for p in windows.rglob('*') if p.is_file()) // 1024 + 160
        run('makensis', '-V3', f'-DVERSION={VERSION}', f'-DPAYLOAD={windows}',
            f'-DOUTPUT={DIST / f"Botonera-{VERSION}-Windows-x64-Setup.exe"}', f'-DSIZE_KB={size_kb}',
            ROOT / 'packaging/windows/botonera.nsi')
        # A current-user package: Library is resolved under the selected user's home.
        macroot = work / 'macroot'
        plugin = macroot / 'Library/Application Support/obs-studio/plugins/obs-control-deck.plugin'
        plugin.parent.mkdir(parents=True)
        run('ditto', '--norsrc', '--noextattr', '--noacl', bundle, plugin)
        components = work / 'components.plist'
        run('pkgbuild', '--analyze', '--root', macroot, components)
        with components.open('rb') as f:
            entries = plistlib.load(f)
        for entry in entries:
            entry['BundleIsRelocatable'] = False
            entry['BundleIsVersionChecked'] = False
            entry['BundleOverwriteAction'] = 'upgrade'
        with components.open('wb') as f:
            plistlib.dump(entries, f)
        component = work / 'botonera-component.pkg'
        identifier = 'local.obs-control-deck.installer'
        run('pkgbuild', '--root', macroot, '--component-plist', components,
            '--identifier', identifier, '--version', VERSION, '--install-location', '/',
            '--ownership', 'recommended', '--filter', r'/\._', component)
        distribution = ET.Element('installer-gui-script', {'minSpecVersion': '2'})
        ET.SubElement(distribution, 'title').text = f'Botonera per a OBS {VERSION}'
        ET.SubElement(distribution, 'options', {'customize': 'never', 'require-scripts': 'false',
                                             'hostArchitectures': 'arm64,x86_64'})
        ET.SubElement(distribution, 'domains', {'enable_anywhere': 'false', 'enable_currentUserHome': 'true',
                                             'enable_localSystem': 'false'})
        ET.SubElement(distribution, 'welcome', {'file': 'welcome.html', 'mime-type': 'text/html'})
        ET.SubElement(distribution, 'conclusion', {'file': 'conclusion.html', 'mime-type': 'text/html'})
        outline = ET.SubElement(distribution, 'choices-outline')
        ET.SubElement(outline, 'line', {'choice': 'botonera'})
        choice = ET.SubElement(distribution, 'choice', {'id': 'botonera', 'visible': 'false', 'title': 'Botonera'})
        ET.SubElement(choice, 'pkg-ref', {'id': identifier})
        package_ref = ET.SubElement(distribution, 'pkg-ref', {'id': identifier, 'version': VERSION,
                                                            'onConclusion': 'none'})
        package_ref.text = component.name
        must_close = ET.SubElement(package_ref, 'must-close')
        ET.SubElement(must_close, 'app', {'id': 'com.obsproject.obs-studio'})
        distribution_path = work / 'Distribution.xml'
        ET.ElementTree(distribution).write(distribution_path, encoding='utf-8', xml_declaration=True)
        resources = work / 'resources'
        resources.mkdir()
        for name in ('welcome', 'conclusion'):
            html = (ROOT / f'packaging/macos/{name}.html').read_text().replace('@VERSION@', escape(VERSION))
            (resources / f'{name}.html').write_text(html)
        run('productbuild', '--distribution', distribution_path, '--resources', resources,
            '--package-path', work, DIST / f'Botonera-{VERSION}-macOS-universal.pkg')
    artifacts = sorted(p for p in DIST.iterdir() if p.suffix in ('.zip', '.exe', '.pkg'))
    (DIST / 'SHA256SUMS.txt').write_text(''.join(
        hashlib.sha256(p.read_bytes()).hexdigest() + '  ' + p.name + '\n' for p in artifacts))
    print(f'Created Windows and macOS installers for {VERSION} in {DIST}')


if __name__ == '__main__':
    main()
