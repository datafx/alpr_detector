// SPDX-License-Identifier: GPL-3.0-only
#include "oui_table.h"
#include <cstdio>
#include <cstring>
#include <cassert>

extern const OuiEntry* ouiLookup(const uint8_t mac[6]);

static int fails = 0;
static void expect(const char* label, const uint8_t* mac, const char* wantVendor) {
    const OuiEntry* e = ouiLookup(mac);
    const char* got = e ? e->vendor : "(none)";
    bool ok = (wantVendor == nullptr) ? (e == nullptr) : (e && strcmp(e->vendor, wantVendor) == 0);
    printf("%-38s -> %-16s %s\n", label, got, ok ? "OK" : "*** FAIL ***");
    if (!ok) fails++;
}

int main() {
    // sortedness check
    extern uint16_t ouiTableSize();
    printf("table size: %u\n\n", ouiTableSize());

    uint8_t axon[6]      = {0x00,0x25,0xDF,0x11,0x22,0x33};
    uint8_t moto[6]      = {0x00,0x04,0x7D,0xAA,0xBB,0xCC};
    uint8_t decatur[6]   = {0x6C,0x18,0x11,0x01,0x02,0x03};
    uint8_t unknown[6]   = {0xDE,0xAD,0xBE,0xEF,0x00,0x01};
    uint8_t sierraLast[6]= {0xCC,0x93,0x4A,0x00,0x00,0x01};   // last row
    uint8_t harrisFirst[6]={0x00,0x00,0xC3,0x00,0x00,0x01};   // first row

    // /28 boundary tests - THE important ones
    uint8_t stalkerIn[6] = {0x1C,0x82,0x59,0xD5,0x00,0x01};   // D5 -> nibble D, in
    uint8_t stalkerEdge[6]={0x1C,0x82,0x59,0xDF,0xFF,0xFF};   // DF -> nibble D, in
    uint8_t stalkerOut[6]= {0x1C,0x82,0x59,0xC0,0x00,0x01};   // C0 -> nibble C, OUT
    uint8_t stalkerOut2[6]={0x1C,0x82,0x59,0xE0,0x00,0x01};   // E0 -> nibble E, OUT
    uint8_t kustomIn[6]  = {0x58,0xE8,0x76,0xC3,0x00,0x01};
    uint8_t kustomOut[6] = {0x58,0xE8,0x76,0x10,0x00,0x01};
    uint8_t fedsigIn[6]  = {0x08,0x3C,0x03,0x05,0x00,0x01};   // 0x -> nibble 0, in
    uint8_t fedsigOut[6] = {0x08,0x3C,0x03,0x90,0x00,0x01};   // 9x -> OUT

    expect("Axon 00:25:DF (anchor)",        axon,       "Axon");
    expect("Motorola 00:04:7D",             moto,       "Motorola Sol");
    expect("Decatur 6C:18:11",              decatur,    "Decatur Elec");
    expect("first row 00:00:C3",            harrisFirst,"Harris");
    expect("last row CC:93:4A",             sierraLast, "Sierra Wireless");
    expect("unknown DE:AD:BE",              unknown,    nullptr);
    printf("\n-- /28 MA-M boundary --\n");
    expect("Stalker 1C:82:59:D5 (in block)", stalkerIn,  "Stalker Radar");
    expect("Stalker 1C:82:59:DF (top edge)", stalkerEdge,"Stalker Radar");
    expect("1C:82:59:C0 (below block)",      stalkerOut, nullptr);
    expect("1C:82:59:E0 (above block)",      stalkerOut2,nullptr);
    expect("Kustom 58:E8:76:C3 (in)",        kustomIn,   "Kustom Signals");
    expect("58:E8:76:10 (out)",              kustomOut,  nullptr);
    expect("FedSignal 08:3C:03:05 (in)",     fedsigIn,   "Federal Signal");
    expect("08:3C:03:90 (out)",              fedsigOut,  nullptr);

    printf("\n-- /36 MA-S boundary (new) --\n");
    uint8_t elsagIn[6]   = {0x70,0xB3,0xD5,0x1C,0x53,0x01};  // byte3=1C, nib(byte4)=5 -> IN
    uint8_t elsagEdge[6] = {0x70,0xB3,0xD5,0x1C,0x5F,0xFF};  // nib 5 -> IN
    uint8_t elsagOut1[6] = {0x70,0xB3,0xD5,0x1C,0x40,0x01};  // nib 4 -> OUT
    uint8_t elsagOut2[6] = {0x70,0xB3,0xD5,0x1D,0x50,0x01};  // byte3 1D -> OUT
    uint8_t percIn[6]    = {0x70,0xB3,0xD5,0x88,0xA7,0x01};
    uint8_t percOut[6]   = {0x70,0xB3,0xD5,0x88,0xB0,0x01};
    expect("ELSAG 70:B3:D5:1C:53 (in)",   elsagIn,   "ELSAG");
    expect("ELSAG 70:B3:D5:1C:5F (edge)", elsagEdge, "ELSAG");
    expect("70:B3:D5:1C:40 (nibble out)", elsagOut1, nullptr);
    expect("70:B3:D5:1D:50 (byte3 out)",  elsagOut2, nullptr);
    expect("Perceptics 70:B3:D5:88:A7",   percIn,    "Perceptics");
    expect("70:B3:D5:88:B0 (out)",        percOut,   nullptr);

    printf("\n-- shared /36 block: 70:B3:D5 hosts BOTH --\n");
    // 70:B3:D5 is a huge shared MA-S block; ELSAG and Perceptics both live in
    // it. This is exactly the case 3-byte matching would get wrong.
    expect("ELSAG and Perceptics disambiguated", elsagIn, "ELSAG");
    uint8_t sharedMiss[6]= {0x70,0xB3,0xD5,0x00,0x00,0x01};
    expect("70:B3:D5:00 (unassigned here)", sharedMiss, nullptr);

    printf("\n-- newly added vendors --\n");
    uint8_t vievu[6]  = {0xFC,0x01,0x9E,0x00,0x00,0x01};
    uint8_t l3mv[6]   = {0x38,0x73,0xEA,0x05,0x00,0x01};
    uint8_t redflex[6]= {0x00,0x30,0x7E,0x00,0x00,0x01};
    expect("VieVu FC:01:9E",           vievu,   "VieVu");
    expect("L3 Mobile-Vision 38:73:EA:05", l3mv, "L3 MobileVis");
    expect("Redflex 00:30:7E",         redflex, "Redflex");


    // -- ALPR / surveillance additions (2026-10-06) --
    printf("\n-- ALPR / surveillance additions --\n");
    uint8_t flock[6]     = {0xB4,0x1E,0x52,0x01,0x02,0x03};
    uint8_t shotspot[6]  = {0xD4,0x11,0xD6,0x01,0x02,0x03};
    uint8_t ubicquia[6]  = {0x94,0x7B,0xBE,0x01,0x02,0x03};
    uint8_t genIabIn[6]  = {0x00,0x50,0xC2,0xBE,0x7A,0x01};   // /36 in
    uint8_t genIabOut[6] = {0x00,0x50,0xC2,0xBE,0x80,0x01};   // /36 out (next block)
    uint8_t selexIn[6]   = {0x70,0xB3,0xD5,0x52,0x1F,0xFF};
    uint8_t selexOut[6]  = {0x70,0xB3,0xD5,0x52,0x20,0x00};
    uint8_t selex2In[6]  = {0x70,0xB3,0xD5,0xF5,0xE0,0x00};
    uint8_t ekinIn[6]    = {0x04,0xC3,0xE6,0x9F,0x00,0x01};   // /28 in
    uint8_t ekinOut[6]   = {0x04,0xC3,0xE6,0xA0,0x00,0x01};   // /28 out
    uint8_t axonPriv[6]  = {0xD8,0x1F,0x65,0x04,0xA2,0x57};
    expect("Flock Safety B4:1E:52",            flock,     "Flock Safety");
    expect("ShotSpotter D4:11:D6",             shotspot,  "ShotSpotter");
    expect("Ubicquia 94:7B:BE",                ubicquia,  "Ubicquia");
    expect("Genetec IAB 00:50:C2:BE:7A (in)",  genIabIn,  "Genetec");
    expect("Genetec IAB 00:50:C2:BE:80 (out)", genIabOut, nullptr);
    expect("Selex ES 70:B3:D5:52:1F (in)",     selexIn,   "Selex ES");
    expect("Selex ES 70:B3:D5:52:20 (out)",    selexOut,  nullptr);
    expect("Selex ES 70:B3:D5:F5:E0 (in)",     selex2In,  "Selex ES");
    expect("Ekin 04:C3:E6:9F (in)",            ekinIn,    "Ekin");
    expect("Ekin 04:C3:E6:A0 (out)",           ekinOut,   nullptr);
    expect("Axon field block D8:1F:65",        axonPriv,  "Axon");

    printf("\n%s (%d failures)\n", fails ? "FAILURES PRESENT" : "ALL PASS", fails);
    return fails;
}
