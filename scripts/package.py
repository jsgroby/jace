"""Package the tested release executable with provenance and SHA-256 checksums."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import zipfile

p = argparse.ArgumentParser()
p.add_argument('--platform', required=True, choices=['macos-arm64', 'linux-x64', 'linux-arm64', 'windows-x64', 'windows-arm64'])
a = p.parse_args()
root = Path(__file__).resolve().parents[1]
version = re.search(r'#define\s+ENGINE_VERSION\s+"([^"]+)"', (root/'defs.h').read_text())[1]
sha = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=root, text=True).strip()
ref = os.environ.get('GITHUB_REF', '')
if ref.startswith('refs/tags/'):
    label = ref.removeprefix('refs/tags/')
    if not re.fullmatch(r'v[0-9]+(?:\.[0-9]+){1,2}(?:-[A-Za-z0-9.-]+)?', label):
        raise SystemExit('Unsupported release tag format')
else:
    label = f'{version}-{sha[:12]}'
name = f'jace-{label}-{a.platform}'
stage = root/'build'/'package'/name
stage.mkdir(parents=True, exist_ok=False)
exe = f'jace-{a.platform}.exe' if a.platform.startswith('windows') else 'jace-baseline'
shutil.copy2(root/exe, stage/exe)
for doc in ['README.md', 'BUILDING.md', 'CI.md']:
    shutil.copy2(root/doc, stage/doc)
# Preserve licensing documents if supplied, without inventing a project license.
for pattern in ['LICENSE*', 'COPYING*', 'NOTICE*']:
    for doc in root.glob(pattern):
        if doc.is_file(): shutil.copy2(doc, stage/doc.name)
(stage/'BUILD.json').write_text(json.dumps({'version': version, 'commit': sha, 'platform': a.platform,
    'ref': ref, 'run_id': os.environ.get('GITHUB_RUN_ID'),
    'compiler': subprocess.check_output([os.environ.get('CC', 'cc'), '--version'], text=True).splitlines()[0],
    'book_included': False}, indent=2)+'\n')
(stage/'SHA256SUMS').write_text(''.join(f'{hashlib.sha256(f.read_bytes()).hexdigest()}  {f.name}\n'
    for f in sorted(stage.iterdir()) if f.is_file()))
out = root/'dist'; out.mkdir(exist_ok=True)
if a.platform.startswith('windows'):
    archive = out/(name+'.zip')
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for f in sorted(stage.iterdir()): z.write(f, name+'/'+f.name)
else:
    archive = out/(name+'.tar.gz')
    with tarfile.open(archive, 'w:gz') as t: t.add(stage, arcname=name)
( out/(archive.name+'.sha256')).write_text(f'{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n')
print(archive)
