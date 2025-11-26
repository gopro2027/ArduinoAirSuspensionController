
#include "Arduino.h"
#include "WiFiMulti.h"
#include "Audio.h"
#include "SD_MMC.h"
#include "FS.h"

#include "es8311.h"
#include "esp_check.h"
#include "Wire.h"

#define SD_MMC_CLK 11
#define SD_MMC_CMD 10
#define SD_MMC_D0 9

#define I2S_MCLK 44
#define I2S_DOUT 16
#define I2S_BCLK 13
#define I2S_LRC  15

#define I2C_SDA 8
#define I2C_SCL 7

#define EXAMPLE_SAMPLE_RATE (16000)
#define EXAMPLE_MCLK_MULTIPLE (256)  // If not using 24-bit data width, 256 should be enough
#define EXAMPLE_MCLK_FREQ_HZ (EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE)
#define EXAMPLE_VOICE_VOLUME (75)

Audio audio;
WiFiMulti wifiMulti;
String ssid = "";
String password = "";


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
  // ESP_RETURN_ON_ERROR(es8311_sample_frequency_config(es_handle, EXAMPLE_SAMPLE_RATE * EXAMPLE_MCLK_MULTIPLE, EXAMPLE_SAMPLE_RATE), TAG, "set es8311 sample frequency failed");
  ESP_RETURN_ON_ERROR(es8311_voice_volume_set(es_handle, EXAMPLE_VOICE_VOLUME, NULL), TAG, "set es8311 volume failed");
  ESP_RETURN_ON_ERROR(es8311_microphone_config(es_handle, false), TAG, "set es8311 microphone failed");
  return ESP_OK;
}


void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);

  es8311_codec_init();

  SD_MMC.setPins(SD_MMC_CLK, SD_MMC_CMD, SD_MMC_D0);
  if (!SD_MMC.begin("/sdmmc", true, false, 20000))
  {
    esp_rom_printf("Card Mount Failed\n");
    while (1)
    {
    };
  }
  if (ssid != "" || password != ""){
    WiFi.mode(WIFI_STA);
    wifiMulti.addAP(ssid.c_str(), password.c_str());
    wifiMulti.run();
    if (WiFi.status() != WL_CONNECTED) {
      WiFi.disconnect(true);
      wifiMulti.run();
    }
  }

  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, I2S_MCLK);
  audio.setVolume(21);  // 0...21

  audio.connecttoFS(SD_MMC, "1.mp3");
  //  audio.connecttohost("http://www.wdr.de/wdrlive/media/einslive.m3u");
  //  audio.connecttohost("http://somafm.com/wma128/missioncontrol.asx"); //  asx
  //  audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.aac"); //  128k aac
  //  audio.connecttohost("http://mp3.ffh.de/radioffh/hqlivestream.mp3"); //  128k mp3
}

void loop() {
  vTaskDelay(1);
  audio.loop();
  if (Serial.available()) {  // put streamURL in serial monitor
    audio.stopSong();
    String r = Serial.readString();
    r.trim();
    if (r.length() > 5) audio.connecttohost(r.c_str());
    log_i("free heap=%i", ESP.getFreeHeap());
  }
}

// optional
void audio_info(const char *info) {
  Serial.print("info        ");
  Serial.println(info);
}
void audio_id3data(const char *info) {  //id3 metadata
  Serial.print("id3data     ");
  Serial.println(info);
}
void audio_eof_mp3(const char *info) {  //end of file
  Serial.print("eof_mp3     ");
  Serial.println(info);
}
void audio_showstation(const char *info) {
  Serial.print("station     ");
  Serial.println(info);
}
void audio_showstreamtitle(const char *info) {
  Serial.print("streamtitle ");
  Serial.println(info);
}
void audio_bitrate(const char *info) {
  Serial.print("bitrate     ");
  Serial.println(info);
}
void audio_commercial(const char *info) {  //duration in sec
  Serial.print("commercial  ");
  Serial.println(info);
}
void audio_icyurl(const char *info) {  //homepage
  Serial.print("icyurl      ");
  Serial.println(info);
}
void audio_lasthost(const char *info) {  //stream URL played
  Serial.print("lasthost    ");
  Serial.println(info);
}
