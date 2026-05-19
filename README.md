# AspeQt

Atari 8-bit SIO peripheral emulator — działa na Android (12, 13, 14, 15), Windows, Linux i macOS.

Emuluje stacje dysków, magnetofon, drukarkę i system plików dla komputerów Atari 8-bit
podłączonych przez kabel SIO2PC-USB lub moduł Bluetooth SIO2BT.

## Funkcje

- **15 napędów dyskowych** (D1-D15) z obsługą formatów: ATR, XFD, PRO, ATX, DCM, SCP, DI + skompresowane gzip
- **Folder Images** — bezpośrednie udostępnienie folderu z hosta jako dysku Atari
- **Magnetofon (CAS)**:
  - Standardowy odtwarzacz dla zwykłych obrazów kaset
  - **Drugi silnik z dekoderem FSK** (algorytm inspirowany [liba8cas](https://sourceforge.net/projects/a8cas/))
    odtwarza obrazy z fragmentami FSK, których stary AspeQt nie umiał
- **Autoboot programów Atari** (XEX, COM, EXE) z dysku hosta
  - Workflow Retry / Open / Cancel
- **Drukarka tekstowa** (ATASCII / ASCII)
- **PCLINK** — pełny system plików hosta jako urządzenie SIO
- **Backendy**: SIO2PC-USB (FTDI) oraz SIO2BT (Bluetooth)
- **8 języków interfejsu**: polski, angielski, niemiecki, hiszpański, francuski, rosyjski, słowacki, turecki

## Wymagania

### Środowisko developerskie

| Komponent | Wersja |
|-----------|--------|
| Qt | 6.11+ (moduły: core, gui, widgets, printsupport, svg, core5compat) |
| C++ Compiler | MinGW 64-bit / GCC / Clang (C++17) |
| Java JDK | 17 (dla Androida) |
| Android NDK | wersja dostarczana przez Qt 6.11 (~26.3) |
| Android SDK | API 34, minSdk 23 |

### Sprzęt do uruchomienia

| Platforma | Wymagania |
|-----------|-----------|
| **Android** | Android 6.0+ (API 23), USB Host/OTG dla SIO2PC-USB, Bluetooth dla SIO2BT |
| **Desktop** | Windows 7+, Linux, macOS; port szeregowy lub adapter SIO2PC |
| **Atari 8-bit** | dowolny model + kabel SIO2PC-USB (np. Lotharek) lub moduł SIO2BT |

## Budowanie

### Desktop (Qt Creator)

1. Otwórz `aspeqt.pro` w Qt Creator
2. Wybierz kit Desktop (MinGW 64-bit / GCC / Clang)
3. Build → Run

### Android (Qt Creator)

1. W Qt Maintenance Tool zainstaluj:
   - Qt 6.11 → Android
   - NDK i Android SDK (Qt zrobi to po wskazaniu JDK 17)
2. Otwórz `aspeqt.pro` w Qt Creator
3. Wybierz kit Android (arm64-v8a)
4. Build — wygenerowany APK znajdziesz w katalogu build

### Tłumaczenia

Źródła `.ts` są w `i18n/`. Pliki `.qm` są regenerowane automatycznie przy buildzie
(`CONFIG += lrelease` w `.pro`). Aby ręcznie zregenerować:

```
lrelease i18n/aspeqt_pl.ts
```

## Pierwsze uruchomienie

1. Podłącz kabel SIO2PC-USB do telefonu przez OTG
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
Ta wersja to niezależna kontynuacja autorstwa **klopeks** (2025-2026),
skupiona na wsparciu nowoczesnego Androida (12+) oraz pełnych tłumaczeniach.

Pełna lista zmian: zobacz [`CHANGES.md`](CHANGES.md).

### Wykorzystane biblioteki / inspiracje
- **usb-serial-for-android** — Mike Wakerly (LGPL 2.1+)
- **SimpleFileDialog** — Hagai Shatz (CPOL 1.02)
- **liba8cas** — Tomasz Krasuski (GPL 3+) — algorytm dekodera FSK jako inspiracja
  (kod nie został skopiowany, własna implementacja w C++/Qt na podstawie opisu algorytmu)
- **Tango Icon Library** (Public Domain)
