# BLE API Documentation - Client Implementation Guide

###### Notice: This documentation was written by AI. Please double check the code if you run into issues, and notify the maintainers

This document provides a complete guide for implementing a BLE client to connect to the OASMan ESP32 Air Suspension Controller. It is based on the implementation patterns used in the Wireless_Controller project.

## Table of Contents

- [Client Implementation Overview](#client-implementation-overview)
- [Complete Client Implementation](#complete-client-implementation)
- [Packet Queue System](#packet-queue-system)
- [Notification Handling](#notification-handling)
- [Sending Commands](#sending-commands)
- [Client State Management](#client-state-management)
- [API Reference](#api-reference)
  - [Service and Characteristics](#service-and-characteristics)
  - [Packet Types](#packet-types)
  - [Constants and Enums](#constants-and-enums)

---

## Client Implementation Overview

The client implementation follows these key patterns:

1. **Packet Queue System**: Thread-safe queue for sending commands (prevents blocking)
2. **Notification-Based Communication**: All responses arrive via characteristic notifications
3. **Status Monitoring**: Tracks connection health via periodic status updates
4. **Authentication Flow**: Must authenticate within 5 seconds of connection
5. **Connection Management**: Handles reconnection, timeouts, and state tracking

For a complete reference implementation, see the Wireless_Controller project code.

---

## Client Implementation Flow

### 1. Initialization

When initializing your BLE client:

- Initialize your BLE stack/library
- Set MTU to 517 bytes before connecting (if supported by your platform)
- Initialize your packet queue system (see Packet Queue System section)
- Set up state variables for tracking connection status, authentication state, and status timeout

**Key State Variables Needed:**

- Connection status flag
- Authentication result state
- Status timeout timestamp
- References to the three characteristics (Status, REST, Valve Control)

### 2. Scanning for Devices

Scan for OASMan devices advertising the service UUID `679425c8-d3b4-4491-9eb2-3e3d15b625f0`.

**Scan Process:**

- Use active scanning for better discovery results
- Filter scan results by checking if devices advertise the OASMan service UUID
- Optional: Maintain a blacklist of devices that previously failed authentication to avoid repeated connection attempts
- Store found devices for connection attempts
- Recommended scan duration: 5 seconds

**What to do with found devices:**

- Store them in a queue or list for connection attempts
- Filter out devices from your auth blacklist (if implemented)

### 3. Connection

Connect to a discovered OASMan device:

**Connection Steps:**

1. Create a BLE client instance
2. Set up security/authentication callbacks if your platform requires it
3. Connect to the device (use async connection if available, with a 1-second timeout)
4. Wait for connection to complete (check connection status in a loop with timeout)
5. If connection fails, cancel and retry

**After Connection:**

1. Discover the service using UUID `679425c8-d3b4-4491-9eb2-3e3d15b625f0`
2. Get references to the three characteristics:
   - Status: `66fda100-8972-4ec7-971c-3fd30b3072ac`
   - REST: `f573f13f-b38e-415e-b8f0-59a6a19a4e02`
   - Valve Control: `e225a15a-e816-4e9d-99b7-c384f91f273b`
3. Subscribe to notifications on all three characteristics (use the same callback for all)
4. Add small delays (50ms) between characteristic subscriptions if needed

**Connection Callbacks:**

- Implement connection/disconnection callbacks to track connection state changes
- Handle security/authentication callbacks if your platform requires them

---

## Packet Queue System

The packet queue system allows non-blocking command sending, which is essential for responsive applications. Instead of blocking while waiting for writes to complete, commands are queued and sent sequentially in your main loop.

### Queue Pattern

**Purpose:** Decouple command creation from BLE transmission to avoid blocking your application.

**How it works:**

1. Your application code creates packets and adds them to a queue (non-blocking operation)
2. Your main loop processes the queue and sends packets one at a time to the REST characteristic
3. Use thread-safe queue operations if your application uses multiple threads

**Queue Functions Needed:**

- `sendRestPacket(packet)` - Add a packet to the queue (called from anywhere)
- `getBTRestPacketToSend(packet)` - Retrieve next packet from queue (called from main loop)
- `clearPackets()` - Clear all queued packets (useful on disconnect/reconnect)

**Queue Size:** Recommended maximum of 10 packets to prevent memory issues and ensure timely command processing.

**Thread Safety:** If your application uses multiple threads, protect queue access with mutexes/semaphores. If single-threaded, simple flags or counters are sufficient.

---

## Notification Handling

All responses arrive via characteristic notifications. Use a single callback function to handle notifications from both Status and REST characteristics.

### Notification Callback Pattern

**Single Callback for All Characteristics:**

- Use one callback function for all three characteristics
- Check the characteristic UUID in the callback to determine which characteristic sent the notification
- Process data based on which characteristic it came from

### Status Characteristic Notifications

**When:** Received approximately every 250ms after authentication

**What to do:**

1. Reset your status timeout timer (set timeout to current time + 5 seconds)
2. Set `hasReceivedStatus = true` (track that you've received at least one status)
3. Parse the `StatusPacket` structure:
   - Extract wheel pressures from `args16()[0-3]` (Front Passenger, Rear Passenger, Front Driver, Rear Driver)
   - Extract tank pressure from `args16()[4]`
   - Extract AI percentage from `args8()[10]`
   - Extract AI ready bitset from `args8()[11]`
   - Extract status bitset from `args32()[3]` and parse individual flags
4. Update your application state/UI with the new values

**Status Bitset Parsing:** Each bit represents a different status flag (see [StatusPacketBittset](#constants-and-enums) in API Reference). Extract individual flags using bit shifting: `(statusBits >> StatusPacketBittset::FLAG_NAME) & 1`

### REST Characteristic Notifications

**When:** Received as responses to commands you send, or for authentication responses

**What to do:**

1. Reset your status timeout timer
2. Cast the data to `BTOasPacket*` and check the `cmd` field
3. **If `cmd == AUTHPACKET`:**
   - Extract `authResult` from the packet
   - Update your authentication state
   - Store the authenticated device address if successful
4. **If authenticated and `cmd == PRESETREPORT`:**
   - Parse `PresetPacket` to extract profile pressures
   - Update your profile data storage
5. **If authenticated and `cmd == GETCONFIGVALUES`:**
   - Parse `ConfigValuesPacket` to extract all configuration values
   - Update your configuration display
6. **If authenticated and `cmd == UPDATESTATUSREQUEST`:**
   - Extract status string from the packet
   - Display firmware update status to user

**Important:** Only process command responses (PRESETREPORT, GETCONFIGVALUES, etc.) when `authenticationResult == AUTHRESULT_SUCCESS`.

---

## Sending Commands

Commands are sent via the packet queue system (see [Packet Queue System](#packet-queue-system)).

### Command Flow

1. Create the appropriate packet type with required parameters (see [Packet Types](#packet-types) in API Reference)
2. Add packet to queue using `sendRestPacket(packet)`
3. Your main loop retrieves packets from queue and writes them to the REST characteristic
4. Responses arrive via REST characteristic notifications (see [Notification Handling](#notification-handling))

### Common Command Patterns

**Air Control:**

- `AirupPacket` - Add air to all bags
- `AiroutPacket` - Release air from all bags
- `AirsmPacket` - Adjust air relative to average pressure
- `AirupQuickPacket` - Load profile then air up (main method)

**Preset Management:**

- `PresetPacket` - Request preset data (returns via notification)
- `SaveCurrentPressuresToProfilePacket` - Save current pressures to a profile
- `ReadProfilePacket` - Load profile settings
- `BaseProfilePacket` - Set base profile

**Configuration:**

- `ConfigValuesPacket` - Get or set system configuration values
  - Set first parameter to `false` to read only
  - Set first parameter to `true` and fill values to save

**Feature Toggles (via ConfigValuesPacket):**

- Rise on start, maintain pressure, air-out on shutoff, height sensor mode, safety mode, AI enabled are set via `ConfigValuesPacket` with `setValues = true` and the appropriate bits in `configFlagsBits` (see ConfigFlagsBit enum).

**Wheel Pressure:**

- `SetAirheightPacket` - Set target pressure for a specific wheel
- Takes wheel index (0-3) and pressure value

See [Packet Types](#packet-types) section for complete list and parameter details.

### Direct Valve Control

Valves can be controlled directly via the Valve Control characteristic without using the packet system.

**Process:**

1. Maintain a 32-bit integer where each bit represents a valve (bit 0 = valve 0, bit 1 = valve 1, etc.)
2. Set bits to 1 to open valves, 0 to close valves
3. Only send when the value changes (optimization)
4. Write 4 bytes (little-endian uint32_t) directly to Valve Control characteristic
5. Server tracks previous state and only toggles valves that changed

**Example State Management:**

- Track previous valve state
- Compare current desired state with previous
- Only write to characteristic when state changes
- This reduces unnecessary BLE traffic

---

## Client State Management

### Main Loop Pattern

Your main loop should handle several tasks:

**When Connected:**

1. **Send Queued Packets:** Retrieve packets from queue and write to REST characteristic (one at a time)
2. **Update Valve Control:** Check if valve state changed, and if so, write to Valve Control characteristic
3. **Check Status Timeout:** If `hasReceivedStatus` is true and current time exceeds `timeoutMS`, disconnect (connection lost)
4. **Monitor Connection:** Check if connection is still active (some platforms provide `isConnected()` methods)

**When Not Connected:**

1. **Scan for Devices:** If scanning is allowed, start a scan
2. **Process Discovered Devices:** Attempt to connect to found devices from your queue/list
3. **Handle Reconnection:** If connection fails, retry or start new scan

**Connection State Checks:**

- Monitor connection status regularly
- Handle disconnection callbacks gracefully
- Clear packet queue on disconnect
- Reset state variables (authentication, status tracking, etc.)

### Status Timeout Management

**Purpose:** Detect when connection is lost even if BLE stack doesn't notify you immediately.

**How it works:**

1. Set `hasReceivedStatus = false` initially
2. On first status notification, set `hasReceivedStatus = true` and `timeoutMS = currentTime + 5000`
3. On each subsequent status notification, reset `timeoutMS = currentTime + 5000`
4. In main loop, check: if `hasReceivedStatus && currentTime > timeoutMS`, then disconnect

**Why:** Status updates arrive every ~250ms when connected. If no status is received for 5 seconds, the connection is likely lost.

### Disconnection Handling

**When disconnecting:**

1. Set connection state to disconnected
2. Stop any active scans
3. Clear packet queue
4. Reset state variables:
   - `connected = false`
   - `hasReceivedStatus = false`
   - `authenticationResult = AUTHRESULT_WAITING`
5. Clean up client resources (cancel connection, delete client instance)
6. Allow scanning again for reconnection
7. Optionally: Show user notification about disconnection reason (auth failed, timeout, etc.)

---

## API Reference

### Service and Characteristics

All packets sent via the REST characteristic use the `BTOasPacket` structure:

```cpp
struct BTOasPacket {
    uint16_t cmd;          // BTOasIdentifier enum value
    uint8_t sender;        // unused (0)
    uint8_t recipient;     // Recipient ID (0 for broadcast)
    uint8_t args[100];     // Payload data (variable length)
};
```

**Total Size**: 104 bytes (`BTOAS_PACKET_SIZE`)

**Note**: The `args` array can be accessed as different types:

- `args8()` - Access as `uint8_t` array
- `args16()` - Access as `uint16_t` array
- `args32()` - Access as `uint32_t` array

All multi-byte values are **little-endian**.

---

### Packet Types

### Client-to-Server Packets (Commands)

#### AUTHPACKET

**Command ID**: `22`  
**Purpose**: Authenticate with the device

**Structure**:

- `args32()[0]`: `uint32_t blePasskey` - Device passkey
- `args32()[1]`: `AuthResult authResult` - Set to `AUTHRESULT_WAITING` (0) for auth request

**Usage**:

1. Create packet with `authResult = AUTHRESULT_WAITING`
2. Write to REST characteristic
3. Server responds with same packet but `authResult` set to `SUCCESS` or `FAIL`

**Example**:

```cpp
AuthPacket packet(passkey, AUTHRESULT_WAITING);
// Write packet.tx() (104 bytes) to REST characteristic
```

---

#### AIRUP

**Command ID**: `2`  
**Purpose**: Add air to all bags

**Structure**: No parameters required

**Usage**: Write empty packet with `cmd = 2` to REST characteristic

---

#### AIROUT

**Command ID**: `3`  
**Purpose**: Release air from all bags

**Structure**: No parameters required

---

#### AIRSM

**Command ID**: `4`  
**Purpose**: Adjust air relative to average pressure

**Structure**:

- `args32()[0]`: `int relativeValue` - Pressure adjustment value (relative to average)

**Example**:

```cpp
AirsmPacket packet(5); // Adjust +5 PSI relative to average
```

---

#### SAVETOPROFILE

**Command ID**: `5`  
**Purpose**: Save current profile settings to specified profile index

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile number to save to

---

#### READPROFILE

**Command ID**: `6`  
**Purpose**: Load pressure settings from specified profile

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile number to read

**Response**: Server loads the profile internally (no packet response)

---

#### AIRUPQUICK

**Command ID**: `7`  
**Purpose**: Load profile then air up (main air up method)

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile to load and air up to

---

#### BASEPROFILE

**Command ID**: `8`  
**Purpose**: Set the base profile

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile to set as base

---

#### SETAIRHEIGHT

**Command ID**: `9`  
**Purpose**: Set target pressure for a specific wheel

**Structure**:

- `args32()[0]`: `int wheelIndex` - Wheel index:
  - `0` = Front Passenger (WHEEL_FRONT_PASSENGER)
  - `1` = Rear Passenger (WHEEL_REAR_PASSENGER)
  - `2` = Front Driver (WHEEL_FRONT_DRIVER)
  - `3` = Rear Driver (WHEEL_REAR_DRIVER)
- `args32()[1]`: `int pressure` - Target pressure in PSI

**Example**:

```cpp
SetAirheightPacket packet(0, 35); // Set front passenger to 35 PSI
```

---

#### RAISEONPRESSURESET

**Command ID**: `11`  
**Purpose**: Enable/disable automatic raise when pressure is set

**Structure**:

- `args32()[0]`: `bool enable` - 1 to enable, 0 to disable

---

#### REBOOT

**Command ID**: `12`  
**Purpose**: Reboot the device

**Structure**: No parameters

---

#### CALIBRATE

**Command ID**: `13`  
**Purpose**: Calibrate system (feature unfinished)

**Structure**: No parameters

---

#### STARTWEB

**Command ID**: `14`  
**Purpose**: Start OTA update mode with WiFi credentials

**Structure**:

- `args[0-49]`: `char[]` - WiFi SSID (null-terminated, max 50 chars)
- `args[50-99]`: `char[]` - WiFi Password (null-terminated, max 50 chars)

**Example**:

```cpp
StartwebPacket packet("MyWiFi", "password123");
```

---

#### SAVECURRENTPRESSURESTOPROFILE

**Command ID**: `17`  
**Purpose**: Save current wheel pressures to a profile

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile number to save current pressures to

---

#### PRESETREPORT

**Command ID**: `18`  
**Purpose**: Request pressure values for a specific profile

**Structure**:

- `args32()[0]`: `int profileIndex` - Profile to query

**Response**: Server responds with `PresetPacket` containing profile pressures

---

#### GETCONFIGVALUES

**Command ID**: `21`  
**Purpose**: Get or set system configuration values

**Structure**:

- `args32()[0]`: `uint32_t systemShutoffTimeM` - System shutoff time in minutes
- `args32()[1]`: `uint32_t configFlagsBits` - User config flags (see ConfigFlagsBit enum)
- `args16()[4]`: `uint16_t pressureSensorMax` - Maximum pressure sensor value
- `args16()[5]`: `uint16_t bagVolumePercentage` - Bag volume percentage
- `args8()[12+0]`: `uint8_t bagMaxPressure` - Maximum bag pressure
- `args8()[12+1]`: `uint8_t compressorOnPSI` - Compressor turn-on PSI
- `args8()[12+2]`: `uint8_t compressorOffPSI` - Compressor turn-off PSI
- `args8()[12+3]`: `bool setValues` - Set to 1 to write values, 0 to only read
- `args8()[12+4]`: `uint8_t rfButtonA` - RF button A preset assignment (read-only)
- `args8()[12+5]`: `uint8_t rfButtonB` - RF button B preset assignment (read-only)
- `args8()[12+6]`: `uint8_t rfButtonC` - RF button C preset assignment (read-only)
- `args8()[12+7]`: `uint8_t rfButtonD` - RF button D preset assignment (read-only)
- `args8()[12+8]`: `uint8_t heightSensorInvertBits` - Per-wheel height sensor invert bits

**ConfigFlagsBit** (bits in `configFlagsBits`): Bit 0 = CONFIG_MAINTAIN_PRESSURE, 1 = CONFIG_RISE_ON_START, 2 = CONFIG_AIR_OUT_ON_SHUTOFF, 3 = CONFIG_HEIGHT_SENSOR_MODE, 4 = CONFIG_SAFETY_MODE, 5 = CONFIG_AI_STATUS_ENABLED.

**Usage**:

- To read: Set `setValues = 0`, write packet, receive response with all values
- To write: Set `setValues = 1`, fill in desired values, write packet, receive confirmation

---

#### COMPRESSORSTATUS

**Command ID**: `24`  
**Purpose**: Enable/disable compressor override

**Structure**:

- `args32()[0]`: `bool enable` - 1 to enable override, 0 to disable

---

#### TURNOFF

**Command ID**: `25`  
**Purpose**: Force system shutdown

**Structure**: No parameters

---

#### DETECTPRESSURESENSORS

**Command ID**: `27`  
**Purpose**: Detect and learn pressure sensors, then reboot

**Structure**: No parameters

**Note**: Triggers system reboot after detection

---

#### RESETAIPKT

**Command ID**: `29`  
**Purpose**: Reset AI pressure data

**Structure**: No parameters

---

#### BP32PKT

**Command ID**: `30`  
**Purpose**: Control Bluepad32 gamepad controller features

**Structure**:

- `args16()[0]`: `BP32CMD command` - Command type:
  - `0` = `BP32CMD_ENABLE_NEW_CONN` - Enable/disable new controller connections
  - `1` = `BP32CMD_FORGET_DEVICES` - Forget all paired controllers
  - `2` = `BP32CMD_DISCONNECT_DEVICES` - Disconnect all controllers
- `args16()[1]`: `bool value` - Value for enable/disable commands (ignored for forget/disconnect)

---

#### BROADCASTNAME

**Command ID**: `35`  
**Purpose**: Change the BLE broadcast name

**Structure**:

- `args[0-7]`: `char[]` - New broadcast name (max 8 characters, null-terminated)

**Example**:

```cpp
BroadcastNamePacket packet("MyOASMan");
```

---

#### UPDATESTATUSREQUEST

**Command ID**: `36`  
**Purpose**: Request firmware update status

**Structure**: No parameters (empty packet)

**Response**: Server responds with `UpdateStatusRequestPacket` containing status string:

- `"v<version>"` - Current version on success
- `"[F] FW DL"` - Failed to download firmware
- `"[F] Install"` - Failed to install
- `"[F] Finding"` - Failed to find version
- `"[F] No WiFi"` - Failed WiFi connection

---

#### RFCOMMAND

**Command ID**: `37`  
**Purpose**: Control RF receiver and button assignments

**Structure**:

- `args32()[0]`: `RfCommandType commandType`:
  - `1` = `RF_COMMAND_CHIP_CMD` - RF chip command
  - `2` = `RF_COMMAND_BUTTON_ASSIGN` - Assign button to preset

**For RF_COMMAND_CHIP_CMD**:

- `args32()[1]`: `RfCommandChipNumber chipCommand`:
  - `1` = `RF_CMD_DELETE` - Delete learned codes
  - `2` = `RF_CMD_LEARN_MOMENTARY` - Learn momentary mode
  - `3` = `RF_CMD_LEARN_TOGGLE` - Learn toggle mode
  - `4` = `RF_CMD_LEARN_RADIOBUTTON` - Learn radio button mode
- `args32()[2]`: Ignored

**For RF_COMMAND_BUTTON_ASSIGN**:

- `args32()[1]`: `RfCommandButtonNumber buttonNumber`:
  - `1` = `RF_BUTTON_A`
  - `2` = `RF_BUTTON_B`
  - `3` = `RF_BUTTON_C`
  - `4` = `RF_BUTTON_D`
- `args32()[2]`: `int presetNumber` - Preset profile to assign (0-based)

**Example - Learn RF code**:

```cpp
RfCommandPacket packet(RF_COMMAND_CHIP_CMD, RF_CMD_LEARN_MOMENTARY, 0);
```

**Example - Assign button to preset**:

```cpp
RfCommandPacket packet(RF_COMMAND_BUTTON_ASSIGN, RF_BUTTON_A, 2); // Assign button A to preset 2
```

---

### Server-to-Client Packets (Responses/Updates)

#### STATUSREPORT (Status Packet)

**Command ID**: `1`  
**Purpose**: Periodic status updates sent via Status characteristic

**Frequency**: Every ~250ms to all authenticated clients

**Structure**:

- `args16()[0]`: `uint16_t` - Front Passenger pressure (PSI)
- `args16()[1]`: `uint16_t` - Rear Passenger pressure (PSI)
- `args16()[2]`: `uint16_t` - Front Driver pressure (PSI)
- `args16()[3]`: `uint16_t` - Rear Driver pressure (PSI)
- `args16()[4]`: `uint16_t` - Tank pressure (PSI)
- `args8()[10]`: `uint8_t` - AI percentage
- `args8()[11]`: `uint8_t` - AI ready bitset
- `args32()[3]`: `uint32_t` - Status bitset (see below)

**Status Bitset Flags** (bits in `args32()[3]`). Only live status flags; user-config toggles (maintain pressure, rise on start, etc.) are in `ConfigValuesPacket` as `configFlagsBits`.

- Bit 0: `COMPRESSOR_FROZEN` - Compressor is frozen (StatusPacketBittset::COMPRESSOR_FROZEN = 0)
- Bit 1: `COMPRESSOR_STATUS_ON` - Compressor is running (StatusPacketBittset::COMPRESSOR_STATUS_ON = 1)
- Bit 2: `ACC_STATUS_ON` - Vehicle ACC is on (StatusPacketBittset::ACC_STATUS_ON = 2)
- Bit 3: `TIMER_STATUS_EXPIRED` - Keep-alive timer expired (StatusPacketBittset::TIMER_STATUS_EXPIRED = 3)
- Bit 4: `CLOCK` - Toggles every 250ms (StatusPacketBittset::CLOCK = 4)
- Bit 5: `EBRAKE_STATUS_ON` - Emergency brake is on (StatusPacketBittset::EBRAKE_STATUS_ON = 5)

**Parsing Example**:

```cpp
uint32_t statusBits = statusPacket->args32()[3].i;
bool compressorOn = (statusBits >> StatusPacketBittset::COMPRESSOR_STATUS_ON) & 1;
bool accOn = (statusBits >> StatusPacketBittset::ACC_STATUS_ON) & 1;
```

**Usage**: Subscribe to notifications on Status characteristic to receive updates

---

#### PRESETREPORT (Preset Packet)

**Command ID**: `18`  
**Purpose**: Response to `PRESETREPORT` request, contains profile pressure values

**Structure**:

- `args16()[0]`: `uint16_t` - Front Passenger pressure (PSI)
- `args16()[1]`: `uint16_t` - Rear Passenger pressure (PSI)
- `args16()[2]`: `uint16_t` - Front Driver pressure (PSI)
- `args16()[3]`: `uint16_t` - Rear Driver pressure (PSI)
- `args16()[4]`: `uint16_t` - Profile index

**Usage**: Received via REST characteristic notification in response to `PRESETREPORT` command

---

#### GETCONFIGVALUES Response

**Command ID**: `21`  
**Purpose**: Response containing all configuration values

**Structure**: Same as request (see GETCONFIGVALUES above)

**Usage**: Received via REST characteristic notification after sending `GETCONFIGVALUES`

---

#### AUTHPACKET Response

**Command ID**: `22`  
**Purpose**: Authentication response

**Structure**: Same as request, but `args32()[1]` contains `AUTHRESULT_SUCCESS` (1) or `AUTHRESULT_FAIL` (2)

---

#### UPDATESTATUSREQUEST Response

**Command ID**: `36`  
**Purpose**: Firmware update status response

**Structure**:

- `args[0-N]`: `char[]` - Status message string (null-terminated)

---

#### IDLE

**Command ID**: `0`  
**Purpose**: Empty/placeholder packet

**Structure**: All zeros

---

## Client Usage Guide

### Connection Flow

1. **Scan and Connect**

   - Initialize BLE: `BLEDevice::init("ClientName")`
   - Set MTU: `NimBLEDevice::setMTU(517)` (recommended before connecting)
   - Scan for devices advertising service UUID `679425c8-d3b4-4491-9eb2-3e3d15b625f0`
   - Use active scanning: `setActiveScan(true)`
   - Scan interval/window: 100ms recommended
   - Filter results: Check `advertisedDevice->isAdvertisingService(serviceUUID)`
   - Connect: `pClient->connect(advertisedDevice, true, true)` (async, use timeout)
   - Connection timeout: 1 second recommended (check `isConnected()` in loop)

2. **Discover Services and Characteristics**

   - Discover primary service: `679425c8-d3b4-4491-9eb2-3e3d15b625f0`
   - Discover characteristics:
     - Status: `66fda100-8972-4ec7-971c-3fd30b3072ac`
     - REST: `f573f13f-b38e-415e-b8f0-59a6a19a4e02`
     - Valve Control: `e225a15a-e816-4e9d-99b7-c384f91f273b`

3. **Subscribe to Notifications**

   - Subscribe to Status characteristic notifications using `subscribe(true, notifyCallback, false)`
   - Subscribe to REST characteristic notifications using `subscribe(true, notifyCallback, false)`
   - Both characteristics can use the same callback function - check UUID in callback to differentiate
   - Alternative: Manually write `0x0100` (little-endian 16-bit: notification enabled) to client configuration handles

4. **Authenticate** (Must complete within 5 seconds)

   - Create `AUTHPACKET` with passkey and `AUTHRESULT_WAITING`
   - Write to REST characteristic value handle (`0x000c`) using `writeValue(data, size, true)`
   - Wait for notification with authentication result (poll or use callback)
   - Server responds via REST characteristic notification with same packet but `authResult` updated
   - If `AUTHRESULT_SUCCESS`, proceed; if `AUTHRESULT_FAIL`, disconnect and optionally blacklist device
   - Timeout: Disconnect if no response within 5 seconds

5. **Send Commands**

   - Create appropriate packet structure
   - Write `packet.tx()` (104 bytes) to REST characteristic value handle (`0x000c`)
   - Monitor REST characteristic notifications for responses

6. **Receive Updates**
   - Status updates arrive automatically via Status characteristic notifications (~250ms interval)
   - Command responses arrive via REST characteristic notifications

### Direct Valve Control

To directly control valves without using commands:

1. Write 4 bytes (little-endian uint32_t) to Valve Control characteristic value handle (`0x000f`)
2. Each bit represents one valve (bit 0 = valve 0, bit 1 = valve 1, etc.)
3. `1` = open valve, `0` = close valve
4. Server tracks previous valve state and only toggles valves that changed

**Example**:

```cpp
unsigned int valveControlValue = 0;
// Open valve 0
valveControlValue |= (1 << 0);
// Open valves 0 and 1
valveControlValue |= (1 << 1);

// Only send when value changes (client-side optimization)
if (previousValveValue != valveControlValue) {
    pRemoteChar_ValveControl->writeValue((uint8_t*)&valveControlValue, 4, true);
    previousValveValue = valveControlValue;
}
```

**Note**: Some mobile apps may send less than 4 bytes. The server handles this by zero-padding, but clients should always send 4 bytes.

### Status Monitoring and Timeouts

**Status Update Timeout**: After authentication, clients must receive Status characteristic notifications. If no status update is received for 5 seconds, the connection is considered lost. The client should:

- Track the last received status timestamp
- Reset the timeout on each status notification received
- Disconnect and reconnect if timeout expires

**Note**: The server sends status updates approximately every 250ms to all authenticated clients.

### Error Handling

- **Authentication Timeout**: If authentication is not completed within 5 seconds of connection, the device will disconnect (unless client is a registered controller)
- **Status Timeout**: If no status update is received for 5 seconds after first status, connection is considered lost
- **Invalid Commands**: Invalid or malformed packets are ignored (no response sent)
- **Connection Loss**: If disconnected, reconnect and re-authenticate
- **Write Failures**: Always check return value of write operations; if write fails, connection may be lost

### Best Practices

1. **Always authenticate first** before sending other commands
2. **Subscribe to notifications** on both Status and REST characteristics using `subscribe(true, notifyCallback, false)`
3. **Handle notifications asynchronously** - responses may arrive at any time in notification callbacks
4. **Use Status updates** for real-time monitoring instead of polling - they arrive automatically every ~250ms
5. **Queue packets** - Use a packet queue mechanism to send commands sequentially and avoid blocking
6. **Track status timeouts** - Monitor last received status timestamp to detect connection loss
7. **Use `writeValue` with response required** - Set the `response` parameter to `true` when writing to characteristics
8. **Set MTU before connecting** - Request MTU of 517 bytes before connection for optimal performance
9. **Handle connection callbacks** - Implement connection/disconnection callbacks to handle state changes
10. **Blacklist failed auth attempts** - Track devices that fail authentication to avoid repeated attempts

### Connection Timing Details

- **Connection timeout**: 1 second (recommended client-side timeout for connection attempts)
- **Authentication wait**: Up to 5 seconds (server-side timeout)
- **Status update interval**: ~250ms
- **Status timeout**: 5 seconds (timeout if no status received)
- **Write delays**: Add small delays (10-50ms) between writes if experiencing issues

### Packet Construction Examples (C++)

```cpp
// Example 1: Create an AIRUP packet using helper class
AirupPacket pkt;
sendRestPacket(&pkt); // Queue packet for sending

// Example 2: Create a preset load command
AirupQuickPacket pkt(presetIndex); // presetIndex is 0-based
sendRestPacket(&pkt);

// Example 3: Set a config toggle (e.g. rise on start) via ConfigValuesPacket
// Update util_configValues._configFlagsBits() with the desired bits, then:
// sendConfigValuesPacket(true);

// Example 4: Create a profile save command
SaveCurrentPressuresToProfilePacket pkt(profileIndex); // 0-based index
sendRestPacket(&pkt);

// Example 5: Request preset data
PresetPacket pkt(presetIndex, 0, 0, 0, 0); // Request preset data
sendRestPacket(&pkt);

// Example 6: Authentication packet
AuthPacket authPacket(passkey, AUTHRESULT_WAITING);
pRemoteChar_Rest->writeValue(authPacket.tx(), BTOAS_PACKET_SIZE, true);

// Example 7: Config values request (read-only)
ConfigValuesPacket pkt(false, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
sendRestPacket(&pkt);
```

**Note**: Most clients use a packet queue system. The `sendRestPacket()` function queues packets that are sent in the main loop via `writeValue()`.

### Packet Parsing Examples (C++)

```cpp
// Example 1: Parse a StatusPacket from Status characteristic notification
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                    uint8_t* pData, size_t length, bool isNotify) {

    if (pBLERemoteCharacteristic->getUUID() == STATUS_CHAR_UUID) {
        StatusPacket* status = (StatusPacket*)pData;

        // Update current pressures
        currentPressures[WHEEL_FRONT_PASSENGER] = status->args16()[WHEEL_FRONT_PASSENGER].i;
        currentPressures[WHEEL_REAR_PASSENGER] = status->args16()[WHEEL_REAR_PASSENGER].i;
        currentPressures[WHEEL_FRONT_DRIVER] = status->args16()[WHEEL_FRONT_DRIVER].i;
        currentPressures[WHEEL_REAR_DRIVER] = status->args16()[WHEEL_REAR_DRIVER].i;
        currentPressures[_TANK_INDEX] = status->args16()[_TANK_INDEX].i;

        // Update status bits
        statusBittset = status->args32()[3].i;
        AIPercentage = status->args8()[10].i;
        AIReadyBittset = status->args8()[11].i;

        // Check individual status flags (live status only; user config from GETCONFIGVALUES)
        bool compressorOn = (statusBittset >> StatusPacketBittset::COMPRESSOR_STATUS_ON) & 1;
        bool accOn = (statusBittset >> StatusPacketBittset::ACC_STATUS_ON) & 1;
        bool compressorFrozen = (statusBittset >> StatusPacketBittset::COMPRESSOR_FROZEN) & 1;
        // ... etc

        // Reset timeout on successful status receipt
        timeoutMS = millis() + 5000;
    }

    // Example 2: Parse REST characteristic notifications
    if (pBLERemoteCharacteristic->getUUID() == REST_CHAR_UUID) {
        BTOasPacket* pkt = (BTOasPacket*)pData;

        // Handle authentication response
        if (pkt->cmd == AUTHPACKET) {
            AuthResult result = ((AuthPacket*)pkt)->getBleAuthResult();
            if (result == AUTHRESULT_SUCCESS) {
                authenticationResult = result;
            }
        }

        // Handle preset response
        if (pkt->cmd == PRESETREPORT) {
            PresetPacket* preset = (PresetPacket*)pkt;
            int profileIndex = preset->getProfile();
            profilePressures[profileIndex][WHEEL_FRONT_PASSENGER] = preset->args16()[WHEEL_FRONT_PASSENGER].i;
            profilePressures[profileIndex][WHEEL_REAR_PASSENGER] = preset->args16()[WHEEL_REAR_PASSENGER].i;
            profilePressures[profileIndex][WHEEL_FRONT_DRIVER] = preset->args16()[WHEEL_FRONT_DRIVER].i;
            profilePressures[profileIndex][WHEEL_REAR_DRIVER] = preset->args16()[WHEEL_REAR_DRIVER].i;
            profileUpdated = true;
        }

        // Handle config values response
        if (pkt->cmd == GETCONFIGVALUES) {
            ConfigValuesPacket* config = (ConfigValuesPacket*)pkt;
            // Extract config values using helper methods
            uint8_t bagMaxPressure = *config->_bagMaxPressure();
            uint32_t shutoffTime = *config->_systemShutoffTimeM();
            uint8_t configFlags = *config->_configFlagsBits();
            bool maintainPressure = (configFlags & (1 << ConfigFlagsBit::CONFIG_MAINTAIN_PRESSURE)) != 0;
            // ... etc
        }
    }
}
```

---

### Constants and Enums

### Characteristic Handles

- Status Value: `0x0009`
- Status Client Config: `0x000a`
- REST Value: `0x000c`
- REST Client Config: `0x000d`
- Valve Control Value: `0x000f`
- Valve Control Client Config: `0x0010`

### Packet Sizes

- Standard Packet: 104 bytes (`BTOAS_PACKET_SIZE`)
- Valve Control: 4 bytes

### Wheel Indices

- `0` = Front Passenger (`WHEEL_FRONT_PASSENGER`)
- `1` = Rear Passenger (`WHEEL_REAR_PASSENGER`)
- `2` = Front Driver (`WHEEL_FRONT_DRIVER`)
- `3` = Rear Driver (`WHEEL_REAR_DRIVER`)
- `4` = Tank (`_TANK_INDEX`) - Used in StatusPacket for tank pressure

### Timeouts

- **Authentication**: 5 seconds (server disconnects if not authenticated)
- **Status Update Interval**: ~250ms (server sends status updates)
- **Status Timeout**: 5 seconds (client should disconnect if no status received)
- **Connection Timeout**: 1 second (recommended client-side connection attempt timeout)

---

## Additional Notes

### Data Format

- All multi-byte integers are **little-endian**
- String fields are **null-terminated**
- Packet size is fixed at **104 bytes** (`BTOAS_PACKET_SIZE`)

### Connection Limits

- The device supports up to **5 simultaneous connections**
- Each connection is tracked separately with its own authentication state

### MTU Size

- Maximum MTU size is **517 bytes** (if negotiated)
- Server requests MTU of 517 bytes during connection
- Packets are designed for minimum 23-byte MTU (default BLE MTU)
- Client should set MTU to 517 before connecting: `NimBLEDevice::setMTU(517)`

### Important Behaviors

- **Some commands trigger system reboots**: `DETECTPRESSURESENSORS`, `STARTWEB`, `REBOOT`
- **RF button assignments**: Can only be read via `GETCONFIGVALUES`, but must be set via `RFCOMMAND`
- **Write responses**: Always use `writeValue(data, size, true)` - the `true` parameter is required with BTStack
- **Connection parameters**: Server requests 12-24ms connection interval after connection
- **Scan parameters**: Use active scanning (setActiveScan(true)) for better discovery

### Enums

```cpp
enum BTOasIdentifier {
    IDLE = 0,
    STATUSREPORT = 1,
    AIRUP = 2,
    AIROUT = 3,
    AIRSM = 4,
    SAVETOPROFILE = 5,
    READPROFILE = 6,
    AIRUPQUICK = 7,
    BASEPROFILE = 8,
    SETAIRHEIGHT = 9,
    RAISEONPRESSURESET = 11,
    REBOOT = 12,
    CALIBRATE = 13,
    STARTWEB = 14,
    ASSIGNRECEPIENT = 15,
    MESSAGE = 16,
    SAVECURRENTPRESSURESTOPROFILE = 17,
    PRESETREPORT = 18,
    GETCONFIGVALUES = 21,
    AUTHPACKET = 22,
    COMPRESSORSTATUS = 24,
    TURNOFF = 25,
    DETECTPRESSURESENSORS = 27,
    RESETAIPKT = 29,
    BP32PKT = 30,
    BROADCASTNAME = 35,
    UPDATESTATUSREQUEST = 36,
    RFCOMMAND = 37
};

enum StatusPacketBittset {
    COMPRESSOR_FROZEN = 0,
    COMPRESSOR_STATUS_ON = 1,
    ACC_STATUS_ON = 2,
    TIMER_STATUS_EXPIRED = 3,
    CLOCK = 4,
    EBRAKE_STATUS_ON = 5
};

enum ConfigFlagsBit {
    CONFIG_MAINTAIN_PRESSURE = 0,
    CONFIG_RISE_ON_START = 1,
    CONFIG_AIR_OUT_ON_SHUTOFF = 2,
    CONFIG_HEIGHT_SENSOR_MODE = 3,
    CONFIG_SAFETY_MODE = 4,
    CONFIG_AI_STATUS_ENABLED = 5
};

enum AuthResult {
    AUTHRESULT_WAITING = 0,
    AUTHRESULT_SUCCESS = 1,
    AUTHRESULT_FAIL = 2,
    AUTHRESULT_UPDATEKEY = 3
};
```

---

_Last Updated: Based on codebase as of commit date_
