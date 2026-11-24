# Motorola Moto G85 5G (`malmo`)

| | |
| --- | --- |
| Device | Motorola Moto G85 5G |
| SoC | Qualcomm Snapdragon 6s Gen 3 / 695 (`sm6375`, Blair); `soc.dtsi` |
| State | Migrated device package to the new `upstream/common` (`xnu-12377.121.6`) layout. Previous iOS 16.5 port (`~97%` working: Adreno 619, WCN3990 Wi-Fi, FS1815, AW86927, FT3683G/GT9916) is preserved on branch `backup/pre-common-2026-11-24`. |

Nothing in the procedure writes to the phone's storage. It is started with `fastboot boot` and runs
from RAM; a reboot brings Android back. It needs an unlocked bootloader.

## State

| What | Feature / boot argument | State |
| --- | --- | --- |
| Device package (`board.dts`, `soc.dtsi`) | — | Validated with `tools/boards.py check` |
| Buttons | `buttons-pmic` / `buttons=1` | Configured (`pon = 0x208`, `volume-up = 0x488`, PM6125/PMR735A) |
| Battery (UEFI charger) | `battery-uefi` / `battery=1` | Configured (`loader { uefi-charger; }`) |
| CPU frequency | `cpufreq-epss` / `cpufreq=1` | Configured (`epss = 0x0fd91000`, `domain-cores = <6 2>`) |
| Brightness | `backlight { type = "soft"; }` / `backlight=1` | Configured (software dimming; DCS needs `dispcc` offsets) |
| Haptics | `haptic-aw86927` / `haptic=5` | Configured (I2C SE8 GPIO 19/20, reset 89, irq 88, `f0 = 0x2c`) |
| Touch (FT3683G / GT9916), GPU (A619), Wi-Fi (WCN3990), ADSP/FS1815, MPSS | — | Pending port from `backup/pre-common-2026-11-24` to `drivers/board/` |

`tools/boards.py status malmo` lists every feature with what it still needs.

## Where the values come from

- `board.dts` and `soc.dtsi`: generated via `tools/boards.py new malmo --soc sm6375 --dts malmo.dts --panel qcom,mdss_dsi_mot_csot_vtdr6115_gt9916_667_1080x2400_dsc_vid`.
- `uefi { }` (`0x9FC00000`, size `0x300000`, Android header v4): from Mu-Silicium `malmo.toml`.
- `loader { fb-base = <0x85200000>; }` and `platform { io-base = <0x00200000>; io-size = <0x3fe00000>; }`: from the Moto G85 port (`backup/pre-common-2026-11-24`).
- `haptic { }`: from the Moto G85 Android device tree (`qupv3_se8_i2c`, AW86927).

## What a boot needs besides this package

- Mu-Silicium UEFI port for `malmo` (`SILICIUM_UEFI.fd`, `BootShim.bin`).
- Firmware files pulled from the phone (`/vendor/firmware*`) for DSP/GPU/Wi-Fi/haptics when those drivers are enabled.
- Darwin 26/27 system and root APFS images built from Apple IPSW (see `Documentation/darwin27-ubuntu.ru.md`).
