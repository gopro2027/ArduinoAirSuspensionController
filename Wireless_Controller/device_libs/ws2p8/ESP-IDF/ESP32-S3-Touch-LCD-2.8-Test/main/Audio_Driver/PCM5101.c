#include "PCM5101.h"

static const char *TAG = "AUDIO PCM5101"; 

static i2s_chan_handle_t i2s_tx_chan; 
static i2s_chan_handle_t i2s_rx_chan; 

uint8_t Volume = Volume_MAX - 2;
bool Music_Next_Flag = 0;
// static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms) {                     // I2S Write Init
//     return i2s_channel_write(i2s_tx_chan, (char *)audio_buffer, len, bytes_written, timeout_ms);
// }
static esp_err_t bsp_i2s_write(void *audio_buffer, size_t len, size_t *bytes_written, uint32_t timeout_ms) {
    int16_t *samples = (int16_t *)audio_buffer;
    size_t sample_count = len / sizeof(int16_t);
    
    // Calculate the volume scaling factor to convert the volume level from 0-100 to the 0.0-1.0 range
    float volume_factor = Volume / 100.0f;


    for (size_t i = 0; i < sample_count; i++) {
        samples[i] = (int16_t)(samples[i] * volume_factor);
    }

    return i2s_channel_write(i2s_tx_chan, (char *)audio_buffer, len, bytes_written, timeout_ms);
}
static esp_err_t bsp_i2s_reconfig_clk(uint32_t rate, uint32_t bits_cfg, i2s_slot_mode_t ch) {                                   // I2S Init
    esp_err_t ret = ESP_OK; 
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(rate),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG((i2s_data_bit_width_t)bits_cfg, ch),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    ret |= i2s_channel_disable(i2s_tx_chan);
    ret |= i2s_channel_reconfig_std_clock(i2s_tx_chan, &std_cfg.clk_cfg);
    ret |= i2s_channel_reconfig_std_slot(i2s_tx_chan, &std_cfg.slot_cfg);
    ret |= i2s_channel_enable(i2s_tx_chan); 
    return ret; 
}

static esp_err_t audio_mute_function(AUDIO_PLAYER_MUTE_SETTING setting) {                                                       // audio mute function
    ESP_LOGI(TAG, "mute setting %d", setting); 
    return ESP_OK; 
}

static esp_err_t bsp_audio_init(const i2s_std_config_t *i2s_config, i2s_chan_handle_t *tx_channel, i2s_chan_handle_t *rx_channel) {     // Audio Init
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(CONFIG_BSP_I2S_NUM, I2S_ROLE_MASTER);
    chan_cfg.auto_clear = true; 
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, tx_channel, rx_channel)); 
    const i2s_std_config_t std_cfg_default = BSP_I2S_DUPLEX_MONO_CFG(22050); 
    const i2s_std_config_t *p_i2s_cfg = (i2s_config != NULL) ? i2s_config : &std_cfg_default; 
    if (tx_channel) {
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(*tx_channel, p_i2s_cfg));
        ESP_ERROR_CHECK(i2s_channel_enable(*tx_channel)); 
    }
    if (rx_channel) {
        ESP_ERROR_CHECK(i2s_channel_init_std_mode(*rx_channel, p_i2s_cfg)); 
        ESP_ERROR_CHECK(i2s_channel_enable(*rx_channel));
    }
    return ESP_OK; 
}

static FILE * Music_File = NULL;
static audio_player_callback_event_t expected_event; 
static QueueHandle_t event_queue; 
static audio_player_callback_event_t event; 

static void audio_player_callback(audio_player_cb_ctx_t *ctx) {
    if (ctx->audio_event == AUDIO_PLAYER_CALLBACK_EVENT_IDLE) {
        ESP_LOGI(TAG, "Playback finished");
        Music_Next_Flag = 1;
    }
    if (ctx->audio_event == expected_event) {
        xQueueSend(event_queue, &(ctx->audio_event), 0);
    }
}

void Audio_Init(void) 
{
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(44100),
        .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
        .gpio_cfg = BSP_I2S_GPIO_CFG,
    };
    esp_err_t ret = bsp_audio_init(&std_cfg, &i2s_tx_chan, &i2s_rx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize audio: %s", esp_err_to_name(ret));
        return;
    }
    audio_player_config_t config = { 
        .mute_fn = audio_mute_function,
        .write_fn = bsp_i2s_write,
        .clk_set_fn = bsp_i2s_reconfig_clk,
        .priority = 3,
        .coreID = 1 
    };
    ret = audio_player_new(config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create audio player: %s", esp_err_to_name(ret));
        return;
    }
    event_queue = xQueueCreate(1, sizeof(audio_player_callback_event_t));
    if (!event_queue) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return;
    }
    ret = audio_player_callback_register(audio_player_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register callback: %s", esp_err_to_name(ret));
        return;
    }
    if (audio_player_get_state() != AUDIO_PLAYER_STATE_IDLE) {
        ESP_LOGE(TAG, "Expected state to be IDLE");                 // The player is not idle
        return;
    }
}
void Play_Music(const char* directory, const char* fileName)
{  
    Music_pause();
    const int maxPathLength = 100; 
    char filePath[maxPathLength];
    if (strcmp(directory, "/") == 0) {                                               
        snprintf(filePath, maxPathLength, "%s%s", directory, fileName);   
    } else {                                                            
        snprintf(filePath, maxPathLength, "%s/%s", directory, fileName);
    }
    Music_File = Open_File(filePath);
    if (!Music_File) {
        ESP_LOGE(TAG, "Failed to open MP3 file: %s", filePath);
        return;
    }

    expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PLAYING;
    esp_err_t ret = audio_player_play(Music_File);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play audio: %s", esp_err_to_name(ret));
        fclose(Music_File);
        return;
    }
    if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
        ESP_LOGE(TAG, "Failed to receive playing event");
        fclose(Music_File);
        return;
    }
    if (audio_player_get_state() != AUDIO_PLAYER_STATE_PLAYING) {
        ESP_LOGE(TAG, "Expected state to be PLAYING");
        fclose(Music_File);
        return;
    }
}
void Music_resume(void)
{
    if (audio_player_get_state() != AUDIO_PLAYER_STATE_PLAYING){
        expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PLAYING;
        esp_err_t ret = audio_player_resume();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to resume audio: %s", esp_err_to_name(ret));
            fclose(Music_File);
            return;
        }
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGE(TAG, "Failed to receive playing event after resume");
            fclose(Music_File);
            return;
        }
        if (audio_player_get_state() != AUDIO_PLAYER_STATE_PLAYING) {
            ESP_LOGE(TAG, "Expected state to be RESUME");
            fclose(Music_File);
            return;
        }
    }
}
void Music_pause(void) 
{
    if (audio_player_get_state() == AUDIO_PLAYER_STATE_PLAYING){
        expected_event = AUDIO_PLAYER_CALLBACK_EVENT_PAUSE;
        esp_err_t ret = audio_player_pause();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to pause audio: %s", esp_err_to_name(ret));
            fclose(Music_File);
            return;
        }
        if (xQueueReceive(event_queue, &event, pdMS_TO_TICKS(100)) != pdPASS) {
            ESP_LOGE(TAG, "Failed to receive pause event");
            fclose(Music_File);
            return;
        }
        if (audio_player_get_state() != AUDIO_PLAYER_STATE_PAUSE) {
            ESP_LOGE(TAG, "Expected state to be PAUSE");
            fclose(Music_File);
            return;
        }
    }
}


void Volume_adjustment(uint8_t Vol) {
    if(Vol > Volume_MAX )
        printf("Audio : The volume value is incorrect. Please enter 0 to 21\r\n");
    else  
        Volume = Vol;
    ESP_LOGI(TAG, "Volume set to %d", Volume);
}
