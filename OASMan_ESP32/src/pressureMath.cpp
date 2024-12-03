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