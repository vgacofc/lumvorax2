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

#endif // SAFE_ARITHMETIC_H