#include "ble.h"

// Based on this file: https://github.com/mo-thunderz/Esp32BlePart2/blob/main/Arduino/BLE_server_2characteristics/BLE_server_2characteristics.ino

// Initialize all pointers
BLEServer *pServer = NULL;                      // Pointer to the server
BLECharacteristic *statusCharacteristic = NULL; // Pointer to Characteristic 1
BLECharacteristic *restCharacteristic = NULL;   // Pointer to Characteristic 2
BLEDescriptor *pDescr_1;                        // Pointer to Descriptor of Characteristic 1
BLE2902 *pBLE2902_1;                            // Pointer to BLE2902 of Characteristic 1
BLE2902 *pBLE2902_2;                            // Pointer to BLE2902 of Characteristic 2

// Some variables to keep track on device connected
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Variable that will continuously be increased and written to the client
uint32_t value = 0;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "679425c8-d3b4-4491-9eb2-3e3d15b625f0"
#define STATUS_CHARACTERISTIC_UUID "66fda100-8972-4ec7-971c-3fd30b3072ac"
#define REST_CHARACTERISTIC_UUID "f573f13f-b38e-415e-b8f0-59a6a19a4e02"

// Callback function that is called whenever a client is connected or disconnected
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
    }
};

void ble_setup()
{

    // Create the BLE Device
    BLEDevice::init("OASMan");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    ble_create_characteristics(pService);

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

uint64_t currentUserNum = 1;
void ble_loop()
{
    if (value > 5)
    {
        value = 2;
    }
    // notify changed value
    if (deviceConnected)
    {
        ble_notify();
    }
    // The code below keeps the connection status uptodate:
    // Disconnecting
    if (!deviceConnected && oldDeviceConnected)
    {
        delay(500);                  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // Connecting
    if (deviceConnected && !oldDeviceConnected)
    {
        // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
        // TODO: Might want to add a delay of like 100ms here? not sure

        // send assignment packet... will be more important after fixing it so we can allow multiple devices to connect
        AssignRecipientPacket *arp = new AssignRecipientPacket(currentUserNum);
        restCharacteristic->setValue(arp->tx(), BTOAS_PACKET_SIZE);

        currentUserNum++;
    }
}

void ble_create_characteristics(BLEService *pService)
{
    // Create a BLE Characteristic
    statusCharacteristic = pService->createCharacteristic(
        STATUS_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);

    restCharacteristic = pService->createCharacteristic(
        REST_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY);

    // // Create a BLE Descriptor
    pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_1->setValue("Status");
    statusCharacteristic->addDescriptor(pDescr_1);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    pBLE2902_1 = new BLE2902();
    pBLE2902_1->setNotifications(true);
    statusCharacteristic->addDescriptor(pBLE2902_1);

    pBLE2902_2 = new BLE2902();
    pBLE2902_2->setNotifications(true);
    restCharacteristic->addDescriptor(pBLE2902_2);

    IdlePacket *ip = new IdlePacket();
    restCharacteristic->setValue(ip->tx(), BTOAS_PACKET_SIZE);
}

void ble_notify()
{
    // statusCharacteristic is an integer that is increased with every second
    // in the code below we send the value over to the client and increase the integer counter
    // BTOasPacket packet = BTOasPacket();
    // packet.cmd = AIRUP;
    // packet.args[0].i = value;
    // packet.args[1].f = 1.0f;

    StatusPacket *statusPacket = new StatusPacket();

    for (int i = 0; i < BTOAS_PACKET_SIZE; i++)
    {
        Serial.print(statusPacket->tx()[i], HEX);
        Serial.print(" ");
    }
    Serial.println("");

    statusCharacteristic->setValue(statusPacket->tx(), BTOAS_PACKET_SIZE);
    statusCharacteristic->notify(); // we don't do this on the other characteristic thats why it has to be read manually
    value++;

    // restCharacteristic is a std::string (NOT a String). In the code below we read the current value
    // write this to the Serial interface and send a different value back to the Client
    // Here the current value is read using getValue()
    std::string rxValue = restCharacteristic->getValue();
    uint8_t *data = restCharacteristic->getData();
    restCharacteristic->getLength();
    Serial.print("Characteristic 2 (getValue): ");
    Serial.println(rxValue.c_str());

    // Here the value is written to the Client using setValue();
    String txValue = "String with random value from Server: " + String(random(1000));
    restCharacteristic->setValue(txValue.c_str());
    Serial.println("Characteristic 2 (setValue): " + txValue);

    // In this example "delay" is used to delay with one second. This is of course a very basic
    // implementation to keep things simple. I recommend to use millis() for any production code
    delay(1000);

    if (value == 8888)
    {
        // add code in here just so they get included in the compile
        airUp();
        airOut();
        airUpRelativeToAverage(10);
        writeProfile(0);
        setBaseProfile(0);
        readProfile(0);
        setRideHeightFrontPassenger(0);
        setRideHeightRearPassenger(0);
        setRideHeightFrontDriver(0);
        setRideHeightRearDriver(0);
        setRiseOnStart(false);
        setRaiseOnPressureSet(false);
        setReboot(true);
    }
}