# ALPR Detector — ESP32-WROOM-32U

Passive BLE + WiFi scanner for ALPR cameras, police surveillance sensors, and
law-enforcement gear. It checks three things:

- **MAC OUIs** against a vendor watchlist (Flock Safety, Genetec, ELSAG/Leonardo,
  ShotSpotter, Axon, Motorola and 40+ LE vendors)
- **SSIDs** in beacons, probe requests and probe responses (e.g. Flock's `Flock-XXXXXX` setup AP)
- **BLE advertisement contents**: device names, company IDs, service UUIDs and
  payload tags (Flock Penguin battery packs, Flock Raven, Axon body cams)

Hits show on an OLED and trigger an LED and buzzer that flash faster as the
signal gets stronger.

Forked 2026-10-06 from [datafx/le_detector](https://github.com/datafx/le_detector),
which is deprecated upstream in favour of
[datafx/le_detector_c5](https://github.com/datafx/le_detector_c5) (adds 5 GHz).
This fork keeps the original WROOM-32U hardware and stays 2.4 GHz-only.

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

2. **Get the code:**

         git clone https://github.com/datafx/le_detector.git
         cd le_detector

   No `git`? Click **Code → Download ZIP** on the GitHub page and extract
   it instead.

3. **Open the project.**
   - VS Code: **File → Open Folder**, select the `le_detector` folder.
     PlatformIO reads `platformio.ini` and configures itself for this
     project automatically — nothing to set up by hand.
   - Command line: just stay inside the `le_detector` directory; every
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
   connect": hold **BOOT**, tap **EN**, release **BOOT**, then retry (see
   Gotchas in `CLAUDE.md` for why this is sometimes needed).

6. **Watch it boot** (optional):

         pio device monitor

   This project doesn't do serial logging during normal operation — a
   deliberate design choice, see `CLAUDE.md` — so past the initial boot
   banner you won't see a running stream of output. That's expected, not
   a sign something's broken; the OLED is the real status display.

### If the board doesn't show up as a serial port

- **Windows/macOS:** you may need the [CP210x USB-to-UART
  driver](https://www.silabs.com/developer-tools/usb-to-uart-bridge-vcp-drivers)
  from Silicon Labs — that's the DevKitC's onboard USB chip.
- **Linux:** the port usually appears automatically (typically
  `/dev/ttyUSB0`), but your user may need group membership to access it
  without `sudo` — `dialout` on Debian/Ubuntu, `uucp` on Arch-based
  distros. Add yourself and re-log-in for it to take effect:

      sudo usermod -aG dialout $USER   # or: uucp, on Arch-based distros

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
| Active buzzer module, 3.3–5V | Must be an *active* module (built-in oscillator, just needs a logic level), not passive/piezo. Check `BUZZER_ACTIVE_LOW` in `config.h` against your specific module — some trigger on LOW (see Gotchas below) |
| U.FL → RP-SMA pigtail | Connects the WROOM-32U's onboard U.FL to an external antenna |
| Alfa APA-M25 (8 dBi directional panel) or equivalent 2.4 GHz antenna | RP-SMA connector |
| Breadboard + jumper wires | Or a PCB — see the EN-pin note below if you're laying one out |
| Micro-USB cable | Power + programming; no external supply needed |

No 5V rail needed anywhere — the buzzer runs off the same 3V3 the DevKitC
outputs on J2 pin 1, and everything else is 3.3V logic.

**Building a PCB instead of breadboarding?** Add a 100nF–1µF ceramic
capacitor between EN and GND. The reference build runs EN bare (just its
onboard pull-up) and hit a burst of spurious resets that pointed at EN-pin
noise on the breadboard's high-impedance wiring — see the reset-loop note in
`CLAUDE.md` for the full writeup. The cap is cheap, standard practice on
ESP32 designs, and worth including on a PCB layout regardless of whether
that was the actual cause on the breadboard.

## Watchlist

80 prefixes across 50+ law-enforcement, ALPR and surveillance vendors (76
compiled in by default — see below). All are IEEE-registered except
`D8:1F:65`, which the registry lists as "Private" and which field capture
attributes to Axon. Any match on the list alerts the same
way — there's no confidence tier, since a weak signal still needs a reaction.
Each entry is tagged by how exclusively its vendor sells to LE (**LE-only**
vs **broad**, meaning it also ships on large volumes of civilian gear) purely
as curation info: broad vendors that turn out to be a false-positive source
get compiled out entirely (see the `EXCLUDE_VENDOR_*` toggles in
`oui_table.cpp` — CradlePoint is off by default for exactly this reason)
rather than downgraded.

Supports /24 (MA-L), /28 (MA-M) and /36 (MA-S) assignments. Prefixes shown with
a trailing `N_` are sub-/24 blocks where only the high nibble of the next byte is
significant. Keep the array sorted ascending; lookup is a binary search. Run
`test/test_oui.cpp` after edits.

Don't want a specific entry? Just comment out its row in `oui_table.cpp` —
removing a row can't break the sort order, so no rebuild flag or macro is
needed. Want to add one locally before submitting it upstream? Drop it in
`user_oui_table.cpp` instead, which is unsorted and just appended to.

### Complete list

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
| `A8:C0:EA` | Pepwave | Veh router | broad |
| `B4:1E:52` | Flock Safety | Plate reader | LE-only |
| `B8:E2:8C` | Motorola Sol | Radio | broad |
| `BC:AD:90` | Kymeta | Veh router | broad |
| `CC:93:4A` | Sierra Wireless | Veh router | broad |
| `D4:11:D6` | ShotSpotter | Surveillance sensor | LE-only |
| `D4:13:F8` | Peplink | Veh router | broad |
| `D8:1F:65` | Axon | Body/car cam | LE-only |
| `E0:DA:DC` | JVC Kenwood | Radio | broad |
| `E4:1E:0A:B_` | Safety Vision | Body/car cam | broad |
| `F4:69:D5:7_` | Rosco Vision | Body/car cam | broad |
| `FC:01:9E` | VieVu | Body/car cam | LE-only |

### Provenance

Every prefix is an IEEE MA-L/MA-M/MA-S assignment, verified against the
registrant's street address (not vendor-name guesses) via an IEEE registry
sweep for known LE equipment makers.

### Deliberately excluded

- **Dell, Panasonic / Panasonic Connect** — Toughbook/laptop MDTs are ubiquitous
  patrol gear, but these prefixes sit on tens of millions of consumer machines,
  so a hit carries no information.
- **WatchGuard Technologies** (`00:01:21`, `00:90:7F`) — Seattle firewall vendor,
  unrelated to WatchGuard Video (Plano TX, police video), which *is* included.
- **Coban SRL** (Italy) — not COBAN Technologies (Houston police video), which has
  no IEEE assignment.
- **TAIT Global LLC** (Lititz PA) — not Tait Electronics (NZ radios), which is
  included.
- Peplink/Pepwave and similar vehicle-router vendors are included but kept at the
  broad tier for the same reason as Sierra Wireless/CradlePoint.

### Still absent from IEEE entirely

Whelen, Code 3, SoundOff Signal, MPH Industries, Getac, and COBAN Technologies
(Houston) have no IEEE assignments — most lightbars and many radar heads aren't
IP devices at all, so there's nothing to match on.

## SSID watchlist

Independent signal from the OUI table, matched against beacon, probe
request, and probe response frames. A client configured for a hidden
network sends its target SSID in cleartext in a directed probe request —
an 802.11 protocol requirement, not a configuration choice — so this
survives MAC address randomisation, a hard limit on OUI matching (see
Known limits below). It's also strictly better evidence than a bare OUI hit
when it fires on a beacon: a CradlePoint OUI just means "some CradlePoint
router," a matched `IBR900-`/`IBR1700-` SSID says which model.

Each entry picks a match mode — **prefix** (`IBR900-` matches
`IBR900-1A2B3C`), **substring** (matches anywhere in the SSID), or
**prefix+substring** (a prefix *and* a second required substring
somewhere after it, e.g. pattern `IBR` + required substring `-mobile`
matches `IBR900-fleet-mobile12` but not `IBR900-fleet` alone — the
two-fragment-with-a-gap shape a glob would write as `IBR*-mobile`) — and
matching is case-insensitive. There's no literal wildcard character or
regex engine, just these three fixed shapes. No IEEE-registry equivalent
exists for SSIDs; every real row should come from an actual field
capture (WiGLE surveys, on-site logging), not a guess.

A probe-request hit is weaker evidence than a beacon/probe-response hit —
a client leaking a network it previously joined isn't proof that network's
AP is nearby now — so the two are tagged differently on the display
(`SSID probe: <name>` vs `SSID beacon: <name>`), though both alert the
same way.

Ships with one live row, `LEDET-TEST`, which is test/demo data, not a real
vendor signature — labeled as such in the source, in this doc, and in the
vendor string itself, so it's unmistakable if it ever fires on the OLED.
Spoof it by renaming a phone hotspot or a saved WiFi profile to
`LEDET-TEST` to exercise the whole detection/display/alert path without
needing real LE gear nearby. Comment it out in `ssid_table.cpp` before
relying on this table for anything real; real entries follow the same
"comment out a row to disable" convention as the OUI table.

### Complete list

| Prefix | Vendor | Category |
|---|---|---|
| `PSP-MVR`  | PSP in-car video   | Body/car cam |
| `PSP-TEST` | PSP facility       | Other |
| `PSPWLAN`  | PSP facility       | Other |
| `PSP_UC`   | PSP facility voice | Other |
| `Flock-`   | Flock Safety setup/health AP | Plate reader |

All four are prefix matches, deliberately — not substring. A substring
`PSP` rule tested against the same survey data matched `PSPPetCenter`,
`PSPRETAIL`, `PSP Neighbor` (a residence), and `BPSPictureU`, all within
730 m of the Hamburg barracks below. The full prefix string, not the
shared `PSP` fragment, is what makes these safe to alert on.

### Provenance

Confirmed via WiGLE survey data cross-checked against known Pennsylvania
State Police barracks coordinates — three independent barracks spanning
282 km across three different troops, every observation 48–132 m from the
building, all on Cisco hardware:

| Barracks | Troop | Coordinates |
|---|---|---|
| Hamburg | L | 40.5641, -76.0025 |
| Dunmore (Scranton area) | — | 41.4357, -75.6151 |
| Bedford | G | 40.0063, -78.3855 |

`PSP-MVR` and `PSP-TEST` were each confirmed at all three sites above.
Cross-site BSSID correlation — matching Cisco OUI and sub-range
assignments across independently surveyed barracks, not just a shared
naming convention — confirms a single statewide deployment rather than
coincidence. `PSP-MVR` (Mobile Video Recorder, PSP's in-car video system)
is the highest-value row, since cruisers are configured for it by
definition, and was last observed 2026-08-25 and 2026-09-05 — current as
of this writing. `PSPWLAN` and `PSP_UC` were observed at Hamburg only, so
carry lower confidence than the other two until cross-checked against a
second site.

**Known limitation**: these are fixed APs at barracks buildings, not
vehicle-mounted equipment. On the road, a hit here means a cruiser's
in-car client is directedly probing for one of these SSIDs while out of
range of the real AP — whether PSP in-car systems actually do that is
**untested in the field**. Treat a hit on this block as a
barracks-proximity indicator, not confirmed mobile detection, until
that's verified.

## BLE signature table

`src/ble_sig_table.cpp` is matched against the raw advertising payload of
**every** advert, whatever its address type. That's deliberate: Flock battery
packs, Raven and newer Axon gear use random or rotating addresses, so their
OUI means nothing. The table is checked top to bottom and the first match
wins. Specific signatures (names, vendor UUIDs) come first and the loose ones
come last (XUNTONG `0x09C8`, the Bluetooth module vendor in Flock's battery
packs, which other products also use). Each row cites its source. To disable a row,
comment it out. Motorola's BLE IDs mostly match two-way radios carried by
retail and venue staff; uncomment `EXCLUDE_VENDOR_MOTOROLA_BLE` to drop them.

The scan is passive, so data that only appears in a scan response is never
received. Every shipped row was reported in the primary advertisement.

Not included on purpose: the Lite-On probe-request OUIs that other projects
use to catch current Falcons acting as WiFi clients. Ordinary laptops send
probes with the same OUIs. Also left out: the bare 10-digit BLE name, which
false-positived on a phone in another project's field testing.

Test off-target:

    g++ -std=c++11 -o /tmp/test_ble_sig test/test_ble_sig.cpp src/ble_sig_table.cpp -I include && /tmp/test_ble_sig

## Known limits

- **No 5 GHz.** The WROOM-32U is 2.4 GHz only, so it can't see Axon Fleet Hub's
  5 GHz APs. `le_detector_c5` exists for that.
- **Cellular-only ALPRs are invisible.** Most non-Flock ALPRs (Vigilant, Genetec
  Cloudrunner, Axon Outpost, ELSAG fixed units) backhaul over LTE. A 2.4 GHz
  receiver can't pick that up; the DeFlock map is the better tool for those.

- **One radio.** WiFi and BLE cannot run at once. The firmware alternates
  (`WIFI_PHASE_MS` / `BLE_PHASE_MS`); during each window it is blind to the other.
- **MAC randomisation.** Only public BLE addresses and non-randomised WiFi MACs
  carry a real OUI. For OUI matching, randomised addresses are skipped because
  their vendor bits are meaningless. SSID matching and BLE signature matching
  don't depend on the address, so randomisation doesn't affect them.
- **PSP SSID rows are barracks-proximity, not confirmed mobile detection.**
  All four are fixed APs at barracks buildings — see SSID watchlist
  Provenance above for the full caveat and why.
- **SSID matching depends on what's actually broadcast.** A wildcard probe
  request or a hidden-SSID beacon carries no SSID bytes at all — nothing to
  match either way. Directed probe requests specifically are a declining
  signal: modern phone OSes (iOS/Android, roughly 2014-2017 onward) mostly
  suppress them (randomised MAC + wildcard probes only), so this
  increasingly only catches older/embedded clients, not phones.
  Beacon/probe-response SSID matching is unaffected by that trend.
- Repeated WiFi/BLE stack init+deinit every few seconds is necessary given the
  single shared radio, but watch for heap fragmentation on long runs.

## Tuning

Everything lives in `include/config.h`: phase lengths, channel dwell,
flash-rate endpoints (`RSSI_WEAK`/`RSSI_STRONG`, `FLASH_PERIOD_SLOW`/`FAST`),
alert hold time, and `BUZZER_ENABLED` for silent bench testing.

## License

GPLv3 (see `LICENSE`), matching the companion `rf_stalker` and
`police_oui_watchlist` projects.

`src/detector.cpp` and `src/main.cpp` carry their own
`SPDX-License-Identifier: MIT` line for portions adapted from third-party
code — see the header comment in each file for attribution, what was
adapted, and what changed. Everything else in this repo is GPLv3.

Signature values (not code) were cross-referenced from
[colonelpanichacks/flock-you](https://github.com/colonelpanichacks/flock-you) (MIT),
[soyboi1312/all-cameras-are-beacons](https://github.com/soyboi1312/all-cameras-are-beacons)
(Apache-2.0), ryanohoro's Flock Falcon research, the Bluetooth SIG assigned
numbers, and the IEEE registry (via Wireshark's `manuf`). Sources are cited per
row in the tables.
