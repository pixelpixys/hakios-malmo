# HakiOS for Motorola Moto G85 5G (`malmo` / Qualcomm SM6375 Blair)

> **Порт Apple Darwin XNU (xnu-8796.121.2) и iOS 16.5 SpringBoard на смартфон Motorola Moto G85 5G**  
> Репозиторий: [github.com/pixelpixys/hakios-malmo](https://github.com/pixelpixys/hakios-malmo)

---

## 📊 Общий прогресс портирования: **~78%**

Интерфейс SpringBoard (iOS 16.5) полностью загружается в оперативную память (Live RAM) без изменения разделов Android. Перезагрузка возвращает телефон в исходное состояние.

---

## ⚙️ Статус компонентов (Hardware Support)

| Компонент / Подсистема | Статус | Процент готовности | Примечания |
|---|:---:|:---:|---|
| **CPU / Multicore** | ✅ Работает | 100% | 8 ядер Kryo (2x Gold + 6x Silver), `bootcpu=7 cpus=7`, SMP |
| **Память / RAM & APFS** | ✅ Работает | 100% | Live APFS ramdisk (`root.apfs`, `system.apfs`), сжатие lz4 |
| **Дисплей (Framebuffer)** | ✅ Работает | 100% | 1080x2400 OLED, плавная развёртка 120 Гц (`fb_hz=120`, `ca_vbl=1`) |
| **GPU (Adreno 619)** | ✅ Работает | 90% | Аппаратное ускорение через `hakios-gl` / Mesa Freedreno, Metal-эмуляция |
| **Сенсорный экран (Touch)** | ✅ Работает | 95% | FocalTech FT3683G (I2C SE1), мультитач, жесты навигации |
| **Кнопки управления** | ✅ Работает | 100% | Питание (Power) и Громкость (+/-) через драйвер PMIC PON |
| **Хаптика / Виброотклик** | ✅ Работает | 85% | Awinic AW86927 (I2C SE8), тактильная отдача при нажатиях |
| **Звук / Динамики (Audio)** | 🟡 В процессе | 65% | Драйвер FourSemi FS1815 написан; запуск ADSP адаптирован под SPMI/RPM-SMD |
| **Wi-Fi** | 🟡 В процессе | 50% | WCN3990: реализован WLAN_EN (GPIO84), MSA QMI (0x86500000), инициализация ICNSS |
| **Bluetooth** | 🟡 В процессе | 45% | Поддержка WCN3990 через UART / QCA coex (GPIO 69), драйвер в доработке |
| **Модем / LTE (Сотовая сеть)** | 🔴 В разработке | 25% | Выгружены таблицы MPSS, изоляция каналов QMI |
| **Камеры** | 🔴 В разработке | 10% | Требуется портирование ISP Spectra и сенсоров Sony LYT-600 |

---

## 🚀 Запуск и аргументы ядра

Запуск системы осуществляется через кастомный загрузчик **HakiOS XNULoader v3** (`loader.efi`) поверх **Mu-Silicium UEFI**:

```bash
XL_SECS=36000 XL_FOLLOW=10 XL_CMDLINE="touchscreen=1 buttons=1 bootcpu=7 cpus=7 gpu=1 fb_hz=120 fb_threads=6 ca_vbl=1" ./xl run
```

### Основные аргументы `XL_CMDLINE`:
- `gpu=1`: включение драйвера Adreno 619 и аппаратного пайплайна отрисовки.
- `fb_hz=120`: форсирование частоты обновления 120 Гц для плавного скроллинга.
- `touchscreen=1`: опрос сенсорного экрана FocalTech (I2C SE1, GPIO 0/1).
- `buttons=1`: поддержка аппаратных кнопок.
- `adsp=1 sound=4`: включение аудиопроцессора Hexagon и стереоусилителей FS1815.
- `wifi=1`: запуск стека Wi-Fi WCN3990 через ICNSS.

---

## 🛠️ Особенности архитектуры порта под Moto G85 (`malmo`)

1. **SoC Snapdragon 695 / 6s Gen 3 (SM6375, Blair):**
   - В отличие от Snapdragon 778G (SC7280/redwood), где используется RPMh RSC, в SM6375 питание управляется через **RPM-SMD** и SPMI.
   - Драйверы ADSP (`drivers/audio/hakios.adsp.c`) и Wi-Fi (`drivers/wifi/hakios.wifi.c`) переписаны с использованием прямого SPMI-мониторинга шин питания (`ldoa1`, `ldoa17`).
2. **Аудиоусилители FourSemi FS1815:**
   - Кастомный драйвер `drivers/audio/hakios.speaker.c` и заголовочный файл `drivers/audio/hakios.fs1815.h` с прошивкой `fs1815.fsm`.
   - Поддержка I2C адресов `0x34` / `0x35` с аппаратными пинами сброса GPIO 48 / GPIO 59.
3. **Радиочип WCN3990:**
   - Реализовано переключение питания `cnss_wlan_en` через GPIO84 и подготовка буферов MSA/QMI для загрузки прошивки `wlanmdsp.mbn`.

---

## 📄 Лицензии и копирайты

- Ядро XNU: [Apple Public Source License 2.0 (APSL)](LICENSE.APSL)
- Драйверы и тулчейн HakiOS: [MIT License](LICENSE)
- Оригинальный проект: [oosexgang/hakios-xnu-kernel](https://github.com/oosexgang/hakios-xnu-kernel)
