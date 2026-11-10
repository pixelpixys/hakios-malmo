# HakiOS for Motorola Moto G85 5G (`malmo` / Qualcomm SM6375 Blair)

> **Port of Apple's open-source Darwin XNU kernel (xnu-8796.121.2) and iOS 16.5 SpringBoard GUI to the Motorola Moto G85 5G**  
> Repository: [github.com/pixelpixys/hakios-malmo](https://github.com/pixelpixys/hakios-malmo)

---

## 📊 Overall Porting Progress: **~78% Complete**

The iOS 16.5 SpringBoard user interface boots directly into live RAM without modifying your device's Android partitions. A simple reboot returns the phone completely back to stock Android.

---

## ⚙️ Hardware Support & Status

| Component / Subsystem | Status | Progress | Notes |
|---|:---:|:---:|---|
| **CPU / Multicore** | ✅ Working | 100% | 8 Kryo cores (2x Gold + 6x Silver), `bootcpu=7 cpus=7`, full SMP |
| **Storage / APFS Ramdisk** | ✅ Working | 100% | Live APFS ramdisk (`root.apfs`, `system.apfs`), lz4 compressed |
| **Display (Framebuffer)** | ✅ Working | 100% | 1080x2400 OLED, smooth 120 Hz refresh rate (`fb_hz=120`, `ca_vbl=1`) |
| **GPU (Adreno 619)** | ✅ Working | 90% | Hardware-accelerated UI via `hakios-gl` / Mesa Freedreno, Metal translation |
| **Touchscreen (Touch)** | ✅ Working | 95% | FocalTech FT3683G (I2C SE1), multi-touch & gesture navigation |
| **Physical Buttons** | ✅ Working | 100% | Power and Volume (+/-) via PMIC PON driver |
| **Haptics / Vibration** | ✅ Working | 85% | Awinic AW86927 (I2C SE8), tactile feedback on taps |
| **Audio / Speakers** | 🟡 In Progress | 65% | FourSemi FS1815 driver written; ADSP adapted for SPMI / RPM-SMD |
| **Wi-Fi** | 🟡 In Progress | 50% | WCN3990: WLAN_EN (GPIO 84) assertion, MSA QMI (0x86500000), ICNSS init |
| **Bluetooth** | 🟡 In Progress | 45% | WCN3990 UART / QCA coex (GPIO 69), driver under active development |
| **Cellular Modem (LTE/5G)** | 🔴 Under Development | 25% | MPSS firmware mapped, QMI channel isolation |
| **Cameras** | 🔴 Planned | 10% | Requires porting Qualcomm Spectra ISP & Sony LYT-600 sensors |

---

## 🚀 Boot Command & Kernel Arguments

The system is booted via the custom **HakiOS XNULoader v3** (`loader.efi`) chained from **Mu-Silicium UEFI**:

```bash
XL_SECS=36000 XL_FOLLOW=10 XL_CMDLINE="touchscreen=1 buttons=1 bootcpu=7 cpus=7 gpu=1 fb_hz=120 fb_threads=6 ca_vbl=1" ./xl run
```

### Key Kernel Arguments (`XL_CMDLINE`):
- `gpu=1`: Enables the Adreno 619 GPU driver and hardware rendering pipeline.
- `fb_hz=120`: Forces 120 Hz display refresh rate for fluid scrolling and animations.
- `touchscreen=1`: Enables the FocalTech FT3683G touch controller (I2C SE1, GPIO 0/1).
- `buttons=1`: Enables power and volume key monitoring.
- `adsp=1 sound=4`: Boots the Hexagon Audio DSP and routes sound through FourSemi FS1815 stereo amplifiers.
- `wifi=1`: Starts the WCN3990 Wi-Fi radio stack via ICNSS.

---

## 🛠️ Port Architectural Highlights (`malmo` / SM6375)

1. **Snapdragon 695 / 6s Gen 3 (SM6375, Blair Platform):**
   - Unlike Snapdragon 778G (SC7280/redwood) which uses RPMh RSC, SM6375 relies on **RPM-SMD** and SPMI power distribution.
   - The ADSP driver (`drivers/audio/hakios.adsp.c`) and Wi-Fi driver (`drivers/wifi/hakios.wifi.c`) were refactored to perform direct SPMI register reads for LPASS rails (`ldoa1`, `ldoa17`).
2. **FourSemi FS1815 Smart Audio Amplifiers:**
   - Dedicated driver in `drivers/audio/hakios.speaker.c` with register sequences from `drivers/audio/hakios.fs1815.h` (extracted from `/vendor/firmware/fs1815.fsm`).
   - Dual-channel I2C addressing (`0x34` right / `0x35` left) with hardware shutdown pins on GPIO 48 and GPIO 59.
3. **WCN3990 Connectivity Radio:**
   - Hardware power-on via `cnss_wlan_en` (GPIO 84) output drive, and MSA memory window preparation for firmware load (`wlanmdsp.mbn`).

---

## 📄 License & Credits

- XNU Kernel: [Apple Public Source License 2.0 (APSL)](LICENSE.APSL)
- HakiOS drivers and userland toolchain: [MIT License](LICENSE)
- Upstream project: [oosexgang/hakios-xnu-kernel](https://github.com/oosexgang/hakios-xnu-kernel)
