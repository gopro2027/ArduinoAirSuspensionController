#include "ps3_controller.h"

#if ENABLE_PS3_CONTROLLER_SUPPORT

int player = 0;
int battery = 0;
bool armed = true;
bool armedIsHeldDown = false;

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

// From Ps3Controller.data.button aka ps3_button_t
enum ps3_button_offsets
{
    ps3_select,
    ps3_l3,
    ps3_r3,
    ps3_start,

    ps3_up,
    ps3_right,
    ps3_down,
    ps3_left,

    ps3_l2,
    ps3_r2,
    ps3_l1,
    ps3_r1,

    ps3_triangle,
    ps3_circle,
    ps3_cross,
    ps3_square,

    ps3_ps,
    PS3_BUTTON_COUNT
};

JoystickMode leftMode = j_none;
JoystickMode rightMode = j_none;

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

void loadProfileAirUpQuick(int profileIndex)
{
    if (profileIndex > MAX_PROFILE_COUNT)
    {
        return;
    }
    // load profile then air up
    readProfile(profileIndex);
    airUp(true);
}

bool crossPressed = false;
bool squarePressed = false;
bool trianglePressed = false;
bool circlePressed = false;
bool upPressed = false;
bool downPressed = false;

#define bitfieldto32(field) (*(int *)&field)
#define isbitfieldset(field, idx) ((bitfieldto32(field) & (1 << idx)) >> idx)
uint8_t konamiIndex = 0;
uint8_t konamiCompletions = 0;
uint8_t konamiCode[] = {
    ps3_button_offsets::ps3_up,
    ps3_button_offsets::ps3_up,
    ps3_button_offsets::ps3_down,
    ps3_button_offsets::ps3_down,
    ps3_button_offsets::ps3_left,
    ps3_button_offsets::ps3_right,
    ps3_button_offsets::ps3_left,
    ps3_button_offsets::ps3_right,
    ps3_button_offsets::ps3_circle,
    ps3_button_offsets::ps3_cross};

void driverUpPassDown(int ms)
{
    // driver goes up, passenger goes down
    Serial.println("Driver up passenger down");
    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->open();
    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->open();
    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->open();
    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->open();
    delay(ms);
    getWheel(WHEEL_FRONT_DRIVER)->getInSolenoid()->close();
    getWheel(WHEEL_REAR_DRIVER)->getInSolenoid()->close();
    getWheel(WHEEL_FRONT_PASSENGER)->getOutSolenoid()->close();
    getWheel(WHEEL_REAR_PASSENGER)->getOutSolenoid()->close();
}
void passUpDriverDown(int ms)
{
    // passenger goes up, driver goes down
    Serial.println("Passenger up driver down");
    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->open();
    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->open();
    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->open();
    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->open();
    delay(ms);
    getWheel(WHEEL_FRONT_DRIVER)->getOutSolenoid()->close();
    getWheel(WHEEL_REAR_DRIVER)->getOutSolenoid()->close();
    getWheel(WHEEL_FRONT_PASSENGER)->getInSolenoid()->close();
    getWheel(WHEEL_REAR_PASSENGER)->getInSolenoid()->close();
}
void doDance()
{
    Serial.println("Doing dance sequence!");
    int timing = 600;
    // initialize by dropping for half the time of one side
    passUpDriverDown(timing / 2);
    for (int i = 0; i < 5; i++)
    {
        driverUpPassDown(timing);
        passUpDriverDown(timing);
    }

    Serial.println("Loading profile 2");
    // TODO: make base profile work (look in other spots in app for this)
    // go to the 2nd profile since that is default right now for ride height.
    loadProfileAirUpQuick(2);
}

void updateKonami()
{
    for (int idx = 0; idx < ps3_button_offsets::PS3_BUTTON_COUNT; idx++)
    {
        if (isbitfieldset(Ps3.event.button_up, idx))
        {
            if (konamiCode[konamiIndex] == idx)
            {
                konamiIndex++;
                Serial.println(konamiIndex);
                if (konamiIndex == (sizeof(konamiCode) / sizeof(*konamiCode)))
                {
                    Serial.println("Konami code complete!");
                    konamiCompletions++;
                    Ps3.setRumble(100.0, 5000);
                    konamiIndex = 0;
                    if (konamiCompletions > 1)
                    {
                        do_dance = true;
                    }
                }
            }
            else
            {
                konamiIndex = 0;
            }
        }
    }
}

// This function is really conveluted, but it seems to fix the issue with sometimes the rumble getting cut off in some way by other updates
bool skipNextPlayerUpate = false;
void feedback()
{
    skipNextPlayerUpate = true;
    delay(50); // this line is impportant bc if we send vibrate too close to a previous cmd it won't work
    Ps3.setRumble(100.0, 100);
    skipNextPlayerUpate = true;
    delay(50);
    skipNextPlayerUpate = true;
}

void notify()
{

    updateKonami();

    if (Ps3.data.button.r1 && Ps3.data.button.l1)
    {
        if (armedIsHeldDown == false)
        {
            // this means we just pressed them, flip value of armed
            armed = !armed;
            feedback();
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

    // Old controllers go bad and the bluetooth starts to act up. I noticed when they act up they don't often go above analog value of 20, but still trigger Ps3.event.button_down. Implemented the same thing but must be greater than 50 to trigger

    // if (abs(Ps3.event.analog_changed.button.up))
    // {
    //     bool prev = upPressed;
    //     upPressed = Ps3.data.analog.button.up > 50;
    //     if (upPressed && prev != upPressed)
    //     {
    //         Serial.println("[Dpad Up] Air up");
    //         airUp();
    //         feedback();
    //     }
    // }

    // if (abs(Ps3.event.analog_changed.button.down))
    // {
    //     bool prev = downPressed;
    //     downPressed = Ps3.data.analog.button.down > 50;
    //     if (downPressed && prev != downPressed)
    //     {
    //         Serial.println("[Dpad Down] Air out");
    //         airOut();
    //         feedback();
    //     }
    // }

    // if (abs(Ps3.event.analog_changed.button.cross))
    // {
    //     bool prev = crossPressed;
    //     crossPressed = Ps3.data.analog.button.cross > 50;
    //     if (crossPressed && prev != crossPressed)
    //     {
    //         Serial.println("[X] Profile 1");
    //         loadProfileAirUpQuick(0);
    //         feedback();
    //     }
    // }

    // if (abs(Ps3.event.analog_changed.button.square))
    // {
    //     bool prev = squarePressed;
    //     squarePressed = Ps3.data.analog.button.square > 50;
    //     if (squarePressed && prev != squarePressed)
    //     {
    //         Serial.println("[Square] Profile 2");
    //         loadProfileAirUpQuick(1);
    //         feedback();
    //     }
    // }

    // if (abs(Ps3.event.analog_changed.button.triangle))
    // {
    //     bool prev = trianglePressed;
    //     trianglePressed = Ps3.data.analog.button.triangle > 50;
    //     if (trianglePressed && prev != trianglePressed)
    //     {
    //         Serial.println("[Triangle] Profile 3");
    //         loadProfileAirUpQuick(2);
    //         feedback();
    //     }
    // }

    // if (abs(Ps3.event.analog_changed.button.circle))
    // {
    //     bool prev = circlePressed;
    //     circlePressed = Ps3.data.analog.button.circle > 50;
    //     if (circlePressed && prev != circlePressed)
    //     {
    //         Serial.println("[Circle] Profile 4");
    //         loadProfileAirUpQuick(3);
    //         feedback();
    //     }
    // }

    // //--- Digital cross/square/triangle/circle button events ---
    // if (Ps3.event.button_down.cross)
    //     Serial.println("Started pressing the cross button");
    // if (Ps3.event.button_up.cross)
    //     Serial.println("Released the cross button");

    // if (Ps3.event.button_down.square)
    //     Serial.println("Started pressing the square button");
    // if (Ps3.event.button_up.square)
    //     Serial.println("Released the square button");

    // if (Ps3.event.button_down.triangle)
    //     Serial.println("Started pressing the triangle button");
    // if (Ps3.event.button_up.triangle)
    //     Serial.println("Released the triangle button");

    // if (Ps3.event.button_down.circle)
    //     Serial.println("Started pressing the circle button");
    // if (Ps3.event.button_up.circle)
    //     Serial.println("Released the circle button");

    // //--------------- Digital D-pad button events --------------
    // if (Ps3.event.button_down.up)
    //     Serial.println("Started pressing the up button");
    // if (Ps3.event.button_up.up)
    //     Serial.println("Released the up button");

    // if (Ps3.event.button_down.right)
    //     Serial.println("Started pressing the right button");
    // if (Ps3.event.button_up.right)
    //     Serial.println("Released the right button");

    // if (Ps3.event.button_down.down)
    //     Serial.println("Started pressing the down button");
    // if (Ps3.event.button_up.down)
    //     Serial.println("Released the down button");

    // if (Ps3.event.button_down.left)
    //     Serial.println("Started pressing the left button");
    // if (Ps3.event.button_up.left)
    //     Serial.println("Released the left button");

    // //------------- Digital shoulder button events -------------
    // if (Ps3.event.button_down.l1)
    //     Serial.println("Started pressing the left shoulder button");
    // if (Ps3.event.button_up.l1)
    //     Serial.println("Released the left shoulder button");

    // if (Ps3.event.button_down.r1)
    //     Serial.println("Started pressing the right shoulder button");
    // if (Ps3.event.button_up.r1)
    //     Serial.println("Released the right shoulder button");

    // //-------------- Digital trigger button events -------------
    // if (Ps3.event.button_down.l2)
    //     Serial.println("Started pressing the left trigger button");
    // if (Ps3.event.button_up.l2)
    //     Serial.println("Released the left trigger button");

    // if (Ps3.event.button_down.r2)
    //     Serial.println("Started pressing the right trigger button");
    // if (Ps3.event.button_up.r2)
    //     Serial.println("Released the right trigger button");

    // //--------------- Digital stick button events --------------
    // if (Ps3.event.button_down.l3)
    //     Serial.println("Started pressing the left stick button");
    // if (Ps3.event.button_up.l3)
    //     Serial.println("Released the left stick button");

    // if (Ps3.event.button_down.r3)
    //     Serial.println("Started pressing the right stick button");
    // if (Ps3.event.button_up.r3)
    //     Serial.println("Released the right stick button");

    // //---------- Digital select/start/ps button events ---------
    // if (Ps3.event.button_down.select)
    //     Serial.println("Started pressing the select button");
    // if (Ps3.event.button_up.select)
    //     Serial.println("Released the select button");

    // if (Ps3.event.button_down.start)
    //     Serial.println("Started pressing the start button");
    // if (Ps3.event.button_up.start)
    //     Serial.println("Released the start button");

    // if (Ps3.event.button_down.ps)
    //     Serial.println("Started pressing the Playstation button");
    // if (Ps3.event.button_up.ps)
    //     Serial.println("Released the Playstation button");

    //---------------- Analog stick value events ---------------
    if (abs(Ps3.event.analog_changed.stick.lx) + abs(Ps3.event.analog_changed.stick.ly) > 2)
    {
        // Serial.print("Moved the left stick:");
        // Serial.print(" x=");
        // Serial.print(Ps3.data.analog.stick.lx, DEC);
        // Serial.print(" y=");
        // Serial.print(Ps3.data.analog.stick.ly, DEC);
        // Serial.println();

        JoystickMode prev = leftMode;
        updateJoystickVal(Ps3.data.analog.stick.lx, Ps3.data.analog.stick.ly, leftMode);
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

    if (abs(Ps3.event.analog_changed.stick.rx) + abs(Ps3.event.analog_changed.stick.ry) > 2)
    {
        // Serial.print("Moved the right stick:");
        // Serial.print(" x=");
        // Serial.print(Ps3.data.analog.stick.rx, DEC);
        // Serial.print(" y=");
        // Serial.print(Ps3.data.analog.stick.ry, DEC);
        // Serial.println();

        JoystickMode prev = rightMode;
        updateJoystickVal(Ps3.data.analog.stick.rx, Ps3.data.analog.stick.ry, rightMode);
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

    // //--------------- Analog D-pad button events ----------------
    // if (abs(Ps3.event.analog_changed.button.up))
    // {
    //     Serial.print("Pressing the up button: ");
    //     Serial.println(Ps3.data.analog.button.up, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.right))
    // {
    //     Serial.print("Pressing the right button: ");
    //     Serial.println(Ps3.data.analog.button.right, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.down))
    // {
    //     Serial.print("Pressing the down button: ");
    //     Serial.println(Ps3.data.analog.button.down, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.left))
    // {
    //     Serial.print("Pressing the left button: ");
    //     Serial.println(Ps3.data.analog.button.left, DEC);
    // }

    // //---------- Analog shoulder/trigger button events ----------
    // if (abs(Ps3.event.analog_changed.button.l1))
    // {
    //     Serial.print("Pressing the left shoulder button: ");
    //     Serial.println(Ps3.data.analog.button.l1, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.r1))
    // {
    //     Serial.print("Pressing the right shoulder button: ");
    //     Serial.println(Ps3.data.analog.button.r1, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.l2))
    // {
    //     Serial.print("Pressing the left trigger button: ");
    //     Serial.println(Ps3.data.analog.button.l2, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.r2))
    // {
    //     Serial.print("Pressing the right trigger button: ");
    //     Serial.println(Ps3.data.analog.button.r2, DEC);
    // }

    // //---- Analog cross/square/triangle/circle button events ----
    // if (abs(Ps3.event.analog_changed.button.triangle))
    // {
    //     Serial.print("Pressing the triangle button: ");
    //     Serial.println(Ps3.data.analog.button.triangle, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.circle))
    // {
    //     Serial.print("Pressing the circle button: ");
    //     Serial.println(Ps3.data.analog.button.circle, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.cross))
    // {
    //     Serial.print("Pressing the cross button: ");
    //     Serial.println(Ps3.data.analog.button.cross, DEC);
    // }

    // if (abs(Ps3.event.analog_changed.button.square))
    // {
    //     Serial.print("Pressing the square button: ");
    //     Serial.println(Ps3.data.analog.button.square, DEC);
    // }
}

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

// this function is ran every 100ms
void setBatteryInLED()
{
    bool flash = (millis() / 2000) % 2; // flip boolean every 2 second
    static bool previousFlash = false;  // store previous value of the 1 second flip
    static int flashCounter = 0;
    if (flash != previousFlash)
    {
        // okay soo this means we are at the start of a new second now...
        flashCounter = 3;
        previousFlash = flash;
    }
    if (flashCounter % 2 == 1 && armed) // will flash on odd numbers. Default set to 3 and decrement every 100ms so it will flash at 3 and 1 and go back to normal for the rest of the second
    {
        Ps3.setPlayer(0);
    }
    else
    {
        player += 1;
        if (player > 10)
            player = 0;
        battery = Ps3.data.status.battery;
        if (battery == ps3_status_battery_charging)
            Ps3.setPlayer(player);
        else if (battery == ps3_status_battery_full)
            Ps3.setPlayer(10);
        else if (battery == ps3_status_battery_high)
            Ps3.setPlayer(9);
        else if (battery == ps3_status_battery_low)
            Ps3.setPlayer(7);
        else if (battery == ps3_status_battery_dying)
            Ps3.setPlayer(4);
        else if (battery == ps3_status_battery_shutdown)
            Ps3.setPlayer(0); // idk what shutdown means lol i guess it's coming soon
    }
    if (flashCounter > 0)
    {
        flashCounter--;
    }
}

void onConnect()
{
    Serial.println("Connected to PS3 Controller.");
}

void ps3_controller_setup()
{
    Ps3.attach(notify);
    Ps3.attachOnConnect(onConnect);
    Ps3.begin(); // Put 00:00:00:00:00:00 here if you want to set the WHOLE esp32's mac address to zeros. This does seem to break BLE however

    Serial.println("PS3 Ready.");
}

void ps3_controller_loop()
{
    if (!Ps3.isConnected())
        return;

    // cheeky fix for the rumble. sometimes calling the led function will shorten the rumble. Just bypass it once.
    if (skipNextPlayerUpate == false)
    {
        setBatteryInLED();
    }

    skipNextPlayerUpate = false;

    //------ Digital cross/square/triangle/circle buttons ------
    // if (Ps3.data.button.cross && Ps3.data.button.down)
    //     Serial.println("Pressing both the down and cross buttons");
    // if (Ps3.data.button.square && Ps3.data.button.left)
    //     Serial.println("Pressing both the square and left buttons");
    // if (Ps3.data.button.triangle && Ps3.data.button.up)
    //     Serial.println("Pressing both the triangle and up buttons");
    // if (Ps3.data.button.circle && Ps3.data.button.right)
    //     Serial.println("Pressing both the circle and right buttons");

    // if (Ps3.data.button.l1 && Ps3.data.button.r1)
    //     Serial.println("Pressing both the left and right bumper buttons");
    // if (Ps3.data.button.l2 && Ps3.data.button.r2)
    //     Serial.println("Pressing both the left and right trigger buttons");
    // if (Ps3.data.button.l3 && Ps3.data.button.r3)
    //     Serial.println("Pressing both the left and right stick buttons");
    if (Ps3.data.button.select && Ps3.data.button.start)
    {
        Serial.println("Pressing both the select and start buttons");
        setinternalReboot(true);
    }

    // printJoystick(leftMode);
    // Serial.print("\t");
    // printJoystick(rightMode);
    // Serial.println();
}

#endif