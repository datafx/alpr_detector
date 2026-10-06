// SPDX-License-Identifier: GPL-3.0-only
#include "ble_sig_table.h"
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// BLE ADVERTISEMENT SIGNATURE WATCHLIST
//
// Unlike the OUI table, there's no IEEE-style registry that says what a given
// product broadcasts. Bluetooth SIG assignments (company IDs, 16-bit member
// UUIDs) only name the vendor who registered them; everything else comes from
// someone's field capture or firmware teardown. The source is cited per row.
// SIG values were checked 2026-10-06 against the Bluetooth SIG's
// assigned_numbers repo (company_identifiers.yaml, member_uuids.yaml).
//
// ORDER MATTERS: the first matching row wins, so specific signatures (names,
// vendor UUIDs) go before loose ones (a shared-silicon company ID).
//
// Passive scanning (config.h BLE_SCAN_PASSIVE) never receives scan-response
// data, so a signature that only appears in a scan response is unreachable
// here. Every row below was reported in the primary advertisement.
//
// Comment out any row to disable it - linear scan, no sort order.
// ---------------------------------------------------------------------------

// Motorola Solutions' BLE IDs mostly fire on MOTOTRBO-class two-way radios
// carried by retail/school/venue staff, not just police. Uncomment to drop
// them, same as EXCLUDE_VENDOR_MOTOROLA in oui_table.cpp (a separate macro
// because that one is local to oui_table.cpp).
// #define EXCLUDE_VENDOR_MOTOROLA_BLE

// Flock accessory GATT service, from a Flock camera firmware dump
// (colonelpanichacks/flock-you, datasets/firmware_derived_signatures.md).
static const uint8_t UUID_FLOCK_ACCESSORY[16] = {
    0xe8,0xcc,0xbb,0x38, 0x95,0x32, 0x46,0xa8, 0x9f,0xe5, 0x18,0x14,0xdf,0x17,0x2e,0x6f
};

static const BleSigEntry BLE_SIG_TABLE[] = {
    // --- Flock Safety (ALPR) -------------------------------------------
    // Penguin battery packs and health beacons. Names are matched anchored,
    // never as bare substrings: "penguin"/"flock" anywhere would match
    // phones and novelty gadgets.
    //   "FS Ext Battery", "Penguin-" + digits: ryanohoro, "Spotting Flock
    //     Safety's Falcon Cameras"; also in the flock-you firmware dump.
    //   "FS-" + hex: all-cameras-are-beacons field capture (FS-BEC46A,
    //     2026-06). "FS-" is a generic white-label prefix, hence the 4+ hex
    //     tail requirement in bleSigLookup().
    //   "Flock" prefix: the brand string - loosest of the names.
    // The bare 10-digit name ryanohoro documents is deliberately NOT matched:
    // it false-positived on a phone named "0102000000" (all-cameras-are-beacons).
    { BLE_SIG_NAME_PREFIX,        "FS Ext Battery", 0,      nullptr, "Flock Safety", CAT_ALPR },
    { BLE_SIG_NAME_PREFIX_DIGITS, "Penguin-",       0,      nullptr, "Flock Safety", CAT_ALPR },
    { BLE_SIG_NAME_PREFIX_HEX,    "FS-",            0,      nullptr, "Flock Safety", CAT_ALPR },
    { BLE_SIG_NAME_PREFIX,        "Flock",          0,      nullptr, "Flock Safety", CAT_ALPR },
    { BLE_SIG_SVC128,             nullptr,          0,      UUID_FLOCK_ACCESSORY, "Flock Safety", CAT_ALPR },

    // --- Flock Raven (gunshot / audio sensor) ----------------------------
    // Raven-specific GATT services, advertised as 16-bit shorts on the
    // Bluetooth base UUID: 0x3100 GPS, 0x3200 power, 0x3300 network, 0x3400
    // upload, 0x3500 error. Source: all-cameras-are-beacons own field capture
    // (docs/signatures.md); flock-you uses the same range.
    { BLE_SIG_SVC16,              nullptr,          0x3100, nullptr, "Flock Raven",  CAT_SENSOR },
    { BLE_SIG_SVC16,              nullptr,          0x3200, nullptr, "Flock Raven",  CAT_SENSOR },
    { BLE_SIG_SVC16,              nullptr,          0x3300, nullptr, "Flock Raven",  CAT_SENSOR },
    { BLE_SIG_SVC16,              nullptr,          0x3400, nullptr, "Flock Raven",  CAT_SENSOR },
    { BLE_SIG_SVC16,              nullptr,          0x3500, nullptr, "Flock Raven",  CAT_SENSOR },

    // --- Axon ------------------------------------------------------------
    // "BWCDEVICE": body cams self-identify with this ASCII tag inside a
    // 128-bit service UUID (AD 0x21, reads "AXJANUSBWCDEVICE" byte-reversed).
    // Survives Axon's move to rotating addresses. Source: all-cameras-are-
    // beacons field capture, owner-confirmed 2026-09-19.
    { BLE_SIG_PAYLOAD_ASCII,      "BWCDEVICE",      0,      nullptr, "Axon bodycam", CAT_BODYCAM },
    // Bluetooth SIG: company ID 0x034D and member UUIDs 0xFE6B/0xFE6C are
    // "TASER International, Inc."; 0xFC81 is "Axon Enterprise, Inc.". These
    // cover Axon gear without the bodycam tag (Signal units, Fleet, docks).
    { BLE_SIG_SVC16,              nullptr,          0xFC81, nullptr, "Axon",         CAT_BODYCAM },
    { BLE_SIG_SVC16,              nullptr,          0xFE6B, nullptr, "Axon",         CAT_BODYCAM },
    { BLE_SIG_SVC16,              nullptr,          0xFE6C, nullptr, "Axon",         CAT_BODYCAM },
    { BLE_SIG_MFG_ID,             nullptr,          0x034D, nullptr, "Axon",         CAT_BODYCAM },

    // --- Utility Inc. BodyWorn -------------------------------------------
    // Activation remote's Complete Local Name. Source: nitekry
    // nite-oui-collection nRF Connect capture (2025-08), via all-cameras-are-beacons.
    { BLE_SIG_NAME_PREFIX,        "BodyWorn Remote", 0,     nullptr, "Utility Inc",  CAT_BODYCAM },

    // --- Motorola Solutions ----------------------------------------------
    // Bluetooth SIG: company ID 0x04EC "Motorola Solutions"; member UUIDs
    // 0xFD8E "Motorola Solutions", 0xFE04 "Motorola Solutions, Inc.".
    // See the toggle above about false positives.
#ifndef EXCLUDE_VENDOR_MOTOROLA_BLE
    { BLE_SIG_SVC16,              nullptr,          0xFD8E, nullptr, "Motorola Sol", CAT_RADIO },
    { BLE_SIG_SVC16,              nullptr,          0xFE04, nullptr, "Motorola Sol", CAT_RADIO },
    { BLE_SIG_MFG_ID,             nullptr,          0x04EC, nullptr, "Motorola Sol", CAT_RADIO },
#endif

    // --- Loose, last ------------------------------------------------------
    // 0x09C8 is Bluetooth SIG "XUNTONG", the module vendor in Flock's
    // battery packs (ryanohoro). Shared silicon, so other XUNTONG-based
    // products will also match this. Kept as a standalone match by explicit choice
    // (2026-10-06). Comment it out if it turns out to be noisy locally.
    { BLE_SIG_MFG_ID,             nullptr,          0x09C8, nullptr, "Flock? (0x09C8)", CAT_ALPR },
};

static const uint16_t BLE_SIG_COUNT = sizeof(BLE_SIG_TABLE) / sizeof(BLE_SIG_TABLE[0]);

uint16_t bleSigTableSize() { return BLE_SIG_COUNT; }

// ---------------------------------------------------------------------------
// Advertisement parsing
// ---------------------------------------------------------------------------

// Bluetooth base UUID 00000000-0000-1000-8000-00805F9B34FB, little-endian as
// it appears on air, minus the 16-bit slot at bytes 12-13.
static const uint8_t BASE_UUID_LE_LOW[12] = {
    0xfb,0x34,0x9b,0x5f, 0x80,0x00, 0x00,0x80, 0x00,0x10, 0x00,0x00
};

static const uint8_t MAX_IDS = 12;

// Everything the matchers need, pulled out of one walk over the AD structures.
struct ParsedAdv {
    char           name[32];
    uint16_t       svc16[MAX_IDS];
    uint8_t        svc16Count;
    uint16_t       mfg[MAX_IDS];
    uint8_t        mfgCount;
    const uint8_t* uuid128[MAX_IDS];   // little-endian, 16 bytes, points into adv
    uint8_t        uuid128Count;
};

static void addSvc16(ParsedAdv& p, uint16_t v) {
    if (p.svc16Count < MAX_IDS) p.svc16[p.svc16Count++] = v;
}

static void addUuid128(ParsedAdv& p, const uint8_t* le) {
    if (p.uuid128Count < MAX_IDS) p.uuid128[p.uuid128Count++] = le;
    // A base-UUID 128-bit entry is just a 16-bit UUID written long-form.
    if (memcmp(le, BASE_UUID_LE_LOW, 12) == 0 && le[14] == 0 && le[15] == 0) {
        addSvc16(p, (uint16_t)(le[12] | (le[13] << 8)));
    }
}

static void parseAdv(const uint8_t* adv, size_t len, ParsedAdv& p) {
    memset(&p, 0, sizeof(p));
    bool haveComplete = false;

    size_t i = 0;
    while (i < len) {
        uint8_t l = adv[i];
        if (l == 0) break;                   // padding / end of significant part
        if (i + 1 + l > len) break;          // length byte not trusted
        uint8_t type = adv[i + 1];
        const uint8_t* d = &adv[i + 2];
        size_t dlen = l - 1;

        switch (type) {
        case 0x02: case 0x03:                // 16-bit service UUID list
            for (size_t k = 0; k + 1 < dlen; k += 2) addSvc16(p, (uint16_t)(d[k] | (d[k + 1] << 8)));
            break;
        case 0x06: case 0x07:                // 128-bit service UUID list
            for (size_t k = 0; k + 15 < dlen; k += 16) addUuid128(p, &d[k]);
            break;
        case 0x16:                           // service data, 16-bit UUID
            if (dlen >= 2) addSvc16(p, (uint16_t)(d[0] | (d[1] << 8)));
            break;
        case 0x21:                           // service data, 128-bit UUID
            if (dlen >= 16) addUuid128(p, d);
            break;
        case 0xFF:                           // manufacturer data: company ID first
            if (dlen >= 2 && p.mfgCount < MAX_IDS) p.mfg[p.mfgCount++] = (uint16_t)(d[0] | (d[1] << 8));
            break;
        case 0x08: case 0x09: {              // shortened / complete local name
            if (haveComplete && type == 0x08) break;
            size_t n = dlen < sizeof(p.name) - 1 ? dlen : sizeof(p.name) - 1;
            memcpy(p.name, d, n);
            p.name[n] = '\0';
            if (type == 0x09) haveComplete = true;
            break;
        }
        default:
            break;
        }
        i += 1 + l;
    }
}

// ---------------------------------------------------------------------------
// Matchers
// ---------------------------------------------------------------------------

static char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

// Case-insensitive "does text start with prefix"; returns the remainder or nullptr.
static const char* ciPrefix(const char* text, const char* prefix) {
    while (*prefix) {
        if (*text == '\0' || lowerAscii(*text) != lowerAscii(*prefix)) return nullptr;
        text++;
        prefix++;
    }
    return text;
}

static bool allDigits(const char* s, size_t minLen) {
    size_t n = 0;
    for (; *s; s++, n++) if (*s < '0' || *s > '9') return false;
    return n >= minLen;
}

static bool allHex(const char* s, size_t minLen) {
    size_t n = 0;
    for (; *s; s++, n++) {
        char c = lowerAscii(*s);
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    }
    return n >= minLen;
}

static bool bytesContain(const uint8_t* hay, size_t hayLen, const char* needle, bool reversed) {
    size_t n = strlen(needle);
    if (n == 0 || n > hayLen) return false;
    for (size_t i = 0; i + n <= hayLen; i++) {
        size_t k = 0;
        for (; k < n; k++) {
            char want = reversed ? needle[n - 1 - k] : needle[k];
            if ((char)hay[i + k] != want) break;
        }
        if (k == n) return true;
    }
    return false;
}

static bool hasId(const uint16_t* ids, uint8_t count, uint16_t v) {
    for (uint8_t i = 0; i < count; i++) if (ids[i] == v) return true;
    return false;
}

static bool hasUuid128(const ParsedAdv& p, const uint8_t* be) {
    for (uint8_t i = 0; i < p.uuid128Count; i++) {
        uint8_t k = 0;
        for (; k < 16; k++) if (p.uuid128[i][k] != be[15 - k]) break;
        if (k == 16) return true;
    }
    return false;
}

static bool matchEntry(const BleSigEntry& e, const ParsedAdv& p,
                       const uint8_t* adv, size_t len) {
    const char* rest;
    switch (e.kind) {
    case BLE_SIG_NAME_EXACT:
        rest = ciPrefix(p.name, e.text);
        return rest && *rest == '\0';
    case BLE_SIG_NAME_PREFIX:
        return p.name[0] && ciPrefix(p.name, e.text);
    case BLE_SIG_NAME_PREFIX_DIGITS:
        rest = ciPrefix(p.name, e.text);
        return rest && allDigits(rest, 4);
    case BLE_SIG_NAME_PREFIX_HEX:
        rest = ciPrefix(p.name, e.text);
        return rest && allHex(rest, 4);
    case BLE_SIG_MFG_ID:
        return hasId(p.mfg, p.mfgCount, e.id16);
    case BLE_SIG_SVC16:
        return hasId(p.svc16, p.svc16Count, e.id16);
    case BLE_SIG_SVC128:
        return e.uuid128 && hasUuid128(p, e.uuid128);
    case BLE_SIG_PAYLOAD_ASCII:
        return bytesContain(adv, len, e.text, false) || bytesContain(adv, len, e.text, true);
    }
    return false;
}

static void describe(const BleSigEntry& e, const ParsedAdv& p, char* out, size_t outLen) {
    switch (e.kind) {
    case BLE_SIG_NAME_EXACT:
    case BLE_SIG_NAME_PREFIX:
    case BLE_SIG_NAME_PREFIX_DIGITS:
    case BLE_SIG_NAME_PREFIX_HEX:
        snprintf(out, outLen, "name %s", p.name);
        break;
    case BLE_SIG_MFG_ID:
        snprintf(out, outLen, "mfg 0x%04X", e.id16);
        break;
    case BLE_SIG_SVC16:
        snprintf(out, outLen, "svc 0x%04X", e.id16);
        break;
    case BLE_SIG_SVC128:
        snprintf(out, outLen, "svc %02x%02x%02x%02x-...", e.uuid128[0], e.uuid128[1],
                 e.uuid128[2], e.uuid128[3]);
        break;
    case BLE_SIG_PAYLOAD_ASCII:
        snprintf(out, outLen, "tag %s", e.text);
        break;
    }
}

const BleSigEntry* bleSigLookup(const uint8_t* adv, size_t len,
                                char* detail, size_t detailLen) {
    if (detail && detailLen) detail[0] = '\0';
    if (!adv || len == 0) return nullptr;

    ParsedAdv p;
    parseAdv(adv, len, p);

    for (uint16_t i = 0; i < BLE_SIG_COUNT; i++) {
        if (matchEntry(BLE_SIG_TABLE[i], p, adv, len)) {
            if (detail && detailLen) describe(BLE_SIG_TABLE[i], p, detail, detailLen);
            return &BLE_SIG_TABLE[i];
        }
    }
    return nullptr;
}
