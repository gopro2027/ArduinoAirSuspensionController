#include "ble.h"

#define ble2_new
#ifdef ble2_new

#pragma region bluetooth packet mover util

namespace packetMover
{

#define BTOASPACKETCOUNT 10
    struct PacketEntry
    {
        bool taken;
        hci_con_handle_t con_handle;
        BTOasPacket packet;
    };
    PacketEntry packets[BTOASPACKETCOUNT];
    static SemaphoreHandle_t restMutex;
    void setupRestSemaphore()
    {
        restMutex = xSemaphoreCreateMutex();
        memset(packets, 0, sizeof(packets));
    }
    void waitRestSemaphore()
    {
        while (xSemaphoreTake(restMutex, 1) != pdTRUE)
        {
            delay(1);
        }
    }
    void giveRestSemaphore()
    {
        xSemaphoreGive(restMutex);
    }

    void clearPackets()
    {
        waitRestSemaphore();
        for (int i = 0; i < BTOASPACKETCOUNT; i++)
        {
            packets[i].taken = false;
        }
        giveRestSemaphore();
    }

    bool getBTRestPacketToSend(BTOasPacket *copyTo, hci_con_handle_t &con_handle)
    {
        bool ret = false;
        waitRestSemaphore();
        for (int i = 0; i < BTOASPACKETCOUNT; i++)
        {
            if (packets[i].taken)
            {
                packets[i].taken = false;
                con_handle = packets[i].con_handle;
                memcpy(copyTo, &packets[i].packet, BTOAS_PACKET_SIZE);
                ret = true;
                break;
            }
        }
        giveRestSemaphore();
        return ret;
    }
    void sendRestPacket(BTOasPacket *packet, hci_con_handle_t con_handle)
    {
        waitRestSemaphore();
        for (int i = 0; i < BTOASPACKETCOUNT; i++)
        {
            if (!packets[i].taken)
            {
                packets[i].taken = true;
                memcpy(&packets[i].packet, packet, BTOAS_PACKET_SIZE);
                packets[i].con_handle = con_handle;
                break;
            }
        }
        giveRestSemaphore();
    }

};

#pragma endregion

// Based on this file: https://github.com/mo-thunderz/Esp32BlePart2/blob/main/Arduino/BLE_server_2characteristics/BLE_server_2characteristics.ino

void ble_notify();

// Service and characteristic handles
const static uint16_t status_characteristic_value_handle = ATT_CHARACTERISTIC_66fda100_8972_4ec7_971c_3fd30b3072ac_01_VALUE_HANDLE;
const static uint16_t status_characteristic_client_configuration_handle = ATT_CHARACTERISTIC_66fda100_8972_4ec7_971c_3fd30b3072ac_01_CLIENT_CONFIGURATION_HANDLE;

const static uint16_t rest_characteristic_value_handle = ATT_CHARACTERISTIC_f573f13f_b38e_415e_b8f0_59a6a19a4e02_01_VALUE_HANDLE;
const static uint16_t rest_characteristic_client_configuration_handle = ATT_CHARACTERISTIC_f573f13f_b38e_415e_b8f0_59a6a19a4e02_01_CLIENT_CONFIGURATION_HANDLE;

const static uint16_t valve_control_characteristic_value_handle = ATT_CHARACTERISTIC_e225a15a_e816_4e9d_99b7_c384f91f273b_01_VALUE_HANDLE;
const static uint16_t valve_control_characteristic_client_configuration_handle = ATT_CHARACTERISTIC_e225a15a_e816_4e9d_99b7_c384f91f273b_01_CLIENT_CONFIGURATION_HANDLE;

// General Discoverable = 0x02
// BR/EDR Not supported = 0x04
#define APP_AD_FLAGS 0x06

// Connection tracking
const int MAX_CONNECTIONS = 5;
static uint8_t adv_data[] = {
    2, BLUETOOTH_DATA_TYPE_FLAGS, APP_AD_FLAGS, // General Discoverable Mode, BR/EDR Not Supported
    7, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, 'O', 'A', 'S', 'M', 'a', 'n',
    17, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_128_BIT_SERVICE_CLASS_UUIDS, // 128-bit Service UUIDs (complete list)
    0xf0, 0x25, 0xb6, 0x15, 0x3d, 0x3e, 0xb2, 0x9e, 0x91, 0x44, 0xb4, 0xd3, 0xc8, 0x25, 0x94, 0x67};

uint64_t currentUserNum = 1;

// Characteristic data buffers
static uint8_t status_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t rest_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t valve_control_characteristic_data[4]; // 32-bit value

std::set<hci_con_handle_t> authedClients;
bool isAuthed(hci_con_handle_t conn_id)
{
    return std::find(authedClients.begin(), authedClients.end(), conn_id) != authedClients.end();
}
void addAuthed(hci_con_handle_t conn_id)
{
    authedClients.insert(conn_id);
}

// code for checking if a client auth times out
struct ClientTime
{
    uint16_t conn_id;
    bd_addr_t addr;
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

                if (!isBTDeviceARegisteredController((*iter).addr))
                {
                    gap_disconnect((*iter).conn_id);
                }
                else
                {
                    log_i("Client is a registered controller, not disconnecting: %s", bd_addr_to_str((*iter).addr));
                }
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

void removeAuthed(hci_con_handle_t conn_id)
{
    auto index = std::find(authedClients.begin(), authedClients.end(), conn_id);
    while (index != authedClients.end())
    {
        log_i("Removing auth from client: %i", conn_id);
        authedClients.erase(index);
        index = std::find(authedClients.begin(), authedClients.end(), conn_id);
    }

    // now remove it from the security check part
    std::vector<ClientTime>::iterator iter;
    for (iter = connectedClientTimer.begin(); iter != connectedClientTimer.end();)
    {

        if ((*iter).conn_id == conn_id)
        {
            iter = connectedClientTimer.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

// ATT read callback
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    if (att_handle == status_characteristic_value_handle)
    {
        return att_read_callback_handle_blob(status_characteristic_data, sizeof(status_characteristic_data), offset, buffer, buffer_size);
    }
    if (att_handle == rest_characteristic_value_handle)
    {
        return att_read_callback_handle_blob(rest_characteristic_data, sizeof(rest_characteristic_data), offset, buffer, buffer_size);
    }
    if (att_handle == valve_control_characteristic_value_handle)
    {
        return att_read_callback_handle_blob(valve_control_characteristic_data, sizeof(valve_control_characteristic_data), offset, buffer, buffer_size);
    }
    if (att_handle == status_characteristic_client_configuration_handle)
    {
        return 1; // att_read_callback_handle_little_endian_16(status_characteristic_client_configuration[con_handle], offset, buffer, buffer_size);
    }
    if (att_handle == rest_characteristic_client_configuration_handle)
    {
        return 1; // att_read_callback_handle_little_endian_16(rest_characteristic_client_configuration[con_handle], offset, buffer, buffer_size);
    }
    if (att_handle == valve_control_characteristic_client_configuration_handle)
    {
        return 1; // att_read_callback_handle_little_endian_16(valve_control_characteristic_client_configuration[con_handle], offset, buffer, buffer_size);
    }
    return 0;
}

void runReceivedPacket(hci_con_handle_t con_handle, BTOasPacket *packet);

// ATT write callback
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    static unsigned int valveTableValues = 0;

    if (att_handle == rest_characteristic_value_handle)
    {

        Serial.println("Received rest command");
        BTOasPacket *packet = (BTOasPacket *)buffer;
        packet->dump();
        if (isAuthed(con_handle))
        {
            runReceivedPacket(con_handle, packet);
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
                    addAuthed(con_handle);
                }
                else
                {
                    ap->setBleAuthResult(AuthResult::AUTHRESULT_FAIL);
                }
                packetMover::sendRestPacket(ap, con_handle);
                //  memcpy(rest_characteristic_data, ap->tx(), BTOAS_PACKET_SIZE);
                //  att_server_notify_SAFE(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
            }
        }
        return 0;
    }

    if (att_handle == valve_control_characteristic_value_handle)
    {

        // for some reason some devices send less than the 4 bytes required for this (notably, only 1 byte gets sent from the mobile app). So this little trick gets us into a 4 byte buffer safely
        uint8_t valveControlBittsetArr[4] = {0};
        memcpy(valveControlBittsetArr, buffer, buffer_size > 3 ? 4 : buffer_size);

        if (isAuthed(con_handle))
        {
            unsigned int valveControlBittset = *(unsigned int *)&valveControlBittsetArr; // little_endian_read_32(buffer, 0);
            Serial.printf("Value received for valve: %i\n", valveControlBittset);

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
        return 0;
    }

    // if (att_handle == status_characteristic_client_configuration_handle)
    // {
    //     log_i("STATUS CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
    //     // status_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0); // setting status_characteristic_client_configuration to 1
    //     return 0;
    // }
    // if (att_handle == rest_characteristic_client_configuration_handle)
    // {
    //     log_i("REST CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
    //     // rest_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0);
    //     return 0;
    // }
    // if (att_handle == valve_control_characteristic_client_configuration_handle)
    // {
    //     log_i("VALVE CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
    //     // valve_control_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0);
    //     return 0;
    // }

    return 0;
}

static void handle_connection_complete(uint8_t *packet)
{
    hci_con_handle_t handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
    uint8_t status = hci_subevent_le_connection_complete_get_status(packet);

    bd_addr_t addr;
    hci_subevent_le_connection_complete_get_peer_address(packet, addr);
    printf("Connected to device with MAC: %s\n", bd_addr_to_str(addr));

    if (status != ERROR_CODE_SUCCESS)
    {
        printf("Connection failed with status 0x%02x\n", status);
        return;
    }

    log_i("Connection established, handle: %04x", handle);

    // Send assign recipient packet
    // AssignRecipientPacket arp(currentUserNum);
    // memcpy(rest_characteristic_data, arp.tx(), BTOAS_PACKET_SIZE);
    currentUserNum++;

    ClientTime ct = {};
    ct.conn_id = handle;
    ct.connectTime = millis();
    memcpy(ct.addr, addr, sizeof(bd_addr_t));

    addConnectedClient(ct);

    gap_advertisements_enable(1);

    // Request faster connection parameters
    gap_request_connection_parameter_update(
        handle,
        12, // min_interval: 15ms
        24, // max_interval: 30ms
        0,  // latency
        100 // timeout: 1000ms
    );
}

// HCI event handler
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    log_i("hci_event_handler called");

    if (packet_type != HCI_EVENT_PACKET)
        return;

    notifyKeepAlive();
    switch (hci_event_packet_get_type(packet))
    {
    case HCI_EVENT_DISCONNECTION_COMPLETE:
        log_i("Client disconnected!");
        removeAuthed(hci_event_disconnection_complete_get_connection_handle(packet));
        gap_advertisements_enable(1);
        break;

    case HCI_EVENT_LE_META:
        switch (hci_event_le_meta_get_subevent_code(packet))
        {
        case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
            handle_connection_complete(packet);
            break;
        }
        break;

    case BTSTACK_EVENT_STATE:
        if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING)
        {
            log_i("BTstack activated, start advertising");
            gap_advertisements_enable(1);
        }
        break;

    default:
        break;
    }
}
#define ESP_GATT_MAX_MTU_SIZE 517
void ble_setup()
{

    packetMover::setupRestSemaphore();

    // Initialize ATT Server with our database
    att_server_init(profile_data, att_read_callback, att_write_callback);

    att_server_register_packet_handler(hci_event_handler);
 
    // Set device name
    gap_set_local_name(getbleName().c_str()); // not working for some reason

    gap_set_max_number_peripheral_connections(MAX_CONNECTIONS);

    l2cap_set_max_le_mtu(ESP_GATT_MAX_MTU_SIZE);

    // Set advertisement parameters

    uint16_t adv_int_min = 32;
    uint16_t adv_int_max = 48;
    uint8_t adv_type = 0;
    bd_addr_t null_addr;
    memset(null_addr, 0, 6);

    gap_advertisements_set_params(adv_int_min, adv_int_max, adv_type, 0, null_addr, 0x07, 0x00);

    // Set advertisement data
    gap_advertisements_set_data(sizeof(adv_data), adv_data);

    gap_advertisements_enable(true);

    // Initialize characteristic data
    IdlePacket idlepacket;
    memcpy(rest_characteristic_data, idlepacket.tx(), BTOAS_PACKET_SIZE);

    int valveValue = 0;
    little_endian_store_32(valve_control_characteristic_data, 0, valveValue);

    Serial.println("Waiting a client connection to notify...");
}

void ble_loop()
{
    static int prevConnectedCount = -1;
    int connectedCount = authedClients.size();
    if (connectedCount != prevConnectedCount)
    {
        Serial.printf("connectedCount: %d\n", connectedCount);
    }
    prevConnectedCount = connectedCount;

    checkConnectedClients();
    ble_notify();
}

uint8_t att_server_notify_SAFE(hci_con_handle_t con_handle, uint16_t attribute_handle, const uint8_t *value, uint16_t value_len)
{

    while (!att_server_can_send_packet_now(con_handle))
    {
        // log_i("\n\n\nCAN'T SEND PACKET\n\n\n");
        delay(10);
    }
    return att_server_notify(con_handle, attribute_handle, value, value_len);
}

extern uint8_t AIReadyBittset; // 4
extern uint8_t AIPercentage;   // 7

void ble_notify()
{

    // first, pending packets
    BTOasPacket packet;
    hci_con_handle_t rest_con_handle;
    while (packetMover::getBTRestPacketToSend(&packet, rest_con_handle))
    {
        delay(40); // This feels really shitty but it wants some delay here in-between packets or it won't send. So there is various delay's throught this file
        packet.dump();
        memcpy(rest_characteristic_data, packet.tx(), BTOAS_PACKET_SIZE);
        att_server_notify_SAFE(rest_con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        Serial.println("Sent rest packet!");
        delay(40);
    }

    //  calculate whether or not to do stuff at a specific interval, in this case, every 1 second we want to send out a notify.
    static bool prevTime = false;
    bool timeChange = (millis() / 250) % 2; // changes from 0 to 1 every 250ms
    bool runNotifications = false;
    if (prevTime != timeChange)
    {
        runNotifications = true;
    }
    prevTime = timeChange;
    if (runNotifications && authedClients.size() > 0)
    {
        uint32_t statusBittset = 0;
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
        if (getaiEnabled())
        {
            statusBittset = statusBittset | (1 << StatusPacketBittset::AI_STATUS_ENABLED);
        }

        // // pack these 2 values together at the top of the statusBittset
        // int aiDataPacked = (AIPercentage << 4) + AIReadyBittset; // combine at bottom
        // aiDataPacked = (aiDataPacked << 21);                     // move to top end (4 + 7 = 11; 32-11 = 21)
        // statusBittset = statusBittset | aiDataPacked;
        StatusPacket statusPacket(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue(), getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue(), getCompressor()->getTankPressure(), statusBittset, AIPercentage, AIReadyBittset, getupdateResult());

        memcpy(status_characteristic_data, statusPacket.tx(), BTOAS_PACKET_SIZE);

        for (hci_con_handle_t handle : authedClients)
        {
            att_server_notify_SAFE(handle, status_characteristic_value_handle, status_characteristic_data, BTOAS_PACKET_SIZE);
            delay(10); // need a delay here or it only sends it to 1 client
        }
    }
}

void runReceivedPacket(hci_con_handle_t con_handle, BTOasPacket *packet)
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
    case BTOasIdentifier::RESETAIPKT:
        clearPressureData();
        break;
    case BTOasIdentifier::CALIBRATE:
        Serial.println("Feature unfinished");
        break;
    case BTOasIdentifier::STARTWEB:
        Serial.println(F("Starting OTA..."));
        setwifiSSID(((StartwebPacket *)packet)->getSSID());
        setwifiPassword(((StartwebPacket *)packet)->getPassword());
        setupdateMode(true);
        setinternalReboot(true);
        break;
    case BTOasIdentifier::PRESETREPORT:
    {
        readProfile(((PresetPacket *)packet)->getProfile());
        PresetPacket presetPacket(((PresetPacket *)packet)->getProfile(), currentProfile[WHEEL_FRONT_PASSENGER], currentProfile[WHEEL_REAR_PASSENGER], currentProfile[WHEEL_FRONT_DRIVER], currentProfile[WHEEL_REAR_DRIVER]);
        packetMover::sendRestPacket(&presetPacket, con_handle);
        presetPacket.dump();

        // memcpy(rest_characteristic_data, presetPacket.tx(), BTOAS_PACKET_SIZE);
        // att_server_notify_SAFE(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        break;
    }
    case BTOasIdentifier::MAINTAINPRESSURE:
        setmaintainPressure(((MaintainPressurePacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::COMPRESSORSTATUS:
        // TODO: THIS MIGHT HAVE THREADING ISSUES BUT IDK
        getCompressor()->enableDisableOverride(((CompressorStatusPacket *)packet)->getBoolean());
        break;
    case BTOasIdentifier::AISTATUSENABLED:
        setaiEnabled(((AIStatusPacket *)packet)->getBoolean());
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
        packetMover::sendRestPacket(&pkt, con_handle);
        //  memcpy(rest_characteristic_data, pkt.tx(), BTOAS_PACKET_SIZE);
        //  att_server_notify_SAFE(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        delay(500);
        break;
    }
    case BTOasIdentifier::AUTHPACKET:
        if (((AuthPacket *)packet)->getBleAuthResult() == AuthResult::AUTHRESULT_UPDATEKEY)
        {
            if (((AuthPacket *)packet)->getBlePasskey() != getblePasskey())
            {
                setblePasskey(((AuthPacket *)packet)->getBlePasskey());
            }
        }
        break;
    case BTOasIdentifier::BROADCASTNAME:

        if (((BroadcastNamePacket *)packet)->getBroadcastName() != getbleName())
        {
            setbleName(((BroadcastNamePacket *)packet)->getBroadcastName());
            //setinternalReboot(true);
            Serial.print("new broacast name:");
            Serial.println(((BroadcastNamePacket *)packet)->getBroadcastName());
        }
        break;
    case BTOasIdentifier::BP32PKT:
    {
        BP32CMD bp32cmd = (BP32CMD)((BP32Packet *)packet)->args16()[0].i;
        bool bp32val = ((BP32Packet *)packet)->args16()[1].i;
        switch (bp32cmd)
        {
        case BP32CMD::BP32CMD_ENABLE_NEW_CONN:
            Serial.println("Enabling new connections!");
            bp32_setAllowNewConnections(bp32val);
            break;
        case BP32CMD::BP32CMD_FORGET_DEVICES:
            Serial.println("Forgetting controllers!");
            bp32_forgetDevices();
            break;
        case BP32CMD::BP32CMD_DISCONNECT_DEVICES:
            Serial.println("disconnecting controllers!");
            bp32_disconnectControllers();
            break;
        }
    }
    break;
    }
}

#endif