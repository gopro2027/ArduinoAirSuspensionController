#include "Wireless.h"


bool WIFI_Connection = 0;
uint8_t WIFI_NUM = 0;
uint8_t BLE_NUM = 0;
bool Scan_finish = 0;
int wifi_scan_number()
{
  printf("/**********WiFi Test**********/\r\n");
  // Set WiFi to station mode and disconnect from an AP if it was previously connected.
  WiFi.mode(WIFI_STA);                           
  WiFi.setSleep(true);     
  // WiFi.scanNetworks will return the number of networks found.
  int count = WiFi.scanNetworks();
  if (count == 0)
  {
    printf("No WIFI device was scanned\r\n");
  }
  else{
    printf("Scanned %d Wi-Fi devices\r\n",count);
  }
  
  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
  WiFi.disconnect(true); 
  WiFi.mode(WIFI_OFF);  
  vTaskDelay(100);         
  printf("/*******WiFi Test Over********/\r\n\r\n");
  return count;
}
int ble_scan_number()
{
  printf("/**********BLE Test**********/\r\n"); 
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setActiveScan(true);

  BLEScanResults* foundDevices = pBLEScan->start(5);  
  vTaskDelay(5000);       
  int count = foundDevices->getCount();
  if (count == 0)
  {
    printf("No Bluetooth device was scanned\r\n");
  }
  else{
    printf("Scanned %d Bluetooth devices\r\n",count);
  }
  pBLEScan->clearResults(); 
  pBLEScan->stop(); 
  BLEDevice::deinit(true); 
  vTaskDelay(100);         
  printf("/**********BLE Test Over**********/\r\n\r\n");
  return count;
}

void WirelessScanTask(void *parameter) {
  WIFI_NUM = wifi_scan_number();
  BLE_NUM = ble_scan_number();
  Scan_finish = 1;
  vTaskDelete(NULL);
}
void Wireless_Test2(){
  xTaskCreatePinnedToCore(
    WirelessScanTask,     
    "WirelessScanTask",   
    4096,                
    NULL,                 
    1,                    
    NULL,                
    0                    
  );
}
