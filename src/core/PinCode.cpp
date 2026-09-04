#include "PinCode.h"

#include <esp_random.h>

namespace pincode {

namespace {

constexpr uint32_t kFnvOffsetBasis = 2166136261u;
constexpr uint32_t kFnvPrime = 16777619u;
constexpr int kStretchRounds = 2000; // absichtlich langsam gegen simples Ausprobieren

uint32_t fnv1a(const String& s, uint32_t seed) {
    uint32_t h = seed;
    for (size_t i = 0; i < s.length(); ++i) {
        h ^= static_cast<uint8_t>(s[i]);
        h *= kFnvPrime;
    }
    return h;
}

uint32_t stretchedHash(uint32_t salt, const String& code) {
    const String salted = String(salt, HEX) + ":" + code;
    uint32_t h = fnv1a(salted, kFnvOffsetBasis);
    for (int i = 0; i < kStretchRounds; ++i) {
        h = fnv1a(String(h, HEX), h);
    }
    return h;
}

} // namespace

Digest hash(const String& code) {
    Digest d;
    d.salt = esp_random();
    d.value = stretchedHash(d.salt, code);
    return d;
}

bool verify(const String& code, const Digest& stored) {
    return stretchedHash(stored.salt, code) == stored.value;
}

} // namespace pincode
