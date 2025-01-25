#include "ble.h"

// Based on this file: https://github.com/mo-thunderz/Esp32BlePart2/blob/main/Arduino/BLE_server_2characteristics/BLE_server_2characteristics.ino

// Initialize all pointers
BLEServer *pServer = NULL;                            // Pointer to the server
BLECharacteristic *statusCharacteristic = NULL;       // Pointer to Characteristic 1
BLECharacteristic *restCharacteristic = NULL;         // Pointer to Characteristic 2
BLECharacteristic *valveControlCharacteristic = NULL; // Pointer to Characteristic 3
BLEDescriptor *pDescr_1;                              // Pointer to Descriptor of Characteristic 1
BLEDescriptor *pDescr_2;                              // Pointer to Descriptor of Characteristic 1
BLEDescriptor *pDescr_3;                              // Pointer to Descriptor of Characteristic 3
BLE2902 *pBLE2902_1;                                  // Pointer to BLE2902 of Characteristic 1
BLE2902 *pBLE2902_2;                                  // Pointer to BLE2902 of Characteristic 2
BLE2902 *pBLE2902_3;                                  // Pointer to BLE2902 of Characteristic 3
BLEAddress *recentAddr;

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
#define VALVECONTROL_CHARACTERISTIC_UUID "e225a15a-e816-4e9d-99b7-c384f91f273b"

// Callback function that is called whenever a client is connected or disconnected
class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
    {
        char remoteAddress[18];

        sprintf(
            remoteAddress,
            "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
            param->connect.remote_bda[0],
            param->connect.remote_bda[1],
            param->connect.remote_bda[2],
            param->connect.remote_bda[3],
            param->connect.remote_bda[4],
            param->connect.remote_bda[5]);

        ESP_LOGI(LOG_TAG, "myServerCallback onConnect, MAC: %s", remoteAddress);
        Serial.println(remoteAddress);
        recentAddr = new BLEAddress(param->connect.remote_bda);
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
    // pServer->updateConnParams

    // Create the BLE Service
    BLEService *pService = pServer->createService(SERVICE_UUID);

    ble_create_characteristics(pService);

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true); // this is required to share the service id on the initial scan for the client
    // pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);
    // BLEAdvertisementData bad = BLEAdvertisementData();
    // bad.
    // pAdvertising->setAdvertisementData();
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

uint64_t currentUserNum = 1;
void ble_loop()
{
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
        delay(100);

        // Serial.print("Connecting to: ");
        // Serial.println(recentAddr->toString().c_str());
        // pServer->connect(*recentAddr /*pServer->getPeerDevices(false)[0].peer_device*/); // causes ESP_GATTC_OPEN_EVT to get called???

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
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);

    restCharacteristic = pService->createCharacteristic(
        REST_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

    valveControlCharacteristic = pService->createCharacteristic(
        VALVECONTROL_CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR); // NR meaning no response from the server

    // STATUS
    // // Create a BLE Descriptor
    pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_1->setValue("Status");
    statusCharacteristic->addDescriptor(pDescr_1);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    pBLE2902_1 = new BLE2902();
    pBLE2902_1->setNotifications(true);
    statusCharacteristic->addDescriptor(pBLE2902_1);

    // RESET
    // // Create a BLE Descriptor
    pDescr_2 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_2->setValue("Rest");
    restCharacteristic->addDescriptor(pDescr_2);

    pBLE2902_2 = new BLE2902();
    pBLE2902_2->setNotifications(true);
    restCharacteristic->addDescriptor(pBLE2902_2);

    IdlePacket *ip = new IdlePacket();
    restCharacteristic->setValue(ip->tx(), BTOAS_PACKET_SIZE);

    // VALVE CONTROL
    //  // Create a BLE Descriptor
    pDescr_3 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_3->setValue("ValveControl");
    valveControlCharacteristic->addDescriptor(pDescr_3);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    pBLE2902_3 = new BLE2902();
    pBLE2902_3->setNotifications(true);
    valveControlCharacteristic->addDescriptor(pBLE2902_3);
    int valveValue = 0;
    valveControlCharacteristic->setValue(valveValue);
}

void ble_notify()
{
    // statusCharacteristic is an integer that is increased with every second
    // in the code below we send the value over to the client and increase the integer counter
    // BTOasPacket packet = BTOasPacket();
    // packet.cmd = AIRUP;
    // packet.args[0].i = value;
    // packet.args[1].f = 1.0f;

    // calculate whether or not to do stuff at a specific interval, in this case, every 1 second we want to send out a notify.
    static bool prevTime = false;
    bool timeChange = (millis() / 1000) % 2; // changes from 0 to 1 every 1000ms
    bool runNotifications = false;
    if (prevTime != timeChange)
    {
        runNotifications = true;
    }
    prevTime = timeChange;

    if (runNotifications)
    {
        StatusPacket *statusPacket = new StatusPacket(getWheel(WHEEL_FRONT_PASSENGER)->getPressure(), getWheel(WHEEL_REAR_PASSENGER)->getPressure(), getWheel(WHEEL_FRONT_DRIVER)->getPressure(), getWheel(WHEEL_REAR_DRIVER)->getPressure(), getCompressor()->getTankPressure());

        for (int i = 0; i < BTOAS_PACKET_SIZE; i++)
        {
            Serial.print(statusPacket->tx()[i], HEX);
            Serial.print(" ");
        }
        Serial.println("");

        statusCharacteristic->setValue(statusPacket->tx(), BTOAS_PACKET_SIZE);
        statusCharacteristic->notify(); // we don't do this on the other characteristic thats why it has to be read manually TODO: THIS CRASHED AT ONE POINT????????? HAVING TROUBLE READING THE BLE VALUE
    }

    // restCharacteristic is a std::string (NOT a String). In the code below we read the current value
    // write this to the Serial interface and send a different value back to the Client
    // Here the current value is read using getValue()
    std::string rxValue = restCharacteristic->getValue();
    uint8_t *data = restCharacteristic->getData();
    restCharacteristic->getLength();
    // Serial.print("Characteristic 2 (getValue): ");
    // Serial.println(rxValue.c_str());

    // Here the value is written to the Client using setValue();
    // String txValue = "String with random value from Server: " + String(random(1000));
    // restCharacteristic->setValue(txValue.c_str());
    // Serial.println("Characteristic 2 (setValue): " + txValue);

    // valve control characteristic reading
    static unsigned int valveTableValues = 0;
    unsigned int valveControlBittset = ((unsigned int *)valveControlCharacteristic->getData())[0];

    for (int i = 0; i < 8; i++)
    {
        bool prevVal = (valveTableValues >> i) & 1;
        bool curVal = (valveControlBittset >> i) & 1;
        if (prevVal != curVal)
        {
            if (curVal)
            {
                Serial.print("Opening ");
                Serial.println(i);
                getSolenoidFromIndex(i)->open();
            }
            else
            {
                Serial.print("Closing ");
                Serial.println(i);
                getSolenoidFromIndex(i)->close();
            }
        }
    }
    valveTableValues = valveControlBittset;

    // In this example "delay" is used to delay with one second. This is of course a very basic
    // implementation to keep things simple. I recommend to use millis() for any production code
    // delay(1000);

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

extern bool startOTAServiceRequest;
void runReceivedPacket(BTOasPacket *packet)
{
    switch (packet->cmd)
    {
    case BTOasIdentifier::IDLE:
        break;
    case BTOasIdentifier::ASSIGNRECEPIENT: // ignore from server
        break;
    case BTOasIdentifier::MESSAGE: // ignore from server
        break;
    case BTOasIdentifier::AIRUP:
        airUp();
        break;
    case BTOasIdentifier::AIROUT:
        airOut();
        break;
    case BTOasIdentifier::AIRSM:
        airUpRelativeToAverage(((AirsmPacket *)packet)->getRelativeValue());
        break;
    case BTOasIdentifier::SAVETOPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        writeProfile(((SaveToProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::READPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        readProfile(((ReadProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::AIRUPQUICK: // add if (profileIndex > MAX_PROFILE_COUNT)
        // load profile then air up
        readProfile(((AirupQuickPacket *)packet)->getProfileIndex());
        airUp(true);
        break;
    case BTOasIdentifier::BASEPROFILE:
        setBaseProfile(((BaseProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::SETAIRHEIGHT:
    {
        SetAirheightPacket *ahp = (SetAirheightPacket *)packet;
        switch (ahp->getWheelIndex())
        {
        case WHEEL_FRONT_PASSENGER:
            setRideHeightFrontPassenger(ahp->getPressure());
            break;
        case WHEEL_REAR_PASSENGER:
            setRideHeightRearPassenger(ahp->getPressure());
            break;
        case WHEEL_FRONT_DRIVER:
            setRideHeightFrontDriver(ahp->getPressure());
            break;
        case WHEEL_REAR_DRIVER:
            setRideHeightRearDriver(ahp->getPressure());
            break;
        }
    }
    break;
    case BTOasIdentifier::RISEONSTART:
        setRiseOnStart(((RiseOnStartPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::RAISEONPRESSURESET:
        setRaiseOnPressureSet(((RaiseOnPressureSetPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::REBOOT:
        setReboot(true);
        Serial.println(F("Rebooting..."));
        break;
    case BTOasIdentifier::CALIBRATE:
        Serial.println(F("calibrate does nothign lmao"));
        break;
    case BTOasIdentifier::STARTWEB:
        Serial.println(F("Starting OTA..."));
        startOTAServiceRequest = true;
        break;
    }
}