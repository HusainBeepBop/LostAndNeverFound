// ============================================================
//  Apple Find My - Public Key (28 bytes)
//
//  HOW TO GENERATE:
//    cd backend/
//    pip install cryptography
//    python generate_keys.py LNF "LNF-MASTER"
//
//  This creates LNF_findmy_key.h — copy it here to replace
//  this placeholder.  Until then the firmware will warn on
//  boot and broadcast a dummy key that Apple will ignore.
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
