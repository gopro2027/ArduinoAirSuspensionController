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
static bool allowScan = true; // default to true so it initiates a scan on start
static unsigned long timeoutMS = 0;
static bool hasReceivedStatus = false;
NimBLEClient *pClient = nullptr;

bool isConnectedToManifold()
{
    return connected;
}

// Define pointer for the BLE connection
// static BLEAdvertisedDevice *myDevice;
BLERemoteCharacteristic *pRemoteChar_Status;
BLERemoteCharacteristic *pRemoteChar_Rest;
BLERemoteCharacteristic *pRemoteChar_ValveControl;

AuthResult authenticationResult = AUTHRESULT_WAITING;

std::stack<const NimBLEAdvertisedDevice *> oasmanClientsFound;
std::vector<ble_addr_t> authblacklist;

void deletePClientIfExist()
{
    if (pClient != nullptr)
    {
        // Fixed but where scanning says it's not connected
        pClient->cancelConnect();
        pClient->disconnect();
        // pClient->end();
        BLEDevice::deleteClient(pClient);
        pClient = nullptr;
    }
}

bool doDisconnect = false;
void disconnect()
{
    connected = false;

    // cheeky call to tell it to restart scanning again i guess?
    allowScan = true;

    // make sure it waits until it received it's first status before checking status timeouts
    hasReceivedStatus = false;

    NimBLEDevice::getScan()->stop();

    if (pClient != nullptr)
    {
        if (authenticationResult == AuthResult::AUTHRESULT_FAIL)
        {
            showDialog("Invalid passkey!", lv_color_hex(0xFF0000), 30000);
        }
        else
        {
            showDialog("Disconnected!", lv_color_hex(0xFF0000), 30000);
        }
        deletePClientIfExist();
    }

    log_i("disconnected... restarting");
}

ble_addr_t *authedBleAddr = nullptr;

// Callback function for Notify function
void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                    uint8_t *pData,
                    size_t length,
                    bool isNotify)
{
    // Serial.print("Got a notify callback: ");
    // Serial.println(pBLERemoteCharacteristic->getUUID().toString().c_str());
    if (authenticationResult == AuthResult::AUTHRESULT_SUCCESS)
    {
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
            statusBittset = status->args32()[3].i;
            AIPercentage = status->args8()[10].i;
            AIReadyBittset = status->args8()[11].i;
            manifoldUpdateStatus = status->args8()[0x10].i;
        }
    }
    if (pBLERemoteCharacteristic->getUUID().toString() == charUUID_Rest.toString())
    {
        timeoutMS = millis() + 5000;
        BTOasPacket *pkt = (BTOasPacket *)pData;
        log_i("Rest packet received: %i", pkt->cmd);
        if (pkt->cmd == AUTHPACKET)
        {
            authenticationResult = ((AuthPacket *)pkt)->getBleAuthResult();
            authedBleAddr = (ble_addr_t *)pBLERemoteCharacteristic->getClient()->getPeerAddress().getBase();
        }
        if (authenticationResult == AuthResult::AUTHRESULT_SUCCESS)
        {
            switch (pkt->cmd)
            {
            case PRESETREPORT:
            {
                PresetPacket *profile = (PresetPacket *)pkt;

                profilePressures[profile->getProfile()][WHEEL_FRONT_PASSENGER] = profile->args16()[WHEEL_FRONT_PASSENGER].i;
                profilePressures[profile->getProfile()][WHEEL_REAR_PASSENGER] = profile->args16()[WHEEL_REAR_PASSENGER].i;
                profilePressures[profile->getProfile()][WHEEL_FRONT_DRIVER] = profile->args16()[WHEEL_FRONT_DRIVER].i;
                profilePressures[profile->getProfile()][WHEEL_REAR_DRIVER] = profile->args16()[WHEEL_REAR_DRIVER].i;

                profileUpdated = true; // this should be a semaphore and just call it directly but fuck it
                break;
            }
            case GETCONFIGVALUES:
                memcpy(util_configValues.args, pkt->args, sizeof(BTOasPacket::args));
                *util_configValues._setValues() = true;
                break;
            }
        }
    }
}

// Callback function that is called whenever a client is connected or disconnected
class MyClientCallback : public NimBLEClientCallbacks
{

    void onConnect(BLEClient *pclient) override
    {
        log_i("onConnect %s", pclient->toString().c_str());

        // NimBLEDevice::injectPassKey(connInfo, BLE_PASSKEY);
        //  NimBLEDevice::startSecurity(desc);

        // pclient->secureConnection(); // okay but this line did cause it to not hang forever on the connect so that's interesting
    }

    void onDisconnect(BLEClient *pclient, int reason) override
    {
        log_i("onDisconnect", pclient->toString().c_str());
        // make sure the one it is disconnecting from is the latest one
        if (connected == true && authedBleAddr == pclient->getPeerAddress().getBase())
        {
            doDisconnect = true;
        }
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
    clearPackets(); // not sure this is actually needed, but leaving in here to just give a clean slate?
    authenticationResult = AuthResult::AUTHRESULT_WAITING;
    Serial.println(charUUID_Status.toString().c_str());
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    deletePClientIfExist();
    pClient = NimBLEDevice::createClient();
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

    // request MTU
    // if (pClient->requestMtu(100)) {
    //     Serial.println("MTU request successful");
    // } else {
    //     Serial.println("MTU request failed");
    // }
    Serial.print("MTU:");
    Serial.println(pClient->getMTU());

    // Obtain a reference to the service we are after in the remote BLE server.
    // pClient->getServices(); // invoke call to receive list of services

    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        // pClient->disconnect();
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

    Serial.println("Checking auth...");

    AuthPacket authPacket(getblePasskey(), AuthResult::AUTHRESULT_WAITING);
    pRemoteChar_Rest->writeValue(authPacket.tx(), BTOAS_PACKET_SIZE);

    unsigned long authEnd = millis() + 5000;
    while (true)
    {
        if (authenticationResult == AuthResult::AUTHRESULT_SUCCESS)
        {
            Serial.println("Auth success!");
            return true;
        }
        if (authenticationResult == AuthResult::AUTHRESULT_FAIL)
        {
            // authblacklist.push_back(*pClient->getPeerAddress().getBase());
            Serial.println("Auth failed");
            // pClient->disconnect();
            return false;
        }
        if (millis() > authEnd)
        {
            Serial.println("Auth timed out");
            // pClient->disconnect();
            return false;
        }
        delay(10);
    }

    Serial.println("Connected Successfully");

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

// Taken from here https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino#L55
/** Define a class to handle the callbacks when scan events are received */
void clearOasmanClientsFound()
{
    while (!oasmanClientsFound.empty())
    {
        oasmanClientsFound.pop();
    }
}
class ScanCallbacks : public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
        // check if advertiseddevice has previously failed auth and don't attempt to connect to it again if it is
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID))
        // if (advertisedDevice.getName() == "OASMan")
        {
            log_i("Found oasman device: %s", advertisedDevice->toString().c_str());
            bool found = false;
            const ble_addr_t *fd = advertisedDevice->getAddress().getBase();
            for (ble_addr_t addr : authblacklist)
            {
                if (ble_addr_cmp(&addr, fd))
                {
                    found = true;
                }
            }
            if (!found)
            {
                oasmanClientsFound.push(advertisedDevice);
                Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
            }
        }
    }

    /** Callback to process the results of the completed scan or restart it */
    void onScanEnd(const NimBLEScanResults &scanResults, int reason) override
    {
        // This function flat out doesn't work (or at least not in any way useful...)
    }

public:
    boolean anyFound = false;
    void init()
    {
        anyFound = false;
    }
} scanCallbacks;

bool scanError = false;
void scan()
{

    deletePClientIfExist();

    // disable scan on next loop
    allowScan = false;

    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.

    BLEScan *pBLEScan = BLEDevice::getScan();
    BLEDevice::setSecurityPasskey(BLE_PASSKEY);
    // NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    // NimBLEDevice::setSecurityAuth(BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM | BLE_SM_PAIR_AUTHREQ_SC);
    //  BLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    scanCallbacks.init();
    pBLEScan->setScanCallbacks(&scanCallbacks, false);

    /** Set scan interval (how often) and window (how long) in milliseconds */
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(100);

    /**
     * Active scan will gather scan response data from advertisers
     *  but will use more energy from both devices
     */
    pBLEScan->setActiveScan(true);

    /** Start scanning for advertisers */
    bool started = pBLEScan->start(5 * 1000);
    if (started == false)
    {
        // error is:
        // Unable to scan - connection in progress.
        scanError = true;
    }
}
#define ESP_GATT_MAX_MTU_SIZE 517
void ble_setup()
{
    Serial.println("Starting Arduino BLE Client application...");
    BLEDevice::init("OASMan_Controller");
    NimBLEDevice::setMTU(ESP_GATT_MAX_MTU_SIZE); // default is 255 if not set here!!

    disconnect(); // sets up variables to get it ready to go
}

void ble_loop()
{

    static unsigned int previousValveInt = 0;

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected)
    {
        BTOasPacket packet;
        bool hasPacketToSend = getBTRestPacketToSend(&packet);
        bool success = true;
        if (hasPacketToSend)
        {
            packet.dump();
            success = pRemoteChar_Rest->writeValue(packet.tx(), BTOAS_PACKET_SIZE);
        }

        unsigned int valveControlValue = getValveControlValue();
        if (previousValveInt != valveControlValue)
        {
            success = success && pRemoteChar_ValveControl->writeValue((uint8_t *)&valveControlValue, 4);
            previousValveInt = valveControlValue;
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
    else if (allowScan)
    {
        scan();
    }

    if (!oasmanClientsFound.empty())
    {
        const NimBLEAdvertisedDevice *advertisedDevice = oasmanClientsFound.top();
        oasmanClientsFound.pop();

        if (connected == false)
        {

            scanCallbacks.anyFound = true;
            Serial.println("Connecting to device! ");

            connected = connectToServer(advertisedDevice);
            if (connected)
            {
                NimBLEDevice::getScan()->stop();
                clearOasmanClientsFound();
                showDialog("Connected to manifold", lv_color_hex(0x22bb33));
                onBLEConnectionCompleted();
                doDisconnect = false;
            }
        }
    }

    if (connected == false && !NimBLEDevice::getScan()->isScanning() && oasmanClientsFound.empty())
    {
        log_i("Searching");
        if (scanCallbacks.anyFound == false)
        {
            if (scanError)
            {
                showDialog("Scan error please reboot controller", lv_color_hex(0xFF0000), 3000);
            }
            else
            {
                showDialog("No manifold found!", lv_color_hex(0xFF0000), 30000);
            }
        }

        Serial.println("disconnecting and rescanning");
        // failed to connect, do disconnect procedure
        disconnect();
    }
    else
    {

        // log_i("Not finished scanning: connected: %i scanning: %i size: %i", connected, NimBLEDevice::getScan()->isScanning(), oasmanClientsFound.size());
    }

    // moving the disconnect from the other thread callback into here as it seems to cause issues
    if (doDisconnect)
    {
        disconnect();
    }
    doDisconnect = false;

    delay(10);
}