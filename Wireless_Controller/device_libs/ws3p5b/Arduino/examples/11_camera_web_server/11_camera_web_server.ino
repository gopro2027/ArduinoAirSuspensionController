#include "esp_camera.h"
#include <WiFi.h>
#define XPOWERS_CHIP_AXP2101
#include "XPowersLib.h"

XPowersPMU power;

#define PWDN_GPIO_NUM -1
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 38
#define SIOD_GPIO_NUM 8
#define SIOC_GPIO_NUM 7

#define Y9_GPIO_NUM 21
#define Y8_GPIO_NUM 39
#define Y7_GPIO_NUM 40
#define Y6_GPIO_NUM 42
#define Y5_GPIO_NUM 46
#define Y4_GPIO_NUM 48
#define Y3_GPIO_NUM 47
#define Y2_GPIO_NUM 45
#define VSYNC_GPIO_NUM 17
#define HREF_GPIO_NUM 18
#define PCLK_GPIO_NUM 41

#define CONFIG_PMU_SDA  8
#define CONFIG_PMU_SCL 7
// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "WSTEST";
const char *password = "waveshare0755";

void startCameraServer();
void setupLedFlash(int pin);

void pmu_init(void)
{
  bool result = power.begin(Wire, AXP2101_SLAVE_ADDRESS, CONFIG_PMU_SDA, CONFIG_PMU_SCL);

  if (result == false) {
    Serial.println("power is not online...");
    while (1) delay(50);
  }

  Serial.printf("getID:0x%x\n", power.getChipID());

  // Set the minimum common working voltage of theZ` PMU VBUS input,
  // below this value will turn off the PMU
  power.setVbusVoltageLimit(XPOWERS_AXP2101_VBUS_VOL_LIM_4V36);

  // Set the maximum current of the PMU VBUS input,
  // higher than this value will turn off the PMU
  power.setVbusCurrentLimit(XPOWERS_AXP2101_VBUS_CUR_LIM_1500MA);

  //BLDO1 IMAX=300mA
  //500~3500mV, 100mV/step,31steps
  power.setBLDO1Voltage(1500);

  //BLDO2 IMAX=300mA
  //500~3500mV, 100mV/step,31steps
  power.setBLDO2Voltage(2800);

  power.enableBLDO1();
  power.enableBLDO2();
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000;
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG;  // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (config.pixel_format == PIXFORMAT_JPEG) {
    if (psramFound()) {
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
    config.fb_count = 2;
#endif
  }

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
}

void loop() {
  // Do nothing. Everything is done in another task by the web server
  delay(10000);
}
