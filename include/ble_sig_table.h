// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "oui_table.h"   // GearCategory

// BLE advertisement-content signatures: a second axis alongside OUI matching.
// Matched against the raw advertising payload (AD structures), NOT the
// address, so they run on every advert regardless of address type - that's
// the point: Flock battery packs, Raven sensors, and newer Axon gear sit on
// random/rotating addresses whose OUI bits mean nothing.
//
// No Arduino dependency - compiles on the host for test/test_ble_sig.cpp.
//
// BLE_SIG_NAME_EXACT        local name equals `text` (case-insensitive)
// BLE_SIG_NAME_PREFIX       local name starts with `text` (case-insensitive)
// BLE_SIG_NAME_PREFIX_DIGITS  `text` prefix followed by 4+ decimal digits
//                           and nothing else, e.g. "Penguin-" -> "Penguin-0123456789"
// BLE_SIG_NAME_PREFIX_HEX   `text` prefix followed by 4+ hex digits and
//                           nothing else, e.g. "FS-" -> "FS-BEC46A"
// BLE_SIG_MFG_ID            manufacturer-specific data (AD 0xFF) company ID == `id16`
// BLE_SIG_SVC16             16-bit service UUID == `id16`, found in a 16-bit
//                           UUID list (AD 0x02/0x03), 16-bit service data
//                           (AD 0x16), or a 128-bit list (AD 0x06/0x07) in
//                           Bluetooth-base-UUID form
// BLE_SIG_SVC128            128-bit service UUID == `uuid128` (written
//                           big-endian, as printed), in a 128-bit list or
//                           128-bit service data (AD 0x21)
// BLE_SIG_PAYLOAD_ASCII     `text` appears as raw bytes anywhere in the
//                           payload, forwards or byte-reversed (a tag carried
//                           inside a little-endian UUID reads backwards)
enum BleSigKind : uint8_t {
    BLE_SIG_NAME_EXACT = 0,
    BLE_SIG_NAME_PREFIX,
    BLE_SIG_NAME_PREFIX_DIGITS,
    BLE_SIG_NAME_PREFIX_HEX,
    BLE_SIG_MFG_ID,
    BLE_SIG_SVC16,
    BLE_SIG_SVC128,
    BLE_SIG_PAYLOAD_ASCII
};

struct BleSigEntry {
    BleSigKind    kind;
    const char*   text;        // NAME_* and PAYLOAD_ASCII; else nullptr
    uint16_t      id16;        // MFG_ID and SVC16; else 0
    const uint8_t* uuid128;    // SVC128 (16 bytes, big-endian); else nullptr
    const char*   vendor;      // shown on the display
    GearCategory  category;
};

// Scans one advertisement (adv data + scan response concatenated, as the
// ESP-IDF scan result delivers it). Returns the first matching table row,
// or nullptr. On a match, writes a short human-readable description of what
// matched into `detail` (always NUL-terminated, truncated to detailLen).
// Every AD length byte is bounds-checked against `len`; a malformed
// structure ends the walk, it is never trusted.
const BleSigEntry* bleSigLookup(const uint8_t* adv, size_t len,
                                char* detail, size_t detailLen);

uint16_t bleSigTableSize();
