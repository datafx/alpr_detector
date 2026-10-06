// SPDX-License-Identifier: GPL-3.0-only
#include "oui_table.h"
#include <string.h>

// ---------------------------------------------------------------------------
// LAW ENFORCEMENT / ALPR EQUIPMENT WATCHLIST
//
// Every entry is an IEEE registry assignment (MA-L /24, MA-M /28, MA-S /36),
// cross-checked against the registrant's street address. Nothing is from
// memory. The one exception (D8:1F:65) is called out below.
//
// Anchor: 00:25:DF -> "Axon Enterprise, Inc., 17800 N 85th St, Scottsdale AZ".
//
// SPECIFICITY is the important column. SPEC_LE_ONLY vendors sell essentially
// nowhere else, so a hit means something. SPEC_BROAD vendors are real police
// suppliers whose prefixes also ship on enormous volumes of civilian gear.
// Curation metadata only - every match alerts the same way; a SPEC_BROAD
// vendor that proves noisy gets an EXCLUDE_VENDOR_* toggle below.
//
// DELIBERATELY EXCLUDED - name collisions that would cause false alerts:
//   00:01:21, 00:90:7F  WatchGuard Technologies (Seattle) = FIREWALLS, not
//                       WatchGuard Video of Plano TX.
//   24:A3:F0:7x,        Coban SRL (Italy) is NOT COBAN Technologies (Houston,
//   00:1B:C5:0B:4x      police in-car video). The Houston firm has no IEEE
//                       assignment at all.
//   8C:1F:64:A7:8x      TAIT Global LLC (Lititz PA) is not Tait Electronics
//                       (Christchurch NZ), the radio maker.
//   00:58:28, 00:C0:D4, Axon Networks Inc. - a defunct 1990s networking
//   84:70:03            vendor - is not Axon Enterprise (00:25:DF above).
//   D4:63:52            Vutility Inc. (energy metering) is not Utility, Inc.
//                       (BodyWorn) at 00:09:BC/00:16:ED above.
//
// DELIBERATELY EXCLUDED - too broad to carry any signal:
//   Dell, Panasonic  - Toughbook/laptop MDTs are ubiquitous patrol gear, but
//                      these prefixes are on tens of millions of consumer
//                      machines. A hit would mean nothing.
//   Peplink D4:13:F8, Pepwave A8:C0:EA - vehicle routers; false alarms only,
//                      zero true positives in field testing.
//
// ALPR / SURVEILLANCE VENDORS. Checked
// against Wireshark's IEEE-derived manuf file (standards-oui.ieee.org itself
// refuses scripted downloads), with registrant names matching:
//   B4:1E:52           Flock Safety (MA-L). Flock's own block - LE_ONLY.
//   D4:11:D6           ShotSpotter, Inc. (MA-L) - gunshot detection sensors.
//   38:1D:14           Skydio Inc. (MA-L) - drone-as-first-responder fleets.
//                      Skydio left the consumer market in 2023, but the block
//                      also covers industrial/utility inspection drones.
//   D8:1F:65           IEEE registrant is "Private". NOT registry-verified -
//                      attributed to Axon body cams by field capture in
//                      soyboi1312/all-cameras-are-beacons (9 MACs, 8 carrying
//                      the BWCDEVICE BLE tag, owner-confirmed 2026-09-19).
//                      The one exception to the IEEE-only rule above; pull it
//                      if a non-Axon device ever shows up on it.
// ALPR-vendor CANDIDATES. These are registered to ALPR makers, but nobody has shown
// that an ALPR product actually transmits from these ranges. Most ALPRs
// backhaul over cellular and never show up on 2.4 GHz at all, so expect these to be
// quiet. Leonardo/ELSAG's SignalTrace roadside sensors are
// receive-only for their core function; the Elsag Datamat / ELSAG / Selex ES
// rows are the only hook on them, and only if a unit backhauls over WiFi.
//   00:50:C2:BE:70/36  Genetec (IAB)          00:40:DE  Elsag Datamat
//   70:B3:D5:52:10/36  Selex ES Inc.          70:B3:D5:F5:E0/36  Selex ES Inc.
//   94:7B:BE           Ubicquia (streetlight ALPR nodes)
//   00:04:4C, 48:E3:C3 Jenoptik               00:E0:6A  Kapsch
//   04:C3:E6:90/28     Ekin Teknoloji         70:1A:D5  Avigilon Alta
// Genetec AutoVu Cloudrunner (CR-H2) and Axon Outpost are solar + LTE units
// with no published WiFi/BLE signature; the Genetec and Axon rows are the
// only coverage until someone captures one in the field.
//
// NOT IN THE IEEE REGISTRY (nothing to add): Whelen, Code 3, SoundOff Signal,
// MPH Industries, EF Johnson, BK Technologies/RELM, Getac, Utility Associates.
// Most lightbars and many radar heads simply are not IP devices.
//
// MUST STAY SORTED ascending by prefix bytes - lookup is a binary search.
// Table body is generated sorted; re-run test/test_oui.cpp after any edit.
//
// DON'T WANT A SPECIFIC OUI? Just comment out its row below with `//`.
// Deleting a row can't break the sort order or the binary search - the
// remaining rows are still sorted, there are just fewer of them. No macro,
// no rebuild flag, nothing else to touch. (The EXCLUDE_VENDOR_* toggles
// below are for the opposite direction: flipping a whole vendor off by
// default for everyone who builds this, without deleting it from the
// source.) Adding a new OUI here means finding its correct sorted spot by
// hand, then adding a matching case to test/test_oui.cpp.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Per-vendor compile-time excludes
//
// These are real LE suppliers (SPEC_BROAD), but their prefixes also ship on
// huge volumes of civilian fleet/telematics gear. Comment a line back out
// before `pio run` to bring that vendor back into your build. Removing
// entries doesn't affect the table's sort order or the binary search, so
// nothing else needs to change.
//
// CRADLEPOINT IS EXCLUDED BY DEFAULT: field testing turned up zero true
// positives on PD vehicles and a steady stream of false positives - school
// buses and cellular PTZ cameras on the turnpike, both ordinary CradlePoint
// fleet-router customers. It's real LE gear too,
// so the toggle is here, not a table deletion - only re-enable it (comment
// the #define back out) if you've independently confirmed CradlePoint gear
// on vehicles you care about detecting; on unfiltered general-purpose
// patrol, its false-positive rate outweighs its signal.
#define EXCLUDE_VENDOR_CRADLEPOINT
// #define EXCLUDE_VENDOR_SIERRA_WIRELESS
// #define EXCLUDE_VENDOR_MOTOROLA
// ---------------------------------------------------------------------------

static const OuiEntry OUI_TABLE[] = {
  { { 0x00,0x00,0xC3,0x00,0x00 }, 24, "Harris",           CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x04,0x4C,0x00,0x00 }, 24, "Jenoptik",         CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate - see block comment
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x00,0x04,0x7D,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0x00,0x06,0xEC,0x00,0x00 }, 24, "Harris",           CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x08,0xB8,0x00,0x00 }, 24, "EF Johnson",       CAT_RADIO,     SPEC_LE_ONLY    },
  { { 0x00,0x09,0xBC,0x00,0x00 }, 24, "Utility Inc",      CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x0A,0x3E,0x00,0x00 }, 24, "EADS Telecom",     CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x0D,0x4F,0x00,0x00 }, 24, "Kenwood",          CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x0D,0xCA,0x00,0x00 }, 24, "Tait",             CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x0E,0x06,0x00,0x00 }, 24, "Team Simoco",      CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x12,0xE0,0x00,0x00 }, 24, "Codan",            CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x14,0x3E,0x00,0x00 }, 24, "AirLink",          CAT_VEHICLE,   SPEC_BROAD      },
  { { 0x00,0x14,0x91,0x00,0x00 }, 24, "Codan Radio",      CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x16,0xED,0x00,0x00 }, 24, "Utility Inc",      CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x17,0x28,0x00,0x00 }, 24, "Selex Comms",      CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x17,0x3D,0x00,0x00 }, 24, "Neology",          CAT_ALPR,      SPEC_BROAD      },
  { { 0x00,0x17,0xF3,0x00,0x00 }, 24, "Harris",           CAT_RADIO,     SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x00,0x18,0x85,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0x00,0x1A,0x08,0x00,0x00 }, 24, "Simoco",           CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x1C,0x3C,0x00,0x00 }, 24, "Seon Design",      CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x1D,0x96,0x00,0x00 }, 24, "WatchGuard Vid",   CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x1E,0x96,0x00,0x00 }, 24, "Sepura",           CAT_RADIO,     SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x00,0x1F,0x92,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0x00,0x1F,0x9C,0x00,0x00 }, 24, "Havis",            CAT_VEHICLE,   SPEC_BROAD      },
  { { 0x00,0x22,0xAF,0x00,0x00 }, 24, "Safety Vision",    CAT_BODYCAM,   SPEC_BROAD      },
  { { 0x00,0x23,0xB9,0x00,0x00 }, 24, "Airbus D&S",       CAT_RADIO,     SPEC_BROAD      },
  { { 0x00,0x23,0xBD,0x00,0x00 }, 24, "Digital Ally",     CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x24,0x39,0x00,0x00 }, 24, "Digital Barr",     CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x24,0xE6,0x00,0x00 }, 24, "In Motion Tech",   CAT_VEHICLE,   SPEC_BROAD      },
  { { 0x00,0x25,0xDF,0x00,0x00 }, 24, "Axon",             CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x00,0x26,0xB3,0x00,0x00 }, 24, "Thales",           CAT_RADIO,     SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_CRADLEPOINT
  { { 0x00,0x30,0x44,0x00,0x00 }, 24, "CradlePoint",      CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x00,0x30,0x7E,0x00,0x00 }, 24, "Redflex",          CAT_ALPR,      SPEC_LE_ONLY    },
  { { 0x00,0x40,0xDE,0x00,0x00 }, 24, "Elsag Datamat",    CAT_ALPR,      SPEC_BROAD      },  // ALPR/SignalTrace candidate
  { { 0x00,0x50,0xC2,0xBE,0x70 }, 36, "Genetec",          CAT_ALPR,      SPEC_BROAD      },  // IAB
  { { 0x00,0x90,0xC7,0x00,0x00 }, 24, "Icom",             CAT_RADIO,     SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0x00,0xA0,0xD5,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x00,0xBF,0x15,0x00,0x00 }, 24, "Genetec",          CAT_ALPR,      SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_CRADLEPOINT
  { { 0x00,0xE0,0x1C,0x00,0x00 }, 24, "CradlePoint",      CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x00,0xE0,0x6A,0x00,0x00 }, 24, "Kapsch",           CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate
  { { 0x04,0xC3,0xE6,0x90,0x00 }, 28, "Ekin",             CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate
  { { 0x08,0x3C,0x03,0x00,0x00 }, 28, "Federal Signal",   CAT_LIGHTBAR,  SPEC_LE_ONLY    },
  { { 0x0C,0xBF,0x15,0x00,0x00 }, 24, "Genetec",          CAT_ALPR,      SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x10,0x74,0x6F,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0x1C,0x82,0x59,0xD0,0x00 }, 28, "Stalker Radar",    CAT_RADAR,     SPEC_LE_ONLY    },
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0x28,0xA3,0x31,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x38,0x1D,0x14,0x00,0x00 }, 24, "Skydio",           CAT_DRONE,     SPEC_BROAD      },  // DFR drones; also enterprise/industrial
  { { 0x38,0x73,0xEA,0x00,0x00 }, 28, "L3 MobileVis",     CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x48,0x46,0x8D,0x00,0x00 }, 24, "Zepcam",           CAT_BODYCAM,   SPEC_LE_ONLY    },
  { { 0x48,0xE3,0xC3,0x00,0x00 }, 24, "Jenoptik",         CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x4C,0xCC,0x34,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0x50,0x13,0x9D,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x58,0x94,0xCF,0x00,0x00 }, 24, "Vertex Std LMR",   CAT_RADIO,     SPEC_BROAD      },
  { { 0x58,0xE8,0x76,0xC0,0x00 }, 28, "Kustom Signals",   CAT_RADAR,     SPEC_LE_ONLY    },
  { { 0x64,0x69,0xBC,0x00,0x00 }, 24, "Hytera",           CAT_RADIO,     SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0x64,0xCE,0x6E,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x68,0xDA,0x73,0xB0,0x00 }, 28, "Gamber-Johnson",   CAT_VEHICLE,   SPEC_BROAD      },
  { { 0x6C,0x18,0x11,0x00,0x00 }, 24, "Decatur Elec",     CAT_RADAR,     SPEC_LE_ONLY    },
  { { 0x70,0x1A,0xD5,0x00,0x00 }, 24, "Avigilon Alta",    CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate
  { { 0x70,0xB3,0xD5,0x1C,0x50 }, 36, "ELSAG",            CAT_ALPR,      SPEC_LE_ONLY    },
  { { 0x70,0xB3,0xD5,0x52,0x10 }, 36, "Selex ES",         CAT_ALPR,      SPEC_BROAD      },  // ELSAG/SignalTrace candidate
  { { 0x70,0xB3,0xD5,0x88,0xA0 }, 36, "Perceptics",       CAT_ALPR,      SPEC_LE_ONLY    },
  { { 0x70,0xB3,0xD5,0xF5,0xE0 }, 36, "Selex ES",         CAT_ALPR,      SPEC_BROAD      },  // ELSAG/SignalTrace candidate
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0x84,0xDB,0x2F,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0x94,0x7B,0xBE,0x00,0x00 }, 24, "Ubicquia",         CAT_ALPR,      SPEC_BROAD      },  // ALPR candidate
  { { 0x9C,0x06,0x6E,0x00,0x00 }, 24, "Hytera",           CAT_RADIO,     SPEC_BROAD      },
  { { 0x9C,0x83,0xBF,0x00,0x00 }, 24, "PRO-VISION",       CAT_BODYCAM,   SPEC_LE_ONLY    },
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0x9C,0x86,0x2B,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0xB4,0x1E,0x52,0x00,0x00 }, 24, "Flock Safety",     CAT_ALPR,      SPEC_LE_ONLY    },
#ifndef EXCLUDE_VENDOR_MOTOROLA
  { { 0xB8,0xE2,0x8C,0x00,0x00 }, 24, "Motorola Sol",     CAT_RADIO,     SPEC_BROAD      },
#endif
  { { 0xBC,0xAD,0x90,0x00,0x00 }, 24, "Kymeta",           CAT_VEHICLE,   SPEC_BROAD      },
#ifndef EXCLUDE_VENDOR_SIERRA_WIRELESS
  { { 0xCC,0x93,0x4A,0x00,0x00 }, 24, "Sierra Wireless",  CAT_VEHICLE,   SPEC_BROAD      },
#endif
  { { 0xD4,0x11,0xD6,0x00,0x00 }, 24, "ShotSpotter",      CAT_SENSOR,    SPEC_LE_ONLY    },
  { { 0xD8,0x1F,0x65,0x00,0x00 }, 24, "Axon",             CAT_BODYCAM,   SPEC_LE_ONLY    },  // IEEE "Private"; field-attributed, see block comment
  { { 0xE0,0xDA,0xDC,0x00,0x00 }, 24, "JVC Kenwood",      CAT_RADIO,     SPEC_BROAD      },
  { { 0xE4,0x1E,0x0A,0xB0,0x00 }, 28, "Safety Vision",    CAT_BODYCAM,   SPEC_BROAD      },
  { { 0xF4,0x69,0xD5,0x70,0x00 }, 28, "Rosco Vision",     CAT_BODYCAM,   SPEC_BROAD      },
  { { 0xFC,0x01,0x9E,0x00,0x00 }, 24, "VieVu",            CAT_BODYCAM,   SPEC_LE_ONLY    },
};

static const uint16_t OUI_COUNT = sizeof(OUI_TABLE) / sizeof(OUI_TABLE[0]);

uint16_t ouiTableSize() { return OUI_COUNT; }

static int cmp3(const uint8_t* a, const uint8_t* b) {
    for (uint8_t i = 0; i < 3; i++) {
        if (a[i] != b[i]) return (int)a[i] - (int)b[i];
    }
    return 0;
}

// Does mac fall inside this entry's assignment?
static bool matches(const OuiEntry& e, const uint8_t mac[6]) {
    if (cmp3(mac, e.prefix) != 0) return false;
    if (e.bits == 24) return true;
    if (e.bits == 28) return (mac[3] & 0xF0) == (e.prefix[3] & 0xF0);
    if (e.bits == 36) return mac[3] == e.prefix[3] &&
                             (mac[4] & 0xF0) == (e.prefix[4] & 0xF0);
    return false;
}

const OuiEntry* ouiLookup(const uint8_t mac[6]) {
    int lo = 0, hi = (int)OUI_COUNT - 1, found = -1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        int c = cmp3(mac, OUI_TABLE[mid].prefix);
        if (c == 0) { found = mid; break; }
        if (c < 0)  hi = mid - 1;
        else        lo = mid + 1;
    }
    if (found < 0) return nullptr;

    // Several vendors can share the same three bytes when a block is carved
    // into /28 or /36 assignments, so walk the whole run.
    int start = found;
    while (start > 0 && cmp3(OUI_TABLE[start - 1].prefix, OUI_TABLE[found].prefix) == 0) start--;

    for (int i = start; i < (int)OUI_COUNT; i++) {
        if (cmp3(OUI_TABLE[i].prefix, mac) != 0) break;
        if (matches(OUI_TABLE[i], mac)) return &OUI_TABLE[i];
    }
    return nullptr;
}
