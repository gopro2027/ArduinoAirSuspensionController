#include "wheel.h"

#define NUM_WHEEL_THREADS 4
std::atomic<bool> flagStartPressureGoalRoutine[NUM_WHEEL_THREADS];

struct PressureGoalValveTiming
{
    int pressureDelta;          // the pressure percision that we are trying to achieve
    int valveTimingUntilWithin; // the amount of time we will leave the valve open until we get within the range of goalPressure +- pressureDelta
    bool isPerciseMeasurement;  // when using air up quick type functions, will not take into account ones that are set to true here
};

// Must put in sorted order of largest to smallest
// So I added *2 because at least in the corvette which i imagine is a fairly standard car, 200% worked great in testing so i may as well make it the default
PressureGoalValveTiming valveTiming[] = {
    {100, 500 * 2, false},
    {50, 250 * 2, false},
    {25, 125 * 2, false},
    {10, 50 * 2, false}, // if current psi outside range of goalPressure +- 10psi, open valves for 75ms until +- 10psi achieved
    {5, 20 * 2, false},  // if current psi outside range of goalPressure +- 5psi, open valves for 20ms until +- 6psi achieved
    {0, 5 * 2, true},    // if current psi outside range of goalPressure +- 0psi (aka psi is not yet exactly goalPressure), open valves for 5ms until exact goal pressure is achieved. Will not be used in quick mode
};
#define VALVE_TIMING_LIST_COUNT (sizeof(valveTiming) / sizeof(PressureGoalValveTiming))

// This function can be updated in the future to use some better algorithm to decide how long to open the valves for to reach the desigred pressure
PressureGoalValveTiming *getValveTiming(int pressureDifferenceAbsolute, bool quickMode)
{
    PressureGoalValveTiming *lastTime = &valveTiming[0];
    for (int i = 0; i < VALVE_TIMING_LIST_COUNT; i++)
    {
        if (quickMode && valveTiming[i].isPerciseMeasurement)
        {
            return lastTime;
        }
        if (pressureDifferenceAbsolute > valveTiming[i].pressureDelta)
        {
            return &valveTiming[i];
        }
        lastTime = &valveTiming[i];
    }
    return lastTime; // should never get to this case but if it does it returns the smallest time
}

int getMinValveOpenPSI(bool quickMode)
{
    return getheightSensorMode() ? 0 : getValveTiming(0, quickMode)->pressureDelta;
}

// TODO: Turn this into a function based on the values above so that it is smooth, and add a multiplier to change in the settings for people with larger volume bags where it's too slow by default
int calculateValveOpenTimeMS(int pressureDifferenceAbsolute, bool quickMode)
{
    return getheightSensorMode() ? 0 : (getValveTiming(pressureDifferenceAbsolute, quickMode)->valveTimingUntilWithin * ((float)getbagVolumePercentage() / 100.0f)); // Note: Added 0 for height sensor mode but it is unused
}

Wheel::Wheel() {}

Wheel::Wheel(Solenoid *solenoidInPin, Solenoid *solenoidOutPin, InputType *pressurePin, InputType *levelSensorPin, byte thisWheelNum)
{
    this->pressurePin = pressurePin;
    this->levelSensorPin = levelSensorPin;
    this->thisWheelNum = thisWheelNum;
    this->s_AirIn = solenoidInPin;
    this->s_AirOut = solenoidOutPin;
    this->pressureValue = 0;
    this->pressureGoal = 0;
    this->routineStartTime = 0;
    //this->flagStartPressureGoalRoutine = false;
    flagStartPressureGoalRoutine[thisWheelNum] = false;
    this->quickMode = false;
}

Solenoid *Wheel::getInSolenoid()
{
    return this->s_AirIn;
}

Solenoid *Wheel::getOutSolenoid()
{
    return this->s_AirOut;
}

InputType *Wheel::getPressurePin()
{
    return this->pressurePin;
}

float analogToRange(int nativeAnalogValue)
{
    float floored = float(nativeAnalogValue) - pressureZeroAnalogValue;  // chop off the 0 value
    float totalRange = pressureMaxAnalogValue - pressureZeroAnalogValue; // get the total analog voltage difference between min and max
    float normalized = floored / totalRange;                             // 0 to 1 where 0 is 0psi and 1 is max psi
    return normalized;
}

float analogToPressure(int nativeAnalogValue)
{
    return analogToRange(nativeAnalogValue) * getpressureSensorMax(); // multiply out 0 to 1 by our max psi
}

float analogToHeightPercentage(int nativeAnalogValue)
{
    return analogToRange(nativeAnalogValue) * getHeightSensorMax(); // multiply out 0 to 1 by 100 to get a percentage
}

// Testing function, convert pressure value back to analog value, exact reverse of analogToPressure
float pressureToAnalog(float psi)
{
    float totalRange = pressureMaxAnalogValue - pressureZeroAnalogValue; // get the total analog voltage difference between min and max
    return (psi / getpressureSensorMax()) * totalRange + pressureZeroAnalogValue;
}

float readPinPressure(InputType *pin, bool heightMode)
{
    if (heightMode == false)
    {
        return analogToPressure(pin->analogRead());
    }
    else
    {
        return analogToHeightPercentage(pin->analogRead());
    }
}

void Wheel::readInputs()
{
    this->pressureValue = readPinPressure(this->pressurePin, false);
    if (getheightSensorMode())
    {
        this->levelValue = readPinPressure(this->levelSensorPin, true);
    }
}

float Wheel::getSelectedInputValue()
{
    float value = getheightSensorMode() ? this->levelValue : this->pressureValue;
    if (value < 0)
    {
        return 0;
    }
    else
    {
        return value;
    }
}

bool Wheel::isActive()
{
    return this->s_AirIn->isOpen() || this->s_AirOut->isOpen();
}

void Wheel::initPressureGoal(int newPressure, bool quick)
{
    if (newPressure > (getheightSensorMode() ? getHeightSensorMax() * 1.03f : getbagMaxPressure()))
    {
        // hardcode not to go above set psi
        return;
    }
    int pressureDif = newPressure - this->getSelectedInputValue(); // negative if airing out, positive if airing up
    if (abs(pressureDif) > getMinValveOpenPSI(quick))
    {
        // okay we need to set the values, but only if we are airing out or if the tank has more pressure than what is currently in the bags
        bool tankIsCapable = true;
        if (getheightSensorMode() == false)
        {
            // it's in pressure mode, check if tank is less than whats currently in the bag and if it is then tankIsCapable is false.
            // we don't care about the goal because we still want to try to reach the goal even if the tank isn't capable. We do care if tank is more than the bag though because if it's less than the bag then it will actually air out.
            if (getCompressor()->getTankPressure() < this->getSelectedInputValue())
            {
                tankIsCapable = false;
            }
        }
        if (pressureDif < 0 || tankIsCapable)
        {
            this->pressureGoal = newPressure;
            this->quickMode = quick;
            this->routineStartTime = millis();
            flagStartPressureGoalRoutine[thisWheelNum] = true;
        }
    }
}

// height sensor: AA-ROT-120 https://www.aliexpress.us/item/3256807527882480.html https://www.amazon.com/Height-Sensor-Suspension-Leveling-AA-ROT-120/dp/B08DJ3HX1B https://www.aliexpress.us/item/3256806751644782.html
// Output voltage UA:0.5-4.5

// 300 and 700 works quite well but way too long of times
// 100 and 400 appeared to not be quite enough for an accurate reading. It bounced a lot going from 20 to 90 and undershot, and also going from 90 to 20 it didn't calculate accurately either
#define VALVE_OPEN_TIME 200
#define VALVE_STABILIZE_TIME 500
// does 1 quick burst to figure out the rest of the values we want
double Wheel::calculatePressureTimingReal(Solenoid *valve)
{
    int pressureDifABS = abs(this->pressureGoal - this->getSelectedInputValue());
    if (pressureDifABS < 15)
    {
        return calculateValveOpenTimeMS(pressureDifABS, this->quickMode);
    }
    double pressures[2];
    double times[2];
    // grab initial value first
    this->readInputs();
    pressures[0] = this->getSelectedInputValue(); // 0
    times[0] = 0;                                 // 0

    // open valve for set time and grab value so we have a graph now so we can estimate the pressure since the graphs are mostly linear

    valve->open();
    delay(VALVE_OPEN_TIME); // too low and we risk innacurate values due to flow timing ect it's not perfect. lower values are less time to stabilize after
    valve->close();
    delay(VALVE_STABILIZE_TIME); // ts jiggles so increased from 150 to 300
    this->readInputs();
    pressures[1] = this->getSelectedInputValue(); // 50
    times[1] = VALVE_OPEN_TIME;                   // 1

    // we can just flip the x and y (time and pressure) so we instead log a typical quadratic equation and then also since our x value is now pressure, which does appear to map correctly, we can just plug in our pressure into the equation and we don't have to use the quadratic formula to solve for y.

    // formula: y-y1=m(x-x1) & m=(y-y1)/(x-x1) & y=mx+b & b=y-mx
    // m = (1 - 0) / (50 - 0) = 0.02
    double m = (times[1] - times[0]) / (pressures[1] - pressures[0]);
    // b = 0 - 0.02 * 0 = 0
    double b = times[0] - m * pressures[0];

    // Formula: f(pressureGoal) - f(currentPressure)
    // Proof:
    // f(currentPressure) = times[1]
    // f(currentPressure) = (m * pressures[1] + b)
    // times[1] = (m * pressures[1] + b) = (0.02 * 50 + 0) = 1
    // timeDif = (0.02 * 100 + 0) - 1 = 1 which matches simulation graph for air up! https://www.reddit.com/r/askmath/comments/1k5kysw/what_type_of_line_is_this_and_how_can_i_make_a/
    double timeDif = (m * pressureGoal + b) - times[1];

    // if value is negative, we must have overshot. returning a negative value will be ignored and valve won't open and that will be just fine

    // if value is more than 5000 just assume something is wrong and use the old method
    if (timeDif > 5000)
    {
        // wack value... do it the old way
        return calculateValveOpenTimeMS(abs(this->pressureGoal - this->getSelectedInputValue()), this->quickMode);
    }

    return timeDif;
}

int wheelLoopBittset = 0;

static SemaphoreHandle_t wheelLockSem;
void setupWheelLockSem()
{
    wheelLockSem = xSemaphoreCreateMutex();
}

void wheelThreadLock() {
    while (xSemaphoreTake(wheelLockSem, 1) != pdTRUE)
    {
        delay(1);
    }
}

void wheelThreadUnlock() {
    xSemaphoreGive(wheelLockSem);
}

// barrier code (for syncing wheel threads) generated from chat-gpt because I don't quite understand it but it seems to work fine lololol
std::atomic<int> waiting_threads(0);
std::atomic<int> generation(0);

int count_participants() {
    // Count how many threads are currently running
    int currently_active = 0;
    for (int i = 0; i < NUM_WHEEL_THREADS; ++i) {
        if (flagStartPressureGoalRoutine[i].load()) {
            currently_active++;
        }
    }
    return currently_active;
}

void custom_barrier_wait(int num_participants) {
    int gen = generation.load();
    waiting_threads.fetch_add(1);

    if (waiting_threads.load() == num_participants) {
        // Last thread arrives, reset and let others go
        wheelThreadLock();
        waiting_threads.store(0);
        generation.fetch_add(1);
        wheelThreadUnlock();
    } else {
        // Poll until generation changes
        while (generation.load() == gen) {
            delay(1);
        }
    }
}

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::loop()
{
    // Serial.println("WheelP: ");
    this->readInputs();
    if (flagStartPressureGoalRoutine[thisWheelNum].load())
    {
        delay(100);// wait for all threads to sync on first call
        //const double oscillation = 1.359142965358979; //e/2 seems like a decent value tbh
        //const double oscillation = 1.75;
        const double oscillation = 1.2;
        int oscillationPow = 0;
        const int startIteration = -1;
        int iteration = startIteration; // - values make it skip the first generation. It won't start dividing until iteration = 1
        bool previousDirection = false;
        for (;;)
        {
            // 10 second timeout in case tank doesn't have a whole lot of air or something
            if (millis() > this->routineStartTime + ROUTINE_TIMEOUT_MS)
            {
                break;
            }

            // Main routine
            this->readInputs();
            int pressureDif = this->pressureGoal - this->getSelectedInputValue();
            int pressureDifABS = abs(pressureDif);

            if (pressureDifABS > getMinValveOpenPSI(this->quickMode))
            {
                // Decide which valve to use
                Solenoid *valve;
                bool up = pressureDif >= 0;
                if (up)
                {
                    valve = this->s_AirIn;
                    this->s_AirOut->close();
                }
                else
                {
                    valve = this->s_AirOut;
                    this->s_AirIn->close();
                }

                if (!getheightSensorMode())
                {
                    // Pressure sensor logic. You can't read the pressure accurately with the valve open. Essentially must open the valve for a guesstimate amount of time, then close it, then you are able to read the pressure.

                    // Choose time to open for
                    int valveTime = calculateValveOpenTimeMS(pressureDifABS, this->quickMode);

                    // right now not going to use this because it doesn't seem to work super well up air up. Results in super low values. Need to do more testing
                    // int valveTime = this->calculatePressureTimingReal(valve);

                    
                    double start_pressure = this->getSelectedInputValue();
                    double end_pressure = this->pressureGoal;
                    double tank_pressure = getCompressor()->getTankPressure();

                    if (canUseAiPrediction(valve->getAIIndex())) {

                        // conversion float to int eliminates inf and nan
                        int aiPredict = getAiPredictionTime(valve->getAIIndex(), start_pressure, end_pressure, tank_pressure);

                        // There are some valid scenarios where we can get inf or nan if say the tank pressure is lower than the end pressure
                        if (aiPredict < 5000 && aiPredict > 0) {
                            valveTime = aiPredict;
                        }
                    }

                    // To help prevent ocellations, check if previous direction is different than new direction. Ex: was going up, but suddently now is going down. It must have jumped over goal. Go ahead and start dividing valve time by (oscillation ^ oscillationPow)
                    if (iteration > startIteration) {
                        if (previousDirection != up) {
                            oscillationPow++;
                        }
                    }
                    valveTime = valveTime / std::pow(oscillation, oscillationPow);

                    // save previous direction.
                    previousDirection = up;

                    if (valveTime > 0)
                    {
                        // Open valve for calculated time
                        valve->open();
                        delay(valveTime);
                        valve->close();

                        
                        // Sleep 150ms to allow time for valve to fully close and pressure to equalize a bit
                        delay(250); // Changed to 250. 150 was... confusing

                        // only bother saving data for first 2 iterations AND when the valve was opened for more than 10ms AND if the pressure change is greater than 3psi
                        if (iteration < startIteration + 2 && valveTime > 10) {
                            this->readInputs();
                            end_pressure = this->getSelectedInputValue(); // gonna be slightly different than the pressureGoal
                            if (abs(start_pressure - end_pressure) > 3) {
                                appendPressureDataToFile(valve->getAIIndex(), start_pressure, end_pressure, tank_pressure, valveTime);
                            }
                        }
                    } else {
                        // calculated valve time is 0 so just break out of loop
                        break;
                    }
                }
                else
                {
                    // for level sensors, just open the valve and read level sensors while valves are open since it doesn't affect the reading
                    valve->open();
                    delay(1);
                }
            }
            else
            {
                // Completed
                break;
            }
            iteration++;

            

            custom_barrier_wait(count_participants());

        }
        custom_barrier_wait(count_participants()); // needed here because when it breaks out of the loop it needs to hit it one last time.
        
        flagStartPressureGoalRoutine[thisWheelNum] = false;
        // close both after (only applies for level sensor logic)
        this->s_AirIn->close();
        this->s_AirOut->close();
    }

    // Maintain Pressure code
    if (getmaintainPressure()) {
        int pressureDif = this->pressureGoal - this->getSelectedInputValue();
        int pressureDifABS = abs(pressureDif);
        if (pressureDifABS >= 10) { // 10 psi difference
            initPressureGoal(this->pressureGoal); // try to go back to the desired pressure
        }
    }
}