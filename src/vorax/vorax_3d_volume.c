#include "vorax_3d_volume.h"

#include <math.h>

int vorax_volume3d_validate(const vorax_volume3d_t* vol) {
    if (!vol || !vol->data) return 0;
    if (vol->depth == 0 || vol->height == 0 || vol->width == 0) return 0;
    if (vol->depth > (SIZE_MAX / vol->height)) return 0;
    if ((vol->depth * vol->height) > (SIZE_MAX / vol->width)) return 0;
    return 1;
}

int vorax_volume3d_normalize(vorax_volume3d_t* vol) {
    if (!vorax_volume3d_validate(vol)) return 0;

    size_t n = vol->depth * vol->height * vol->width;
    float min_v = vol->data[0];
    float max_v = vol->data[0];

    for (size_t i = 1; i < n; i++) {
        float v = vol->data[i];
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
    }

    float range = max_v - min_v;
    if (fabsf(range) < 1e-12f) {
        for (size_t i = 0; i < n; i++) vol->data[i] = 0.0f;
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        vol->data[i] = (vol->data[i] - min_v) / range;
    }
    return 1;
}

int vorax_volume3d_threshold(const vorax_volume3d_t* vol, float threshold, uint8_t* out_mask, size_t out_len) {
    if (!vorax_volume3d_validate(vol) || !out_mask) return 0;

    size_t n = vol->depth * vol->height * vol->width;
    if (out_len < n) return 0;

    for (size_t i = 0; i < n; i++) {
        out_mask[i] = (vol->data[i] > threshold) ? 1 : 0;
    }
    return 1;
}
