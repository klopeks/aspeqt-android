# AspeQt for Android

🇵🇱 **Polski** | [🇬🇧 English](#english)

---

Atari 8-bit SIO peripheral emulator dla Androida (12, 13, 14, 15+).

Zamienia smartfon w stację dysków, magnetofon i drukarkę dla komputerów Atari 8-bit
podłączonych przez kabel SIO2PC-USB.

## Funkcje

- **6 napędów dyskowych** (D1-D6) z obsługą formatów: ATR, XFD, PRO, ATX, DCM, SCP, DI + skompresowane gzip
- **Folder Images** — bezpośrednie udostępnienie folderu z telefonu jako dysku Atari
- **Magnetofon (CAS)**:
  - Standardowy odtwarzacz dla zwykłych obrazów kaset
  - **Drugi silnik z dekoderem FSK** (algorytm inspirowany [liba8cas](https://sourceforge.net/projects/a8cas/))
    pozwala próbować załadować obrazy z fragmentami FSK, których stary AspeQt nie umiał
    > ⚠️ **Uwaga**: ładowanie programów z sektorami FSK **nie jest gwarantowane**.
    > Część plików załaduje się prawidłowo, jednak wszystko zależy od złożoności
    > zakodowanych sektorów. Niektóre obrazy z bardziej skomplikowanym kodowaniem FSK
    > mogą nie wczytać się poprawnie.
- **Autoboot programów Atari** (XEX, COM, EXE)
  - Workflow Retry / Open / Cancel
- **Drukarka tekstowa** (ATASCII / ASCII)
- **PCLINK** — pełny system plików telefonu jako urządzenie SIO
- **8 języków interfejsu**: polski, angielski, niemiecki, hiszpański, francuski, rosyjski, słowacki, turecki

## Wymagania

### Sprzęt do uruchomienia

| Element | Wymaganie |
|---------|-----------|
| **Telefon** | Android 6.0+ (API 23), USB Host/OTG, ekran w trybie pionowym |
| **Kabel** | SIO2PC-USB z chipem FTDI FT232R (np. Lotharek) |
| **Adapter** | USB-C → USB-A OTG (do podłączenia kabla do telefonu) |
| **Atari 8-bit** | dowolny model z portem SIO |

### Środowisko developerskie (budowa APK)

| Komponent | Wersja |
|-----------|--------|
| Qt | 6.11+ for Android (moduły: core, gui, widgets, printsupport, svg, core5compat) |
| Java JDK | 17 |
| Android NDK | dostarczany przez Qt (~26.3) |
| Android SDK | API 34, minSdk 23 |

## Budowanie APK

1. W **Qt Maintenance Tool** zainstaluj:
   - Qt 6.11 → Android
   - NDK i Android SDK (Qt Creator zrobi to po wskazaniu JDK 17)
2. Otwórz `aspeqt.pro` w Qt Creator
3. Wybierz kit **Android arm64-v8a**
4. Build → wygenerowany APK znajdziesz w katalogu build

### Tłumaczenia

Źródła `.ts` są w `i18n/`. Pliki `.qm` są regenerowane automatycznie przy buildzie
(`CONFIG += lrelease` w `.pro`). Aby ręcznie zregenerować:

```
lrelease i18n/aspeqt_pl.ts
```

## Pierwsze uruchomienie

1. Podłącz kabel SIO2PC-USB do telefonu przez adapter OTG
2. Połącz drugi koniec z portem SIO Atari
3. Uruchom AspeQt, w opcjach ustaw:
   - **Interface**: SIO2PC (USB)
   - **Handshake**: CTS (domyślne dla SIO2PC-USB FT232R)
4. Zamontuj obraz dysku w slocie D1
5. Włącz Atari trzymając OPTION (by wyłączyć BASIC)

Pełniejsza polska instrukcja: `AspeQt - Krótka instrukcja obsługi.html` w repo.

## Licencja

GNU General Public License version 2 (or any later version) — patrz `license.txt`.

## Podziękowania

AspeQt został pierwotnie stworzony przez **Ray Ataergin** i rozwijany przez
**greblus** ([github.com/greblus/aspeqt](https://github.com/greblus/aspeqt)).
Ta wersja to niezależna kontynuacja autorstwa **klopeks** (2026),
skupiona na wsparciu nowoczesnego Androida (12+) oraz pełnych tłumaczeniach.

Pełna lista zmian: zobacz [`CHANGES.md`](CHANGES.md).

### Wykorzystane biblioteki / inspiracje
- **usb-serial-for-android** — Mike Wakerly (LGPL 2.1+)
- **SimpleFileDialog** — Hagai Shatz (CPOL 1.02)
- **liba8cas** — Tomasz Krasuski (GPL 3+) — algorytm dekodera FSK jako inspiracja
  (kod nie został skopiowany, własna implementacja w C++/Qt na podstawie opisu algorytmu)
- **Tango Icon Library** (Public Domain)

---
<a name="english"></a>

# AspeQt for Android — English

[🇵🇱 Polski](#aspeqt-for-android) | 🇬🇧 **English**

---

Atari 8-bit SIO peripheral emulator for Android (12, 13, 14, 15+).

Turns your smartphone into a disk drive, cassette player and printer for
Atari 8-bit computers connected via SIO2PC-USB cable.

## Features

- **6 disk drives** (D1-D6) supporting formats: ATR, XFD, PRO, ATX, DCM, SCP, DI + gzip-compressed variants
- **Folder Images** — share a folder from your phone directly as an Atari disk
- **Cassette (CAS)**:
  - Standard playback engine for regular cassette images
  - **Second engine with FSK decoder** (algorithm inspired by [liba8cas](https://sourceforge.net/projects/a8cas/))
    lets you attempt to load images containing FSK chunks that the old AspeQt could not handle
    > ⚠️ **Note**: loading programs with FSK sectors is **not guaranteed**.
    > Some files will load correctly, but it all depends on the complexity of
    > the encoded sectors. Images with more complex FSK encoding may fail to load properly.
- **Atari executable autoboot** (XEX, COM, EXE)
  - Retry / Open / Cancel workflow
- **Text printer** (ATASCII / ASCII)
- **PCLINK** — full filesystem of the phone as an SIO device
- **8 UI languages**: Polish, English, German, Spanish, French, Russian, Slovak, Turkish

## Requirements

### Hardware to run

| Item | Requirement |
|------|-------------|
| **Phone** | Android 6.0+ (API 23), USB Host/OTG, portrait orientation |
| **Cable** | SIO2PC-USB with FTDI FT232R chip (e.g. Lotharek) |
| **Adapter** | USB-C → USB-A OTG (to connect the cable to the phone) |
| **Atari 8-bit** | any model with an SIO port |

### Development environment (to build the APK)

| Component | Version |
|-----------|---------|
| Qt | 6.11+ for Android (modules: core, gui, widgets, printsupport, svg, core5compat) |
| Java JDK | 17 |
| Android NDK | provided by Qt (~26.3) |
| Android SDK | API 34, minSdk 23 |

## Building the APK

1. In **Qt Maintenance Tool** install:
   - Qt 6.11 → Android
   - NDK and Android SDK (Qt Creator will do this after you point it to JDK 17)
2. Open `aspeqt.pro` in Qt Creator
3. Pick the **Android arm64-v8a** kit
4. Build → you will find the generated APK in the build directory

### Translations

The `.ts` source files are in `i18n/`. `.qm` files are regenerated automatically
during the build (`CONFIG += lrelease` in `.pro`). To regenerate manually:

```
lrelease i18n/aspeqt_pl.ts
```

## First launch

1. Connect the SIO2PC-USB cable to the phone via OTG adapter
2. Connect the other end to the Atari SIO port
3. Launch AspeQt, in Options set:
   - **Interface**: SIO2PC (USB)
   - **Handshake**: CTS (default for SIO2PC-USB FT232R)
4. Mount a disk image in slot D1
5. Power on the Atari while holding OPTION (to disable BASIC)

A fuller Polish quick-start guide is available in the repo:
`AspeQt - Krótka instrukcja obsługi.html`.

## License

GNU General Public License version 2 (or any later version) — see `license.txt`.

## Acknowledgments

AspeQt was originally created by **Ray Ataergin** and later maintained by
**greblus** ([github.com/greblus/aspeqt](https://github.com/greblus/aspeqt)).
This version is an independent continuation by **klopeks** (2026),
focused on modern Android support (12+) and full translations.

Full list of changes: see [`CHANGES.md`](CHANGES.md).

### Libraries / inspirations used
- **usb-serial-for-android** — Mike Wakerly (LGPL 2.1+)
- **SimpleFileDialog** — Hagai Shatz (CPOL 1.02)
- **liba8cas** — Tomasz Krasuski (GPL 3+) — FSK decoder algorithm as inspiration
  (no code was copied; the decoder is a fresh C++/Qt implementation based on the algorithm description)
- **Tango Icon Library** (Public Domain)
