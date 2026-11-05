#!/usr/bin/env python3
"""
Pack UEFI (with the hook that starts the XNU loader) and an optional payload
into an Android boot image for `fastboot boot`.

  kernel = gzip(BootShim + UEFI FD + ["XNUP" container]) + DTB

The boot shim copies the FD to its base and the payload to PAYLOAD_BASE.

The UEFI pieces are taken from a Mu-Silicium tree built here or from
devices/<device>/prebuilt/.

usage: mkxnuimg.py [--device moto-g85|poco-x5-pro] [-p payload.bin] [-o out.img]
"""
import argparse, gzip, os, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
MU = os.path.join(HERE, '..', 'Mu-Silicium') if os.path.isdir(os.path.join(HERE, '..', 'Mu-Silicium')) else os.path.join(HERE, 'Mu-Silicium')

DEVICES = {
    'moto-g85': {
        'pkg': 'malmo',
        'fd_base': 0x9FC00000,
        'fd_size': 0x300000,
        'header_version': 4,
        'dir': 'moto-g85',
        'default_out': 'xnu-malmo.img',
    },
    'poco-x5-pro': {
        'pkg': 'redwood',
        'fd_base': 0x9FD00000,
        'fd_size': 0x200000,
        'header_version': 1,
        'dir': 'poco-x5-pro',
        'default_out': 'xnu-redwood.img',
    },
    'redwood': {
        'pkg': 'redwood',
        'fd_base': 0x9FD00000,
        'fd_size': 0x200000,
        'header_version': 1,
        'dir': 'poco-x5-pro',
        'default_out': 'xnu-redwood.img',
    }
}

PAYLOAD_BASE, PAYLOAD_MAX = 0xF0000000, 0x04000000

ap = argparse.ArgumentParser()
ap.add_argument('--device', '-d', default=os.environ.get('DEVICE', 'moto-g85'))
ap.add_argument('-p', '--payload')
ap.add_argument('-o', '--output')
ap.add_argument('--mode', default='RELEASE')
args = ap.parse_args()

dev_cfg = DEVICES.get(args.device)
if not dev_cfg:
    sys.exit(f"Unknown device: {args.device}. Supported: {', '.join(DEVICES.keys())}")

FD_BASE = dev_cfg['fd_base']
FD_SIZE = dev_cfg['fd_size']
HEADER_VERSION = dev_cfg['header_version']
PREBUILT = os.path.join(HERE, 'devices', dev_cfg['dir'], 'prebuilt')
OUT_FILE = args.output or os.path.join(HERE, dev_cfg['default_out'])

fd_path = os.path.join(MU, f"Build/{dev_cfg['pkg']}Pkg/{args.mode}_CLANGPDB/FV/SILICIUM_UEFI.fd")
if os.path.isfile(fd_path):
    shim_dir = os.path.join(MU, 'BootShim')
    dtb_path = os.path.join(MU, f"Resources/DTBs/{dev_cfg['pkg']}.dtb")
    parts = {
        'shim': os.path.join(shim_dir, 'BootShim.bin'),
        'fd': fd_path,
        'dtb': dtb_path if os.path.isfile(dtb_path) else os.path.join(PREBUILT, 'device-tree.dtb'),
        'ramdisk': os.path.join(MU, 'Resources/ramdisk'),
        'mkbootimg': os.path.join(MU, 'Resources/Scripts/mkbootimg.py')
    }
else:
    parts = {
        'shim': os.path.join(PREBUILT, 'BootShim.bin'),
        'fd': os.path.join(PREBUILT, 'SILICIUM_UEFI.fd'),
        'dtb': os.path.join(PREBUILT, 'device-tree.dtb'),
        'ramdisk': os.path.join(PREBUILT, 'ramdisk'),
        'mkbootimg': os.path.join(PREBUILT, 'mkbootimg.py')
    }

shim = open(parts['shim'], 'rb').read()
fd = open(parts['fd'], 'rb').read()
assert len(fd) == FD_SIZE, f"Expected FD size {hex(FD_SIZE)}, got {hex(len(fd))}"

blob = shim + fd
if args.payload:
    data = open(args.payload, 'rb').read()
    # the payload file is a complete "XNUP" container (see xnuloader/mkpayload.py)
    assert data[:4] == b'XNUP' and len(data) <= PAYLOAD_MAX - 0x1000, 'bad or oversized payload'
    blob += data
    blob += b'\0' * (-len(blob) % 16)
    print(f'payload: {len(data)} bytes, bytesum={sum(data) & 0xffffffff:08x}')

dtb = open(parts['dtb'], 'rb').read()
kernel = OUT_FILE + '.kernel~'
open(kernel, 'wb').write(gzip.compress(blob, 6) + dtb)
try:
    cmd = [
        sys.executable, parts['mkbootimg'],
        '--kernel', kernel,
        '--ramdisk', parts['ramdisk'],
        '--header_version', str(HEADER_VERSION),
        '-o', OUT_FILE
    ]
    subprocess.check_call(cmd)
finally:
    if os.path.exists(kernel):
        os.unlink(kernel)
print(f"{OUT_FILE}: {os.path.getsize(OUT_FILE)} bytes (device {args.device}, header_version {HEADER_VERSION}, shim {len(shim)}, uncompressed {len(blob)})")
