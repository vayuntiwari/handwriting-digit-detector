#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern volatile uint8_t img[28][28];

extern volatile bool    frame_ready;

static inline void ImageFrameDone(void) { frame_ready = true; }

#ifdef __cplusplus
}
#endif