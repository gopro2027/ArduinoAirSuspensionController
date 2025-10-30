#include <Arduino.h>

#include <NimBLEDevice.h>
#include <Wire.h>

// ---- CONFIG ----
#define DEVICE_ID 7        // change for each ESP32: 1, 2, 3, 4, ...
#define ADV_INTERVAL_MS 20 // 20–100 ms recommended
#define ADV_JITTER_MS 5    // random +/- jitter (ms)
#define SLAVE_ADDRESS 0x10 // I2C Indicates the address of the secondary device
#define COMMAND 0x00       // order
#define DATA_LENGTH 9      // data length
unsigned char tflunacommand[] = {0x5A, 0x05, 0x00, 0x01, 0x60};
// -----------------

#define STRINGIFY_IMPL(x) #x
#define STRINGIFY(x) STRINGIFY_IMPL(x)

NimBLEAdvertising *pAdvertising;
NimBLEAdvertisementData advData;
uint16_t lastDistance = 0;
unsigned long lastUpdate = 0;

// ======== TF-Luna I2C Read Function ========
bool readTFLunaDistance(uint16_t &distance, uint16_t &strength, int16_t &temperature)
{
  // Wire.beginTransmission(TFLUNA_ADDR);
  // Wire.write(0x00); // starting register
  // if (Wire.endTransmission(false) != 0)
  // {
  //   return false; // transmission error
  // }

  // Wire.requestFrom(TFLUNA_ADDR, (uint8_t)9);
  // if (Wire.available() < 9)
  //   return false;

  // uint8_t frame[9];
  // for (int i = 0; i < 9; i++)
  //   frame[i] = Wire.read();

  // // Verify frame header 0x59 0x59
  // if (frame[0] != 0x59 || frame[1] != 0x59)
  //   return false;

  // distance = frame[2] | (frame[3] << 8);
  // strength = frame[4] | (frame[5] << 8);
  // return true;
  Wire.beginTransmission(SLAVE_ADDRESS);        // The I2C data transmission starts
  Wire.write(tflunacommand, 5);                 // send instructions
  Wire.endTransmission();                       // The I2C data transfer is complete
  Wire.requestFrom(SLAVE_ADDRESS, DATA_LENGTH); // Request data from an I2C device
  uint8_t data[DATA_LENGTH] = {0};
  int checksum = 0;
  int index = 0;
  while (Wire.available() > 0 && index < DATA_LENGTH)
  {
    data[index++] = Wire.read(); // Read data into an array
  }
  if (index == DATA_LENGTH)
  {
    distance = data[2] + data[3] * 256;    //  DistanceValue
    strength = data[4] + data[5] * 256;    // signal strength
    temperature = data[6] + data[7] * 256; // Chip temperature
    // Serial.print("Distance: ");
    // Serial.print(distance);
    // Serial.print(" cm, strength: ");
    // Serial.print(strength);
    // Serial.print(", temperature: ");
    // Serial.println(temperature / 8.0 - 256.0);
  }
  return true;
}

// ======== SETUP ========
void setup()
{
  Serial.begin(115200);
  Wire.begin(17, 16); // SDA=21, SCL=22 default
  delay(200);
  Serial.printf("Starting TF-Luna BLE advertiser (ID=%d)...\n", DEVICE_ID);

  NimBLEDevice::init("TFLuna_" STRINGIFY(DEVICE_ID));
  NimBLEDevice::setPower(ESP_PWR_LVL_P7); // Max TX power

  pAdvertising = NimBLEDevice::getAdvertising();
  // pAdvertising->setScanResponse(false);
  advData = NimBLEAdvertisementData();

  // BLE intervals in 0.625ms units
  uint16_t intervalUnits = (uint16_t)(ADV_INTERVAL_MS / 0.625);
  pAdvertising->setMinInterval(intervalUnits);
  pAdvertising->setMaxInterval(intervalUnits + 8);
}

// ======== LOOP ========
void loop()
{
  unsigned long now = millis();
  if (now - lastUpdate >= ADV_INTERVAL_MS)
  {
    lastUpdate = now;
    delay(random(0, ADV_JITTER_MS)); // jitter

    uint16_t distance = 0, strength = 0;
    int16_t temperature = 0;
    bool ok = readTFLunaDistance(distance, strength, temperature);

    if (ok)
    {
      lastDistance = distance;
    }
    else
    {
      Serial.println("TF-Luna read error");
    }

    // ---- Encode BLE payload ----
    // [0] = Device ID
    // [1-2] = Distance (uint16)
    // [3-4] = Strength (uint16)
    uint8_t payload[5];
    payload[0] = DEVICE_ID;
    payload[1] = (uint8_t)(lastDistance & 0xFF);
    payload[2] = (uint8_t)(lastDistance >> 8);
    payload[3] = (uint8_t)(strength & 0xFF);
    payload[4] = (uint8_t)(strength >> 8);

    std::string mdata((char *)payload, sizeof(payload));
    advData.clearData();
    advData.setManufacturerData(mdata);
    advData.setServiceData(NimBLEUUID((uint16_t)0x181A), mdata); // Environmental Sensing UUID
    advData.setName("OASMANSENSOR");

    // Update advertising data
    pAdvertising->setAdvertisementData(advData);

    // Restart advertising to push new data
    pAdvertising->start();
    delay(5);
    pAdvertising->stop();

    Serial.printf("Advertised ID=%d  Dist=%u cm  Strength=%u\n",
                  DEVICE_ID, lastDistance, strength);
  }
  else
  {
    delay(1);
  }
}