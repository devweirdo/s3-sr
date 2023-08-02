#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
#include "driver/i2s_std.h"
#include "driver/i2s_tdm.h"
#include "soc/soc_caps.h"
#else
#include "driver/i2s.h"
#endif
#include "driver/gpio.h"

#include "bsp_board.h"

/* I2S Interface Defines */
#define I2S_MASTER_NUM  I2S_NUM_0
#define I2S_MCLK_IO     GPIO_NUM_NC
#define I2S_SCLK_IO     GPIO_NUM_42
#define I2S_WS_IO       GPIO_NUM_40
#define I2S_DSDIN_IO    GPIO_NUM_41
#define I2S_SDOUT_IO    GPIO_NUM_NC

#define ADC_I2S_CHANNEL 2

static const char* TAG = "bsp_board";

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
static i2s_chan_handle_t rx_handle = NULL;  // I2S rx channel handler
#endif

esp_err_t bsp_i2s_init(void) {
    ESP_LOGI(TAG, "i2s init");
    esp_err_t ret_val = ESP_OK;

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_MASTER_NUM, I2S_ROLE_MASTER);
    ret_val |= i2s_new_channel(&chan_cfg, NULL, &rx_handle);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(16000),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(32, I2S_SLOT_MODE_MONO),
        .gpio_cfg =
            {
                       .mclk = I2S_MCLK_IO,
                       .bclk = I2S_SCLK_IO,
                       .ws   = I2S_WS_IO,
                       .din  = I2S_DSDIN_IO,
                       .dout = I2S_SDOUT_IO,
                       .invert_flags =
                       {
                       .mclk_inv = false,
                       .bclk_inv = false,
                       .ws_inv   = false,
                       }, },
    };

    std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT;

    ret_val |= i2s_channel_init_std_mode(rx_handle, &std_cfg);
    ret_val |= i2s_channel_enable(rx_handle);

#else
    i2s_config_t i2s_config = {
        .mode                 = I2S_MODE_MASTER | I2S_MODE_RX,
        .sample_rate          = 16000,
        .bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format       = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count        = 6,
        .dma_buf_len          = 160,
        .use_apll             = false,
        .tx_desc_auto_clear   = true,
        .fixed_mclk           = 0,
        .mclk_multiple        = I2S_MCLK_MULTIPLE_DEFAULT,
        .bits_per_chan        = I2S_BITS_PER_CHAN_32BIT,
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num   = I2S_SCLK_IO,
        .ws_io_num    = I2S_WS_IO,
        .data_out_num = I2S_SDOUT_IO,
        .data_in_num  = I2S_DSDIN_IO,
        .mck_io_num   = I2S_MCLK_IO,
    };

    ret_val |= i2s_driver_install(I2S_MASTER_NUM, &i2s_config, 0, NULL);
    ret_val |= i2s_set_pin(I2S_MASTER_NUM, &pin_config);
#endif
    return ret_val;
}

esp_err_t bsp_i2s_deinit(void) {
    esp_err_t ret_val = ESP_OK;
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret_val |= i2s_channel_disable(rx_handle);
    ret_val |= i2s_del_channel(rx_handle);
    rx_handle = NULL;
#else
    ret_val |= i2s_stop(i2s_num);
    ret_val |= i2s_driver_uninstall(i2s_num);
#endif
    return ret_val;
}

esp_err_t bsp_get_feed_data(bool is_get_raw_channel, int16_t* buffer, int buffer_len) {
    esp_err_t ret = ESP_OK;
    size_t bytes_read;
    int audio_chunksize = buffer_len / (sizeof(int32_t));

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    ret = i2s_channel_read(rx_handle, buffer, buffer_len, &bytes_read, portMAX_DELAY);
#else
    ret = i2s_read(I2S_MASTER_NUM, buffer, buffer_len, &bytes_read, portMAX_DELAY);
#endif

    int32_t* tmp_buff = buffer;
    for (int i = 0; i < audio_chunksize; i++) {
        tmp_buff[i] = tmp_buff[i] >> 14;
        // 32:8 is the effective bit, 8:0 is the lower 8 bits, all are
        // 0, the input of AFE is 16-bit voice data, and 29:13 bits are
        // used to amplify the voice signal.
    }

    return ret;
}

int bsp_get_feed_channel(void) {
    return ADC_I2S_CHANNEL;
}