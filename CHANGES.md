# Modifications to AspeQt

This is a modified version of AspeQt by Ray Ataergin and greblus.
Original repo: https://github.com/greblus/aspeqt
Distributed under GPL 2.0 or later (see `license.txt`).

## Modifications by klopeks (2025-2026)

### Cassette engine — major rework

- **Brand new FSK decoder** (~1000 lines in `sioworker.cpp`) implementing
  two complementary algorithms, inspired by **liba8cas** by Tomasz Krasuski
  (https://sourceforge.net/projects/a8cas/, GPL 3+). No code was copied;
  the decoder is a fresh C++/Qt implementation based on the algorithm
  description.
  - `decodeFskSignalsWithDeserializer` — primary decoder, recovers bits
    from FSK signal runs via a bit deserializer with auto baud-rate detection
  - `decodeFskSignalsBySampling` — fallback sampling decoder near the
    nominal baud rate (handles noisier recordings)
- New cassette playback engine selector with two modes:
  - `CassettePlaybackStandard` — original behavior (data chunks only)
  - `CassettePlaybackFskCompatibility` — converts FSK chunks into standard
    data records so games using FSK encoding can boot from CAS
- New "FSK cassette detected" dialog asks user whether to try the
  compatibility engine when FSK chunks are found
- Cassette dialog (`cassettedialog.cpp` +216 lines) rewritten with full
  progress display, time remaining, and updated instructions explaining
  the RETURN key + OK button synchronization

### XEX executable loader — fixes and improvements

- **Memory bug fix** in `autoboot.cpp`: `QByteArray data` was being
  written with `data[i]` before allocation — replaced with
  `QByteArray data(6, 0)` to prevent undefined behavior / potential crashes
- Autoboot dialog (`autobootdialog.cpp` +116 lines) rebuilt:
  - Retry / Open / Cancel button workflow (was only Cancel before)
  - `openRequested()` / `reloadRequested()` state tracking
  - "Program loaded" final state lets user start the same exe again,
    pick another exe, or close — without restarting Atari
  - Reload tooltip explaining when to use the button
- `mainwindow.cpp::chooseAndBootExe()` updated to handle the new
  Retry/Open flow with the dialog state

### Android compatibility (Android 12+, tested on 14 and 15)

- `AndroidManifest.xml`:
  - Scoped storage + `MANAGE_EXTERNAL_STORAGE` (Android 11+) so the file
    picker can browse anywhere on /sdcard
  - Modern Bluetooth permissions: `BLUETOOTH_CONNECT`, `BLUETOOTH_SCAN`
    (with `neverForLocation` flag) for Android 12+
  - Legacy BT/storage permissions kept with `maxSdkVersion` for older OS
  - `allowBackup="false"` to prevent Android Auto Backup restoring stale
    settings after reinstall
  - `android:exported="true"` for Activity (required by API 31+)
- `SerialActivity.java`:
  - `attachBaseContext()` override with `LocaleHelper.wrap()` so Java-side
    dialogs follow the in-app language choice
  - Runtime permission requests for `BLUETOOTH_CONNECT/SCAN` on API 31+
  - `Environment.isExternalStorageManager()` check + friendly dialog
    explaining how to grant All Files Access in system settings
- `SIO2PCUS4A.java`:
  - `PendingIntent.FLAG_IMMUTABLE` required by Android 12+
  - Explicit `permIntent.setPackage(sa.getPackageName())` to avoid the
    "implicit intent not allowed" crash on Android 14
  - Buffer purge on `openDevice()` and `closeDevice()` to fix stuck state
  - Permission check before requesting USB permission again
- New file `LocaleHelper.java`:
  - Reads `aspeqt_lang.txt` (written by C++ side) and wraps Context with
    `Locale.forLanguageTag()` + `createConfigurationContext()`
  - `LocaleHelper.getString(ctx, R.string.X)` for per-string locale wrapping

### usb-serial-for-android fixes (Mike Wakerly, LGPL 2.1+)

- `FtdiSerialDriver.setDTR()` / `setRTS()` — were **empty stubs**, so the
  FTDI cable's output buffer was never enabled. Implemented via FTDI
  `SIO_SET_MODEM_CTRL_REQUEST` (1) with proper high/low values
  (0x0101 / 0x0100 / 0x0202 / 0x0200). This was THE blocker preventing
  SIO communication from working on Android at all.
- `getStatus()` byte sign-extension fix: `return buf[0] & 0xff;`
  (was returning negative values for status bytes ≥ 0x80)

### File picker (`SimpleFileDialog.java`, CPOL 1.02)

- All `getResources().getString(R.string.X)` calls replaced with
  `LocaleHelper.getString(m_context, R.string.X)` so file picker labels
  follow in-app language
- Hardcoded `"OK"` button replaced with `android.R.string.ok`
  (system-provided localization for all languages)

### UX / behavior

- Auto-start emulation on launch
- Auto-cycle SIO on disk mount (without manual stop/start)
- Default handshake method changed from SOFT to **CTS** —
  the correct setting for Lotharek SIO2PC-USB with FT232R chip
- `loadTranslators()` rewrite (`mainwindow.cpp`):
  - Bug fix: original code loaded `qt_*.qm` into a translator then
    immediately overwrote it with `qtbase_*.qm` — the `qt_*` part was
    always lost
  - Now uses 3 separate translators (qt, qtbase, aspeqt) with explicit
    install order
  - Fallback locale lookup: tries full code (`fr_FR`) then short (`fr`)
- Localized "AspeQt started at" log timestamp via `QLocale::system()`
- C++ writes current language code to `<filesDir>/aspeqt_lang.txt` on
  startup and on language change, so Java side picks it up
- Settings migration on first run

### Translations — major overhaul

- **Added French (fr)** — full new translation, including:
  - Cassette/autoboot/options dialogs
  - First-run dialog
  - All major UI labels and menus
  - `qt_fr.qm` and `qtbase_fr.qm` bundled
- **Polish (pl) refined throughout** — translations polished, terminology
  consistent, cassette dialog updated, FSK dialog added, file picker
  strings added, all permission dialogs translated
- **de, es, ru, sk, tr** — filled in missing dialog translations:
  - Cassette dialog new long instructions (RETURN key explanation)
  - "Please reboot... Disable BASIC" (Android variant)
  - "Atari is loading the booter/program" (autoboot states)
  - "Program loaded" with Retry/Open/Cancel
  - "Use this button to re-load the executable" tooltip
  - FSK cassette detected dialog
  - "Starting emulation" log message
- **Slovak**: added missing `QPlatformTheme` + `QAndroidPlatformTheme`
  contexts (Qt 6 looks up standard dialog buttons there, but Slovak
  `qtbase_sk.qm` from Qt 6.11 lacks these contexts — Retry/Open/Cancel
  were always falling back to English)
- **German**: fixed half-translated "High Speed Modus Baudrate" →
  "Baudrate im Hochgeschwindigkeitsmodus", same for POKEY divisor
- **Russian**: translated remaining "Save window positions and sizes"
- **Turkish**: removed stale `type="obsolete"` from "Starting emulation"
  entry which prevented lrelease from including it in the .qm
- Native language names in the language selector (was showing 3× "English"
  before because sk/tr/fr/ru didn't translate the source "English" string)
- `CONFIG += lrelease` in `.pro` so `.ts` edits trigger `.qm` regeneration
  automatically at build time

### Build / project

- Custom `android/build.gradle` overriding Qt's default template:
  - `resConfigs "en", "pl", "de", "es", "ru", "sk", "tr", "fr"` — Qt's
    default `resConfig "en"` was stripping all non-English `values-XX/`
    folders from the APK
  - JavaVersion.VERSION_17, Kotlin 2.3.0, AGP 9.0.0
- `aspeqt.pro` updated: removed broken `qt_*.ts` references that caused
  qmake warnings; added all language `.ts` files including French
- Removed `android/libs/android-support-v4.jar` (legacy, replaced by
  AndroidX in `compileSdk 34`)

### Documentation

- `about.html` rewritten with klopeks 2026 edition info
- Added `AspeQt - Krótka instrukcja obsługi.html` (Polish quick start)
- Updated `AspeQt User Manual-Polish.html` and `-English.html`
- Added `README.md` and this `CHANGES.md`
