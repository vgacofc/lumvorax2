#ifndef SAFE_ARITHMETIC_H
#define SAFE_ARITHMETIC_H

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <limits.h>

// Protection overflow multiplication avec check arithmétique
bool safe_multiply_size_t(size_t a, size_t b, size_t* result) {
    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    if (a > SIZE_MAX / b) {
        errno = ERANGE;
        return false;
    }

    *result = a * b;
    return true;
}

// Protection overflow addition
bool safe_add_size_t(size_t a, size_t b, size_t* result) {
    if (a > SIZE_MAX - b) {
        errno = ERANGE;
        return false;
    }

    *result = a + b;
    return true;
}

// Protection overflow soustraction
bool safe_subtract_size_t(size_t a, size_t b, size_t* result) {
    if (b > a) {
        errno = ERANGE;
        return false;
    }

    *result = a - b;
    return true;
}

// Protection overflow multiplication uint64_t
static inline bool safe_multiply_uint64(uint64_t a, uint64_t b, uint64_t* result) {
    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    if (a > UINT64_MAX / b) {
        errno = ERANGE;
        return false;
    }

    *result = a * b;
    return true;
}

// Protection overflow multiplication uint32_t
static inline bool safe_multiply_uint32(uint32_t a, uint32_t b, uint32_t* result) {
    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    if (a > UINT32_MAX / b) {
        errno = ERANGE;
        return false;
    }

    *result = a * b;
    return true;
}

// Protection overflow multiplication int (signed)
static inline bool safe_multiply_int(int a, int b, int* result) {
    if (a == 0 || b == 0) {
        *result = 0;
        return true;
    }

    // Vérifier les cas de débordement pour les entiers signés
    if (a > 0) {
        if (b > 0) {
            if (a > INT_MAX / b) {
                errno = ERANGE;
                return false;
            }
        } else {
            if (b < INT_MIN / a) {
                errno = ERANGE;
                return false;
            }
        }
    } else {
        if (b > 0) {
            if (a < INT_MIN / b) {
                errno = ERANGE;
                return false;
            }
        } else {
            if (a != 0 && b < INT_MAX / a) {
                errno = ERANGE;
                return false;
            }
        }
    }

    *result = a * b;
    return true;
}

// Alias générique safe_multiply() pour compatibilité
#define safe_multiply(a, b, result) _Generic((result), \
    uint64_t*: safe_multiply_uint64, \
    uint32_t*: safe_multiply_uint32, \
    int*: safe_multiply_int, \
    size_t*: safe_multiply_size_t \
)(a, b, result)

#endif // SAFE_ARITHMETIC_H