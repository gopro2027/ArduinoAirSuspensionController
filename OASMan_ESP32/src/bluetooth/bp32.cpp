// TODO: After BTstack is implemented we need to implement it all in here (take working code from main if it is compattable with btstack)

// SPDX-License-Identifier: Apache-2.0
// Copyright 2021 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "bp32.h"

//
// README FIRST, README FIRST, README FIRST
//
// Bluepad32 has a built-in interactive console.
// By default, it is enabled (hey, this is a great feature!).
// But it is incompatible with Arduino "Serial" class.
//
// Instead of using "Serial" you can use Bluepad32 "Console" class instead.
// It is somewhat similar to Serial but not exactly the same.
//
// Should you want to still use "Serial", you have to disable the Bluepad32's console
// from "sdkconfig.defaults" with:
//    CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n

ControllerPtr myControllers[BP32_MAX_GAMEPADS];

// struct OASMANControllerData
// {
//     int32_t x, y, rx, ry;
// };

struct OASMANJoystickState
{
    bool up, down, left, right = false;
    bool rup, rdown, rleft, rright = false;
};
// OASMANControllerData previousControllerData[BP32_MAX_GAMEPADS];
OASMANJoystickState oasmanJoystickState[BP32_MAX_GAMEPADS];

// This callback gets called any time a new gamepad is connected.
// Up to 4 gamepads can be connected at the same time.
void onConnectedController(ControllerPtr ctl)
{

    if (!checkAndAllowBluetoothDevice(ctl->getProperties().btaddr))
    {
        Serial.println("BP32: Bluetooth device attempted to connect but not allowed");
        ctl->disconnect();
        return;
    }
    bool foundEmptySlot = false;
    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (myControllers[i] == nullptr)
        {
            Serial.printf("CALLBACK: Controller is connected, index=%d\n", i);
            // Additionally, you can get certain gamepad properties like:
            // Model, VID, PID, BTAddr, flags, etc.
            ControllerProperties properties = ctl->getProperties();
            Serial.printf("Controller model: %s, VID=0x%04x, PID=0x%04x, BTAddr=%02x:%02x:%02x:%02x:%02x:%02x\n", ctl->getModelName(), properties.vendor_id,
                          properties.product_id, properties.btaddr[0], properties.btaddr[1], properties.btaddr[2], properties.btaddr[3], properties.btaddr[4], properties.btaddr[5]);
            myControllers[i] = ctl;
            foundEmptySlot = true;
            // previousControllerData[i] = {0};
            break;
        }
    }
    if (!foundEmptySlot)
    {
        Serial.println("CALLBACK: Controller connected, but could not found empty slot");
    }

    // controller paired, turn back off allow pairing
    bp32_setAllowNewConnections(false);
}

void onDisconnectedController(ControllerPtr ctl)
{
    bool foundController = false;

    for (int i = 0; i < BP32_MAX_GAMEPADS; i++)
    {
        if (myControllers[i] == ctl)
        {
            Serial.printf("CALLBACK: Controller disconnected from index=%d\n", i);
            myControllers[i] = nullptr;
            foundController = true;
            break;
        }
    }

    if (!foundController)
    {
        Serial.println("CALLBACK: Controller disconnected, but not found in myControllers");
    }
}

void dumpGamepad(ControllerPtr ctl)
{
    Serial.printf(
        "idx=%d, dpad: 0x%02x, buttons: 0x%04x, axis L: %4d, %4d, axis R: %4d, %4d, brake: %4d, throttle: %4d, "
        "misc: 0x%02x, gyro x:%6d y:%6d z:%6d, accel x:%6d y:%6d z:%6d\n",
        ctl->index(),       // Controller Index
        ctl->dpad(),        // D-pad
        ctl->buttons(),     // bitmask of pressed buttons
        ctl->axisX(),       // (-511 - 512) left X Axis
        ctl->axisY(),       // (-511 - 512) left Y axis
        ctl->axisRX(),      // (-511 - 512) right X axis
        ctl->axisRY(),      // (-511 - 512) right Y axis
        ctl->brake(),       // (0 - 1023): brake button
        ctl->throttle(),    // (0 - 1023): throttle (AKA gas) button
        ctl->miscButtons(), // bitmask of pressed "misc" buttons
        ctl->gyroX(),       // Gyro X
        ctl->gyroY(),       // Gyro Y
        ctl->gyroZ(),       // Gyro Z
        ctl->accelX(),      // Accelerometer X
        ctl->accelY(),      // Accelerometer Y
        ctl->accelZ()       // Accelerometer Z
    );
}

// void dumpMouse(ControllerPtr ctl)
// {
//     Serial.printf("idx=%d, buttons: 0x%04x, scrollWheel=0x%04x, delta X: %4d, delta Y: %4d\n",
//                    ctl->index(),       // Controller Index
//                    ctl->buttons(),     // bitmask of pressed buttons
//                    ctl->scrollWheel(), // Scroll Wheel
//                    ctl->deltaX(),      // (-511 - 512) left X Axis
//                    ctl->deltaY()       // (-511 - 512) left Y axis
//     );
// }

// void dumpKeyboard(ControllerPtr ctl)
// {
//     static const char *key_names[] = {
//         // clang-format off
//         // To avoid having too much noise in this file, only a few keys are mapped to strings.
//         // Starts with "A", which is offset 4.
//         "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V",
//         "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
//         // Special keys
//         "Enter", "Escape", "Backspace", "Tab", "Spacebar", "Underscore", "Equal", "OpenBracket", "CloseBracket",
//         "Backslash", "Tilde", "SemiColon", "Quote", "GraveAccent", "Comma", "Dot", "Slash", "CapsLock",
//         // Function keys
//         "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12",
//         // Cursors and others
//         "PrintScreen", "ScrollLock", "Pause", "Insert", "Home", "PageUp", "Delete", "End", "PageDown",
//         "RightArrow", "LeftArrow", "DownArrow", "UpArrow",
//         // clang-format on
//     };
//     static const char *modifier_names[] = {
//         // clang-format off
//         // From 0xe0 to 0xe7
//         "Left Control", "Left Shift", "Left Alt", "Left Meta",
//         "Right Control", "Right Shift", "Right Alt", "Right Meta",
//         // clang-format on
//     };
//     Serial.printf("idx=%d, Pressed keys: ", ctl->index());
//     for (int key = Keyboard_A; key <= Keyboard_UpArrow; key++)
//     {
//         if (ctl->isKeyPressed(static_cast<KeyboardKey>(key)))
//         {
//             const char *keyName = key_names[key - 4];
//             Serial.printf("%s,", keyName);
//         }
//     }
//     for (int key = Keyboard_LeftControl; key <= Keyboard_RightMeta; key++)
//     {
//         if (ctl->isKeyPressed(static_cast<KeyboardKey>(key)))
//         {
//             const char *keyName = modifier_names[key - 0xe0];
//             Serial.printf("%s,", keyName);
//         }
//     }
//     Serial.printf("\n");
// }

void dumpBalanceBoard(ControllerPtr ctl)
{
    Serial.printf("idx=%d,  TL=%u, TR=%u, BL=%u, BR=%u, temperature=%d\n",
                  ctl->index(),       // Controller Index
                  ctl->topLeft(),     // top-left scale
                  ctl->topRight(),    // top-right scale
                  ctl->bottomLeft(),  // bottom-left scale
                  ctl->bottomRight(), // bottom-right scale
                  ctl->temperature()  // temperature: used to adjust the scale value's precision
    );
}

#pragma region joystick stuff

int player = 0;
int battery = 0;
bool armed = true;
bool armedIsHeldDown = false;

void runJoystickInput(bool *val,
                      Solenoid *a,
                      Solenoid *b, bool cmp)
{
    if (cmp)
    {
        // open valve for left and set oasmanJoystickState flag
        a->open();
        b->open();
        // if (*val != true)
        // {
        //     Serial.println("Opening valve");
        // }
        *val = true;
    }
    else
    {
        // first check if valve flag oasmanJoystickState is set, and if so, close valve and remove flag
        if (*val)
        {
            *val = false;
            // Serial.println("Closing valve");
            a->close();
            b->close();
        }
    }
}

void joystickLoop2(ControllerPtr ctl, bool right = false)
{

    // steps:
    // 1. check if any criteria is hit to open valves
    // 2. if criteria is hit to open valves, also save that in OASMANJoystickState
    // 3. check if criteria to close valve (basically, boolean not on the criteria to open)
    // 4. if should close, check

    OASMANJoystickState *thisJoystickState = &oasmanJoystickState[ctl->index()];

    int32_t x = right ? ctl->axisRX() : ctl->axisX();
    int32_t y = right ? ctl->axisRY() : ctl->axisY();

    // for single joystick controllers, go ahead and override it so if we are holding the b button it grabs the only joystick available
    if (ctl->b())
    {
        if (right)
        {
            x = ctl->axisX();
            y = ctl->axisY();
        }
        else
        {
            // left joystick is always 0 while b button is pressed
            x = 0;
            y = 0;
        }
    }

    const int threshold = 200;

    bool *val;
    Solenoid *a;
    Solenoid *b;

    // left
    val = right ? &thisJoystickState->rleft : &thisJoystickState->left;
    a = right ? getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid() : getWheel(WHEEL_REAR_DRIVER)->getInSolenoid();
    runJoystickInput(val, a, b, x <= -threshold);

    // right
    val = right ? &thisJoystickState->rright : &thisJoystickState->right;
    a = right ? getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid() : getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid();
    runJoystickInput(val, a, b, x >= threshold);

    // up
    val = right ? &thisJoystickState->rup : &thisJoystickState->up;
    a = right ? getWheel(WHEEL_REAR_DRIVER)->getInSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid() : getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid();
    runJoystickInput(val, a, b, y <= -threshold);

    // down
    val = right ? &thisJoystickState->rup : &thisJoystickState->up;
    a = right ? getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid() : getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid();
    runJoystickInput(val, a, b, y >= threshold);
}

#ifdef oldjoystickcode
enum JoystickMode
{
    j_none,
    j_left,
    j_right,
    j_up,
    j_down
};

enum OpenClose
{
    v_close,
    v_open
};

JoystickMode leftMode = j_none;
JoystickMode rightMode = j_none;

void printJoystick(JoystickMode js)
{
    if (js == j_none)
    {
        Serial.print("idle");
    }
    if (js == j_up)
    {
        Serial.print("up");
    }
    if (js == j_down)
    {
        Serial.print("down");
    }
    if (js == j_left)
    {
        Serial.print("left");
    }
    if (js == j_right)
    {
        Serial.print("right");
    }
}

void updateJoystickVal(int8_t x, int8_t y, JoystickMode &joystickMode)
{
    // this function will only return the opposite of what it currently is. So ie can't go from left to right... would have to go left to none, and then on the next call go to right. This is so it knows to close the valve. ps3 controller should generally be quick enouch to catch a joystick movement mid move to do this properly

    // check if new values of x and y mean we should go into a mode
    if (joystickMode == j_none)
    {
        if (x <= -100)
        {
            joystickMode = j_left;
        }
        else if (x >= 100)
        {
            joystickMode = j_right;
        }
        else if (y <= -100)
        {
            joystickMode = j_up;
        }
        else if (y >= 100)
        {
            joystickMode = j_down;
        }
    }
    else
    {
        // currently was running, check if we need to disable it based on new inputs
        if (joystickMode == j_left && x > -100)
        {
            joystickMode = j_none;
        }
        else if (joystickMode == j_right && x < 100)
        {
            joystickMode = j_none;
        }
        else if (joystickMode == j_up && y > -100)
        {
            joystickMode = j_none;
        }
        else if (joystickMode == j_down && y < 100)
        {
            joystickMode = j_none;
        }
    }
}

void joystickLoop(ControllerPtr ctl)
{
    OASMANControllerData *thispreviousControllerData = &previousControllerData[ctl->index()];
    OASMANControllerData currentControllerData;
    currentControllerData.x = ctl->axisX();
    currentControllerData.y = ctl->axisY();
    currentControllerData.rx = ctl->axisRX();
    currentControllerData.ry = ctl->axisRY();
    //---------------- Analog stick value events ---------------
    if (abs(currentControllerData.x - thispreviousControllerData->x) + abs(currentControllerData.y - thispreviousControllerData->y) > 2)
    {
        Serial.print("Moved the left stick:");
        Serial.print(" x=");
        Serial.print(currentControllerData.x, DEC);
        Serial.print(" y=");
        Serial.print(currentControllerData.y, DEC);
        Serial.println();

        JoystickMode prev = leftMode;
        updateJoystickVal(currentControllerData.x, currentControllerData.y, leftMode);
        if (prev != leftMode)
        {

            // by default we assume prev is j_none and the new leftMode is a side so we would be opening the valve
            OpenClose openOrClose = v_open;
            JoystickMode cur = leftMode;

            // if it is the opposite and we are actually going from valve open to close, reverse it
            if (leftMode == j_none)
            { // just stopped so close instead
                openOrClose = v_close;
                cur = prev;
            }

            // left joystick specifically

            // left joystick left would be air up left side of car
            if (cur == j_left)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->open();
                    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->close();
                    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->close();
                }
            }

            // left joystick right would be air out left side of car
            if (cur == j_right)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->open();
                    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->close();
                    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->close();
                }
            }

            // left joystick up would be air up front side of car
            if (cur == j_up)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->open();
                    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->close();
                    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->close();
                }
            }

            // left joystick down would be air down front side of car
            if (cur == j_down)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->open();
                    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->close();
                    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->close();
                }
            }
        }
    }

    if (abs(currentControllerData.rx - thispreviousControllerData->rx) + abs(currentControllerData.ry - thispreviousControllerData->ry) > 2)
    {
        // Serial.print("Moved the right stick:");
        // Serial.print(" x=");
        // Serial.print(Ps3.data.analog.stick.rx, DEC);
        // Serial.print(" y=");
        // Serial.print(Ps3.data.analog.stick.ry, DEC);
        // Serial.println();

        JoystickMode prev = rightMode;
        updateJoystickVal(currentControllerData.rx, currentControllerData.ry, rightMode);
        if (prev != rightMode)
        {

            // by default we assume prev is j_none and the new rightMode is a side so we would be opening the valve
            OpenClose openOrClose = v_open;
            JoystickMode cur = rightMode;

            // if it is the opposite and we are actually going from valve open to close, reverse it
            if (rightMode == j_none)
            { // just stopped so close instead
                openOrClose = v_close;
                cur = prev;
            }

            // right joystick specifically

            // right joystick left would be air out right side of car
            if (cur == j_left)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->open();
                    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->close();
                    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->close();
                }
            }

            // right joystick right would be air up right side of car
            if (cur == j_right)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->open();
                    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->close();
                    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->close();
                }
            }

            // right joystick up would be air up rear side of car
            if (cur == j_up)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->open();
                    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->close();
                    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->close();
                }
            }

            // right joystick down would be air down rear side of car
            if (cur == j_down)
            {
                if (openOrClose == v_open)
                {
                    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->open();
                    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->open();
                }
                else
                {
                    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->close();
                    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->close();
                }
            }
        }
    }

    printJoystick(leftMode);
    Serial.print("\t");
    printJoystick(rightMode);
    Serial.println();

    // copy over controller data
    memcpy(thispreviousControllerData, &currentControllerData, sizeof(OASMANControllerData));
}

#endif

#pragma endregion

void processGamepad(ControllerPtr ctl)
{
    // There are different ways to query whether a button is pressed.
    // // By query each button individually:
    // //  a(), b(), x(), y(), l1(), etc...
    // if (ctl->a())
    // {
    //     static int colorIdx = 0;
    //     // Some gamepads like DS4 and DualSense support changing the color LED.
    //     // It is possible to change it by calling:
    //     switch (colorIdx % 3)
    //     {
    //     case 0:
    //         // Red
    //         ctl->setColorLED(255, 0, 0);
    //         break;
    //     case 1:
    //         // Green
    //         ctl->setColorLED(0, 255, 0);
    //         break;
    //     case 2:
    //         // Blue
    //         ctl->setColorLED(0, 0, 255);
    //         break;
    //     }
    //     colorIdx++;
    // }

    // if (ctl->b())
    // {
    //     // Turn on the 4 LED. Each bit represents one LED.
    //     static int led = 0;
    //     led++;
    //     // Some gamepads like the DS3, DualSense, Nintendo Wii, Nintendo Switch
    //     // support changing the "Player LEDs": those 4 LEDs that usually indicate
    //     // the "gamepad seat".
    //     // It is possible to change them by calling:
    //     ctl->setPlayerLEDs(led & 0x0f);
    // }

    // if (ctl->x())
    // {
    //     // Some gamepads like DS3, DS4, DualSense, Switch, Xbox One S, Stadia support rumble.
    //     // It is possible to set it by calling:
    //     // Some controllers have two motors: "strong motor", "weak motor".
    //     // It is possible to control them independently.
    //     ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
    //                         0x40 /* strongMagnitude */);
    // }

    // // Another way to query controller data is by getting the buttons() function.
    // // See how the different "dump*" functions dump the Controller info.
    dumpGamepad(ctl);

    // // See ArduinoController.h for all the available functions.

    if (ctl->r1() && ctl->l1())
    {
        if (armedIsHeldDown == false)
        {
            // this means we just pressed them, flip value of armed
            armed = !armed;
            ctl->playDualRumble(0 /* delayedStartMs */, 250 /* durationMs */, 0x80 /* weakMagnitude */,
                                0x40 /* strongMagnitude */);
        }
        armedIsHeldDown = true;
    }
    else
    {
        armedIsHeldDown = false;
    }

    if (!armed)
    {
        return;
    }

    // okay armed, let code run

    if (ctl->miscSystem()) // pressing the system button on the controller
    {
        // Serial.println("Pressing both the select and start buttons");
        //  setinternalReboot(true);
        ctl->disconnect();
    }

    // joystickLoop(ctl);
    joystickLoop2(ctl, false);
    joystickLoop2(ctl, true);
}

// void processMouse(ControllerPtr ctl)
// {
//     // This is just an example.
//     if (ctl->scrollWheel() > 0)
//     {
//         // Do Something
//     }
//     else if (ctl->scrollWheel() < 0)
//     {
//         // Do something else
//     }

//     // See "dumpMouse" for possible things to query.
//     dumpMouse(ctl);
// }

// void processKeyboard(ControllerPtr ctl)
// {
//     if (!ctl->isAnyKeyPressed())
//         return;

//     // This is just an example.
//     if (ctl->isKeyPressed(Keyboard_A))
//     {
//         // Do Something
//         Serial.println("Key 'A' pressed");
//     }

//     // Don't do "else" here.
//     // Multiple keys can be pressed at the same time.
//     if (ctl->isKeyPressed(Keyboard_LeftShift))
//     {
//         // Do something else
//         Serial.println("Key 'LEFT SHIFT' pressed");
//     }

//     // Don't do "else" here.
//     // Multiple keys can be pressed at the same time.
//     if (ctl->isKeyPressed(Keyboard_LeftArrow))
//     {
//         // Do something else
//         Serial.println("Key 'Left Arrow' pressed");
//     }

//     // See "dumpKeyboard" for possible things to query.
//     dumpKeyboard(ctl);
// }

#ifdef djkfgbhvdjkfhvguidesrhgvuidr_oldbalanceboardcode
void balanceBoardCode(ControllerPtr ctl, uint16_t tl, uint16_t tr, uint16_t bl, uint16_t br, bool right = false)
{

    int32_t rightY = tr - br;
    int32_t leftY = tl - bl;

    OASMANJoystickState *thisJoystickState = &oasmanJoystickState[ctl->index()];

    int32_t y = right ? rightY : leftY;

    const int threshold = 10000;

    bool *val;
    Solenoid *a;
    Solenoid *b;

    // up
    val = right ? &thisJoystickState->rup : &thisJoystickState->up;
    a = right ? getWheel(WHEEL_REAR_DRIVER)->getInSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid() : getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid();
    runJoystickInput(val, a, b, y <= -threshold);

    // down
    val = right ? &thisJoystickState->rdown : &thisJoystickState->down;
    a = right ? getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid() : getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid();
    b = right ? getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid() : getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid();
    runJoystickInput(val, a, b, y >= threshold);

    // getManifold()->debugOut();
}

void processBalanceBoard(ControllerPtr ctl)
{

    uint16_t tl = 0, tr = 0, bl = 0, br = 0;
    static bool right = false;

    uint16_t largest = 0;
    if (ctl->topLeft() > largest)
        largest = ctl->topLeft();
    if (ctl->topRight() > largest)
        largest = ctl->topRight();
    if (ctl->bottomLeft() > largest)
        largest = ctl->bottomLeft();
    if (ctl->bottomRight() > largest)
        largest = ctl->bottomRight();

    // This is just an example.
    if (largest > 10000)
    {
        if (largest == ctl->topLeft())
        {
            // Serial.println("Top Left");
            tl = largest;
        }
        else if (largest == ctl->topRight())
        {
            // Serial.println("Top Right");
            tr = largest;
            right = true;
        }
        else if (largest == ctl->bottomLeft())
        {
            // Serial.println("Bottom Left");
            bl = largest;
        }
        else if (largest == ctl->bottomRight())
        {
            // Serial.println("Bottom Right");
            br = largest;
            right = true;
        }
    }

    // TODO: update this code so we can have both right and left pressed at the same time. Probably requires modifying the code above this so i can have more than one pressed register at a time. maybe do the joystick calculation above where we do joystick = top - buttom ????
    balanceBoardCode(ctl, tl, tr, bl, br, right);
    balanceBoardCode(ctl, 0, 0, 0, 0, !right); // other side is not being pressed, reset it. Later we can separate the right and left more to make it so you can press both right and left at the same time

    // See "dumpBalanceBoard" for possible things to query.
    // dumpBalanceBoard(ctl);
}

#endif

void processBalanceBoardRight(ControllerPtr ctl)
{
    uint16_t tr = 0, br = 0;

    uint16_t largest = 0;
    if (ctl->topRight() > largest)
        largest = ctl->topRight();
    if (ctl->bottomRight() > largest)
        largest = ctl->bottomRight();

    const int threshold = 10000;

    if (largest > threshold)
    {
        if (largest == ctl->topRight())
        {
            tr = largest;
        }
        else if (largest == ctl->bottomRight())
        {
            br = largest;
        }
    }

    int32_t joystickVal = (int32_t)tr - (int32_t)br;

    OASMANJoystickState *thisJoystickState = &oasmanJoystickState[ctl->index()];

    // up
    runJoystickInput(&thisJoystickState->rup, getWheel(WHEEL_REAR_DRIVER)->getInSolenoid(), getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid(), joystickVal >= threshold);

    // down
    runJoystickInput(&thisJoystickState->rdown, getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid(), getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid(), joystickVal <= -threshold);
}
void processBalanceBoardLeft(ControllerPtr ctl)
{
    uint16_t tl = 0, bl = 0;

    uint16_t largest = 0;
    if (ctl->topLeft() > largest)
        largest = ctl->topLeft();
    if (ctl->bottomLeft() > largest)
        largest = ctl->bottomLeft();

    const int threshold = 10000;

    if (largest > threshold)
    {
        if (largest == ctl->topLeft())
        {
            tl = largest;
        }
        else if (largest == ctl->bottomLeft())
        {
            bl = largest;
        }
    }

    int32_t joystickVal = (int32_t)tl - (int32_t)bl;

    OASMANJoystickState *thisJoystickState = &oasmanJoystickState[ctl->index()];

    // up
    runJoystickInput(&thisJoystickState->up, getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid(), getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid(), joystickVal >= threshold);

    // down
    runJoystickInput(&thisJoystickState->down, getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid(), getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid(), joystickVal <= -threshold);
}

void processControllers()
{
    for (auto myController : myControllers)
    {
        if (myController && myController->isConnected() && myController->hasData())
        {
            if (myController->isGamepad())
            {
                processGamepad(myController);
            }
            // else if (myController->isMouse())
            // {
            //     processMouse(myController);
            // }
            // else if (myController->isKeyboard())
            // {
            //     processKeyboard(myController);
            // }
            else if (myController->isBalanceBoard())
            {
                processBalanceBoardLeft(myController);
                processBalanceBoardRight(myController);
                getManifold()->debugOut();
            }
            else
            {
                Serial.printf("Unsupported controller\n");
                dumpGamepad(myController);
            }
        }
    }
}

// Arduino setup function. Runs in CPU 1
void bp32_setup()
{
    Serial.printf("Firmware: %s\n", BP32.firmwareVersion());
    const uint8_t *addr = BP32.localBdAddress();
    Serial.printf("BD Addr: %2X:%2X:%2X:%2X:%2X:%2X\n", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    loadAllowedBluetoothDevices();

    bp32_setAllowNewConnections(false);

    // Setup the Bluepad32 callbacks, and the default behavior for scanning or not.
    // By default, if the "startScanning" parameter is not passed, it will do the "start scanning".
    // Notice that "Start scanning" will try to auto-connect to devices that are compatible with Bluepad32.
    // E.g: if a Gamepad, keyboard or mouse are detected, it will try to auto connect to them.
    BP32.setup(&onConnectedController, &onDisconnectedController);

    // Notice that scanning can be stopped / started at any time by calling:
    // bp32_setAllowNewConnections(enabled);

    // "bp32_forgetDevices()" should be called when the user performs
    // a "device factory reset", or similar.
    // Calling "bp32_forgetDevices" in setup() just as an example.
    // Forgetting Bluetooth keys prevents "paired" gamepads to reconnect.
    // But it might also fix some connection / re-connection issues.
    // BP32.bp32_forgetDevices();

    // Enables mouse / touchpad support for gamepads that support them.
    // When enabled, controllers like DualSense and DualShock4 generate two connected devices:
    // - First one: the gamepad
    // - Second one, which is a "virtual device", is a mouse.
    // By default, it is disabled.
    BP32.enableVirtualDevice(false);

    // Enables the BLE Service in Bluepad32.
    // This service allows clients, like a mobile app, to setup and see the state of Bluepad32.
    // By default, it is disabled.
    BP32.enableBLEService(false);

    // by default lock it down so random controllers cannot connect!
    bp32_setAllowNewConnections(false);
}

// Arduino loop function. Runs in CPU 1.
void bp32_loop()
{
    // This call fetches all the controllers' data.
    // Call this function in your main loop.
    bool dataUpdated = BP32.update();
    if (dataUpdated)
        processControllers();

    // The main loop must have some kind of "yield to lower priority task" event.
    // Otherwise, the watchdog will get triggered.
    // If your main loop doesn't have one, just add a simple `vTaskDelay(1)`.
    // Detailed info here:
    // https://stackoverflow.com/questions/66278271/task-watchdog-got-triggered-the-tasks-did-not-reset-the-watchdog-in-time

    delay(100);
}

struct BTDeviceMac
{
    uint8_t addr[6];
};
#define MAX_ALLOWED_BLUETOOTH_DEVICES 20
static BTDeviceMac allowedBluetoothDevices[MAX_ALLOWED_BLUETOOTH_DEVICES];

#define BLUETOOTH_SAVED_DEVICES_FILE "/allowed_bt_devices.dat"
void clearAllowedBluetoothDevices()
{
    deleteFile(BLUETOOTH_SAVED_DEVICES_FILE);
    loadAllowedBluetoothDevices();
}
void loadAllowedBluetoothDevices()
{
    Serial.println("Loading allowed Bluetooth devices...");
    memset(allowedBluetoothDevices, 0, sizeof(allowedBluetoothDevices));
    readBytes(BLUETOOTH_SAVED_DEVICES_FILE, allowedBluetoothDevices, sizeof(allowedBluetoothDevices));
}

void addAllowedBluetoothDevice(const uint8_t *addr)
{
    bool written = false;
    for (int i = 0; i < MAX_ALLOWED_BLUETOOTH_DEVICES; i++)
    {
        if (allowedBluetoothDevices[i].addr[0] == 0 && allowedBluetoothDevices[i].addr[1] == 0 &&
            allowedBluetoothDevices[i].addr[2] == 0 && allowedBluetoothDevices[i].addr[3] == 0 &&
            allowedBluetoothDevices[i].addr[4] == 0 && allowedBluetoothDevices[i].addr[5] == 0) // could have just converted the 4 bytes to an int32 and checked if 0 and probably been fine but figure'd why not be exact
        {
            memcpy(allowedBluetoothDevices[i].addr, addr, 6);
            written = true;
            break;
        }
    }
    if (written)
    {
        writeBytes(BLUETOOTH_SAVED_DEVICES_FILE, allowedBluetoothDevices, sizeof(allowedBluetoothDevices));
    }
    else
    {
        Serial.println("BP32: No space left to add new Bluetooth device.");
    }
}

bool isBTDeviceARegisteredController(const uint8_t *addr)
{
    for (int i = 0; i < MAX_ALLOWED_BLUETOOTH_DEVICES; i++)
    {
        if (memcmp(allowedBluetoothDevices[i].addr, addr, 6) == 0)
        {
            return true;
        }
    }
    return false;
}

bool areNewConnectionsAllowed = false;
bool checkAndAllowBluetoothDevice(const uint8_t *addr)
{
    for (int i = 0; i < MAX_ALLOWED_BLUETOOTH_DEVICES; i++)
    {
        if (memcmp(allowedBluetoothDevices[i].addr, addr, 6) == 0)
        {
            Serial.printf("BP32: Allowing previously accepted device: %02x:%02x:%02x:%02x:%02x:%02x\n",
                          addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
            return true;
        }
    }
    if (areNewConnectionsAllowed)
    {
        Serial.printf("BP32: Allowing new device: %02x:%02x:%02x:%02x:%02x:%02x\n",
                      addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
        addAllowedBluetoothDevice(addr);
        return true;
    }
    return false;
}

void bp32_disconnectControllers()
{
    for (auto myController : myControllers)
    {
        if (myController && myController->isConnected())
        {
            myController->disconnect();
        }
    }
}

void bp32_forgetDevices()
{
    Serial.println("Forgetting Bluetooth keys...");
    BP32.forgetBluetoothKeys();
    clearAllowedBluetoothDevices(); // delete our saved file of bt macs

    // disconnect all currently connected controllers
    bp32_disconnectControllers();
}

void bp32_setAllowNewConnections(bool allow)
{
    Serial.printf("BP32: Setting allow connections to %d\n", allow);
    BP32.enableNewBluetoothConnections(allow);
    areNewConnectionsAllowed = allow;
}
