#pragma once
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
void cnn_setup(void);
void cnn_run_if_frame_ready(void);
int return_digit();
#ifdef __cplusplus
}
#endif