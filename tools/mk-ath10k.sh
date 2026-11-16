#!/bin/bash
# Build Linux's ath10k (for WCN3990 SNOC on Blair / Moto G85) against FreeBSD LinuxKPI + wifi/lx.
set -e
VERSION=7.0.14
ROOT=$(cd "$(dirname "$(readlink -f "$0")")/.." && pwd)
B=$ROOT/xnu-deps/build/ath10k
B11=$ROOT/xnu-deps/build/ath11k
W=$ROOT/wifi/lx
mkdir -p "$B"
cd "$B"

FREEBSD=15.1
FREEBSD_SHA256=cf5762da53fd52e1eaf0f9ceee9bf58cbe314c821031d0d9ffa76823185a89e1
FSYS="compat/linuxkpi sys arm64/include net net80211 netinet netinet6 vm kern tools libkern ddb
	security fs/pseudofs crypto opencrypto geom bsm isa cam rpc xdr x86/include ofed/include
	dev/pci dev/ofw dev/fdt dev/acpica dev/usb dev/iicbus dev/backlight
	contrib/ck/include contrib/dev/acpica/include contrib/libnv"
PATCHES=$(cat "$W"/patches/*.patch 2>/dev/null | sha256sum | cut -d' ' -f1)
if [ "$(cat freebsd/.release 2>/dev/null)" != "$FREEBSD $FREEBSD_SHA256 $PATCHES" ]; then
	TXZ=$B11/freebsd-$FREEBSD-src.txz
	[ -f "$TXZ" ] || { echo "mk-ath10k.sh: missing $TXZ" >&2; exit 1; }
	rm -rf freebsd gen obj
	mkdir freebsd
	tar xJf "$TXZ" -C freebsd --strip-components=2 usr/src/COPYRIGHT \
		$(for d in $FSYS; do echo usr/src/sys/$d; done)
	for p in "$W"/patches/*.patch; do
		[ -f "$p" ] && git -C "$ROOT" apply --directory=xnu-deps/build/ath10k/freebsd "$p"
	done
	echo "$FREEBSD $FREEBSD_SHA256 $PATCHES" > freebsd/.release
fi
S=$B/freebsd/sys

if [ ! -d gen ]; then
	mkdir gen
	(
		cd gen
		ln -s $S/arm64/include machine
		for m in kern/device_if.m kern/bus_if.m dev/pci/pci_if.m dev/pci/pci_iov_if.m dev/pci/pcib_if.m \
			dev/iicbus/iicbus_if.m dev/iicbus/iicbb_if.m dev/backlight/backlight_if.m dev/usb/usb_if.m \
			dev/ofw/ofw_bus_if.m; do
			awk -f $S/tools/makeobjops.awk $S/$m -h
		done
		for o in h p q; do awk -f $S/tools/vnode_if.awk $S/kern/vnode_if.src -$o; done
		for o in $(grep -rhoE "opt_[a-z0-9_]+\.h" $S | sort -u) lkpi_iic_if.h; do : > $o; done
		printf '#define INET 1\n#define INET6 1\n' > opt_inet.h
		printf '#define INET6 1\n' > opt_inet6.h
		printf '#define SMP 1\n' > opt_global.h
	)
fi

DRIVER="core mac ce htc htt htt_rx htt_tx hw txrx wmi wmi-tlv swap snoc"
CC="clang -target arm64-apple-ios16.5 -U__APPLE__"
LD=$ROOT/xnu-deps/host/bin/ld64
CFLAGS="-mkernel -mgeneral-regs-only -fno-builtin -fno-common -ffreestanding -fno-strict-aliasing
	-funsigned-char -nostdinc -std=gnu17 -O2 -g -w
	-D__FreeBSD__=15 -D_KERNEL -DKLD_MODULE -DHAVE_KERNEL_OPTION_HEADERS -include opt_global.h
	-I$W/include -Igen -I$S -I$S/contrib/ck/include
	-I$S/compat/linuxkpi/common/include -I$S/compat/linuxkpi/dummy/include
	-include $S/compat/linuxkpi/common/include/linux/kconfig.h -DLINUXKPI_VERSION=70000"
DFLAGS="-Isrc/ath10k -Isrc -include $W/include/lx_prelude.h -DCONFIG_ATH10K_SNOC -DCONFIG_ATH10K_DEBUG
	-DKBUILD_MODNAME=\"ath10k\""

source "$ROOT/xnu-env.sh"
for h in lx qmi; do
	[ -f "$XNU_SRC/bsd/sys/hakios.$h.h" ] || { echo "mk-ath10k.sh: no bsd/sys/hakios.$h.h" >&2; exit 1; }
done
DFLAGS="$DFLAGS -DLX_HAKIOS_LX_H=\"$XNU_SRC/bsd/sys/hakios.lx.h\" -DLX_HAKIOS_QMI_H=\"$XNU_SRC/bsd/sys/hakios.qmi.h\""

mkdir -p obj out
rm -f obj/*.err
compile() {
	local src=$1 obj=$2
	shift 2
	if [ -f "$obj" ] && [ -f "$obj.d" ] && [ -z "$(find $(sed -e 's/^[^:]*://' -e 's/\\$//' "$obj.d") "$0" -newer "$obj" 2>/dev/null | head -1)" ]; then
		return
	fi
	$CC $CFLAGS "$@" -MMD -MF "$obj.d" -c "$src" -o "$obj" 2> "$obj.err" || rm -f "$obj"
}

for f in $DRIVER; do compile src/ath10k/$f.c obj/ath10k_$f.o $DFLAGS & done
for f in lx_base lx_80211 lx_wpa lx_bus_ath10k; do
	[ -f "$W/src/$f.c" ] && compile "$W/src/$f.c" obj/$f.o $DFLAGS -I$W/src &
done
wait
bad=
for f in obj/*.err; do
	[ -s "$f" ] && [ ! -f "${f%.err}" ] && { echo "--- $(basename "$f" .o.err)"; grep -m6 -A3 "error:" "$f"; bad=1; }
done
[ -z "$bad" ] || exit 1

OBJECTS=$(for f in $DRIVER; do echo obj/ath10k_$f.o; done)
for f in lx_base lx_80211 lx_wpa lx_bus_ath10k; do
	[ -f "obj/$f.o" ] && OBJECTS="$OBJECTS obj/$f.o"
done

KERNEL="memcmp memcpy memmove memset strcmp strncmp strlen strchr strrchr strlcpy strlcat bzero
	snprintf vsnprintf __stack_chk_fail __stack_chk_guard"
$LD -r -arch arm64 $OBJECTS -o out/all.o 2> out/ld.err || { cat out/ld.err; exit 1; }
llvm-nm -u --format=just-symbols out/all.o | sed 's/^_//' | sort > out/undefined.txt
: > out/stubs.txt
: > out/stubs.s
missing=
for s in $(cat out/undefined.txt); do
	case " $(echo $KERNEL) " in *" $s "*) continue ;; esac
	case $s in
	lx_os_*|qmi_*|hakios_qmi_*|section\$*) continue ;;
	ath10k_*|__ath10k_*) ;;
	*) echo "mk-ath10k.sh: nothing defines $s" >&2; missing=1; continue ;;
	esac
	echo $s >> out/stubs.txt
	printf '\t.globl _%s\n_%s:\n\tadrp x0, 1f@PAGE\n\tadd x0, x0, 1f@PAGEOFF\n\tb _lx_stub\n\t.cstring\n1:\t.asciz "%s"\n\t.text\n\t.p2align 2\n' \
		$s $s $s >> out/stubs.s
done
[ -z "$missing" ] || exit 1
$CC -c out/stubs.s -o out/stubs.o
printf '_lx_prepare\n_lx_start\n_lx_stop\n_lx_transmit\n' > out/exported.txt
$LD -r -arch arm64 -exported_symbols_list out/exported.txt out/all.o out/stubs.o -o out/ath10k.o 2> out/ld.err ||
	{ cat out/ld.err; exit 1; }
rm -f out/all.o
echo "mk-ath10k.sh: out/ath10k.o: $(llvm-size out/ath10k.o | awk 'NR == 2 { print $1 + $2 }') bytes of code and data"
