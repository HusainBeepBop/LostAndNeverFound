// ============================================================
//  Apple Find My - Public Key (28 bytes)
//
//  MUST match the key in tracker_main/findmy_key.h exactly.
//  The checkpoint derives the tracker BLE MAC from this key.
//
//  HOW TO GENERATE:
//    cd backend/
//    pip install cryptography
//    python generate_keys.py LNF "LNF-MASTER"
//
//  Copy the generated LNF_findmy_key.h here.
// ============================================================

#ifndef FINDMY_KEY_H
#define FINDMY_KEY_H

#include <stdint.h>

static const uint8_t findmy_public_key[28] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#endif
