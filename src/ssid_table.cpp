// SPDX-License-Identifier: GPL-3.0-only
#include "ssid_table.h"

// ---------------------------------------------------------------------------
// SSID WATCHLIST
//
// Independent signal from the OUI table, matched against beacon, probe
// request and probe response frames. A client configured for a hidden
// network must send directed probe requests containing that SSID in
// cleartext (802.11 protocol requirement, not a config choice), so this
// survives MAC address randomisation.
//
// No registry equivalent to the OUI table's IEEE lookup exists for SSIDs -
// every real row should come from an actual observed network name, not a
// guess. Keep patterns as specific as the observed SSID allows; a short,
// generic prefix risks matching consumer networks.
//
// Comment out any row to disable it - plain linear scan, no sort order.
// ---------------------------------------------------------------------------
static const SsidEntry SSID_TABLE[] = {
    // FLOCK SAFETY - Falcon/Sparrow cameras stand up a setup/health SoftAP
    // named "Flock-" + the last bytes of the MAC (built in the camera
    // firmware's WifiApService as "Flock-" + MAC).
    // Sources: ryanohoro "Spotting Flock Safety's Falcon Cameras", GainSec
    // WiFi research, and colonelpanichacks/flock-you's firmware-dump notes.
    // Field reports say the AP was largely switched off around Dec 2025, so
    // older or unpatched units are what this catches; the BLE signatures in
    // ble_sig_table.cpp cover the battery packs. Anchored prefix - a bare
    // "flock" substring would match any "...Flock..." home network.
    { "Flock-",       "Flock Safety",                   CAT_ALPR  },

    // TEST/DEMO ONLY - NOT A REAL VENDOR SIGNATURE. Rename a phone hotspot to
    // "ALPRDET-TEST" (or anything starting with it) to exercise the
    // display/alert path end to end without real gear nearby. The vendor
    // string is what renders on the OLED, so it says TEST/DEMO too.
    { "ALPRDET-TEST", "TEST/DEMO SSID - not real gear", CAT_OTHER },
};

static const uint16_t SSID_TABLE_SIZE =
    sizeof(SSID_TABLE) / sizeof(SSID_TABLE[0]);

static char lowerAscii(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c + 32) : c;
}

static bool ciStartsWith(const char* text, const char* prefix) {
    while (*prefix) {
        if (*text == '\0') return false;
        if (lowerAscii(*text) != lowerAscii(*prefix)) return false;
        text++;
        prefix++;
    }
    return true;
}

const SsidEntry* ssidLookup(const char* ssid) {
    if (!ssid || ssid[0] == '\0') return nullptr;

    for (uint16_t i = 0; i < SSID_TABLE_SIZE; i++) {
        if (ciStartsWith(ssid, SSID_TABLE[i].pattern)) return &SSID_TABLE[i];
    }
    return nullptr;
}

uint16_t ssidTableSize() { return SSID_TABLE_SIZE; }
