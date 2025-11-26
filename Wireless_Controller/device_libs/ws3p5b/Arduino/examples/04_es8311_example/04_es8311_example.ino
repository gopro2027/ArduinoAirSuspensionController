#include <Arduino.h>
#include "ESP_I2S.h"

#include "esp_check.h"

#include "Wire.h"
#include "es8311.h"


#define I2C_SDA 8
#define I2C_SCL 7

#define I2S_NUM I2S_NUM_0 
#define I2S_MCK_PIN 44    
#define I2S_BCK_PIN 13    
#define I2S_LRCK_PIN 15   
#define I2S_DOUT_PIN 16   
#define I2S_DIN_PIN 14    


#define EXAMPLE_SAMPLE_RATE (16000)
#define EXAMPLE_MCLK_MULTIPLE (256)  // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME (70)

I2SClass i2s;


void setupI2S() {
  i2s.setPins(I2S_BCK_PIN, I2S_LRCK_PIN, I2S_DOUT_PIN, I2S_DIN_PIN, I2S_MCK_PIN);
  // Initialize the I2S bus in standard mode
  if (!i2s.begin(I2S_MODE_STD, EXAMPLE_SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
    Serial.println("Failed to initialize I2S bus!");
    return;
  }
}


static esp_err_t es8311_codec_init(void) {

  es8311_handle_t es_handle = es8311_create(I2C_NUM_0, ES8311_ADDRRES_0);
  ESP_RETURN_ON_FALSE(es_handle, ESP_FAIL, TAG, "es8311 create failed");
  const es8311_clock_config_t es_clk = {
    .mclk_inverted = false,
    .sclk_inverted = false,
    .mclk_from_mclk_pin = true,
    .mclk_frequency = EXAMPLE_MCLK_FREQ_HZ,
    .sample_frequency = EXAMPLE_SAMPLE_RATE
  };

  ESP_ERROR_CHECK(es8311_init(es_handle, &es_clk, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16));
  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");

  return ESP_OK;
}

void setup() {
  uint8_t *wav_buffer;
  size_t wav_size;
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  es8311_codec_init();

  setupI2S();
  Serial.println("I2S Initialized");

  wav_buffer = i2s.recordWAV(2, &wav_size);
  delay(1000);
  Serial.println("I2S playWAV");
  i2s.playWAV(wav_buffer, wav_size);
}

void loop() {
  delay(1000);
}
