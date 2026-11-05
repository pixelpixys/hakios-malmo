#!/usr/bin/env python3
"""Fetch one member of the iOS 16.5 iPhone X firmware archive without downloading the rest.

usage: fetch-from-ipsw.py MEMBER OUTPUT

The IPSW is a ZIP64 archive on Apple's CDN; its directory is read with HTTP range requests and
the member is streamed (and inflated, if it is compressed) straight to OUTPUT.
"""
import concurrent.futures
import http.client
import os
import struct
import sys
import threading
import time
import urllib.request
import zlib

ATTEMPTS = 20

URL = ('https://updates.cdn-apple.com/2023SpringFCS/fullrestores/032-85130/6AB0D10A-8AF9-4F6C-8CF5-A1B315CABE88/'
       'iPhone10,3,iPhone10,6_16.5_20F66_Restore.ipsw')
SIZE = 6165698198


def fetch(start, end):
    request = urllib.request.Request(URL, headers={'Range': f'bytes={start}-{end}'})
    return urllib.request.urlopen(request, timeout=120)


def directory():
    tail = fetch(SIZE - 65536, SIZE - 1).read()
    at = tail.rfind(b'PK\x06\x06')
    cd_size, cd_offset = struct.unpack('<QQ', tail[at + 40:at + 56])
    cd = fetch(cd_offset, cd_offset + cd_size - 1).read()
    p = 0
    while cd[p:p + 4] == b'PK\x01\x02':
        method, = struct.unpack('<H', cd[p + 10:p + 12])
        csize, usize, nl, el, cl = struct.unpack('<IIHHH', cd[p + 20:p + 34])
        offset, = struct.unpack('<I', cd[p + 42:p + 46])
        name = cd[p + 46:p + 46 + nl].decode()
        extra = cd[p + 46 + nl:p + 46 + nl + el]
        q = 0
        while q + 4 <= len(extra):
            tag, length = struct.unpack('<HH', extra[q:q + 4])
            if tag == 1:
                values = list(struct.unpack('<' + 'Q' * (length // 8), extra[q + 4:q + 4 + length // 8 * 8]))
                if usize == 0xFFFFFFFF:
                    usize = values.pop(0)
                if csize == 0xFFFFFFFF:
                    csize = values.pop(0)
                if offset == 0xFFFFFFFF:
                    offset = values.pop(0)
            q += 4 + length
        yield name, method, csize, usize, offset
        p += 46 + nl + el + cl


def download(data, method, csize, output):
    """Download member whose bytes start at DATA in the archive; returns size of OUTPUT."""
    if method == 0:
        chunk_size = 16 * 1024 * 1024
        workers = 8
        fd = os.open(output, os.O_RDWR | os.O_CREAT, 0o644)
        try:
            os.ftruncate(fd, csize)
            total_chunks = (csize + chunk_size - 1) // chunk_size
            downloaded = 0
            lock = threading.Lock()
            last_print = time.time()
            start_time = time.time()

            def fetch_chunk(i):
                nonlocal downloaded, last_print
                chunk_start = i * chunk_size
                chunk_end = min(csize, (i + 1) * chunk_size) - 1
                for attempt in range(5):
                    try:
                        with fetch(data + chunk_start, data + chunk_end) as stream:
                            content = stream.read()
                            if len(content) != (chunk_end - chunk_start + 1):
                                raise IOError(f"short read: {len(content)} vs {chunk_end - chunk_start + 1}")
                            os.pwrite(fd, content, chunk_start)
                            with lock:
                                downloaded += len(content)
                                now = time.time()
                                if now - last_print >= 1.5 or downloaded == csize:
                                    pct = (downloaded / csize) * 100
                                    mb_done = downloaded / (1024 * 1024)
                                    mb_total = csize / (1024 * 1024)
                                    speed = (downloaded / max(0.1, now - start_time)) / (1024 * 1024)
                                    print(f"\r  [{pct:5.1f}%] {mb_done:.1f} MB / {mb_total:.1f} MB ({speed:.2f} MB/s)", end="", flush=True)
                                    last_print = now
                            return
                    except Exception as e:
                        if attempt == 4:
                            raise e
                        time.sleep(2)

            with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
                list(executor.map(fetch_chunk, range(total_chunks)))
            print()
        finally:
            os.close(fd)
        return os.path.getsize(output)

    # method 8: deflated stream
    inflater = zlib.decompressobj(-15)
    written = 0
    read_bytes = 0
    start_time = time.time()
    last_print = time.time()
    with fetch(data, data + csize - 1) as stream, open(output, 'wb') as out:
        while chunk := stream.read(1 << 20):
            read_bytes += len(chunk)
            decomp = inflater.decompress(chunk)
            out.write(decomp)
            written += len(decomp)
            now = time.time()
            if now - last_print >= 1.5 or read_bytes == csize:
                pct = (read_bytes / csize) * 100
                mb_read = read_bytes / (1024 * 1024)
                mb_total = csize / (1024 * 1024)
                speed = (read_bytes / max(0.1, now - start_time)) / (1024 * 1024)
                print(f"\r  [{pct:5.1f}%] {mb_read:.1f} MB / {mb_total:.1f} MB compressed ({speed:.2f} MB/s)", end="", flush=True)
                last_print = now
        tail = inflater.flush()
        out.write(tail)
        written += len(tail)
        print()
    return written


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    member, output = sys.argv[1], sys.argv[2]
    for name, method, csize, usize, offset in directory():
        if name != member:
            continue
        if method not in (0, 8):
            sys.exit(f'{member}: compression method {method} is not supported')
        header = fetch(offset, offset + 29).read()
        nl, el = struct.unpack('<HH', header[26:30])
        data = offset + 30 + nl + el
        written = -1
        for attempt in range(1, ATTEMPTS + 1):
            try:
                written = download(data, method, csize, output)
            except (OSError, http.client.HTTPException, zlib.error) as error:
                print(f'\n{member}: {error}', file=sys.stderr)
            if written == usize:
                break
            print(f'\n{member}: the download broke off, trying again ({attempt} of {ATTEMPTS})', file=sys.stderr)
            time.sleep(5)
        print(f'{member}: {written} bytes (expected {usize})')
        sys.exit(0 if written == usize else 1)
    sys.exit(f'{member}: not in the archive')


if __name__ == '__main__':
    main()
