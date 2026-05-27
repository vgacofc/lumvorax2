/*
 * Test C356 - Version simplifiée sans __constant
 * Test leading zeros sur 1000 nonces
 */

// Constantes SHA-256 K (inline dans le code)
#define K0 0x428a2f98
#define K1 0x71374491
#define K2 0xb5c0fbcf
#define K3 0xe9b5dba5
#define K4 0x3956c25b
#define K5 0x59f111f1
#define K6 0x923f82a4
#define K7 0xab1c5ed5

// Rotation droite
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

// Fonctions SHA-256
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ ((x) >> 10))

// Compte les leading zeros d'un hash Bitcoin (little-endian)
uint count_leading_zeros(__private uint* hash) {
    uint total = 0;
    
    // Parcourir depuis hash[7] (MSB en little-endian) vers hash[0]
    for (int i = 7; i >= 0; i--) {
        uint word = hash[i];
        if (word == 0) {
            total += 32;
        } else {
            total += clz(word);  // clz() compte depuis bit 31 (MSB)
            break;
        }
    }
    
    return total;
}

// SHA-256 simplifié (seulement 8 rounds pour test rapide)
void sha256_simple(__private uint* state, __private const uchar* data, uint len) {
    // Init state
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
    
    // Message schedule (simplifié - seulement 16 mots)
    uint w[16];
    for (int i = 0; i < 16; i++) {
        if (i * 4 < len) {
            w[i] = ((uint)data[i*4] << 24) | 
                   ((uint)data[i*4+1] << 16) |
                   ((uint)data[i*4+2] << 8) |
                   ((uint)data[i*4+3]);
        } else {
            w[i] = 0;
        }
    }
    
    // 8 rounds seulement (au lieu de 64)
    uint a = state[0], b = state[1], c = state[2], d = state[3];
    uint e = state[4], f = state[5], g = state[6], h = state[7];
    
    for (int i = 0; i < 8; i++) {
        uint t1 = h + EP1(e) + CH(e, f, g) + K0 + w[i];
        uint t2 = EP0(a) + MAJ(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

__kernel void test_leading_zeros(
    __global const uchar* block_header,
    uint start_nonce,
    __global uint* results  // [nonce, leading_zeros, hash[8]] * count
) {
    uint gid = get_global_id(0);
    uint nonce = start_nonce + gid;
    
    // Copier header et insérer nonce
    uchar header[80];
    for (int i = 0; i < 76; i++) {
        header[i] = block_header[i];
    }
    header[76] = (nonce >> 24) & 0xFF;
    header[77] = (nonce >> 16) & 0xFF;
    header[78] = (nonce >> 8) & 0xFF;
    header[79] = nonce & 0xFF;
    
    // Premier SHA-256
    uint hash1[8];
    sha256_simple(hash1, header, 80);
    
    // Deuxième SHA-256 (Bitcoin = double SHA-256)
    uint hash2[8];
    uchar hash1_bytes[32];
    for (int i = 0; i < 8; i++) {
        hash1_bytes[i*4+0] = (hash1[i] >> 24) & 0xFF;
        hash1_bytes[i*4+1] = (hash1[i] >> 16) & 0xFF;
        hash1_bytes[i*4+2] = (hash1[i] >> 8) & 0xFF;
        hash1_bytes[i*4+3] = hash1[i] & 0xFF;
    }
    sha256_simple(hash2, hash1_bytes, 32);
    
    // Compter leading zeros
    uint leading_zeros = count_leading_zeros(hash2);
    
    // Écrire résultats
    uint offset = gid * 10;
    results[offset + 0] = nonce;
    results[offset + 1] = leading_zeros;
    for (int i = 0; i < 8; i++) {
        results[offset + 2 + i] = hash2[i];
    }
}
