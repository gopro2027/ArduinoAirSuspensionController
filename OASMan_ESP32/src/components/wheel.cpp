#include "wheel.h"

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
    this->flagStartPressureGoalRoutine = false;
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
            this->flagStartPressureGoalRoutine = true;
        }
    }
}

// height sensor: AA-ROT-120 https://www.aliexpress.us/item/3256807527882480.html https://www.amazon.com/Height-Sensor-Suspension-Leveling-AA-ROT-120/dp/B08DJ3HX1B https://www.aliexpress.us/item/3256806751644782.html
// Output voltage UA:0.5-4.5

void calc_parabola_vertex(double x1, double y1, double x2, double y2, double x3, double y3, double &A, double &B, double &C)
{
    // Adapted and modifed to get the unknowns for defining a parabola:
    // http://stackoverflow.com/questions/717762/how-to-calculate-the-vertex-of-a-parabola-given-three-points
    double denom = (x1 - x2) * (x1 - x3) * (x2 - x3);
    if (abs(denom) < 0.0001)
        return;
    A = (x3 * (y2 - y1) + x2 * (y1 - y3) + x1 * (y3 - y2)) / denom;
    B = (x3 * x3 * (y1 - y2) + x2 * x2 * (y3 - y1) + x1 * x1 * (y2 - y3)) / denom;
    C = (x2 * x3 * (x2 - x3) * y1 + x3 * x1 * (x3 - x1) * y2 + x1 * x2 * (x1 - x2) * y3) / denom;
}

double calc_parabola_y(double A, double B, double C, double x_val)
{
    return (A * (x_val * x_val)) + (B * x_val) + C;
}

// does 2 quick bursts to figure out the rest of the values we want
double Wheel::calculatePressureTimingReal(Solenoid *valve)
{
    int pressureDif = this->pressureGoal - this->getSelectedInputValue();
    int pressureDifABS = abs(pressureDif);
    double valveTime = calculateValveOpenTimeMS(pressureDifABS, this->quickMode);
    if (pressureDifABS < 15)
    {
        return valveTime;
    }
    double pressures[3];
    double times[3];
    // grab initial value first
    this->readInputs();
    pressures[0] = this->getSelectedInputValue();
    times[0] = 0;
    // run 2 more times for 50 seconds each to get 3 points to make a graph
    for (int i = 1; i < 3; i++)
    {
        valve->open();
        delay(100); // too low and we risk innacurate values due to flow timing ect it's not perfect. lower values are less time to stabilize after
        valve->close();
        delay(200); // ts jiggles so increased from 150 to 300
        this->readInputs();
        pressures[i] = this->getSelectedInputValue();
        times[i] = times[i - 1] + 150;
    }

    // This was my attempt to calculate it with time on the x axis but then I realized it actually would form a sideways parabola aka a logarithmic function like y = a * log(x - b) + c
    // Unfortunately to solve for 3 points on that it is very difficult. It is also annoying to have to use the quadratic formula to solve for y
    // double A, B, C;
    // calc_parabola_vertex(times[0], pressures[0], times[1], pressures[1], times[2], pressures[2], A, B, C);
    // double y = this->pressureGoal;
    // double Cadj = C - y;
    // double discriminant = B * B - 4 * A * Cadj;

    // // if A is 0, can't divide by 0. If discriminant is less than 0 that means the resulting number is complex aka we asked for a y value that is not contained in the parabola
    // if ((A < 0.0001 && A > -0.0001) || discriminant < 0)
    // {
    //     valveTime;
    // }
    // double root1 = (-B + std::sqrt(discriminant)) / (2 * A);
    // double root2 = (-B - std::sqrt(discriminant)) / (2 * A);
    // // if both are less than zero, then there is no solution (in practice this is probably never going to happen since our three x values are on the positive side of the x axis)
    // if (root1 < 0 && root2 < 0)
    // {
    //     return valveTime;
    // }
    // double root = (root1 < 0) ? root2 : root1;
    // double timeToGoal = root - times[2];
    // return timeToGoal;

    // instead... we can just flip the x and y (time and pressure) so we instead log a typical quadratic equation and then also since our x value is now pressure, which does appear to map correctly, we can just plug in our pressure into the equation and we don't have to use the quadratic formula to solve for y.
    double A, B, C;
    A = B = C = 0;
    calc_parabola_vertex(pressures[0], times[0], pressures[1], times[1], pressures[2], times[2], A, B, C);
    double result = calc_parabola_y(A, B, C, this->pressureGoal) - times[2];
    if (result < 0)
    {
        // i think this would mean we overshot the goal value so go ahead and return 0 so the valve doesn't open
        return 0;
    }
    else if (result > 5000)
    {
        // wack value... do it the old way
        return calculateValveOpenTimeMS(abs(this->pressureGoal - this->getSelectedInputValue()), this->quickMode);
    }
    // Serial.print("Result: ");
    // Serial.print(result);
    // Serial.print(" A: ");
    // Serial.print(A);
    // Serial.print(" B: ");
    // Serial.print(B);
    // Serial.print(" C: ");
    // Serial.println(C);

    // log_i("A: %d B: %d C: %d Result: %d", A, B, C, result);
    return result;
}

// logic https://www.figma.com/board/YOKnd1caeojOlEjpdfY5NF/Untitled?node-id=0-1&node-type=canvas&t=p1SyY3R7azjm1PKs-0
void Wheel::loop()
{
    // Serial.println("WheelP: ");
    this->readInputs();
    if (this->flagStartPressureGoalRoutine)
    {
        this->flagStartPressureGoalRoutine = false;
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
                if (this->pressureGoal > this->getSelectedInputValue())
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
                    // int valveTime = calculateValveOpenTimeMS(pressureDifABS, this->quickMode);
                    int valveTime = this->calculatePressureTimingReal(valve);

                    if (valveTime > 0)
                    {
                        // Open valve for calculated time
                        valve->open();
                        delay(valveTime);
                        valve->close();

                        // Sleep 150ms to allow time for valve to fully close and pressure to equalize a bit
                        delay(150);
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
        }
        // close both after (only applies for level sensor logic)
        this->s_AirIn->close();
        this->s_AirOut->close();
    }
}