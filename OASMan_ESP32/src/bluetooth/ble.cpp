#include "ble.h"

#define ble2_new
#ifdef ble2_new

#pragma region bluetooth packets

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
// void ble_create_characteristics();

// BTSTACK handles and variables
// static btstack_packet_callback_registration_t hci_event_callback_registration;
// static btstack_packet_callback_registration_t sm_event_callback_registration;
// static uint16_t att_read_callback_handle;
// static uint16_t att_write_callback_handle;

// Service and characteristic handles
// static uint16_t service_handle = ;
// static uint16_t status_characteristic_handle;
const static uint16_t status_characteristic_value_handle = ATT_CHARACTERISTIC_66fda100_8972_4ec7_971c_3fd30b3072ac_01_VALUE_HANDLE;
const static uint16_t status_characteristic_client_configuration_handle = ATT_CHARACTERISTIC_66fda100_8972_4ec7_971c_3fd30b3072ac_01_CLIENT_CONFIGURATION_HANDLE;
// static uint16_t rest_characteristic_handle;
const static uint16_t rest_characteristic_value_handle = ATT_CHARACTERISTIC_f573f13f_b38e_415e_b8f0_59a6a19a4e02_01_VALUE_HANDLE;
const static uint16_t rest_characteristic_client_configuration_handle = ATT_CHARACTERISTIC_f573f13f_b38e_415e_b8f0_59a6a19a4e02_01_CLIENT_CONFIGURATION_HANDLE;
// static uint16_t valve_control_characteristic_handle;
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

// UUIDs - converted to uint8_t arrays for BTSTACK
// static uint8_t service_uuid[] = {0x67, 0x94, 0x25, 0xc8, 0xd3, 0xb4, 0x44, 0x91, 0x9e, 0xb2, 0x3e, 0x3d, 0x15, 0xb6, 0x25, 0xf0};
// static uint8_t status_characteristic_uuid[] = {0x66, 0xfd, 0xa1, 0x00, 0x89, 0x72, 0x4e, 0xc7, 0x97, 0x1c, 0x3f, 0xd3, 0x0b, 0x30, 0x72, 0xac};
// static uint8_t rest_characteristic_uuid[] = {0xf5, 0x73, 0xf1, 0x3f, 0xb3, 0x8e, 0x41, 0x5e, 0xb8, 0xf0, 0x59, 0xa6, 0xa1, 0x9a, 0x4e, 0x02};
// static uint8_t valve_control_characteristic_uuid[] = {0xe2, 0x25, 0xa1, 0x5a, 0xe8, 0x16, 0x4e, 0x9d, 0x99, 0xb7, 0xc3, 0x84, 0xf9, 0x1f, 0x27, 0x3b};

// Characteristic data buffers
static uint8_t status_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t rest_characteristic_data[BTOAS_PACKET_SIZE];
static uint8_t valve_control_characteristic_data[4]; // 32-bit value

// Client configuration values
// static std::unordered_map<uint16_t, uint16_t> status_characteristic_client_configuration;
// static std::unordered_map<uint16_t, uint16_t> rest_characteristic_client_configuration;
// static std::unordered_map<uint16_t, uint16_t> valve_control_characteristic_client_configuration;

std::set<hci_con_handle_t> authedClients;
bool isAuthed(hci_con_handle_t conn_id)
{
    return std::find(authedClients.begin(), authedClients.end(), conn_id) != authedClients.end();
}
void addAuthed(hci_con_handle_t conn_id)
{
    authedClients.insert(conn_id);
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
    // log_i("checkConnectedClients");
    unsigned long curtime = millis();

    std::vector<ClientTime>::iterator iter;
    for (iter = connectedClientTimer.begin(); iter != connectedClientTimer.end();)
    {
        // Serial.println("Connected");
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
static uint16_t att_read_callback(hci_con_handle_t con_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    log_i("wow in the read callback\n\n\n\n\n");
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

    log_i("write callback called %i %i", att_handle, buffer_size);
    // for (int i = 0; i < buffer_size; i++)
    //     Serial.printf("%i,", buffer[i]);
    // BTOasPacket *packet = (BTOasPacket *)buffer;
    // packet->dump();
    if (att_handle == rest_characteristic_value_handle)
    {
        log_i("wow we got a rest value");
        // if (buffer_size >= BTOAS_PACKET_SIZE)
        // {
        Serial.println("Received rest command");
        BTOasPacket *packet = (BTOasPacket *)buffer;
        packet->dump();
        if (isAuthed(con_handle))
        {
            log_i("authed success");
            runReceivedPacket(con_handle, packet);
        }

        // Authed thing on connect
        if (packet->cmd == BTOasIdentifier::AUTHPACKET)
        {
            Serial.println("This is an auth packet!!");
            AuthPacket *ap = ((AuthPacket *)packet);
            if (ap->getBleAuthResult() == AuthResult::AUTHRESULT_WAITING)
            {
                // AUTH REQUEST
                if (ap->getBlePasskey() == getblePasskey())
                {
                    ap->setBleAuthResult(AuthResult::AUTHRESULT_SUCCESS);
                    addAuthed(con_handle);
                    Serial.println("We have authed!");
                }
                else
                {
                    ap->setBleAuthResult(AuthResult::AUTHRESULT_FAIL);
                }
                packetMover::sendRestPacket(ap, con_handle);
                //  memcpy(rest_characteristic_data, ap->tx(), BTOAS_PACKET_SIZE);
                //  att_server_notify(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
            }
        }
        //}
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

    if (att_handle == status_characteristic_client_configuration_handle)
    {
        log_i("STATUS CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
        // status_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0); // setting status_characteristic_client_configuration to 1
        return 0;
    }
    if (att_handle == rest_characteristic_client_configuration_handle)
    {
        log_i("REST CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
        // rest_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0);
        return 0;
    }
    if (att_handle == valve_control_characteristic_client_configuration_handle)
    {
        log_i("VALVE CHARACTERISTIC WRITTEN?? %i", little_endian_read_16(buffer, 0));
        // valve_control_characteristic_client_configuration[con_handle] = little_endian_read_16(buffer, 0);
        return 0;
    }

    return 0;
}

// HCI event handler
static void hci_event_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    UNUSED(channel);
    UNUSED(size);

    log_i("hci_event_handler called");

    if (packet_type != HCI_EVENT_PACKET)
        return;

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
            hci_con_handle_t handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
            log_i("Connection established, handle: %04x", handle);

            // Send assign recipient packet
            // AssignRecipientPacket arp(currentUserNum);
            // memcpy(rest_characteristic_data, arp.tx(), BTOAS_PACKET_SIZE);
            currentUserNum++;

            addConnectedClient({handle, millis()});

            gap_advertisements_enable(1);

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
    // gap_advertisements_enable(1);
}

#include "btstack_config.h"
#include "bluetooth.h"
#include "btstack_defines.h"
#include "btstack_memory.h"
#include "btstack_run_loop.h"
#include "btstack_util.h"
#include "btstack_debug.h"
#include "btstack_event.h"
#include "att_db_util.h"
#include "hci.h"
#include "gap.h"

#include "ble/att_db_util.h"

// // Dummy value storage
// static uint8_t notify_value[20];

// // Custom UUIDs
// static const uint8_t service_uuid[] = {0xf0, 0x25, 0xb6, 0x5b, 0xd1, 0xe3, 0xb2, 0x9e, 0x91, 0x44, 0xb4, 0xd3, 0xc8, 0x25, 0x94, 0x67};

// static uint8_t status_characteristic_uuid[] = {0x66, 0xfd, 0xa1, 0x00, 0x89, 0x72, 0x4e, 0xc7, 0x97, 0x1c, 0x3f, 0xd3, 0x0b, 0x30, 0x72, 0xac};
// static uint8_t rest_characteristic_uuid[] = {0xf5, 0x73, 0xf1, 0x3f, 0xb3, 0x8e, 0x41, 0x5e, 0xb8, 0xf0, 0x59, 0xa6, 0xa1, 0x9a, 0x4e, 0x02};
// static uint8_t valve_control_characteristic_uuid[] = {0xe2, 0x25, 0xa1, 0x5a, 0xe8, 0x16, 0x4e, 0x9d, 0x99, 0xb7, 0xc3, 0x84, 0xf9, 0x1f, 0x27, 0x3b};

// void create_gatt_db(void)
// {

//     // Add service
//     // att_db_util_add_service_uuid128(service_uuid);

//     // Add characteristic (READ + NOTIFY)
//     att_db_util_add_characteristic_uuid128(
//         status_characteristic_uuid,
//         ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY,
//         ATT_SECURITY_NONE, ATT_SECURITY_NONE,
//         notify_value,
//         sizeof(notify_value));

//     att_db_util_add_characteristic_uuid128(
//         rest_characteristic_uuid,
//         ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY,
//         ATT_SECURITY_NONE, ATT_SECURITY_NONE,
//         notify_value,
//         sizeof(notify_value));

//     att_db_util_add_characteristic_uuid128(
//         valve_control_characteristic_uuid,
//         ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY,
//         ATT_SECURITY_NONE, ATT_SECURITY_NONE,
//         notify_value,
//         sizeof(notify_value));

//     // att_db_util_add_cccd(false); // false = support notify + indicate
// }

#define ATT_PERMISSIONS_READ 0x01
#define ATT_PERMISSIONS_WRITE 0x02

void ble_setup()
{

    packetMover::setupRestSemaphore();
    //    Initialize BTstack
    //    btstack_memory_init();
    //    btstack_run_loop_init(btstack_run_loop_freertos_get_instance());

    // Initialize HCI with ESP32 transport
    // hci_init(hci_transport_h4_instance_for_uart(btstack_uart_block_freertos_instance()), NULL);

    // const uint8_t *att_db;
    // uint16_t att_db_size;

    // create_gatt_db();

    // att_db = att_db_util_get_address();
    // att_db_size = att_db_util_get_size();

    // Initialize ATT Server with our database
    att_server_init(profile_data, att_read_callback, att_write_callback);

    // When creating your characteristic, include notify/indicate properties
    uint8_t characteristic_props = ATT_PROPERTY_READ | ATT_PROPERTY_NOTIFY;

    // The CCCD descriptor should be automatically added by BTStack when you
    // include NOTIFY or INDICATE properties, but you may need to explicitly add it:

    // Add CCCD descriptor (UUID 0x2902)
    uint8_t cccd_value[2] = {0x00, 0x00};
    att_db_util_add_descriptor_uuid16(0x2902, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_NOTIFY, ATT_SECURITY_NONE, ATT_SECURITY_NONE, cccd_value, sizeof(cccd_value));

    // for some reason it crashes now if i don't have these 3 lines so I guess they are required now lmao
    // att_db_util_add_descriptor_uuid128(status_characteristic_uuid, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_NOTIFY, ATT_SECURITY_NONE, ATT_SECURITY_NONE, cccd_value, sizeof(cccd_value));
    // att_db_util_add_descriptor_uuid128(rest_characteristic_uuid, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_NOTIFY, ATT_SECURITY_NONE, ATT_SECURITY_NONE, cccd_value, sizeof(cccd_value));
    // att_db_util_add_descriptor_uuid128(valve_control_characteristic_uuid, ATT_PROPERTY_READ | ATT_PROPERTY_WRITE | ATT_PROPERTY_NOTIFY, ATT_SECURITY_NONE, ATT_SECURITY_NONE, cccd_value, sizeof(cccd_value));

    // att_db_util_add_characteristic_uuid128(
    //     characteristic_uuid,
    //     ATT_PROPERTY_NOTIFY | ATT_PROPERTY_READ,
    //     ATT_PERMISSION_READ,
    //     buffer,
    //     sizeof(buffer));

    // Set up HCI event handler
    // hci_event_callback_registration.callback = &hci_event_handler;
    // hci_add_event_handler(&hci_event_callback_registration);
    att_server_register_packet_handler(hci_event_handler);

    // Initialize L2CAP
    // l2cap_init();

    // Initialize SM
    // sm_init();

    // Set device name
    gap_set_local_name("OASMan"); // not working for some reason

    gap_set_max_number_peripheral_connections(MAX_CONNECTIONS);

    // Set advertisement parameters
    // gap_advertisements_set_params(800, 1600, 0, 0, 0, 0x00, 0x00);
    bd_addr_t null_addr = {0};
    // gap_advertisements_set_params(0x0030, 0x0030, 0, 0, null_addr, 0x07, 0x00);
    // gap_advertisements_set_params(800, 1600, 0, 0, null_addr, 0x00, 0x00);
    gap_advertisements_set_params(0x6, 0x12, 0, 0, null_addr, 0x00, 0x00);

    // Set advertisement data
    gap_advertisements_set_data(sizeof(adv_data), adv_data);

    gap_advertisements_enable(true);

    uint8_t *att_db = att_db_util_get_address();
    Serial.printf("ATT db 0x%X\n", att_db_util_get_size());

    // Create characteristics
    // ble_create_characteristics();

    // Initialize characteristic data
    IdlePacket idlepacket;
    memcpy(rest_characteristic_data, idlepacket.tx(), BTOAS_PACKET_SIZE);

    int valveValue = 0;
    little_endian_store_32(valve_control_characteristic_data, 0, valveValue);

    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);

    Serial.println("Waiting a client connection to notify...");

    // btstack_run_loop_execute();
}

void ble_loop()
{
    // log_i("ble_loop");
    static int prevConnectedCount = -1;
    int connectedCount = authedClients.size();
    if (connectedCount != prevConnectedCount)
    {
        Serial.printf("connectedCount: %d\n", connectedCount);
    }
    prevConnectedCount = connectedCount;

    checkConnectedClients();
    ble_notify();

    // Run BTstack main loop
    // btstack_run_loop_execute();
}

// void ble_create_characteristics()
// {
//     // Note: With BTSTACK, characteristics are typically defined in a .gatt file
//     // and compiled into profile_data. For this conversion, we'll assume the
//     // profile_data contains the service and characteristic definitions.
//     // The actual handles would be obtained from the generated profile.

//     // These handle values would normally come from the generated profile
//     // For now, we'll use placeholder values - in a real implementation,
//     // these would be defined in the generated att_db.h file
//     service_handle = 0x0001;
//     status_characteristic_handle = 0x0002;
//     status_characteristic_value_handle = 0x0003;
//     status_characteristic_client_configuration_handle = 0x0004;
//     rest_characteristic_handle = 0x0005;
//     rest_characteristic_value_handle = 0x0006;
//     rest_characteristic_client_configuration_handle = 0x0007;
//     valve_control_characteristic_handle = 0x0008;
//     valve_control_characteristic_value_handle = 0x0009;
//     valve_control_characteristic_client_configuration_handle = 0x000A;
// }

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
        att_server_notify(rest_con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        Serial.println("Sent rest packet!");
        delay(40);
    }

    // log_i("Doing ble_notify");
    //  calculate whether or not to do stuff at a specific interval, in this case, every 1 second we want to send out a notify.
    static bool prevTime = false;
    bool timeChange = (millis() / 250) % 2; // changes from 0 to 1 every 250ms
    bool runNotifications = false;
    if (prevTime != timeChange)
    {
        runNotifications = true;
    }
    prevTime = timeChange;
    // Serial.printf("ABout to do notification shit %i", runNotifications);
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

        StatusPacket statusPacket(getWheel(WHEEL_FRONT_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_REAR_PASSENGER)->getSelectedInputValue(), getWheel(WHEEL_FRONT_DRIVER)->getSelectedInputValue(), getWheel(WHEEL_REAR_DRIVER)->getSelectedInputValue(), getCompressor()->getTankPressure(), statusBittset, AIPercentage, AIReadyBittset, getupdateResult());

        memcpy(status_characteristic_data, statusPacket.tx(), BTOAS_PACKET_SIZE);

        // log_i("About to notify server! %i", status_characteristic_client_configuration & 0x0001);

        // Send notification if client has enabled notifications
        // if (status_characteristic_client_configuration & 0x0001)
        // {
        for (hci_con_handle_t handle : authedClients)
        {
            att_server_notify(handle, status_characteristic_value_handle, status_characteristic_data, BTOAS_PACKET_SIZE);
            delay(10); // need a delay here or it only sends it to 1 client
        }
        //}
        // Serial.println("called att server notif");
    }
    // Serial.println("wow notif wow");
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

        // Serial.println("Sending preset packet: ");

        // // if (rest_characteristic_client_configuration & 0x0001)
        // // {
        // Serial.printf("preset handle: %i\n", con_handle);
        // att_server_notify(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        // // }
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
        //   if (rest_characteristic_client_configuration & 0x0001)
        //   {
        //  att_server_notify(con_handle, rest_characteristic_value_handle, rest_characteristic_data, BTOAS_PACKET_SIZE);
        //  }
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