# ALPR Detector — ESP32-WROOM-32U

Passive BLE + WiFi scanner for ALPR cameras, police surveillance sensors, and
law-enforcement gear. It checks three things:

- **MAC OUIs** against a vendor watchlist (Flock Safety, Genetec, ELSAG/Leonardo,
  ShotSpotter, Axon, Motorola and 40+ LE vendors)
- **SSIDs** in beacons, probe requests and probe responses (e.g. Flock's `Flock-XXXXXX` setup AP)
- **BLE advertisement contents**: device names, company IDs, service UUIDs and
  payload tags (Flock Penguin battery packs, Flock Raven, Axon body cams)

Hits show on an OLED and trigger an LED and buzzer that flash faster as the
signal gets stronger. It only receives — it never transmits.

## What it can and can't see

| Target | How it's detected here | Coverage |
|---|---|---|
| Flock Safety Falcon/Sparrow | OUI `B4:1E:52`, SSID `Flock-…`, BLE battery-pack names (`FS Ext Battery`, `Penguin-<digits>`, `FS-<hex>`, `Flock…`), Flock accessory 128-bit service UUID, company ID `0x09C8` | Good, though field reports say Flock has been turning these off over time |
| Flock Raven (gunshot/audio) | BLE services `0x3100`–`0x3500` | Good |
| Axon body cams / Signal / Fleet | `BWCDEVICE` BLE tag, Bluetooth SIG IDs (`0x034D`, `0xFC81`, `0xFE6B`, `0xFE6C`), OUIs `00:25:DF`, `D8:1F:65` | Good over BLE. **Fleet Hub's hidden APs are 5 GHz, which this board can't receive** |
| Axon Outpost (fixed ALPR) | Axon IDs only | Unknown, no published signature (LTE backhaul) |
| Genetec AutoVu Cloudrunner | Genetec OUIs | Weak: solar + LTE, no published WiFi/BLE signature |
| ELSAG / Leonardo, incl. **SignalTrace** | Elsag Datamat, ELSAG and Selex ES OUIs | Weak: SignalTrace is a passive receiver and only shows up here if a unit backhauls over WiFi |
| ShotSpotter | OUI `D4:11:D6` | Only if a unit uses WiFi |
| Skydio (drone-as-first-responder) | OUI `38:1D:14` | The same block also covers industrial drones |
| Motorola/Vigilant, Rekor, Neology, Ubicquia, Jenoptik, Kapsch | Candidate OUIs where registered | Mostly cellular, so expect little |

## Build

If you already have PlatformIO Core set up:

    pio run -t upload
    pio device monitor

### Building from source if you're new to PlatformIO

1. **Install PlatformIO.** Pick whichever fits how you like to work:
   - **VS Code** (recommended if you'd rather not touch a terminal beyond
     the basics): install [VS Code](https://code.visualstudio.com/), then
     install the **PlatformIO IDE** extension from the Extensions panel
     (search "platformio"). It downloads everything else it needs the
     first time it runs — that can take a few minutes.
   - **Command line:** install Python 3 if you don't already have it, then:

         pip install --user platformio

     (`pipx install platformio` works too, if that's your preference.)

2. **Get the code** — clone or download this repository into a folder
   named `alpr_detector`.

3. **Open the project.**
   - VS Code: **File → Open Folder**, select the `alpr_detector` folder.
     PlatformIO reads `platformio.ini` and configures itself for this
     project automatically — nothing to set up by hand.
   - Command line: just stay inside the `alpr_detector` directory; every
     `pio` command below assumes that's your working directory.

4. **Plug in the board** via micro-USB. Chip type, flash size, and the
   partition table are already specified in `platformio.ini`, so there's
   no board selection step.

5. **Build and flash:**
   - VS Code: click the checkmark icon in the bottom status bar to build,
     then the right-arrow (→) icon to build *and* upload.
   - Command line:

         pio run -t upload

   This should auto-detect the board and flash it without pressing
   anything on the board itself — the DevKitC's onboard USB-serial chip
   handles the reset. If upload fails with a timeout or "Failed to
   connect": hold **BOOT**, tap **EN**, release **BOOT**, then retry.

6. **Watch it boot** (optional):

         pio device monitor

   There's no serial logging during normal operation — a deliberate design
   choice — so past the initial boot banner you won't see a running stream
   of output. The OLED is the status display.

### If the board doesn't show up as a serial port

- **Windows/macOS:** you may need the [CP210x USB-to-UART
  driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)
  from Silicon Labs — that's the DevKitC's onboard USB chip.
- **Linux:** the port usually appears automatically (typically
  `/dev/ttyUSB0`), but your user may need group membership to access it
  without `sudo` — `dialout` on Debian/Ubuntu, `uucp` on Arch-based
  distros. Add yourself and re-log-in for it to take effect:

      sudo usermod -aG dialout $USER   # or: uucp, on Arch-based distros

## Using it

- **Tap BOOT** — mute the buzzer for the current alert (the LED keeps flashing).
- **Hold BOOT 1 s** — cycle band mode: WiFi only → BLE only → both. Saved
  across power cycles.

The header shows `ALL CLEAR` or `** ALERT **` and the current band. During
an alert the screen shows the vendor, what matched (`SSID beacon: …`,
`SSID probe: …`, `BLE name …`, `BLE svc 0x…`, `via WiFi OUI`, …) and a
signal-strength bar.

## Wiring

| Signal | Pin | Header |
|---|---|---|
| OLED SDA | GPIO21 | J3 pin 6 |
| OLED SCL | GPIO22 | J3 pin 3 |
| Alert LED (+270R) | GPIO25 | J2 pin 9 |
| Buzzer SIG | GPIO26 | J2 pin 10 |

Power via micro-USB. EN is left unwired.

## Parts list

| Part | Notes |
|---|---|
| ESP32-WROOM-32U DevKitC (38-pin) | The `-U` variant — external antenna via U.FL, not the PCB-antenna `-D` variant |
| SSD1306 128x64 OLED, I2C | Any standard 4-pin (GND/VCC/SCL/SDA) module |
| LED, any color | + 270R resistor, GPIO25 to GND |
| Active buzzer module, 3.3–5V | Must be an *active* module (built-in oscillator, just needs a logic level), not passive/piezo. Check `BUZZER_ACTIVE_LOW` in `config.h` against your module — some trigger on LOW |
| U.FL → RP-SMA pigtail | Connects the WROOM-32U's onboard U.FL to an external antenna |
| Alfa APA-M25 (8 dBi directional panel) or equivalent 2.4 GHz antenna | RP-SMA connector |
| Breadboard/perfboard + wire | Or a PCB — see the EN-pin note below |
| Micro-USB cable | Power + programming; no external supply needed |

No 5V rail needed anywhere — the buzzer runs off the same 3V3 the DevKitC
outputs on J2 pin 1, and everything else is 3.3V logic.

**Building a PCB?** Add a 100nF–1µF ceramic capacitor between EN and GND.
A bare EN pin (onboard pull-up only) on breadboard wiring produced bursts of
spurious resets that pointed at EN-pin noise. The cap is cheap and standard
practice on ESP32 designs.

## OUI watchlist

78 prefixes across 50+ law-enforcement, ALPR and surveillance vendors (76
compiled in by default — CradlePoint is excluded, see below). All are
IEEE-registered except `D8:1F:65`, which the registry lists as "Private" and
which field capture attributes to Axon. Any match alerts the same way.

Each entry is tagged by how exclusively its vendor sells to LE (**LE-only** vs
**broad**, meaning it also ships on lots of civilian gear) purely as curation
info: broad vendors that turn out to be a false-positive source get compiled
out with an `EXCLUDE_VENDOR_*` toggle in `oui_table.cpp` (CradlePoint is off by
default for exactly this reason).

Supports /24 (MA-L), /28 (MA-M) and /36 (MA-S) assignments. Prefixes shown
with a trailing `N_` are sub-/24 blocks where only the high nibble of the next
byte is significant. Keep the array sorted ascending; lookup is a binary
search. Run `test/test_oui.cpp` after edits. To drop an entry, comment out its
row — that can't break the sort order.

| Prefix | Vendor | Category | Specificity |
|---|---|---|---|
| `00:00:C3` | Harris | Radio | broad |
| `00:04:4C` | Jenoptik | Plate reader | broad |
| `00:04:7D` | Motorola Sol | Radio | broad |
| `00:06:EC` | Harris | Radio | broad |
| `00:08:B8` | EF Johnson | Radio | LE-only |
| `00:09:BC` | Utility Inc | Body/car cam | LE-only |
| `00:0A:3E` | EADS Telecom | Radio | broad |
| `00:0D:4F` | Kenwood | Radio | broad |
| `00:0D:CA` | Tait | Radio | broad |
| `00:0E:06` | Team Simoco | Radio | broad |
| `00:12:E0` | Codan | Radio | broad |
| `00:14:3E` | AirLink | Veh router | broad |
| `00:14:91` | Codan Radio | Radio | broad |
| `00:16:ED` | Utility Inc | Body/car cam | LE-only |
| `00:17:28` | Selex Comms | Radio | broad |
| `00:17:3D` | Neology | Plate reader | broad |
| `00:17:F3` | Harris | Radio | broad |
| `00:18:85` | Motorola Sol | Radio | broad |
| `00:1A:08` | Simoco | Radio | broad |
| `00:1C:3C` | Seon Design | Body/car cam | LE-only |
| `00:1D:96` | WatchGuard Vid | Body/car cam | LE-only |
| `00:1E:96` | Sepura | Radio | broad |
| `00:1F:92` | Motorola Sol | Radio | broad |
| `00:1F:9C` | Havis | Veh router | broad |
| `00:22:AF` | Safety Vision | Body/car cam | broad |
| `00:23:B9` | Airbus D&S | Radio | broad |
| `00:23:BD` | Digital Ally | Body/car cam | LE-only |
| `00:24:39` | Digital Barr | Body/car cam | LE-only |
| `00:24:E6` | In Motion Tech | Veh router | broad |
| `00:25:DF` | Axon | Body/car cam | LE-only |
| `00:26:B3` | Thales | Radio | broad |
| `00:30:44` | CradlePoint | Veh router | broad |
| `00:30:7E` | Redflex | Plate reader | LE-only |
| `00:40:DE` | Elsag Datamat | Plate reader | broad |
| `00:50:C2:BE:7_` | Genetec | Plate reader | broad |
| `00:90:C7` | Icom | Radio | broad |
| `00:A0:D5` | Sierra Wireless | Veh router | broad |
| `00:BF:15` | Genetec | Plate reader | broad |
| `00:E0:1C` | CradlePoint | Veh router | broad |
| `00:E0:6A` | Kapsch | Plate reader | broad |
| `04:C3:E6:9_` | Ekin | Plate reader | broad |
| `08:3C:03:0_` | Federal Signal | Lightbar | LE-only |
| `0C:BF:15` | Genetec | Plate reader | broad |
| `10:74:6F` | Motorola Sol | Radio | broad |
| `1C:82:59:D_` | Stalker Radar | Radar | LE-only |
| `28:A3:31` | Sierra Wireless | Veh router | broad |
| `38:1D:14` | Skydio | Drone | broad |
| `38:73:EA:0_` | L3 MobileVis | Body/car cam | LE-only |
| `48:46:8D` | Zepcam | Body/car cam | LE-only |
| `48:E3:C3` | Jenoptik | Plate reader | broad |
| `4C:CC:34` | Motorola Sol | Radio | broad |
| `50:13:9D` | Sierra Wireless | Veh router | broad |
| `58:94:CF` | Vertex Std LMR | Radio | broad |
| `58:E8:76:C_` | Kustom Signals | Radar | LE-only |
| `64:69:BC` | Hytera | Radio | broad |
| `64:CE:6E` | Sierra Wireless | Veh router | broad |
| `68:DA:73:B_` | Gamber-Johnson | Veh router | broad |
| `6C:18:11` | Decatur Elec | Radar | LE-only |
| `70:1A:D5` | Avigilon Alta | Plate reader | broad |
| `70:B3:D5:1C:5_` | ELSAG | Plate reader | LE-only |
| `70:B3:D5:52:1_` | Selex ES | Plate reader | broad |
| `70:B3:D5:88:A_` | Perceptics | Plate reader | LE-only |
| `70:B3:D5:F5:E_` | Selex ES | Plate reader | broad |
| `84:DB:2F` | Sierra Wireless | Veh router | broad |
| `94:7B:BE` | Ubicquia | Plate reader | broad |
| `9C:06:6E` | Hytera | Radio | broad |
| `9C:83:BF` | PRO-VISION | Body/car cam | LE-only |
| `9C:86:2B` | Motorola Sol | Radio | broad |
| `B4:1E:52` | Flock Safety | Plate reader | LE-only |
| `B8:E2:8C` | Motorola Sol | Radio | broad |
| `BC:AD:90` | Kymeta | Veh router | broad |
| `CC:93:4A` | Sierra Wireless | Veh router | broad |
| `D4:11:D6` | ShotSpotter | Surveillance sensor | LE-only |
| `D8:1F:65` | Axon | Body/car cam | LE-only |
| `E0:DA:DC` | JVC Kenwood | Radio | broad |
| `E4:1E:0A:B_` | Safety Vision | Body/car cam | broad |
| `F4:69:D5:7_` | Rosco Vision | Body/car cam | broad |
| `FC:01:9E` | VieVu | Body/car cam | LE-only |

### Deliberately excluded

- **Dell, Panasonic** — Toughbook/laptop MDTs are ubiquitous patrol gear, but
  these prefixes sit on tens of millions of consumer machines.
- **Peplink / Pepwave** — vehicle routers; false alarms only in field testing.
- **Name collisions:** WatchGuard Technologies (Seattle firewalls, not
  WatchGuard Video), Coban SRL (Italy, not COBAN Technologies of Houston), TAIT
  Global LLC (not Tait Electronics), Axon Networks Inc. (not Axon Enterprise),
  Vutility Inc. (not Utility Inc./BodyWorn).
- **Lite-On probe-request OUIs** (`24:B2:B9`, `F4:6A:DD`, `D8:F3:BC`,
  `C0:35:32`) — other projects use these to catch current Flock Falcons acting
  as WiFi clients, but ordinary laptops send probes with the same OUIs.

Whelen, Code 3, SoundOff Signal, MPH Industries, Getac and COBAN Technologies
have no IEEE assignments — most lightbars and many radar heads aren't IP
devices, so there's nothing to match on.

## SSID watchlist

Matched against beacon, probe request and probe response frames,
independent of the OUI table. A client configured for a hidden network sends
its target SSID in cleartext in a directed probe request, so this survives MAC
randomisation. Patterns are case-insensitive prefixes.

| Prefix | Vendor | Notes |
|---|---|---|
| `Flock-` | Flock Safety | Falcon/Sparrow setup/health AP (`Flock-` + MAC). Largely switched off since ~Dec 2025, so this catches older or unpatched units |
| `ALPRDET-TEST` | TEST/DEMO | Not a real signature. Rename a phone hotspot to this to test the alert path |

A probe-request hit is weaker evidence than a beacon/probe-response hit — a
client leaking a network it joined before isn't proof that network's AP is
nearby now — so the display labels them differently (`SSID probe:` vs
`SSID beacon:`), though both alert the same way.

## BLE signature table

`src/ble_sig_table.cpp` is matched against the raw advertising payload of
**every** advert, whatever its address type. That's deliberate: Flock battery
packs, Raven and newer Axon gear use random or rotating addresses, so their
OUI means nothing. The table is checked top to bottom and the first match
wins. Specific signatures (names, vendor UUIDs) come first and the loose ones
come last (XUNTONG `0x09C8`, the Bluetooth module vendor in Flock's battery
packs, which other products also use). Each row cites its source. To disable a
row, comment it out. Motorola's BLE IDs mostly match two-way radios carried by
retail and venue staff; uncomment `EXCLUDE_VENDOR_MOTOROLA_BLE` to drop them.

The scan is passive, so data that only appears in a scan response is never
received. Every shipped row was reported in the primary advertisement.

Not included on purpose: the bare 10-digit BLE name some Flock research
mentions, which false-positived on a phone in another project's field testing.

## Testing off-target

The OUI and BLE tables have no Arduino dependency and compile on the host:

    g++ -std=c++11 -o /tmp/test_oui test/test_oui.cpp src/oui_table.cpp -I include && /tmp/test_oui
    g++ -std=c++11 -o /tmp/test_ble_sig test/test_ble_sig.cpp src/ble_sig_table.cpp -I include && /tmp/test_ble_sig

## Known limits

- **One radio.** WiFi and BLE cannot run at once. The firmware alternates
  (`WIFI_PHASE_MS` / `BLE_PHASE_MS`); during each window it is blind to the other.
- **No 5 GHz.** The WROOM-32U is 2.4 GHz only, so it can't see Axon Fleet
  Hub's 5 GHz APs.
- **Cellular-only ALPRs are invisible.** Most non-Flock ALPRs (Vigilant,
  Genetec Cloudrunner, Axon Outpost, ELSAG fixed units) backhaul over LTE. A
  2.4 GHz receiver can't pick that up; the DeFlock map is the better tool for
  those.
- **MAC randomisation.** For OUI matching, randomised addresses are skipped
  because their vendor bits are meaningless. SSID and BLE signature matching
  don't depend on the address, so randomisation doesn't affect them.
- **SSID matching depends on what's actually broadcast.** A wildcard probe
  request or a hidden-SSID beacon carries no SSID bytes at all. Modern phones
  mostly send wildcard probes only, so directed-probe matching increasingly
  only catches older or embedded clients.
- Repeated WiFi/BLE stack init+deinit every few seconds is necessary given the
  single shared radio; watch for heap fragmentation on long runs.

## Tuning

Everything lives in `include/config.h`: phase lengths, channel dwell,
flash-rate endpoints (`RSSI_WEAK`/`RSSI_STRONG`, `FLASH_PERIOD_SLOW`/`FAST`),
alert hold times, and `BUZZER_ENABLED` for silent bench testing.

## License

GPLv3 (see `LICENSE`).

`src/detector.cpp` and `src/main.cpp` carry their own
`SPDX-License-Identifier: MIT` line for portions adapted from
[nyanBOX](https://github.com/jbohack/nyanBOX) (MIT) — the header comment in
each file says what was adapted and what changed. That attribution is a
condition of the MIT license and has to stay.

Signature values (not code) were cross-referenced from
[colonelpanichacks/flock-you](https://github.com/colonelpanichacks/flock-you) (MIT),
[soyboi1312/all-cameras-are-beacons](https://github.com/soyboi1312/all-cameras-are-beacons)
(Apache-2.0), ryanohoro's Flock Falcon research, the Bluetooth SIG assigned
numbers, and the IEEE registry (via Wireshark's `manuf`). Sources are cited per
row in the tables.
