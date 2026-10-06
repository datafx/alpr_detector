// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <stdint.h>
#include "oui_table.h"   // GearCategory

// Each pattern is a case-insensitive prefix: "Flock-" matches "Flock-1A2B3C".
// Plain byte comparison - no wildcards, regex, or dynamic allocation.
struct SsidEntry {
    const char*   pattern;
    const char*   vendor;
    GearCategory  category;
};

// Case-insensitive lookup against the watchlist. `ssid` must be NUL-terminated
// (the caller already bounds-checks and copies the raw IE bytes into a fixed
// buffer before calling this). Returns nullptr when nothing matches, which
// includes an empty string - there's nothing to match against a wildcard
// probe or a hidden-SSID beacon.
const SsidEntry* ssidLookup(const char* ssid);

uint16_t ssidTableSize();
