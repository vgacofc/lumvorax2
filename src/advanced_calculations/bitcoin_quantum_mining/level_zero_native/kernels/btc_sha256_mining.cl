/*
 * LumVorax Bitcoin Mining - Kernel SHA256 Double Hash
 *
 * OBJECTIF : Calculer SHA256(SHA256(block_header)) pour Bitcoin mining
 * 
 * INPUT :
 * - block_header : 80 bytes (version, prev_hash, merkle_root, timestamp, bits, nonce)
 * - target : 32 bytes (difficulté Bitcoin)
 * - nonce_start : uint32_t (nonce de départ)
 * 
 * OUTPUT :
 * - results : uint32_t[batch_size] (1 si hash < target, 0 sinon)
 * - hashes : uint256_t[batch_size] (hashes calculés pour validation)
 */

/* SHA-256 Constants (first 32 bits of fractional parts of cube roots of first 64 primes) */
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

/* SHA-256 Initial Hash Values (first 32 bits of fractional parts of square roots of first 8 primes) */
__constant uint H0[8] = {
    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
};

/* Rotate right */
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

/* SHA-256 Functions */
#define CH(x, y, z)  (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x)       (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x)       (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x)      (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x)      (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

/**
 * SHA-256 Transform (single block)
 */
void sha256_transform(uint* state, const uchar* data) {
    uint a, b, c, d, e, f, g, h, t1, t2, m[64];
    
    /* Prepare message schedule */
    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        m[i] = ((uint)data[j] << 24) | ((uint)data[j + 1] << 16) |
               ((uint)data[j + 2] << 8) | ((uint)data[j + 3]);
    }
    for (int i = 16; i < 64; ++i) {
        m[i] = SIG1(m[i - 2]) + m[i - 7] + SIG0(m[i - 15]) + m[i - 16];
    }
    
    /* Initialize working variables */
    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];
    f = state[5];
    g = state[6];
    h = state[7];
    
    /* Main loop */
    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e, f, g) + K[i] + m[i];
        t2 = EP0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }
    
    /* Add compressed chunk to current hash value */
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

/**
 * SHA-256 Hash (complete)
 */
void sha256_hash(const uchar* data, uint len, uint* hash) {
    uint state[8];
    uchar block[64];
    
    /* Initialize state */
    for (int i = 0; i < 8; i++) {
        state[i] = H0[i];
    }
    
    /* Process full blocks */
    uint blocks = len / 64;
    for (uint i = 0; i < blocks; i++) {
        sha256_transform(state, data + i * 64);
    }
    
    /* Process final block with padding */
    uint remaining = len % 64;
    for (uint i = 0; i < remaining; i++) {
        block[i] = data[blocks * 64 + i];
    }
    block[remaining] = 0x80;  /* Append '1' bit */
    
    /* Pad with zeros */
    for (uint i = remaining + 1; i < 64; i++) {
        block[i] = 0;
    }
    
    /* If not enough space for length, process block and start new one */
    if (remaining >= 56) {
        sha256_transform(state, block);
        for (int i = 0; i < 64; i++) {
            block[i] = 0;
        }
    }
    
    /* Append length in bits (big-endian) */
    ulong bitlen = (ulong)len * 8;
    block[63] = (uchar)(bitlen);
    block[62] = (uchar)(bitlen >> 8);
    block[61] = (uchar)(bitlen >> 16);
    block[60] = (uchar)(bitlen >> 24);
    block[59] = (uchar)(bitlen >> 32);
    block[58] = (uchar)(bitlen >> 40);
    block[57] = (uchar)(bitlen >> 48);
    block[56] = (uchar)(bitlen >> 56);
    
    /* Final transform */
    sha256_transform(state, block);
    
    /* Copy state to output */
    for (int i = 0; i < 8; i++) {
        hash[i] = state[i];
    }
}

/**
 * Bitcoin Mining Kernel
 * 
 * Chaque work-item teste un nonce différent
 */
__kernel void btc_sha256_mining(
    __global const uchar* block_header,  /* 80 bytes */
    __global const uint* target,         /* 8 uint32 (256 bits) */
    uint nonce_start,                    /* Nonce de départ */
    __global uint* results,              /* 1 si trouvé, 0 sinon */
    __global uint* hashes                /* Hashes calculés (8 uint32 par work-item) */
) {
    uint gid = get_global_id(0);
    uint nonce = nonce_start + gid;
    
    /* Copier block header et insérer nonce */
    uchar header[80];
    for (int i = 0; i < 76; i++) {
        header[i] = block_header[i];
    }
    
    /* Nonce à la position 76-79 (little-endian) */
    header[76] = (uchar)(nonce);
    header[77] = (uchar)(nonce >> 8);
    header[78] = (uchar)(nonce >> 16);
    header[79] = (uchar)(nonce >> 24);
    
    /* Premier SHA-256 */
    uint hash1[8];
    sha256_hash(header, 80, hash1);
    
    /* Convertir hash1 en bytes pour second SHA-256 */
    uchar hash1_bytes[32];
    for (int i = 0; i < 8; i++) {
        hash1_bytes[i * 4 + 0] = (uchar)(hash1[i] >> 24);
        hash1_bytes[i * 4 + 1] = (uchar)(hash1[i] >> 16);
        hash1_bytes[i * 4 + 2] = (uchar)(hash1[i] >> 8);
        hash1_bytes[i * 4 + 3] = (uchar)(hash1[i]);
    }
    
    /* Second SHA-256 */
    uint hash2[8];
    sha256_hash(hash1_bytes, 32, hash2);
    
    /* Sauvegarder hash final */
    for (int i = 0; i < 8; i++) {
        hashes[gid * 8 + i] = hash2[i];
    }
    
    /* Comparer avec target (little-endian) */
    bool found = true;
    for (int i = 7; i >= 0; i--) {
        if (hash2[i] > target[i]) {
            found = false;
            break;
        } else if (hash2[i] < target[i]) {
            break;
        }
    }
    
    results[gid] = found ? 1 : 0;
}
