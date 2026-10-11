#include "sha256.h"

#include <stdint.h>
#include <string.h>

static uint32_t Rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (32u - n));
}

static uint32_t LoadBe32(const unsigned char *p)
{
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static void StoreBe32(unsigned char *p, uint32_t v)
{
    p[0] = (unsigned char)(v >> 24);
    p[1] = (unsigned char)(v >> 16);
    p[2] = (unsigned char)(v >> 8);
    p[3] = (unsigned char)v;
}

static const uint32_t K[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

static void Transform(uint32_t state[8], const unsigned char block[64])
{
    uint32_t w[64];
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
    uint32_t d = 0;
    uint32_t e = 0;
    uint32_t f = 0;
    uint32_t g = 0;
    uint32_t h = 0;
    int i = 0;

    for (i = 0; i < 16; i++) w[i] = LoadBe32(block + i*4);
    for (i = 16; i < 64; i++)
    {
        uint32_t s0 = Rotr(w[i - 15], 7) ^ Rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
        uint32_t s1 = Rotr(w[i - 2], 17) ^ Rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];

    for (i = 0; i < 64; i++)
    {
        uint32_t S1 = Rotr(e, 6) ^ Rotr(e, 11) ^ Rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + S1 + ch + K[i] + w[i];
        uint32_t S0 = Rotr(a, 2) ^ Rotr(a, 13) ^ Rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void Sha256(const void *data, size_t len, unsigned char out[32])
{
    uint32_t state[8] = {
        0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
        0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u
    };
    const unsigned char *bytes = (const unsigned char *)data;
    unsigned char block[64];
    size_t offset = 0;
    uint64_t bits = (uint64_t)len*8ull;

    while (len - offset >= 64)
    {
        Transform(state, bytes + offset);
        offset += 64;
    }

    memset(block, 0, sizeof(block));
    memcpy(block, bytes + offset, len - offset);
    block[len - offset] = 0x80;

    if ((len - offset) >= 56)
    {
        Transform(state, block);
        memset(block, 0, sizeof(block));
    }

    block[56] = (unsigned char)(bits >> 56);
    block[57] = (unsigned char)(bits >> 48);
    block[58] = (unsigned char)(bits >> 40);
    block[59] = (unsigned char)(bits >> 32);
    block[60] = (unsigned char)(bits >> 24);
    block[61] = (unsigned char)(bits >> 16);
    block[62] = (unsigned char)(bits >> 8);
    block[63] = (unsigned char)bits;
    Transform(state, block);

    for (offset = 0; offset < 8; offset++) StoreBe32(out + offset*4, state[offset]);
}

void Sha256Hex(const void *data, size_t len, char out[65])
{
    static const char *hex = "0123456789abcdef";
    unsigned char raw[32];
    int i = 0;

    Sha256(data, len, raw);
    for (i = 0; i < 32; i++)
    {
        out[i*2] = hex[raw[i] >> 4];
        out[i*2 + 1] = hex[raw[i] & 15];
    }
    out[64] = '\0';
}
