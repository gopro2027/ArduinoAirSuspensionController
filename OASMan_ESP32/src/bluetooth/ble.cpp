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

uint64_t currentUserNum = 1;

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID "679425c8-d3b4-4491-9eb2-3e3d15b625f0"
#define STATUS_CHARACTERISTIC_UUID "66fda100-8972-4ec7-971c-3fd30b3072ac"
#define REST_CHARACTERISTIC_UUID "f573f13f-b38e-415e-b8f0-59a6a19a4e02"
#define VALVECONTROL_CHARACTERISTIC_UUID "e225a15a-e816-4e9d-99b7-c384f91f273b"

BLEUUID serviceUUID(SERVICE_UUID);
BLEUUID charUUID_Status(STATUS_CHARACTERISTIC_UUID);
BLEUUID charUUID_Rest(REST_CHARACTERISTIC_UUID);
BLEUUID charUUID_ValveControl(VALVECONTROL_CHARACTERISTIC_UUID);

std::vector<uint16_t> authedClients;
bool isAuthed(uint16_t conn_id)
{
    return std::find(authedClients.begin(), authedClients.end(), conn_id) != authedClients.end();
}
void addAuthed(uint16_t conn_id)
{
    authedClients.push_back(conn_id);
}
void removeAuthed(uint16_t conn_id)
{
    auto index = std::find(authedClients.begin(), authedClients.end(), conn_id);
    while (index != authedClients.end())
    {
        log_i("Removing auth from client: %i", conn_id);
        authedClients.erase(index);
        index = std::find(authedClients.begin(), authedClients.end(), conn_id);
    }
}

// code for checking if a client auth times out
struct ClientTime
{
    uint16_t conn_id;
    unsigned long connectTime;
};
std::vector<ClientTime> connectedClientTimer;
void addConnectedClient(ClientTime cl)
{
    connectedClientTimer.push_back(cl);
}

#define AUTH_TIMEOUT 5000
void checkConnectedClients()
{
    unsigned long curtime = millis();

    std::vector<ClientTime>::iterator iter;
    for (iter = connectedClientTimer.begin(); iter != connectedClientTimer.end();)
    {
        if ((*iter).connectTime + AUTH_TIMEOUT < curtime)
        {
            // check for authed
            if (!isAuthed((*iter).conn_id))
            {
                // not authed, go ahead and disconnect
                log_i("Client auth timed out... disconnecting: %i", (*iter).conn_id);
                pServer->disconnect((*iter).conn_id);
            }
            // remove
            iter = connectedClientTimer.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

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

        log_i("myServerCallback onConnect, MAC: %s", remoteAddress);
        recentAddr = new BLEAddress(param->connect.remote_bda);
        AssignRecipientPacket arp(currentUserNum);
        restCharacteristic->setValue(arp.tx(), BTOAS_PACKET_SIZE);

        currentUserNum++;

        addConnectedClient({param->connect.conn_id, millis()});
        BLEDevice::startAdvertising();
    };

    void onDisconnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
    {
        log_i("Client disconnected!");
        removeAuthed(param->connect.conn_id);
        BLEDevice::startAdvertising();
    }
};
class CharacteristicCallback : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pChar, esp_ble_gatts_cb_param_t *param) override
    {

        static unsigned int valveTableValues = 0;
        if (pChar->getUUID().toString() == charUUID_Rest.toString())
        {
            Serial.println("Received rest command");
            BTOasPacket *packet = (BTOasPacket *)pChar->getData();
            packet->dump();
            if (isAuthed(param->connect.conn_id))
            {
                log_i("authed success");
                runReceivedPacket(packet);
            }

            // Authed thing on connect
            if (packet->cmd == BTOasIdentifier::AUTHPACKET)
            {
                AuthPacket *ap = ((AuthPacket *)packet);
                if (ap->getBleAuthResult() == AuthResult::AUTHRESULT_WAITING)
                {
                    // AUTH REQUEST
                    if (ap->getBlePasskey() == getblePasskey())
                    {
                        ap->setBleAuthResult(AuthResult::AUTHRESULT_SUCCESS);
                        addAuthed(param->connect.conn_id);
                    }
                    else
                    {
                        ap->setBleAuthResult(AuthResult::AUTHRESULT_FAIL);
                    }
                    restCharacteristic->setValue(ap->tx(), BTOAS_PACKET_SIZE);
                    restCharacteristic->notify();
                }
            }
        }

        if (pChar->getUUID().toString() == charUUID_ValveControl.toString())
        {
            if (isAuthed(param->connect.conn_id))
            {
                unsigned int valveControlBittset = ((unsigned int *)pChar->getData())[0];

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
                            getManifold()->get(i)->open();
                        }
                        else
                        {
                            Serial.print("Closing ");
                            Serial.println(i);
                            getManifold()->get(i)->close();
                        }
                    }
                }
                valveTableValues = valveControlBittset;
            }
        }
    }
} characteristicCallback;

// https://www.youtube.com/watch?v=TwexLJwdLEw&ab_channel=ThatProject
// void bleSecurity()
// {
//     esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
//     esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
//     uint8_t key_size = 16;
//     uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
//     uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
//     uint32_t passkey = getblePasskey();
//     uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE;
//     auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE; // allows mini device to connect TODO: remove this line and force the little device to enter the passkey too
//     esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &passkey, sizeof(uint32_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
//     esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
// }

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
    pAdvertising->setScanResponse(true); // this is required to share the service id on the initial scan for the client
    // pAdvertising->setMinPreferred(0x0); // set value to 0x00 to not advertise this parameter
    pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
    pAdvertising->setMaxPreferred(0x12);

    pServer->startAdvertising();

    // bleSecurity(); // can comment this line to disable security

    Serial.println("Waiting a client connection to notify...");
}

void ble_loop()
{
    static int prevConnectedCount = -1;
    int connectedCount = pServer->getConnectedCount();
    if (connectedCount != prevConnectedCount)
    {
        Serial.printf("connectedCount: %d\n", connectedCount);
    }
    prevConnectedCount = connectedCount;

    checkConnectedClients();
    ble_notify();
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
        BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE); // used to be PROPERTY_WRITE_NR

    // statusCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    // restCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    // valveControlCharacteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);

    // STATUS
    // // Create a BLE Descriptor
    pDescr_1 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_1->setValue("Status");
    statusCharacteristic->addDescriptor(pDescr_1);

    // Add the BLE2902 Descriptor because we are using "PROPERTY_NOTIFY"
    pBLE2902_1 = new BLE2902();
    pBLE2902_1->setNotifications(true);
    statusCharacteristic->addDescriptor(pBLE2902_1);

    // REST
    // // Create a BLE Descriptor
    pDescr_2 = new BLEDescriptor((uint16_t)0x2901);
    pDescr_2->setValue("Rest");
    restCharacteristic->addDescriptor(pDescr_2);

    pBLE2902_2 = new BLE2902();
    pBLE2902_2->setNotifications(true);
    restCharacteristic->addDescriptor(pBLE2902_2);

    IdlePacket idlepacket;
    restCharacteristic->setValue(idlepacket.tx(), BTOAS_PACKET_SIZE);

    restCharacteristic->setCallbacks(&characteristicCallback);

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

    valveControlCharacteristic->setCallbacks(&characteristicCallback);
}

void ble_notify()
{
    // calculate whether or not to do stuff at a specific interval, in this case, every 1 second we want to send out a notify.
    static bool prevTime = false;
    bool timeChange = (millis() / 250) % 2; // changes from 0 to 1 every 250ms
    bool runNotifications = false;
    if (prevTime != timeChange)
    {
        runNotifications = true;
    }
    prevTime = timeChange;

    if (runNotifications)
    {
        uint16_t statusBittset = 0;
        if (getCompressor()->isFrozen())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::COMPRESSOR_FROZEN);
        }
        if (getCompressor()->isOn())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::COMPRESSOR_STATUS_ON);
        }
        if (isVehicleOn())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::ACC_STATUS_ON);
        }
        if (isKeepAliveTimerExpired())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::TIMER_STATUS_EXPIRED);
        }
        if (timeChange)
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::CLOCK);
        }
        if (getriseOnStart())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::RISE_ON_START);
        }
        if (getmaintainPressure())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::MAINTAIN_PRESSURE);
        }
        if (getairOutOnShutoff())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::AIR_OUT_ON_SHUTOFF);
        }
        if (getheightSensorMode())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::HEIGHT_SENSOR_MODE);
        }
        if (getsafetyMode())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::SAFETY_MODE);
        }
        StatusPacket statusPacket(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue(), getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue(), getCompressor()->getTankPressure(), statusBittset);

        statusCharacteristic->setValue(statusPacket.tx(), BTOAS_PACKET_SIZE);
        statusCharacteristic->notify(); // we don't do this on the other characteristic thats why it has to be read manually TODO: THIS CRASHED AT ONE POINT????????? HAVING TROUBLE READING THE BLE VALUE. TRY RUNNING IN VERBOSE MODE AND SET IT TO IDLE FOR A LONG TIME TO DEBUG
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
        Serial.println("Calling air up!");
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
    case BTOasIdentifier::SAVECURRENTPRESSURESTOPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        Serial.println("Calling Save Current Pressures To Profile!");
        savePressuresToProfile(((SaveCurrentPressuresToProfilePacket *)packet)->getProfileIndex(), getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue(), getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue());
        break;
    case BTOasIdentifier::READPROFILE: // add if (profileIndex > MAX_PROFILE_COUNT)
        readProfile(((ReadProfilePacket *)packet)->getProfileIndex());
        break;
    case BTOasIdentifier::AIRUPQUICK: // add if (profileIndex > MAX_PROFILE_COUNT)
        // load profile then air up
        Serial.println("Calling air up quick!");
        readProfile(((AirupQuickPacket *)packet)->getProfileIndex());
        airUp(false); // typically this was true but im changing it to not be because now this is the main air up method on the controller :)
        break;
    case BTOasIdentifier::BASEPROFILE:
        setbaseProfile(((BaseProfilePacket *)packet)->getProfileIndex());
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
        setriseOnStart(((RiseOnStartPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::FALLONSHUTDOWN:
        setairOutOnShutoff(((FallOnShutdownPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::HEIGHTSENSORMODE:
        setheightSensorMode(((HeightSensorModePacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::SAFETYMODE:
        setsafetyMode(((SafetyModePacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::DETECTPRESSURESENSORS:
        setlearnPressureSensors(true);
        setinternalReboot(true);
        break;
    case BTOasIdentifier::RAISEONPRESSURESET:
        setraiseOnPressure(((RaiseOnPressureSetPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::REBOOT:
        setinternalReboot(true);
        Serial.println(F("Rebooting..."));
        break;
    case BTOasIdentifier::TURNOFF:
        Serial.println(F("Turning off..."));
        forceShutoff = true;
        break;
    case BTOasIdentifier::CALIBRATE:
#ifdef parabolaLearn
        Serial.println("Starting parabola calibration task");
        start_parabolaLearnTask();
#else
        Serial.println("Feature unfinished");
#endif
        break;
    case BTOasIdentifier::STARTWEB:
        Serial.println(F("Starting OTA..."));
        startOTAServiceRequest = true;
        break;
    case BTOasIdentifier::PRESETREPORT:
    {
        readProfile(((PresetPacket *)packet)->getProfile());
        PresetPacket presetPacket(((PresetPacket *)packet)->getProfile(), currentProfile[WHEEL_FRONT_PASSENGER], currentProfile[WHEEL_REAR_PASSENGER], currentProfile[WHEEL_FRONT_DRIVER], currentProfile[WHEEL_REAR_DRIVER]);

        restCharacteristic->setValue(presetPacket.tx(), BTOAS_PACKET_SIZE);
        restCharacteristic->notify();
        break;
    }
    case BTOasIdentifier::MAINTAINPRESSURE:
        setmaintainPressure(((MaintainPressurePacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::COMPRESSORSTATUS:
        // TODO: THIS MIGHT HAVE THREADING ISSUES BUT IDK
        getCompressor()->enableDisableOverride(((CompressorStatusPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::GETCONFIGVALUES:
    {
        ConfigValuesPacket *recpkt = (ConfigValuesPacket *)packet;
        if (*recpkt->_setValues())
        {
            setbagMaxPressure(*recpkt->_bagMaxPressure());
            setsystemShutoffTimeM(*recpkt->_systemShutoffTimeM());
            setcompressorOnPSI(*recpkt->_compressorOnPSI());
            setcompressorOffPSI(*recpkt->_compressorOffPSI());
            setpressureSensorMax(*recpkt->_pressureSensorMax());
            setbagVolumePercentage(*recpkt->_bagVolumePercentage());
        }
        ConfigValuesPacket pkt(false, getbagMaxPressure(), getsystemShutoffTimeM(), getcompressorOnPSI(), getcompressorOffPSI(), getpressureSensorMax(), getbagVolumePercentage());
        restCharacteristic->setValue(pkt.tx(), BTOAS_PACKET_SIZE);
        restCharacteristic->notify();
        break;
    }
    case BTOasIdentifier::AUTHPACKET:
        if (((AuthPacket *)packet)->getBleAuthResult() == AuthResult::AUTHRESULT_UPDATEKEY)
        {
            setblePasskey(((AuthPacket *)packet)->getBlePasskey());
        }
        break;
    }
}