#pragma once

#include <stdbool.h>
#include "esp_err.h"
#include "esp_idf_version.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float acceleration_mg[3];
    float angular_rate_mdps[3];
    float magnetic_mG[3];
} imu_9dof_data_t;

typedef void* esp_board_handle_t;

esp_err_t bsp_i2s_init(void);

esp_err_t bsp_i2s_deinit(void);

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t* buffer, int buffer_len);

int bsp_get_feed_channel(void);

#ifdef __cplusplus
}
#endif