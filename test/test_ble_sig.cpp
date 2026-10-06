// SPDX-License-Identifier: GPL-3.0-only
// Off-target test for the BLE advertisement signature table.
//   g++ -std=c++11 -o /tmp/test_ble_sig test/test_ble_sig.cpp src/ble_sig_table.cpp -I include
#include "ble_sig_table.h"
#include <cstdio>
#include <cstring>
#include <vector>

static int fails = 0;

// Builds an advert out of AD structures: each is {type, payload...}.
struct Adv {
    std::vector<uint8_t> b;
    Adv& ad(uint8_t type, const std::vector<uint8_t>& data) {
        b.push_back((uint8_t)(data.size() + 1));
        b.push_back(type);
        b.insert(b.end(), data.begin(), data.end());
        return *this;
    }
    Adv& name(const char* n, uint8_t type = 0x09) {
        return ad(type, std::vector<uint8_t>(n, n + strlen(n)));
    }
};

static void expect(const char* label, const Adv& a, const char* wantVendor) {
    char detail[33];
    const BleSigEntry* e = bleSigLookup(a.b.data(), a.b.size(), detail, sizeof(detail));
    const char* got = e ? e->vendor : "(none)";
    bool ok = (wantVendor == nullptr) ? (e == nullptr) : (e && strcmp(e->vendor, wantVendor) == 0);
    printf("%-44s -> %-16s %-26s %s\n", label, got, detail, ok ? "OK" : "*** FAIL ***");
    if (!ok) fails++;
}

int main() {
    printf("table size: %u\n\n", bleSigTableSize());
    const std::vector<uint8_t> flags = {0x06};

    // --- Flock names ---
    expect("FS Ext Battery",                  Adv().ad(0x01, flags).name("FS Ext Battery"), "Flock Safety");
    expect("fs ext battery (case)",           Adv().name("fs ext battery"),                 "Flock Safety");
    expect("Penguin-0123456789",              Adv().name("Penguin-0123456789"),             "Flock Safety");
    expect("Penguin-12ab (non-digit tail)",   Adv().name("Penguin-12ab"),                   nullptr);
    expect("Penguin-12 (tail too short)",     Adv().name("Penguin-12"),                     nullptr);
    expect("FS-BEC46A",                       Adv().name("FS-BEC46A"),                      "Flock Safety");
    expect("FS-Speaker (not hex)",            Adv().name("FS-Speaker"),                     nullptr);
    expect("Flock (shortened name)",          Adv().name("Flock-1234", 0x08),               "Flock Safety");
    expect("MyFlock (not anchored)",          Adv().name("MyFlock"),                        nullptr);
    expect("bare 10-digit name (excluded)",   Adv().name("0102000000"),                     nullptr);

    // --- Flock accessory 128-bit UUID (little-endian on air) ---
    std::vector<uint8_t> flockUuid = {0x6f,0x2e,0x17,0xdf,0x14,0x18,0xe5,0x9f,
                                      0xa8,0x46,0x32,0x95,0x38,0xbb,0xcc,0xe8};
    expect("Flock accessory svc128",          Adv().ad(0x07, flockUuid),                    "Flock Safety");
    std::vector<uint8_t> flockUuidBE = {0xe8,0xcc,0xbb,0x38,0x95,0x32,0x46,0xa8,
                                        0x9f,0xe5,0x18,0x14,0xdf,0x17,0x2e,0x6f};
    expect("Flock UUID wrong byte order",     Adv().ad(0x07, flockUuidBE),                  nullptr);

    // --- Raven: 16-bit list, and 128-bit base-UUID form ---
    expect("Raven 0x3100 in 16-bit list",     Adv().ad(0x03, {0x0a,0x18, 0x00,0x31}),       "Flock Raven");
    std::vector<uint8_t> raven128 = {0xfb,0x34,0x9b,0x5f,0x80,0x00,0x00,0x80,
                                     0x00,0x10,0x00,0x00,0x00,0x35,0x00,0x00};
    expect("Raven 0x3500 as base-UUID 128",   Adv().ad(0x07, raven128),                     "Flock Raven");
    expect("0x180A alone (std SIG, no hit)",  Adv().ad(0x03, {0x0a,0x18}),                  nullptr);
    expect("0x3600 (outside Raven set)",      Adv().ad(0x03, {0x00,0x36}),                  nullptr);

    // --- Axon ---
    const char* tagRev = "ECIVEDCWBSUNAJXA";   // "AXJANUSBWCDEVICE" reversed, as on air
    expect("Axon BWCDEVICE reversed in 0x21", Adv().ad(0x21, std::vector<uint8_t>(tagRev, tagRev + 16)), "Axon bodycam");
    const char* tagFwd = "xxBWCDEVICE";
    expect("Axon BWCDEVICE forwards",         Adv().ad(0xFF, std::vector<uint8_t>(tagFwd, tagFwd + 11)), "Axon bodycam");
    expect("Axon svc 0xFE6B service data",    Adv().ad(0x16, {0x6b,0xfe, 0x01,0x02}),       "Axon");
    expect("Axon mfg 0x034D",                 Adv().ad(0xFF, {0x4d,0x03, 0xaa}),            "Axon");

    // --- Utility BodyWorn, Motorola ---
    expect("BodyWorn Remote",                 Adv().name("BodyWorn Remote"),                "Utility Inc");
    expect("Motorola mfg 0x04EC",             Adv().ad(0xFF, {0xec,0x04}),                  "Motorola Sol");

    // --- XUNTONG 0x09C8 standalone (loose, last) ---
    expect("mfg 0x09C8 alone",                Adv().ad(0xFF, {0xc8,0x09, 0x00}),            "Flock? (0x09C8)");
    expect("Penguin name + 0x09C8 -> name wins", Adv().name("Penguin-0000000001").ad(0xFF, {0xc8,0x09}), "Flock Safety");

    // --- Negative / malformed ---
    expect("Apple mfg 0x004C",                Adv().ad(0xFF, {0x4c,0x00, 0x10,0x05}),       nullptr);
    expect("empty advert",                    Adv(),                                        nullptr);
    Adv bad; bad.b = {0x1F, 0x09, 'F','l','o','c','k'};   // length byte overruns buffer
    expect("truncated AD (length lies)",      bad,                                          nullptr);
    Adv pad; pad.b = {0x00, 0x00, 0x00};
    expect("zero padding",                    pad,                                          nullptr);

    printf("\n%s (%d failure%s)\n", fails ? "FAILED" : "ALL PASS", fails, fails == 1 ? "" : "s");
    return fails ? 1 : 0;
}
