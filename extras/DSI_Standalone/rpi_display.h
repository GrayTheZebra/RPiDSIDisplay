#pragma once
#include <stdbool.h>
#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
esp_err_t rpi_display_begin(void);
esp_err_t rpi_display_pattern(void);
#ifdef __cplusplus
}
#endif
