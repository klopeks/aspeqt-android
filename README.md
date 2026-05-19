# AspeQt for Android

Atari 8-bit SIO peripheral emulator dla Androida (12, 13, 14, 15+).

Zamienia smartfon w stację dysków, magnetofon i drukarkę dla komputerów Atari 8-bit
podłączonych przez kabel SIO2PC-USB.

## Funkcje

- **15 napędów dyskowych** (D1-D15) z obsługą formatów: ATR, XFD, PRO, ATX, DCM, SCP, DI + skompresowane gzip
- **Folder Images** — bezpośrednie udostępnienie folderu z telefonu jako dysku Atari
- **Magnetofon (CAS)**:
  - Standardowy odtwarzacz dla zwykłych obrazów kaset
  - **Drugi silnik z dekoderem FSK** (algorytm inspirowany [liba8cas](https://sourceforge.net/projects/a8cas/))
    odtwarza obrazy z fragmentami FSK, których stary AspeQt nie umiał
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
Ta wersja to niezależna kontynuacja autorstwa **klopeks** (2025-2026),
skupiona na wsparciu nowoczesnego Androida (12+) oraz pełnych tłumaczeniach.

Pełna lista zmian: zobacz [`CHANGES.md`](CHANGES.md).

### Wykorzystane biblioteki / inspiracje
- **usb-serial-for-android** — Mike Wakerly (LGPL 2.1+)
- **SimpleFileDialog** — Hagai Shatz (CPOL 1.02)
- **liba8cas** — Tomasz Krasuski (GPL 3+) — algorytm dekodera FSK jako inspiracja
  (kod nie został skopiowany, własna implementacja w C++/Qt na podstawie opisu algorytmu)
- **Tango Icon Library** (Public Domain)
