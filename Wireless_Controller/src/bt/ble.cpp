/**
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * updated by chegewara and MoThunderz
 */

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

// Some variables to keep track on device connected
static bool doConnect = false;
static bool connected = false;
static bool doScan = false;

// Define pointer for the BLE connection
// static BLEAdvertisedDevice *myDevice;
BLERemoteCharacteristic *pRemoteChar_Status;
BLERemoteCharacteristic *pRemoteChar_Rest;
BLERemoteCharacteristic *pRemoteChar_ValveControl;

// Callback function for Notify function
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData,
                    size_t length,
                    bool isNotify)
{
    Serial.print("Got a notify callback: ");
    Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
    if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_Status.toString())
    {

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
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        Serial.println("onConnect");
    }

    void onDisconnect(BLEClient *pclient)
    {
        connected = false;
        Serial.println("onDisconnect");
    }
};

// Function that is run whenever the server is connected
bool connectToServer(const BLEAdvertisedDevice *myDevice)
{
    Serial.println(charUUID_Status.toString().c_str());
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient *pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    Serial.println("Set callbacks");

    // Connect to the remove BLE Server.
    Serial.print("Address type: ");
    Serial.println(myDevice->getAddressType());
    // myDevice->setAddressType(BLE_ADDR_TYPE_RPA_PUBLIC);
    // delay(1000);
    bool _connected = pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    if (!_connected)
    {
        Serial.println("Connection error");
        // delete pClient;
        delete myDevice;
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

    connected = true;
    pRemoteChar_Status = pRemoteService->getCharacteristic(charUUID_Status);
    pRemoteChar_Rest = pRemoteService->getCharacteristic(charUUID_Rest);
    pRemoteChar_ValveControl = pRemoteService->getCharacteristic(charUUID_ValveControl);
    if (connectCharacteristic(pRemoteService, pRemoteChar_Status) == false)
        connected = false;
    else if (connectCharacteristic(pRemoteService, pRemoteChar_Rest) == false)
        connected = false;
    else if (connectCharacteristic(pRemoteService, pRemoteChar_ValveControl) == false)
        connected = false;

    if (connected == false)
    {
        // pClient->disconnect();
        Serial.println("At least one characteristic UUID not found");
        return false;
    }
    return true;
}

// Function to chech Characteristic
bool connectCharacteristic(BLERemoteService *pRemoteService, BLERemoteCharacteristic *l_BLERemoteChar)
{
    // Obtain a reference to the characteristic in the service of the remote BLE server.
    if (l_BLERemoteChar == nullptr)
    {
        Serial.print("Failed to find one of the characteristics");
        // Serial.print(l_BLERemoteChar->getUUID().toString().c_str());
        return false;
    }
    // Serial.println(" - Found characteristic: " + String(l_BLERemoteChar->getUUID().toString().c_str()));

    // if (l_BLERemoteChar->canNotify())
    //     l_BLERemoteChar->registerForNotify(notifyCallback);
    // if (l_BLERemoteChar->canNotify())
    l_BLERemoteChar->subscribe(true, notifyCallback, false);

    return true;
}

// //Scan for BLE servers and find the first one that advertises the service we are looking for.
// class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
// {
//     // Called for each advertising BLE server.
//     void onResult(BLEAdvertisedDevice advertisedDevice)
//     {
//         // Serial.print("BLE Advertised Device found: ");
//         // Serial.println(advertisedDevice.toString().c_str());

//         // for (advertisedDevice.getServiceUUIDCount()) {
//         // advertisedDevice.getServiceUUID()

//         // Serial.println(advertisedDevice.isAdvertisingService(serviceUUID));
//         // Serial.println(advertisedDevice.haveServiceUUID());

//         // We have found a device, let us now see if it contains the service we are looking for. EDIT THIS DOESN"T WORK FOR SOME REASON SERVICE NAMES NOT FOUND :(
//         if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
//         // if (advertisedDevice.getName() == "OASMan")
//         {
//             Serial.println("Connecting to device! ");
//             BLEDevice::getScan()->stop();
//             //myDevice = new BLEAdvertisedDevice(advertisedDevice);
//             // delay(500);
//             // connectToServer();
//             doConnect = true;
//             doScan = true;

//         } // Found our server
//     } // onResult
// }; // MyAdvertisedDeviceCallbacks

void ble_setup()
{
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("OASMan_Controller");

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();

    // pBLEScan->setScanCallbacks(new MyAdvertisedDeviceCallbacks()); // TODO: Consider using this callback instead
    NimBLEScanResults results = pBLEScan->getResults(10 * 1000);
    for (int i = 0; i < results.getCount(); i++)
    {
        const NimBLEAdvertisedDevice *device = results.getDevice(i);

        // if (device->isAdvertisingService(serviceUuid))
        // {
        //     // create a client and connect
        // }

        if (device->haveServiceUUID() && device->isAdvertisingService(serviceUUID))
        // if (advertisedDevice.getName() == "OASMan")
        {
            Serial.println("Connecting to device! ");
            BLEDevice::getScan()->stop();
            // myDevice = new BLEAdvertisedDevice(*device);
            //  delay(500);
            connectToServer(device);
            doConnect = true;
            doScan = true;
        }
    }

    // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    // pBLEScan->setInterval(1349);
    // pBLEScan->setWindow(449);
    // pBLEScan->setActiveScan(true);
    // pBLEScan->start(5, false);
} // End of setup.

void ble_loop()
{

    // If the flag "doConnect" is true then we have scanned for and found the desired
    // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
    // connected we set the connected flag to be true.
    // if (doConnect == true)
    // {
    //     if (connectToServer())
    //     {
    //         Serial.println("We are now connected to the BLE Server.");
    //     }
    //     else
    //     {
    //         Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    //     }
    //     doConnect = false;
    // }

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
    }
    else if (doScan)
    {
        // BLEDevice::getScan()->start(0); // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
    }

    // In this example "delay" is used to delay with one second. This is of course a very basic
    // implementation to keep things simple. I recommend to use millis() for any production code
    delay(1000);
}