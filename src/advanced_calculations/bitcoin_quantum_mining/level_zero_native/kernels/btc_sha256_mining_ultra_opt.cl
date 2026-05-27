// LumVorax Bitcoin Mining - Kernel SHA256 ULTRA OPTIMISE Gen9
// FORCE SIMD8 + GRF64 + Scratch minimal

__constant uint K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

__constant uint H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)      (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

// SHA256 transform inline (pas de fonction separee = moins de stack)
#define SHA256_TRANSFORM(state, data) \
{ \
    uint a = state[0], b = state[1], c = state[2], d = state[3]; \
    uint e = state[4], f = state[5], g = state[6], h = state[7]; \
    uint w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10, w11, w12, w13, w14, w15; \
    uint t1, t2; \
    w0 = data[0]; t1 = h + EP1(e) + CH(e,f,g) + K[0] + w0; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w1 = data[1]; t1 = h + EP1(e) + CH(e,f,g) + K[1] + w1; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w2 = data[2]; t1 = h + EP1(e) + CH(e,f,g) + K[2] + w2; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w3 = data[3]; t1 = h + EP1(e) + CH(e,f,g) + K[3] + w3; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w4 = data[4]; t1 = h + EP1(e) + CH(e,f,g) + K[4] + w4; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w5 = data[5]; t1 = h + EP1(e) + CH(e,f,g) + K[5] + w5; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w6 = data[6]; t1 = h + EP1(e) + CH(e,f,g) + K[6] + w6; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w7 = data[7]; t1 = h + EP1(e) + CH(e,f,g) + K[7] + w7; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w8 = data[8]; t1 = h + EP1(e) + CH(e,f,g) + K[8] + w8; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w9 = data[9]; t1 = h + EP1(e) + CH(e,f,g) + K[9] + w9; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w10 = data[10]; t1 = h + EP1(e) + CH(e,f,g) + K[10] + w10; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w11 = data[11]; t1 = h + EP1(e) + CH(e,f,g) + K[11] + w11; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w12 = data[12]; t1 = h + EP1(e) + CH(e,f,g) + K[12] + w12; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w13 = data[13]; t1 = h + EP1(e) + CH(e,f,g) + K[13] + w13; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w14 = data[14]; t1 = h + EP1(e) + CH(e,f,g) + K[14] + w14; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w15 = data[15]; t1 = h + EP1(e) + CH(e,f,g) + K[15] + w15; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w0 = SIG1(w14) + w9 + SIG0(w1) + w0; t1 = h + EP1(e) + CH(e,f,g) + K[16] + w0; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w1 = SIG1(w15) + w10 + SIG0(w2) + w1; t1 = h + EP1(e) + CH(e,f,g) + K[17] + w1; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w2 = SIG1(w0) + w11 + SIG0(w3) + w2; t1 = h + EP1(e) + CH(e,f,g) + K[18] + w2; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w3 = SIG1(w1) + w12 + SIG0(w4) + w3; t1 = h + EP1(e) + CH(e,f,g) + K[19] + w3; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w4 = SIG1(w2) + w13 + SIG0(w5) + w4; t1 = h + EP1(e) + CH(e,f,g) + K[20] + w4; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w5 = SIG1(w3) + w14 + SIG0(w6) + w5; t1 = h + EP1(e) + CH(e,f,g) + K[21] + w5; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w6 = SIG1(w4) + w15 + SIG0(w7) + w6; t1 = h + EP1(e) + CH(e,f,g) + K[22] + w6; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w7 = SIG1(w5) + w0 + SIG0(w8) + w7; t1 = h + EP1(e) + CH(e,f,g) + K[23] + w7; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w8 = SIG1(w6) + w1 + SIG0(w9) + w8; t1 = h + EP1(e) + CH(e,f,g) + K[24] + w8; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w9 = SIG1(w7) + w2 + SIG0(w10) + w9; t1 = h + EP1(e) + CH(e,f,g) + K[25] + w9; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w10 = SIG1(w8) + w3 + SIG0(w11) + w10; t1 = h + EP1(e) + CH(e,f,g) + K[26] + w10; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w11 = SIG1(w9) + w4 + SIG0(w12) + w11; t1 = h + EP1(e) + CH(e,f,g) + K[27] + w11; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w12 = SIG1(w10) + w5 + SIG0(w13) + w12; t1 = h + EP1(e) + CH(e,f,g) + K[28] + w12; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w13 = SIG1(w11) + w6 + SIG0(w14) + w13; t1 = h + EP1(e) + CH(e,f,g) + K[29] + w13; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w14 = SIG1(w12) + w7 + SIG0(w15) + w14; t1 = h + EP1(e) + CH(e,f,g) + K[30] + w14; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w15 = SIG1(w13) + w8 + SIG0(w0) + w15; t1 = h + EP1(e) + CH(e,f,g) + K[31] + w15; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w0 = SIG1(w14) + w9 + SIG0(w1) + w0; t1 = h + EP1(e) + CH(e,f,g) + K[32] + w0; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w1 = SIG1(w15) + w10 + SIG0(w2) + w1; t1 = h + EP1(e) + CH(e,f,g) + K[33] + w1; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w2 = SIG1(w0) + w11 + SIG0(w3) + w2; t1 = h + EP1(e) + CH(e,f,g) + K[34] + w2; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w3 = SIG1(w1) + w12 + SIG0(w4) + w3; t1 = h + EP1(e) + CH(e,f,g) + K[35] + w3; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w4 = SIG1(w2) + w13 + SIG0(w5) + w4; t1 = h + EP1(e) + CH(e,f,g) + K[36] + w4; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w5 = SIG1(w3) + w14 + SIG0(w6) + w5; t1 = h + EP1(e) + CH(e,f,g) + K[37] + w5; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w6 = SIG1(w4) + w15 + SIG0(w7) + w6; t1 = h + EP1(e) + CH(e,f,g) + K[38] + w6; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w7 = SIG1(w5) + w0 + SIG0(w8) + w7; t1 = h + EP1(e) + CH(e,f,g) + K[39] + w7; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w8 = SIG1(w6) + w1 + SIG0(w9) + w8; t1 = h + EP1(e) + CH(e,f,g) + K[40] + w8; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w9 = SIG1(w7) + w2 + SIG0(w10) + w9; t1 = h + EP1(e) + CH(e,f,g) + K[41] + w9; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w10 = SIG1(w8) + w3 + SIG0(w11) + w10; t1 = h + EP1(e) + CH(e,f,g) + K[42] + w10; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w11 = SIG1(w9) + w4 + SIG0(w12) + w11; t1 = h + EP1(e) + CH(e,f,g) + K[43] + w11; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w12 = SIG1(w10) + w5 + SIG0(w13) + w12; t1 = h + EP1(e) + CH(e,f,g) + K[44] + w12; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w13 = SIG1(w11) + w6 + SIG0(w14) + w13; t1 = h + EP1(e) + CH(e,f,g) + K[45] + w13; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w14 = SIG1(w12) + w7 + SIG0(w15) + w14; t1 = h + EP1(e) + CH(e,f,g) + K[46] + w14; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w15 = SIG1(w13) + w8 + SIG0(w0) + w15; t1 = h + EP1(e) + CH(e,f,g) + K[47] + w15; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w0 = SIG1(w14) + w9 + SIG0(w1) + w0; t1 = h + EP1(e) + CH(e,f,g) + K[48] + w0; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w1 = SIG1(w15) + w10 + SIG0(w2) + w1; t1 = h + EP1(e) + CH(e,f,g) + K[49] + w1; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w2 = SIG1(w0) + w11 + SIG0(w3) + w2; t1 = h + EP1(e) + CH(e,f,g) + K[50] + w2; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w3 = SIG1(w1) + w12 + SIG0(w4) + w3; t1 = h + EP1(e) + CH(e,f,g) + K[51] + w3; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w4 = SIG1(w2) + w13 + SIG0(w5) + w4; t1 = h + EP1(e) + CH(e,f,g) + K[52] + w4; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w5 = SIG1(w3) + w14 + SIG0(w6) + w5; t1 = h + EP1(e) + CH(e,f,g) + K[53] + w5; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w6 = SIG1(w4) + w15 + SIG0(w7) + w6; t1 = h + EP1(e) + CH(e,f,g) + K[54] + w6; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w7 = SIG1(w5) + w0 + SIG0(w8) + w7; t1 = h + EP1(e) + CH(e,f,g) + K[55] + w7; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w8 = SIG1(w6) + w1 + SIG0(w9) + w8; t1 = h + EP1(e) + CH(e,f,g) + K[56] + w8; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w9 = SIG1(w7) + w2 + SIG0(w10) + w9; t1 = h + EP1(e) + CH(e,f,g) + K[57] + w9; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w10 = SIG1(w8) + w3 + SIG0(w11) + w10; t1 = h + EP1(e) + CH(e,f,g) + K[58] + w10; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w11 = SIG1(w9) + w4 + SIG0(w12) + w11; t1 = h + EP1(e) + CH(e,f,g) + K[59] + w11; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w12 = SIG1(w10) + w5 + SIG0(w13) + w12; t1 = h + EP1(e) + CH(e,f,g) + K[60] + w12; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w13 = SIG1(w11) + w6 + SIG0(w14) + w13; t1 = h + EP1(e) + CH(e,f,g) + K[61] + w13; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w14 = SIG1(w12) + w7 + SIG0(w15) + w14; t1 = h + EP1(e) + CH(e,f,g) + K[62] + w14; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    w15 = SIG1(w13) + w8 + SIG0(w0) + w15; t1 = h + EP1(e) + CH(e,f,g) + K[63] + w15; t2 = EP0(a) + MAJ(a,b,c); h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2; \
    state[0] += a; state[1] += b; state[2] += c; state[3] += d; \
    state[4] += e; state[5] += f; state[6] += g; state[7] += h; \
}

__attribute__((intel_reqd_sub_group_size(8)))
__attribute__((reqd_work_group_size(64, 1, 1)))
__kernel void btc_sha256_mining_ultra_opt(
    __global const uint* block_header,
    __global const uint* target,
    uint nonce_start,
    __global uint* results,
    __global uint* hashes
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;
    
    uint header[20];
    for (int i = 0; i < 19; i++) header[i] = block_header[i];
    header[19] = nonce;
    
    uint state1[8] = {H0[0], H0[1], H0[2], H0[3], H0[4], H0[5], H0[6], H0[7]};
    SHA256_TRANSFORM(state1, header);
    
    uint block2[16];
    block2[0] = header[16]; block2[1] = header[17]; block2[2] = header[18]; block2[3] = header[19];
    block2[4] = 0x80000000;
    for (int i = 5; i < 15; i++) block2[i] = 0;
    block2[15] = 640;
    
    SHA256_TRANSFORM(state1, block2);
    
    uint state2[8] = {H0[0], H0[1], H0[2], H0[3], H0[4], H0[5], H0[6], H0[7]};
    uint block3[16];
    for (int i = 0; i < 8; i++) block3[i] = state1[i];
    block3[8] = 0x80000000;
    for (int i = 9; i < 15; i++) block3[i] = 0;
    block3[15] = 256;
    
    SHA256_TRANSFORM(state2, block3);
    
    __global uint* hash_out = hashes + gid * 8;
    for (int i = 0; i < 8; i++) hash_out[i] = state2[i];
    
    uint result = 1;
    for (int i = 7; i >= 0; i--) {
        if (state2[i] > target[i]) { result = 0; break; }
        else if (state2[i] < target[i]) break;
    }
    results[gid] = result;
}

