#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_afe_sr_iface.h"
#include "esp_afe_sr_models.h"
#include "esp_mn_iface.h"
#include "esp_mn_models.h"
#include "esp_process_sdkconfig.h"
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"

#include "bsp_board.h"

static esp_afe_sr_iface_t* afe_handle = NULL;
static srmodel_list_t* models         = NULL;

extern void sr_detect_action_execute(int cmd_id, int phr_id, char* str, float prob);

void feed_Task(void* arg) {
    esp_afe_sr_data_t* afe_data = arg;
    int audio_chunksize         = afe_handle->get_feed_chunksize(afe_data);
    int nch                     = afe_handle->get_channel_num(afe_data);
    int feed_channel            = bsp_get_feed_channel();
    assert(nch <= feed_channel);
    int16_t* i2s_buff = malloc(audio_chunksize * sizeof(int16_t) * feed_channel);
    assert(i2s_buff);

    while (1) {
        bsp_get_feed_data(false, i2s_buff, audio_chunksize * sizeof(int16_t) * feed_channel);
        afe_handle->feed(afe_data, i2s_buff);
    }

    if (i2s_buff) {
        free(i2s_buff);
        i2s_buff = NULL;
    }

    vTaskDelete(NULL);
}

void detect_Task(void* arg) {
    esp_afe_sr_data_t* afe_data = arg;
    int detect_flag             = 0;
    int afe_chunksize           = afe_handle->get_fetch_chunksize(afe_data);
    char* mn_name               = esp_srmodel_filter(models, ESP_MN_PREFIX, ESP_MN_ENGLISH);

    printf("multinet:%s\n", mn_name);

    esp_mn_iface_t* multinet       = esp_mn_handle_from_name(mn_name);
    model_iface_data_t* model_data = multinet->create(mn_name, 6000);
    int mu_chunksize               = multinet->get_samp_chunksize(model_data);
    assert(mu_chunksize == afe_chunksize);
    // print active speech commands
    multinet->print_active_speech_commands(model_data);

    printf("------------detect start------------\n");

    while (1) {
        afe_fetch_result_t* res = afe_handle->fetch(afe_data);
        if (!res || res->ret_value == ESP_FAIL) {
            printf("fetch error!\n");
            break;
        }

        if (res->wakeup_state == WAKENET_DETECTED) {
            printf("WAKEWORD DETECTED\n");
            multinet->clean(model_data);
        } else if (res->wakeup_state == WAKENET_CHANNEL_VERIFIED) {
            detect_flag = 1;
            printf("AFE_FETCH_CHANNEL_VERIFIED, channel index: %d\n", res->trigger_channel_id);
            // afe_handle->disable_wakenet(afe_data);
            // afe_handle->disable_aec(afe_data);
        }

        if (detect_flag == 1) {
            esp_mn_state_t mn_state = multinet->detect(model_data, res->data);

            if (mn_state == ESP_MN_STATE_DETECTING) {
                continue;
            }

            if (mn_state == ESP_MN_STATE_DETECTED) {
                esp_mn_results_t* mn_result = multinet->get_results(model_data);
                for (int i = 0; i < mn_result->num; i++) {
                    printf(
                        "TOP %d, command_id: %d, phrase_id: %d, string: %s, "
                        "prob: %f\n",
                        i + 1, mn_result->command_id[i], mn_result->phrase_id[i], mn_result->string, mn_result->prob[i]);
                }

                // Take action when detected
                sr_detect_action_execute(mn_result->command_id[0], mn_result->phrase_id[0], mn_result->string, mn_result->prob[0]);
                printf("-----------listening-----------\n");
            }

            if (mn_state == ESP_MN_STATE_TIMEOUT) {
                esp_mn_results_t* mn_result = multinet->get_results(model_data);
                printf("timeout, string:%s\n", mn_result->string);
                afe_handle->enable_wakenet(afe_data);
                detect_flag = 0;
                printf("\n-----------awaits to be waken up-----------\n");
                continue;
            }
        }
    }

    if (model_data) {
        multinet->destroy(model_data);
        model_data = NULL;
    }

    printf("detect exit\n");
    vTaskDelete(NULL);
}

void app_main() {
    models = esp_srmodel_init("model");
    ESP_ERROR_CHECK(bsp_i2s_init());

    afe_config_t afe_config            = AFE_CONFIG_DEFAULT();
    afe_config.wakenet_model_name      = esp_srmodel_filter(models, ESP_WN_PREFIX, NULL);
    afe_config.aec_init                = false;
    afe_config.pcm_config.total_ch_num = 2;
    afe_config.pcm_config.mic_num      = 1;
    afe_config.pcm_config.ref_num      = 1;

    afe_handle                         = (esp_afe_sr_iface_t*)&ESP_AFE_SR_HANDLE;
    esp_afe_sr_data_t* afe_data        = afe_handle->create_from_config(&afe_config);

    xTaskCreatePinnedToCore(&detect_Task, "detect", 8 * 1024, (void*)afe_data, 5, NULL, 1);
    xTaskCreatePinnedToCore(&feed_Task, "feed", 8 * 1024, (void*)afe_data, 5, NULL, 0);
}