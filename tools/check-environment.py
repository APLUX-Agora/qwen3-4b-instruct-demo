#!/usr/bin/env python3
"""Check a device before following the README. This script does not install packages."""
import argparse
import os
import platform
import re
import shutil
import sys
from pathlib import Path

MODEL = 'qwen'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--soc', default=os.environ.get('AGORA_SOC'), required=not bool(os.environ.get('AGORA_SOC')))
    parser.add_argument('--ubuntu', required=True)
    args = parser.parse_args()
    if not os.environ.get('AGORA_WORKSPACE'):
        print('[FAILED] Workspace | Expected: set AGORA_WORKSPACE | Actual: missing')
        print('STOP: Set the workspace in step 1, then rerun this check.')
        return 1
    workspace = Path(os.environ['AGORA_WORKSPACE']).expanduser()
    try:
        workspace.mkdir(parents=True, exist_ok=True)
    except OSError as error:
        print(f'[FAILED] Workspace | Expected: writable directory | Actual: {error}')
        print('STOP: Fix the workspace path or permissions, then rerun this check.')
        return 1
    results = []
    def check(name, expected, actual, ok, pending=False):
        status = 'CHECKED' if ok else 'NEEDS_SETUP' if pending else 'FAILED'
        results.append(status)
        print(f'[{status}] {name} | Expected: {expected} | Actual: {actual}')
    check('Architecture', 'aarch64', platform.machine(), platform.machine() == 'aarch64')
    release = dict(re.findall(r'^([A-Z_]+)=["\']?([^"\'\n]*)', Path('/etc/os-release').read_text(), re.M))
    check('System', 'Ubuntu ' + args.ubuntu, release.get('PRETTY_NAME', 'Unknown'), release.get('ID') == 'ubuntu' and release.get('VERSION_ID') == args.ubuntu)
    fingerprint = ''
    for name in ['/sys/devices/soc0/machine', '/proc/device-tree/compatible']:
        try: fingerprint += Path(name).read_text().replace('\x00', ' ').lower()
        except OSError: pass
    aliases = {'QCS6490':['qcs6490'], 'QCS8550':['qcs8550','kalamap'], 'QCS8625':['qcs8625'], 'IQ8':['iq8','qcs8275'], 'IQ9':['iq9','qcs9075','qcs9100']}
    check('SoC', args.soc, fingerprint.strip() or 'Unknown', any(a in fingerprint for a in aliases.get(args.soc, [])))
    free = shutil.disk_usage(workspace).free / 1024**3
    minimum = 8 if MODEL == 'qwen' else 1
    check('Workspace', f'writable, at least {minimum} GiB free', str(workspace) + f' ({free:.1f} GiB free)', os.access(workspace, os.W_OK) and free >= minimum)
    check('Python', '3.10 or newer', platform.python_version(), sys.version_info >= (3,10), True)
    tools = ['git', 'cmake', 'g++'] if MODEL == 'qwen' else ['git']
    for tool in tools: check(tool, 'installed', shutil.which(tool) or 'missing', bool(shutil.which(tool)), True)
    runtime = 'aidgen' if MODEL == 'qwen' else 'aidlite'
    backend = '248' if MODEL == 'qwen' or args.soc == 'QCS8550' else '236'
    for lib in [f'lib{runtime}.so', f'lib{runtime}_qnn{backend}.so']:
        exists = (Path('/usr/local/lib') / lib).is_file()
        check(lib, 'installed by aid-pkg', 'found' if exists else 'missing', exists, True)
    print(f'\nChecks complete: {results.count("FAILED")} failed, {results.count("NEEDS_SETUP")} need setup.')
    if 'FAILED' in results:
        print('STOP: Do not continue. For SoC/system mismatches, return to hardware selection; fix workspace/disk issues on the device. Rerun this check.')
        return 1
    if 'NEEDS_SETUP' in results:
        print('NEXT: Install the missing dependencies in step 3, then rerun this check before downloading or running the model.')
        return 2
    print('READY: Environment checks passed. Continue to the next step.')
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
