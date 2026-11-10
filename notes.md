# Журнал адаптации и портирования HakiOS (Darwin 22.5.0 / iOS 16.5) на Motorola Moto G85 5G (malmo / SM6375 Blair)

## 1. Общие сведения
- **Устройство:** Motorola Moto G85 5G (`malmo`)
- **Платформа / SoC:** Qualcomm SM6375 (Blair / Bengal-family)
- **Операционная система:** Darwin 22.5.0 (Apple XNU 8796.121.2) + iOS 16.5 Userland
- **Загрузчик:** Project Mu-Silicium UEFI (`Mu-malmo.img`) + HakiOS XNULoader v3 (`loader.efi` / `xnu-usb.img`)
- **Состояние интерфейса:** Полностью запущен SpringBoard, аппаратно ускоренный рендеринг GPU (OpenGL на Adreno 619), 120 Гц дисплей (`fb_hz=120`), сенсорный экран (`touchscreen=1`), физические кнопки питания и громкости.

---

## 2. Проделанная работа и ключевые изменения в драйверах

### Аудио подсистема (FS1815 Smart Amplifiers & ADSP)
1. **Драйвер динамиков (`drivers/audio/hakios.speaker.c` и `drivers/audio/hakios.fs1815.h`):**
   - Redwood использовал усилители Awinic AW88261 на шине SE3.
   - Для Moto G85 драйвер полностью переписан под **FourSemi FS1815** (chip ID `0x1220`):
     - Шина I2C: `qupv3_se10_i2c` (GPIO 4 SDA, GPIO 5 SCL).
     - Адреса усилителей: `0x35` (левый канал) и `0x34` (правый канал).
     - Линии аппаратного сброса/shutdown (SDZ): `GPIO 48` для 0x35 и `GPIO 59` для 0x34.
     - Интегрированы полные таблицы инициализации регистров и сценариев воспроизведения музыки (извлечены из оригинальной прошивки `/vendor/firmware/fs1815.fsm`).
     - Реализованы функции `hakios_speaker_prepare`, `hakios_speaker_start`, `hakios_speaker_volume` и `hakios_speaker_stop`.

2. **Драйвер DSP звука (`drivers/audio/hakios.adsp.c`):**
   - Устранено падение ядра при `adsp=1`: на SoC SM6375 (Blair) отсутствует шина RPMh RSC и Command DB (вместо них на Blair работает RPM-SMD через регуляторы PMIC `ldoa1` и `ldoa17`). Для платформы `HAKIOS_SOC_BASE_BLAIR` вызовы RPMh `rails()` отключены.
   - Выгружена полная оригинальная прошивка DSP (`adsp.mdt` и все сегменты `adsp.b00`–`adsp.b37`, всего 38 файлов, ~16 МБ) с устройства в `fw-audio/`.

### Хаптика / Вибромотор (Awinic AW86927)
1. **Драйвер хаптики (`drivers/haptic/hakios.haptic.c`):**
   - Добавлена поддержка контроллера Awinic **AW86927** (chip ID `0x9270`).
   - Настройка питания через SPMI PMIC PM7250B: LDO каналов 390 (`0x5300`) переведен на режим 3.0В (`0x87 VREG_READY`).
   - Настройка шины `qupv3_se8_i2c`: GPIO 19 (SDA), GPIO 20 (SCL), пин аппаратного сброса GPIO 89 и прерывания GPIO 88.
   - Загрузка прошивки RAM для вибромотора (`fw-haptic/haptic_ram.bin`).

### Сеть и Модем (MPSS & Wi-Fi)
1. **Драйвер модема (`drivers/modem/hakios.mpss.c`):**
   - Адаптированы адреса регистров и диапазоны памяти для SM6375.
   - Настроена изоляция каналов QMI и поддержка shared памяти.
2. **Драйвер Wi-Fi (`drivers/wifi/hakios.wifi.c`):**
   - Добавлена конфигурация контекста SMMU 4 для отображения области MSA (`0x86500000`, 2 МБ) и IOVA `0xb0000000` (`pil_mpss_wlan_mem` `0x8b800000`, 8 МБ) для `wlan_pd`.

### Сборка и Userland
1. **Скрипт сборки системы (`userland/build.sh`):**
   - Добавлена упаковка прошивки `fw-audio/` (`adsp.*`) в `root.apfs` и `system.apfs` при флагах `ADSP=1` и `SYSTEM=1`.
   - Поддержка совместной сборки `GL=1` (аппаратный OpenGL Mesa) и `ADSP=1`.
2. **Ядро (`tools/make-kernel.sh`):**
   - Собрано и синхронизировано обновленное ядро `prebuilt/mach.development.bcm2837` с поддержкой всех периферийных блоков Moto G85.

---

## 3. Команды запуска и параметры

### Запуск интерфейса iOS со всеми сервисами, GPU и сенсорным экраном:
```bash
XL_SECS=36000 XL_FOLLOW=10 XL_CMDLINE="touchscreen=1 buttons=1 bootcpu=7 cpus=7 gpu=1 fb_hz=120 fb_threads=6 ca_vbl=1" ./xl run
```

### Параметры командной строки ядра (`XL_CMDLINE`):
- `touchscreen=1`: активация драйвера сенсорного экрана FocalTech FT3683G (I2C SE1, GPIO 0/1).
- `buttons=1`: опрос кнопок питания и громкости через PMIC PON.
- `bootcpu=7 cpus=7`: конфигурация многоядерности.
- `gpu=1`: запуск Adreno GPU и аппаратно ускоренного конвейера рендеринга.
- `adsp=1 sound=4`: активация аудиопроцессора и маршрутизация системного звука Darwin на стереодинамики FourSemi FS1815.
- `fb_hz=120 fb_threads=6 ca_vbl=1`: 120 Гц развертка и синхронизация кадров CoreAnimation.
- `XL_SECS=0` или `XL_SECS=36000`: продолжительность сеанса без автоматического сброса.
