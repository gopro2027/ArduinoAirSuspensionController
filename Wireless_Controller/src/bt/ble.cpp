/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara and MoThunderz
 */

// Please see this for a good reference at some setup auth and ect: https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino

#include "ble.h"

#define SERVICE_UUID "679425c8-d3b4-4491-9eb2-3e3d15b625f0"
#define STATUS_CHARACTERISTIC_UUID "66fda100-8972-4ec7-971c-3fd30b3072ac"
#define REST_CHARACTERISTIC_UUID "f573f13f-b38e-415e-b8f0-59a6a19a4e02"
#define VALVECONTROL_CHARACTERISTIC_UUID "e225a15a-e816-4e9d-99b7-c384f91f273b"

// Define UUIDs:
BLEUUID serviceUUID(SERVICE_UUID);
BLEUUID charUUID_Status(STATUS_CHARACTERISTIC_UUID);
BLEUUID charUUID_Rest(REST_CHARACTERISTIC_UUID);
BLEUUID charUUID_ValveControl(VALVECONTROL_CHARACTERISTIC_UUID);

static bool connected = false;
static bool doScan = true; // default to true so it initiates a scan on start
static unsigned long timeoutMS = 0;
static bool hasReceivedStatus = false;
BLEClient *pClient = nullptr;

// Define pointer for the BLE connection
// static BLEAdvertisedDevice *myDevice;
BLERemoteCharacteristic *pRemoteChar_Status;
BLERemoteCharacteristic *pRemoteChar_Rest;
BLERemoteCharacteristic *pRemoteChar_ValveControl;

void disconnect()
{
    connected = false;

    // cheeky call to tell it to restart scanning again i guess?
    doScan = true;

    // make sure it waits until it received it's first status before checking status timeouts
    hasReceivedStatus = false;

    if (pClient != nullptr)
    {
        pClient->disconnect();
        pClient->end();
        BLEDevice::deleteClient(pClient);
        pClient = nullptr;
    }
}

// Callback function for Notify function
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData,
                    size_t length,
                    bool isNotify)
{
    // Serial.print("Got a notify callback: ");
    // Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
    if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_Status.toString())
    {
        timeoutMS = millis() + 5000;
        hasReceivedStatus = true;

        // convert received bytes to integer
        StatusPacket *status = (StatusPacket *)pData;
        // Serial.print("WHEEL_FRONT_PASSENGER: ");
        // Serial.println(status->args16()[WHEEL_FRONT_PASSENGER].i);
        // Serial.print("WHEEL_REAR_PASSENGER: ");
        // Serial.println(status->args16()[WHEEL_REAR_PASSENGER].i);
        // Serial.print("WHEEL_FRONT_DRIVER: ");
        // Serial.println(status->args16()[WHEEL_FRONT_DRIVER].i);
        // Serial.print("WHEEL_REAR_DRIVER: ");
        // Serial.println(status->args16()[WHEEL_REAR_DRIVER].i);
        // Serial.print("TANK: ");
        // Serial.println(status->args16()[_TANK_INDEX].i);

        currentPressures[WHEEL_FRONT_PASSENGER] = status->args16()[WHEEL_FRONT_PASSENGER].i;
        currentPressures[WHEEL_REAR_PASSENGER] = status->args16()[WHEEL_REAR_PASSENGER].i;
        currentPressures[WHEEL_FRONT_DRIVER] = status->args16()[WHEEL_FRONT_DRIVER].i;
        currentPressures[WHEEL_REAR_DRIVER] = status->args16()[WHEEL_REAR_DRIVER].i;
        currentPressures[_TANK_INDEX] = status->args16()[_TANK_INDEX].i;
    }
    if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_Rest.toString())
    {

        // convert received bytes to integer
        uint32_t counter = pData[0];
        for (int i = 1; i < length; i++)
        {
            counter = counter | (pData[i] << i * 8);
        }

        // print to Serial
        Serial.print("Characteristic 2 (Rest) (Notify) from server: ");
        Serial.println(counter);
    }
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public NimBLEClientCallbacks
{
    void onConnect(BLEClient *pclient) override
    {
        log_i("onConnect");
        // NimBLEDevice::injectPassKey(connInfo, BLE_PASSKEY);
        //  NimBLEDevice::startSecurity(desc);

        // pclient->secureConnection(); // okay but this line did cause it to not hang forever on the connect so that's interesting
    }

    void onDisconnect(BLEClient *pclient, int reason) override
    {
        log_i("onDisconnect");
        disconnect();
    }
    void onPassKeyEntry(NimBLEConnInfo &connInfo) override
    {
        log_i("onPassKeyEntry");
        NimBLEDevice::injectPassKey(connInfo, BLE_PASSKEY);
    }
    void onConfirmPasskey(NimBLEConnInfo &connInfo, uint32_t pin) override
    {
        log_i("The passkey YES/NO number: %" PRIu32 "\n", pin);
        NimBLEDevice::injectConfirmPasskey(connInfo, true);
    };
    void onAuthenticationComplete(NimBLEConnInfo &connInfo) override
    {
        log_i("On auth complete");
        log_i("onAuthenticationComplete");
        if (!connInfo.isEncrypted())
        {
            log_i("Encrypt connection failed - disconnecting\n");
            /** Find the client with the connection handle provided in desc */
            NimBLEDevice::getClientByHandle(connInfo.getConnHandle())->disconnect();
            return;
        }
    }
} clientCallbacks;

// Function that is run whenever the server is connected
bool connectToServer(const BLEAdvertisedDevice *myDevice)
{
    Serial.println(charUUID_Status.toString().c_str());
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    // pClient->secureConnection(true);
    // BLEDevice::setSecurityPasskey(BLE_PASSKEY);
    // BLEDevice::setSecurityAuth(true, true, true);
    // BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    // log_i("Set passkey: %i", BLE_PASSKEY);

    // NimBLEDevice::setSecurityAuth(true, true, false);

    BLEDevice::setSecurityPasskey(BLE_PASSKEY);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY); // Should use the passkey
    NimBLEDevice::setSecurityAuth(false, false, true);      // Default

    pClient->setClientCallbacks(&clientCallbacks, false);

    Serial.println("Set callbacks");

    // Connect to the remove BLE Server.
    Serial.print("Address type: ");
    Serial.println(myDevice->getAddressType());
    bool _connected = pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    if (!_connected)
    {
        Serial.println("Connection error");
        // delete pClient;
        return false;
    }
    Serial.println(" - Connected to server");

    // delay(1000);
    // pClient->getServices();
    // delay(1000);

    // Obtain a reference to the service we are after in the remote BLE server.
    // pClient->getServices(); // invoke call to receive list of services

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // TODO: it will lock up if it fails in this characteristic code below. Not sure why tbh
    pRemoteChar_Status = pRemoteService->getCharacteristic(charUUID_Status);
    pRemoteChar_Rest = pRemoteService->getCharacteristic(charUUID_Rest);
    pRemoteChar_ValveControl = pRemoteService->getCharacteristic(charUUID_ValveControl);
    if (connectCharacteristic(pRemoteService, pRemoteChar_Status) == false)
        _connected = false;
    else if (connectCharacteristic(pRemoteService, pRemoteChar_Rest) == false)
        _connected = false;
    else if (connectCharacteristic(pRemoteService, pRemoteChar_ValveControl) == false)
        _connected = false;

    if (_connected == false)
    {
        // pClient->disconnect();
        Serial.println("At least one characteristic UUID not found");
        return false;
    }
    Serial.println("Connected Successfully");

    // TODO: Send rest command and wait for response in here to verify we are connected. If we return false here it will continue on the other found oasman devices!

    return true;
}

// Function to chech Characteristic
bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar)
{
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    if (l_BLERemoteChar == nullptr)
    {
        Serial.print("Failed to find one of the characteristics");
        return false;
    }

    if (l_BLERemoteChar->canNotify())
        l_BLERemoteChar->subscribe(true, notifyCallback, false);

    return true;
}

void scan()
{

    // disable scan on next loop
    doScan = false;

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    BLEDevice::setSecurityPasskey(BLE_PASSKEY);
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);
    //  BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);

    boolean anyFound = false;

    // pBLEScan->setScanCallbacks(new MyAdvertisedDeviceCallbacks()); // TODO: Consider using this callback instead
    NimBLEScanResults results = pBLEScan->getResults(5 * 1000);
    for (int i = 0; i < results.getCount(); i++)
    {
        const NimBLEAdvertisedDevice *device = results.getDevice(i);

        if (device->haveServiceUUID() && device->isAdvertisingService(serviceUUID))
        // if (advertisedDevice.getName() == "OASMan")
        {
            anyFound = true;
            Serial.println("Connecting to device! ");
            BLEDevice::getScan()->stop();
            connected = connectToServer(device);
            if (connected)
            {
                showDialog("Connected to manifold", lv_color_hex(0x22bb33));
                return; // might as well return, no use in trying to connect still
            }
            else
            {
                // disconnect(); Todo test this later
                showDialog("Error connecting!", lv_color_hex(0xFF0000), 30000);
            }
        }
    }

    if (anyFound == false && connected == false)
    {
        showDialog("No manifold found!", lv_color_hex(0xFF0000), 30000);
    }

    if (!connected)
    {
        // failed to connect, do disconnect procedure
        disconnect();
    }
}
void ble_setup()
{
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("OASMan_Controller");

    disconnect(); // sets up variables to get it ready to go
}

void ble_loop()
{

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected)
    {
        // std::string rxValue = pRemoteChar_Rest->readValue();
        // Serial.print("Characteristic 2 (readValue): ");
        // Serial.println(rxValue.c_str());

        // String txValue = "String with random value from client: " + String(-random(1000));
        // Serial.println("Characteristic 2 (writeValue): " + txValue);

        // // Set the characteristic's value to be the array of bytes that is actually a string.
        // pRemoteChar_Rest->writeValue(txValue.c_str(), txValue.length());

        BTOasPacket packet;
        bool hasPacketToSend = getBTRestPacketToSend(&packet);
        bool success = true;
        if (hasPacketToSend)
        {
            packet.dump();
            success = pRemoteChar_Rest->writeValue(packet.tx(), BTOAS_PACKET_SIZE);
        }

        if (!success)
        {
            showDialog("Error sending command!", lv_color_hex(0xFF0000), 3000);
        }

        // check for connection issue
        if (hasReceivedStatus && timeoutMS < millis())
        {
            showDialog("BLE Connection Timed Out!", lv_color_hex(0xFF0000), 30000);
            disconnect();
        }
    }
    else if (doScan)
    {
        scan();
    }

    delay(10);
}