#ifndef VORAX_3D_VOLUME_H
#define VORAX_3D_VOLUME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float* data;
    size_t depth;
    size_t height;
    size_t width;
} vorax_volume3d_t;

int vorax_volume3d_validate(const vorax_volume3d_t* vol);
int vorax_volume3d_normalize(vorax_volume3d_t* vol);
int vorax_volume3d_threshold(const vorax_volume3d_t* vol, float threshold, uint8_t* out_mask, size_t out_len);

#ifdef __cplusplus
}
#endif

#endif
