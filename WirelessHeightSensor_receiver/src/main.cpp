#include <Arduino.h>

#include <NimBLEDevice.h>
long lastTime = 0;

class MyAdvertisedDeviceCallbacks : public NimBLEScanCallbacks
{
  void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
  {
    std::string mdata = advertisedDevice->getManufacturerData();
    // std::string sdata = advertisedDevice->getServiceData(NimBLEUUID((uint16_t)0x181A)); // Environmental Sensing
    // Serial.printf("Advertised Device: %s \n", advertisedDevice->toString().c_str());
    if (advertisedDevice->getName() != "OASMANSENSOR")
    {
      return;
    }

    if (mdata.length() == 5)
    { // our expected 1-byte ID + 4-byte int
      uint8_t deviceID = mdata[0];
      uint16_t lastDistance = (uint8_t)mdata[1] | ((uint8_t)mdata[2] << 8);
      uint16_t signalStrength = (uint8_t)mdata[3] | ((uint8_t)mdata[4] << 8);
      int time = millis() - lastTime;
      lastTime = millis();

      Serial.printf("Got data: ID=%u  lastDistance=%u  signalStrength=%u  RSSI=%d  delta-ms=%d\n",
                    deviceID, lastDistance, signalStrength, advertisedDevice->getRSSI(), time);
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting scanner...");
  // esp_wifi_stop();

  NimBLEDevice::setScanDuplicateCacheSize(10);
  NimBLEDevice::init("Central_Receiver");
  NimBLEScan *pScan = NimBLEDevice::getScan();

  pScan->setScanCallbacks(new MyAdvertisedDeviceCallbacks());

  // pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  // NimBLEDevice::whiteListAdd(NimBLEAddress("14:33:5c:38:36:36", BLE_ADDR_PUBLIC));

  pScan->setActiveScan(false); // active = request scan response (fine)
  pScan->setInterval(20);      // ms between scan windows
  pScan->setWindow(20);        // continuous scan (window ≈ interval)
  pScan->setMaxResults(0);
  pScan->setDuplicateFilter(false);
  pScan->start(0, false, false); // 0 = no timeout → continuous
}

void loop()
{
  // NimBLEScan *pScan = NimBLEDevice::getScan();
  // pScan->start(100, false, false); // scan for 1 s
  delay(100);
  // pScan->stop(); // NimBLE requires stop before restart
  // delay(5);      // small gap avoids watchdog reset
  // Serial.println("Restarting scan...");
}