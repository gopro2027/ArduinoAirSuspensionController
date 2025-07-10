#include "ble.h"

//#define ble2_new
#ifdef ble2_new

// Based on this file: https://github.com/mo-thunderz/Esp32BlePart2/blob/main/Arduino/BLE_server_2characteristics/BLE_server_2characteristics.ino

void ble_notify();
void ble_create_characteristics();

// ATT Database - manually created since we don't have a .gatt file
static const uint8_t att_database[] = {
    // Service: OASMan Service
    0x00, 0x01, 0x02, 0x00, 0x10, 0x00, 0x00, 0x28,  // Service declaration
    0x67, 0x94, 0x25, 0xc8, 0xd3, 0xb4, 0x44, 0x91, 0x9e, 0xb2, 0x3e, 0x3d, 0x15, 0xb6, 0x25, 0xf0,
    
    // Characteristic: Status
    0x00, 0x02, 0x02, 0x00, 0x03, 0x00, 0x03, 0x28,  // Characteristic declaration
    0x12, 0x03, 0x00, 0x66, 0xfd, 0xa1, 0x00, 0x89, 0x72, 0x4e, 0xc7, 0x97, 0x1c, 0x3f, 0xd3, 0x0b, 0x30, 0x72, 0xac,
    
    0x00, 0x03, 0x02, 0x00, 0x00, 0x00,  // Status characteristic value
    0x66, 0xfd, 0xa1, 0x00, 0x89, 0x72, 0x4e, 0xc7, 0x97, 0x1c, 0x3f, 0xd3, 0x0b, 0x30, 0x72, 0xac,
    
    0x00, 0x04, 0x02, 0x00, 0x02, 0x00, 0x02, 0x29,  // Client characteristic configuration
    0x00, 0x00,
    
    // Characteristic: Rest
    0x00, 0x05, 0x02, 0x00, 0x03, 0x00, 0x03, 0x28,  // Characteristic declaration
    0x1A, 0x06, 0x00, 0xf5, 0x73, 0xf1, 0x3f, 0xb3, 0x8e, 0x41, 0x5e, 0xb8, 0xf0, 0x59, 0xa6, 0xa1, 0x9a, 0x4e, 0x02,
    
    0x00, 0x06, 0x02, 0x00, 0x00, 0x00,  // Rest characteristic value
    0xf5, 0x73, 0xf1, 0x3f, 0xb3, 0x8e, 0x41, 0x5e, 0xb8, 0xf0, 0x59, 0xa6, 0xa1, 0x9a, 0x4e, 0x02,
    
    0x00, 0x07, 0x02, 0x00, 0x02, 0x00, 0x02, 0x29,  // Client characteristic configuration
    0x00, 0x00,
    
    // Characteristic: Valve Control
    0x00, 0x08, 0x02, 0x00, 0x03, 0x00, 0x03, 0x28,  // Characteristic declaration
    0x1A, 0x09, 0x00, 0xe2, 0x25, 0xa1, 0x5a, 0xe8, 0x16, 0x4e, 0x9d, 0x99, 0xb7, 0xc3, 0x84, 0xf9, 0x1f, 0x27, 0x3b,
    
    0x00, 0x09, 0x02, 0x00, 0x00, 0x00,  // Valve Control characteristic value
    0xe2, 0x25, 0xa1, 0x5a, 0xe8, 0x16, 0x4e, 0x9d, 0x99, 0xb7, 0xc3, 0x84, 0xf9, 0x1f, 0x27, 0x3b,
    
    0x00, 0x0A, 0x02, 0x00, 0x02, 0x00, 0x02, 0x29,  // Client characteristic configuration
    0x00, 0x00,
};

// BTSTACK handles and variables
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;
static uint16_t att_read_callback_handle;
static uint16_t att_write_callback_handle;

// Service and characteristic handles
static uint16_t service_handle;
static uint16_t status_characteristic_handle;
static uint16_t status_characteristic_value_handle;
static uint16_t status_characteristic_client_configuration_handle;
static uint16_t rest_characteristic_handle;
static uint16_t rest_characteristic_value_handle;
static uint16_t rest_characteristic_client_configuration_handle;
static uint16_t valve_control_characteristic_handle;
static uint16_t valve_control_characteristic_value_handle;
static uint16_t valve_control_characteristic_client_configuration_handle;

// Connection tracking
static hci_con_handle_t connection_handle = HCI_CON_HANDLE_INVALID;
static uint8_t adv_data[] = {
    0x02, 0x01, 0x06,  // General Discoverable Mode, BR/EDR Not Supported
    0x11, 0x07,        // 128-bit Service UUIDs (complete list)
    0xf0, 0x25, 0xb6, 0x15, 0x3d, 0x3e, 0xb2, 0x9e, 0x91, 0x44, 0xb4, 0xd3, 0xc8, 0x25, 0x94, 0x67
};

uint64_t currentUserNum = 1;

// UUIDs - converted to uint8_t arrays for BTSTACK
static uint8_t service_uuid[] = {0x67, 0x94, 0x25, 0xc8, 0xd3, 0xb4, 0x44, 0x91, 0x9e, 0xb2, 0x3e, 0x3d, 0x15, 0xb6, 0x25, 0xf0};
static uint8_t status_characteristic_uuid[] = {0x66, 0xfd, 0xa1, 0x00, 0x89, 0x72, 0x4e, 0xc7, 0x97, 0x1c, 0x3f, 0xd3, 0x0b, 0x30, 0x72, 0xac};
static uint8_t rest_characteristic_uuid[] = {0xf5, 0x73, 0xf1, 0x3f, 0xb3, 0x8e, 0x41, 0x5e, 0xb8, 0xf0, 0x59, 0xa6, 0xa1, 0x9a, 0x4e, 0x02};
static uint8_t valve_control_characteristic_uuid[] = {0xe2, 0x25, 0xa1, 0x5a, 0xe8, 0x16, 0x4e, 0x9d, 0x99, 0xb7, 0xc3, 0x84, 0xf9, 0x1f, 0x27, 0x3b};

// Characteristic data buffers
static uint8_t status_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t rest_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t valve_control_characteristic_data[4]; // 32-bit value

// Client configuration values
static uint16_t status_characteristic_client_configuration = 0;
static uint16_t rest_characteristic_client_configuration = 0;
static uint16_t valve_control_characteristic_client_configuration = 0;

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
                gap_disconnect((*iter).conn_id);
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

// ATT read callback
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t * buffer, uint16_t buffer_size)
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
        return att_read_callback_handle_little_endian_16(status_characteristic_client_configuration, offset, buffer, buffer_size);
    }
    if (att_handle == rest_characteristic_client_configuration_handle)
    {
        return att_read_callback_handle_little_endian_16(rest_characteristic_client_configuration, offset, buffer, buffer_size);
    }
    if (att_handle == valve_control_characteristic_client_configuration_handle)
    {
        return att_read_callback_handle_little_endian_16(valve_control_characteristic_client_configuration, offset, buffer, buffer_size);
    }
    return 0;
}

// ATT write callback
static int att_write_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    static unsigned int valveTableValues = 0;
    
    if (att_handle == rest_characteristic_value_handle)
    {
        if (buffer_size >= BTOAS_PACKET_SIZE)
        {
            Serial.println("Received rest command");
            BTOasPacket *packet = (BTOasPacket *)buffer;
            packet->dump();
            if (isAuthed(con_handle))
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
                        addAuthed(con_handle);
                    }
                    else
                    {
                        ap->setBleAuthResult(AuthResult::AUTHRESULT_FAIL);
                    }
                    memcpy(rest_characteristic_data, ap->tx(), BTOAS_PACKET_SIZE);
                    att_server_notify(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
                }
            }
        }
        return 0;
    }

    if (att_handle == valve_control_characteristic_value_handle)
    {
        if (isAuthed(con_handle) && buffer_size >= 4)
        {
            unsigned int valveControlBittset = little_endian_read_32(buffer, 0);

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

    if (att_handle == status_characteristic_client_configuration_handle)
    {
        status_characteristic_client_configuration = little_endian_read_16(buffer, 0);
        return 0;
    }
    if (att_handle == rest_characteristic_client_configuration_handle)
    {
        rest_characteristic_client_configuration = little_endian_read_16(buffer, 0);
        return 0;
    }
    if (att_handle == valve_control_characteristic_client_configuration_handle)
    {
        valve_control_characteristic_client_configuration = little_endian_read_16(buffer, 0);
        return 0;
    }

    return 0;
}

// HCI event handler
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    if (packet_type != HCI_EVENT_PACKET) return;

    switch (hci_event_packet_get_type(packet))
    {
        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_i("Client disconnected!");
            connection_handle = HCI_CON_HANDLE_INVALID;
            removeAuthed(hci_event_disconnection_complete_get_connection_handle(packet));
            gap_advertisements_enable(1);
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet))
            {
                case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                    connection_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    log_i("Connection established, handle: %04x", connection_handle);
                    
                    // Send assign recipient packet
                    AssignRecipientPacket arp(currentUserNum);
                    memcpy(rest_characteristic_data, arp.tx(), BTOAS_PACKET_SIZE);
                    currentUserNum++;
                    
                    addConnectedClient({connection_handle, millis()});
                    break;

                // default:
                //     break;
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

void ble_setup()
{
    // Initialize BTstack
    //btstack_memory_init();
    //btstack_run_loop_init(btstack_run_loop_freertos_get_instance());



    // Initialize HCI with ESP32 transport
    //hci_init(hci_transport_h4_instance_for_uart(btstack_uart_block_freertos_instance()), NULL);

    // Set up HCI event handler
    hci_event_callback_registration.callback = &hci_event_handler;
    hci_add_event_handler(&hci_event_callback_registration);

    // Initialize L2CAP
    //l2cap_init();

    // Initialize ATT Server with our database
    att_server_init(att_database, att_read_callback, att_write_callback);

    // Initialize SM
    sm_init();

    // Set device name
    gap_set_local_name("OASMan");

    // Set advertisement data
    gap_advertisements_set_data(sizeof(adv_data), adv_data);

    // Set advertisement parameters
    gap_advertisements_set_params(800, 1600, 0, 0, 0, 0x00, 0x00);

    // Create characteristics
    ble_create_characteristics();

    // Initialize characteristic data
    IdlePacket idlepacket;
    memcpy(rest_characteristic_data, idlepacket.tx(), BTOAS_PACKET_SIZE);
    
    int valveValue = 0;
    little_endian_store_32(valve_control_characteristic_data, 0, valveValue);

    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);

    Serial.println("Waiting a client connection to notify...");
}

void ble_loop()
{
    static int prevConnectedCount = -1;
    int connectedCount = (connection_handle != HCI_CON_HANDLE_INVALID) ? 1 : 0;
    if (connectedCount != prevConnectedCount)
    {
        Serial.printf("connectedCount: %d\n", connectedCount);
    }
    prevConnectedCount = connectedCount;

    checkConnectedClients();
    ble_notify();
    
    // Run BTstack main loop
    btstack_run_loop_execute();
}

void ble_create_characteristics()
{
    // Note: With BTSTACK, characteristics are typically defined in a .gatt file
    // and compiled into profile_data. For this conversion, we'll assume the
    // profile_data contains the service and characteristic definitions.
    // The actual handles would be obtained from the generated profile.
    
    // These handle values would normally come from the generated profile
    // For now, we'll use placeholder values - in a real implementation,
    // these would be defined in the generated att_db.h file
    service_handle = 0x0001;
    status_characteristic_handle = 0x0002;
    status_characteristic_value_handle = 0x0003;
    status_characteristic_client_configuration_handle = 0x0004;
    rest_characteristic_handle = 0x0005;
    rest_characteristic_value_handle = 0x0006;
    rest_characteristic_client_configuration_handle = 0x0007;
    valve_control_characteristic_handle = 0x0008;
    valve_control_characteristic_value_handle = 0x0009;
    valve_control_characteristic_client_configuration_handle = 0x000A;
}

extern uint8_t AIReadyBittset; // 4
extern uint8_t AIPercentage;   // 7

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

    if (runNotifications && connection_handle != HCI_CON_HANDLE_INVALID)
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

        StatusPacket statusPacket(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue(), getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue(), getCompressor()->getTankPressure(), statusBittset, AIPercentage, AIReadyBittset, getupdateResult());

        memcpy(status_characteristic_data, statusPacket.tx(), BTOAS_PACKET_SIZE);
        
        // Send notification if client has enabled notifications
        if (status_characteristic_client_configuration & 0x0001)
        {
            att_server_notify(connection_handle, status_characteristic_value_handle, status_characteristic_data, BTOAS_PACKET_SIZE);
        }
    }
}

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

        memcpy(rest_characteristic_data, presetPacket.tx(), BTOAS_PACKET_SIZE);
        if (rest_characteristic_client_configuration & 0x0001)
        {
            att_server_notify(connection_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        }
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
        memcpy(rest_characteristic_data, pkt.tx(), BTOAS_PACKET_SIZE);
        if (rest_characteristic_client_configuration & 0x0001)
        {
            att_server_notify(connection_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        }
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

#endif