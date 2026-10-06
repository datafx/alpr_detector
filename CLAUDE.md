# CLAUDE.md — ALPR Detector

Session start: **"Read CLAUDE.md and continue."**

Decisions get locked before code, one item at a time. If something in
"Open decisions" is unresolved, resolve it explicitly before writing code that
depends on it. Don't silently pick a default.

---

## What this is

A windshield-mounted passive detector on an ESP32-WROOM-32U for ALPR cameras,
police surveillance sensors, and LE gear. It scans BLE advertisements and
802.11 management frames and matches three independent watchlists:

- transmitter MAC OUIs (`oui_table.cpp`)
- SSIDs in beacons / probe requests / probe responses (`ssid_table.cpp`)
- BLE advertisement contents — names, company IDs, service UUIDs, payload
  tags (`ble_sig_table.cpp`)

Hits are signalled on an OLED, an LED, and a buzzer.

The BLE GAP scan structure and WiFi promiscuous handler are adapted from
**nyanBOX** (<https://github.com/jbohack/nyanBOX>), Copyright (c) 2025
jbohack, MIT. The attribution headers in `src/detector.cpp` and
`src/main.cpp` are an MIT license condition — keep them.

---

## Status

Builds clean (`pio run`); both off-target suites pass. The OUI/SSID/alert
path has been through full hardware bring-up. The BLE signature path has
**not yet been flashed or field-tested** — see bring-up step 6.

---

## Hardware

ESP32-WROOM-32U DevKitC (38-pin), external 2.4 GHz antenna via U.FL → RP-SMA
pigtail to an Alfa APA-M25 8 dBi directional panel. Currently soldered to
perfboard on a temporary in-vehicle mount.

| Signal | GPIO | Header | Notes |
|---|---|---|---|
| OLED SDA | 21 | J3 pin 6 | SSD1306 128x64 I2C |
| OLED SCL | 22 | J3 pin 3 | |
| Alert LED | 25 | J2 pin 9 | via 270R to GND |
| Buzzer SIG | 26 | J2 pin 10 | active module, active-low on this build |
| BOOT button | 0 | onboard | tap = mute, hold 1 s = cycle band mode |
| 3V3 | — | J2 pin 1 | **output**, feeds the + rail |
| GND | — | J2 p14 / J3 p1 | |

- Powered from micro-USB; onboard regulator drives 3V3. Nothing needs 5V.
- **EN is deliberately unwired.** It has an onboard pull-up. Tying it to 3V3
  breaks the USB auto-reset/bootloader circuit.
- Any future PCB must include a 100nF–1µF ceramic cap between EN and GND
  (bursts of spurious `POWERON_RESET`s on breadboard wiring pointed at EN
  noise).

---

## Locked decisions

1. **Framework**: Arduino-ESP32 under PlatformIO, calling raw `esp_wifi.h` /
   `esp_gap_ble_api.h` directly, u8g2 for the OLED.

2. **One radio, time-sliced.** The ESP32 has a single 2.4 GHz radio. One
   stack is fully torn down before the other starts (`WIFI_PHASE_MS` 3500 /
   `BLE_PHASE_MS` 3000). Don't try to run both — it isn't possible.

3. **BLE scan is PASSIVE.** Receive only; never transmit. Consequence:
   scan-response data is never received, so a BLE signature that only lives
   in a scan response is unreachable.

4. **OUI matching only on non-randomised addresses** (BLE public; WiFi with
   the locally-administered bit clear). **SSID and BLE-signature matching
   run on every address type** — Flock battery packs, Raven, and newer Axon
   gear use random/rotating addresses, so the payload is what identifies
   them. A BLE signature hit takes priority over an OUI hit on the same
   advert.

5. **Fixed device table, LRU eviction.** 48 entries, no dynamic allocation.
   RSSI is EMA-smoothed 1/3.

6. **No confidence tiers — alert or no alert.** `Specificity`
   (`SPEC_LE_ONLY`/`SPEC_BROAD`) is curation metadata only. A noisy vendor
   gets compiled out (`EXCLUDE_VENDOR_*`), not downgraded.

7. **Alert model**: LED and buzzer driven from one shared phase. Flash period
   scales with the strongest RSSI. Pure decay hold timer, reset on every
   match: `DUAL_MODE_HOLD_MS` 11000 / `SINGLE_MODE_HOLD_MS` 5500. Mute
   silences the buzzer only, for the current alert only.

8. **Band mode** (WiFi only / BLE only / both) via BOOT hold, persisted in
   NVS (`Preferences` namespace `"alprdet"`, key `"bandMode"`, default both).

9. **No serial logging / survey mode.** Standalone alerter, not a capture
   device. Don't re-propose an on-device history either.

10. **WiFi channels 1–11 only** (US). 250 ms dwell.

11. **Prefix lengths**: /24, /28 and /36 supported. Lookup binary-searches the
    first 3 bytes, then walks the run applying the full mask. Shared blocks
    like `70:B3:D5` must be entered at their real /36, never /24.

12. **Watchlist curation**:
    - OUIs must be IEEE-registered, registrant verified. Sole exception:
      `D8:1F:65` (IEEE "Private"), field-attributed to Axon — pull it if a
      non-Axon device ever shows up on it.
    - BLE rows cite a Bluetooth SIG assignment or a named field capture.
    - SSID rows come from observed network names, never guesses; always
      anchored prefixes (substring matching false-positives on consumer SSIDs).
    - Name collisions stay excluded (WatchGuard Technologies, Coban SRL, TAIT
      Global, Axon Networks, Vutility) — don't "helpfully" re-add them.
    - Excluded as too broad: Dell, Panasonic, Peplink/Pepwave. CradlePoint is
      compiled out by default (`EXCLUDE_VENDOR_CRADLEPOINT`) after field false
      positives on school buses and turnpike cameras.

13. **Weak signals — decided 2026-10-06:**
    - IN: BLE company ID `0x09C8` (XUNTONG, Flock battery module) as a
      standalone match, ordered last; ALPR-vendor candidate OUIs (Genetec IAB,
      Ubicquia, Elsag Datamat, Selex ES, Jenoptik, Kapsch, Ekin, Avigilon Alta).
    - OUT: Lite-On probe-request OUIs (`24:B2:B9`, `F4:6A:DD`, `D8:F3:BC`,
      `C0:35:32`) — consumer laptops probe with them. Revisit only by explicit
      decision; if ever added, gate to probe-request frames only.
    - OUT: the bare 10-digit BLE name (false-positived on a phone elsewhere).
    - Motorola BLE IDs are IN by default with `EXCLUDE_VENDOR_MOTOROLA_BLE`.

14. **Licence: GPLv3**, except the MIT-licensed nyanBOX-derived portions of
    `src/detector.cpp` and `src/main.cpp` (SPDX line + header kept).

---

## Coverage — what can't be detected on this hardware

Don't re-research these without new field data:

- **Genetec AutoVu Cloudrunner (CR-H2)** — solar + 4G/LTE, no published
  WiFi/BLE signature. Only the Genetec OUIs, if a unit has a setup radio on.
- **Axon Outpost** — LTE; no known signature.
- **Axon Fleet 3 / Fleet Hub hidden APs** — 5 GHz; the WROOM-32U is 2.4 GHz
  only. Fleet gear may still hit over BLE via the Axon SIG IDs.
- **Leonardo/ELSAG SignalTrace (formerly EOC Plus)** — passive receiver of
  WiFi/BLE/915 MHz RFID beside ELSAG ALPRs (patents US11941716B2,
  US12236780B2). Its core function doesn't transmit on 2.4 GHz. Only hook:
  the Elsag Datamat / ELSAG / Selex ES OUIs, if a unit backhauls over WiFi.
- **Motorola/Vigilant, Rekor, Neology, Jenoptik, Conduent** — cellular/wired.
- **Cell-site simulators** — not on 2.4 GHz at all.

Signature sources: colonelpanichacks/flock-you (MIT),
soyboi1312/all-cameras-are-beacons (Apache-2.0, `docs/signatures.md`),
ryanohoro's Flock write-up, Bluetooth SIG assigned_numbers, Wireshark
`manuf` (IEEE-derived; standards-oui.ieee.org refuses scripted downloads).
Values only — no code copied.

---

## Open decisions

- **Phase timing under real driving.** ~7 s revisit interval per band at
  3.5 s WiFi + 3 s BLE + stack transitions; ~670 ft at 65 mph. Untested.
- **Field validation of BLE signatures** — none of the BLE rows has been
  confirmed by this device yet. If `0x09C8` or Motorola BLE turns out noisy,
  decide whether to drop them.
- **Remote ID drone detection** (ASTM F3411 over WiFi beacons / BLE),
  filtered by manufacturer serial prefix to pick out DFR fleets (Skydio,
  Brinc, Flock Aerodome). Raised, not designed.

## Future ideas

- **Custom PCB + 3D-printed enclosure**, mounted on an off-the-shelf
  radar-detector windshield mount. Must include the EN cap. Possibly
  dedicated buttons instead of the overloaded BOOT button.
- **Second radio (e.g. nRF52840 over UART) for simultaneous BLE + WiFi.**
  Would revise decision 2; needs its own decision pass (merging detections
  into one table, power, physical fit).

---

## Layout

```
platformio.ini
include/
  config.h          all tuning: pins, phase timing, flash rate, thresholds
  detector.h        TrackedDevice, DetectorStatus, phase control
  alert.h           LED/buzzer state machine
  ui.h              OLED
  oui_table.h       OuiEntry, GearCategory, Specificity, lookup
  ssid_table.h      SsidEntry, lookup
  ble_sig_table.h   BleSigEntry, BleSigKind, advert parser + lookup
src/
  main.cpp          setup/loop, WiFi<->BLE phase machine, BOOT button, NVS
  detector.cpp      BLE GAP callback, WiFi promiscuous handler, device table
  alert.cpp         RSSI -> flash period, synced LED+buzzer drive
  ui.cpp            status screen
  oui_table.cpp     OUI watchlist (MUST stay sorted)
  ssid_table.cpp    SSID watchlist (prefix, case-insensitive)
  ble_sig_table.cpp BLE advertisement signature watchlist (first match wins)
test/
  test_oui.cpp      OUI table, incl. /28 and /36 boundaries
  test_ble_sig.cpp  BLE advert parser + signature table
```

All tuning lives in `config.h`. Prefer changing constants there over editing
logic.

---

## Testing off-target

The OUI and BLE tables have no Arduino dependency. Re-run after any table edit:

```bash
g++ -std=c++11 -o /tmp/test_oui test/test_oui.cpp src/oui_table.cpp -I include && /tmp/test_oui
g++ -std=c++11 -o /tmp/test_ble_sig test/test_ble_sig.cpp src/ble_sig_table.cpp -I include && /tmp/test_ble_sig
```

To test an `EXCLUDE_VENDOR_*` toggle, add e.g. `-DEXCLUDE_VENDOR_MOTOROLA` to
the `g++` line and confirm `ouiTableSize()` drops. CradlePoint's `#define`
lives in `oui_table.cpp` itself — comment it out to test it re-included.

## Build

```bash
pio run                  # compile
pio run -t upload        # flash
pio device monitor       # 115200
```

If upload fails: hold **BOOT**, tap **EN**, release BOOT.

---

## Bring-up sequence

Validate in this order; don't skip ahead when something fails.

1. **Boot screen** — confirms OLED wiring. Shows OUI, SSID and BLE signature
   counts so you can tell you flashed the tables you meant to.
2. **Power-on self test** — LED + buzzer pulse ~120 ms together.
3. **Radio phase indicator** — header alternates `AUTO: WiFi` / `AUTO: BLE`.
   If it sticks, a stack failed to init.
4. **WiFi OUI alert path** — spoof an AP with a watchlisted OUI (e.g. a
   NetworkManager hotspot with cloned MAC `00:25:DF:..`), confirm vendor,
   RSSI bar, LED and buzzer.
5. **SSID alert path** — rename a phone hotspot to `ALPRDET-TEST`; the second
   line should read `SSID beacon: ALPRDET-TEST`. Remove that row before
   relying on the table for anything real.
6. **BLE signature path** (not yet run on hardware) — with nRF Connect on a
   phone, advertise local name `Penguin-0123456789` (or manufacturer data
   with company ID `0x09C8`). Expect `Flock Safety` / `BLE name Penguin-...`
   (or `Flock? (0x09C8)` / `BLE mfg 0x09C8`). Phones advertise from random
   addresses, so this also confirms decision 4.
7. **Flash-rate scaling** — move the test device closer/further; flash speeds
   up as RSSI rises.

---

## Gotchas

- `esp_wifi_init`/`deinit` and bluedroid init/deinit run every few seconds.
  Watch heap over long runs (`ESP.getFreeHeap()`); fragmentation is the
  plausible failure mode for hours in a car.
- The WiFi promiscuous callback is `IRAM_ATTR` and runs in the WiFi task.
  Keep it short; no `Serial.print` or anything blocking.
- The BLE callback runs in the Bluedroid task. The advert parser walks
  ≤62 bytes and never trusts an AD length byte — keep it that way.
- `DetectorStatus.best` points into the static device array. Valid within
  one loop iteration only.
- `esp_ble_gap_start_scanning(0)` scans until stopped; it's started from the
  GAP callback after params are set, not inline.
- OLED header bar inverts during an alert; anything drawn on it needs
  `setDrawColor(0)` or it's white-on-white.
- The default `esp32dev` partition table is too small for this firmware;
  `platformio.ini` uses `huge_app.csv` (single ~3MB app, no OTA).
- This board's buzzer module is active-low (`BUZZER_ACTIVE_LOW = true`). If
  a buzzer drones at idle and goes quiet only on the POST pulse, flip it.
- Opening the serial port with plain `cat`/`stty` holds the board in reset.
  For diagnostics use pyserial with `dtr=False, rts=False`.
