#include "pressureMath.h"

#if pressure_math_remove_this_line_initial_pid_code

// >> > Please implement PID controller to flow the air from 1 tank to a smaller tank to a specific PSI value in C++ Here's an example of how you can implement a PID controller in C++ for controlling airflow from one tank to another based on a target pressure.This code assumes that the input and output signals are measured using sensors.

// ```cpp

// https://www.youtube.com/watch?v=tFVAaUcOm4I&ab_channel=DigiKey

/* Try using these instead!
https://github.com/Dlloydev/QuickPID
https://github.com/Dlloydev/sTune
 */

#include <iostream>
#include <cmath>

double calculateProcessValue()
{
    // Implement your pressure measurement logic here
    return 0.0;
}

#define TIME_INTERVAL 1

class PIDController
{
private:
    double Kp, Ki, Kd, GoalPressure;
    double integral = 0.0;
    double errorLast = 0.0;

public:
    PIDController(double kp, double ki, double kd, double goalPressure) : Kp(kp), Ki(ki), Kd(kd), GoalPressure(goalPressure) {}

    double update(double error, double readPressure)
    {

        int error = GoalPressure - readPressure;
        integral = integral + (error * TIME_INTERVAL); // should i used the actual time instead of a pretend interval? idk. That would mean the previous result is the interval
        double derivative = (error - errorLast) / TIME_INTERVAL;
        double result = (Kp * error) + (Ki * integral) + (Kd * derivative);
        errorLast = error;

        return result; // should be how long to open it for
    }

    void reset()
    {
        integral = 0.0;
        errorLast = 0.0;
    }
};

class TankController
{
private:
    PIDController pid;
    double minFlow, maxFlow;

public:
    TankController(double goal, double kp, double ki, double kd, double minFlow, double maxFlow)
        : pid(kp, ki, kd, goal), minFlow(minFlow), maxFlow(maxFlow) {}

    void setTargetPressure(double targetPressure)
    {
        double readPressure = calculateProcessValue();
        double error = targetPressure - calculateProcessValue();
        double uOut = pid.update(error, readPressure);

        // Limit the output value to the flow range
        if (uOut < minFlow)
        {
            uOut = minFlow;
        }
        else if (uOut > maxFlow)
        {
            uOut = maxFlow;
        }

        std::cout << "Target Pressure: " << targetPressure << ", Actual Flow: " << uOut << std::endl;
    }
};

int main()
{
    TankController controller(1.0, 0.1, 0.05, 10.0, 50.0);

    while (true)
    {
        double targetPressure = 20.0; // Set the desired pressure
        controller.setTargetPressure(targetPressure);
        // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return 0;
}
// ```

//     This code defines two classes : `PIDController` and `TankController`.The `PIDController` class implements a PID
//         controller using the specified gains.The `TankController` class integrates the PID controller with the tank flow
//             control.

//     In the `main()` function,
//     we create an instance of the `TankController` class with the desired gains, minimum and maximum flow rates, and target pressure.We then enter a loop where we continuously set the target pressure using the `setTargetPressure()` method, which calls the PID controller's `update()` method to calculate the output value.The actual flow is printed to the console.

//                                                                                                                                                                                                                                                To use this code,
//     replace the `calculateProcessValue()` function in the `TankController` class with your own
//     implementation of measuring the process value based on the pressure measurements from the sensors.

#endif

#if fulltauexample
#include <stdio.h>
#include <math.h>

// --- Calibrated Constants ---

// From your test:
// Initial: P_A0 = 180 psi, P_B0 = 100 psi => D = 80
// After 1 sec: P_A = 160, P_B = 130 => implies final pressure P_f ~ 145
// So: alpha = (P_f - P_B0) / D = (145 - 100) / 80 = 0.5625
const double alpha = 0.5625;

// From the same test and using the exponential model:
// P_B(t) = P_f - (P_f - P_B0) * exp(-t / tau)
// 130 = 145 - (145 - 100) * exp(-1 / tau)
// Solving gives: tau ≈ 0.91 seconds
const double tau = 0.91;

void calculateAlphaTau()
{
    int P_A0 = 180; // our max tank psi
    int P_B0 = 0;   // aired out bag
    int P_D = P_A0 - P_B0;

    // SUDOCODE open valve of bag for 1 second
    int T = 1;
    int P_B1 = 70; // read value

    int alpha = (P_B1 - P_B0) / P_D;
}

// --- Function to calculate time to reach target pressure ---
double calculate_fill_time(double P_B0, double P_goal, double D)
{
    // Prevent invalid calculations
    if (D <= 0 || P_goal >= (P_B0 + alpha * D))
    {
        return 0.0; // Already at or above target, or invalid input
    }

    // Estimate final pressure based on alpha
    double P_f = P_B0 + alpha * D;

    // Apply exponential model to solve for time
    double T = -tau * log((P_f - P_goal) / (alpha * D));

    return T;
}

// --- Example usage ---
int main()
{
    double P_B0 = 100.0;   // Initial pressure in tank B (psi)
    double P_goal = 135.0; // Target pressure in tank B (psi)
    double D = 80.0;       // Initial pressure difference (P_A - P_B)

    double time_needed = calculate_fill_time(P_B0, P_goal, D);
    printf("Open solenoid for %.3f seconds\n", time_needed);

    return 0;
}

#endif

#if pidcodenumbertwo

#include <stdio.h>

// --- PID constants (tune these!) ---
const double Kp = 0.05;
const double Ki = 0.01;
const double Kd = 0.02;

// --- PID state ---
// double integral = 0.0;
// double prev_error = 0.0;

// --- PID function to compute new valve open time ---
double compute_pid_time(double goal, double current_pressure, double delta_t, double &integral, double &prev_error)
{
    double error = goal - current_pressure;

    // Update integral and derivative
    integral += error * delta_t;
    double derivative = (error - prev_error) / delta_t;

    // PID output
    double T = Kp * error + Ki * integral + Kd * derivative;

    // Save error for next step
    prev_error = error;

    // Clamp T to valid range (e.g. 0 to 5 seconds)
    if (T < 0.0)
        T = 0.0;
    if (T > 0.5)
        T = 0.5;

    return T;
}

// start with lastOpenTime = 1.0
double startIteratePressurePID(double goal, double bag_current, double lastOpenTime, double &integral, double &prev_error)
{

    // First valve open time guess
    double T = compute_pid_time(goal, bag_current, lastOpenTime, integral, prev_error);

    // printf("Open solenoid for %.3f seconds\n", T);
    return T;
}

#endif

// #define PARABOLA_MATH
#ifdef parabolaLearn

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

// x is pressures, y is times
void calculateAverageOfSamples(double *x, double *y, int sz, double &A, double &B, double &C)
{
    A = 0;
    B = 0;
    C = 0;
    for (int i = 0; i < sz - 2; i++)
    {
        double tA, tB, tC;
        tA = tB = tC = 0;
        calc_parabola_vertex(x[i], y[i], x[i + 1], y[i + 1], x[i + 2], y[i + 2], tA, tB, tC);
        A = ((A * i) + tA) / (i + 1);
        B = ((B * i) + tB) / (i + 1);
        C = ((C * i) + tC) / (i + 1);
    }
}

// int main()
// {
//     const int sz = 10;
//     double pressures[sz];
//     double times[sz];
//     // fill with simulated test data from  y=0.2x^{2}
//     for (int i = 0; i < sz; i++)
//     {
//         pressures[i] = i * (140 / sz); // 140 is a decent max value to fill to
//         times[i] = calc_parabola_y(0.2, 0, 0, pressures[i]);
//     }
//     double A, B, C;
//     A = B = C = 0;
//     calculateAverageOfSamples(pressures, times, sz, A, B, C);
//     std::cout << "A: " << A << std::endl;
//     std::cout << "B: " << B << std::endl;
//     std::cout << "C: " << C << std::endl;
//     return 0;
// }
#endif