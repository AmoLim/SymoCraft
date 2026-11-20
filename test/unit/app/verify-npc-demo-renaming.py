"""Explicit native-window regression for an edited Idle display name.

Run after building npc_demo_probe. Each run uses a new output directory; the
source assets, normal build assets and installed game are never rewritten.
"""

import argparse
from pathlib import Path
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-directory', type=Path, required=True)
    parser.add_argument('--output-directory', type=Path, required=True)
    parser.add_argument('--renderer', choices=('d3d12', 'vulkan'), required=True)
    args = parser.parse_args()
    binary = args.build_directory.resolve(strict=True) / 'bin'
    executable = binary / 'npc_demo_probe.exe'
    if not executable.is_file():
        raise ValueError('Build the npc_demo_probe target first.')
    output = args.output_directory.resolve()
    # Refuse reuse instead of removing or replacing a previous result.
    output.mkdir(parents=True, exist_ok=False)
    package = output / 'package'
    package.mkdir()
    shutil.copy2(executable, package / executable.name)
    for library in binary.glob('*.dll'):
        shutil.copy2(library, package / library.name)
    shutil.copytree(binary / 'assets', package / 'assets')
    tree_path = package / 'assets/ai/human_harvest.xml'
    tree = ET.parse(tree_path)
    idle_nodes = tree.findall('.//Idle')
    if not idle_nodes:
        raise ValueError('Human fixture has no Idle action to rename.')
    for idle in idle_nodes:
        idle.set('name', 'settled_idle')
    tree.write(tree_path, encoding='utf-8', xml_declaration=True)
    log = output / 'native.log'
    with log.open('wb') as stream:
        result = subprocess.run([str(package / executable.name), args.renderer], cwd=package,
                                stdout=stream, stderr=subprocess.STDOUT, timeout=95,
                                creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))
    text = log.read_text(encoding='utf-8', errors='replace')
    passed = (result.returncode == 0 and '[demo-probe] passed;' in text and
              'settled_idle' in text and '[demo] completed;' in text and
              not re.search(r'Validation Error|Validation Warning|VUID-|D3D12 ERROR|D3D12 WARNING', text))
    if not passed:
        print(text[-6000:], file=sys.stderr)
        print(f'Renamed-Idle native regression failed; inspect {log}', file=sys.stderr)
        return 1
    print(f'Renamed Idle: {args.renderer} passed; completion, replay and state isolation verified; {log}')
    return 0


if __name__ == '__main__':
    try:
        sys.exit(main())
    except (OSError, ValueError, subprocess.TimeoutExpired, ET.ParseError) as error:
        print(str(error), file=sys.stderr)
        sys.exit(1)
