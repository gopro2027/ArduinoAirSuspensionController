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

//#define ai_pressure
#ifdef ai_pressure

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <math.h>

// https://www.reddit.com/r/askmath/comments/1k5kysw/comment/mok9n9f/?context=3
// https://www.reddit.com/media?url=https%3A%2F%2Fpreview.redd.it%2Fwhat-type-of-line-is-this-and-how-can-i-make-a-formula-from-v0-f6d6iwzbniwe1.png%3Fwidth%3D640%26format%3Dpng%26auto%3Dwebp%26s%3Df9708852f49211a61e9ed69150bda683acbd733d 
double airOutSimulationFormula(double pressure) {
    return -3.170644334432285*std::log(pressure)+16.465020074051143;
}

double simulateRealWorldTest(double startPressure, double goalPressure) {
    return airOutSimulationFormula(goalPressure) - airOutSimulationFormula(startPressure);
}

double simulateRealWorldTestRandomly(double startPressure, double goalPressure) {
    std::uniform_real_distribution<double> unif(-1,1);
    std::default_random_engine re;
    double a_random_double = unif(re);
    return simulateRealWorldTest(startPressure, goalPressure) + a_random_double;
}

#define MAX_GENE_VAL 10.0

// Define a structure to represent an individual (chromosome)
struct Individual {
    std::vector<double> genes;
    double fitness;

    Individual(int numGenes) : genes(numGenes), fitness(0) {}
};

double getEstimatedTimeToOpenValve(const std::vector<double>& genes, double startPressure, double goalPressure) {
    double sum = 0;
    for (double gene : genes) {
        sum += gene * ((startPressure-goalPressure) / 180);
    }
    return sum;
}



// Function to calculate fitness (example: maximize sum of genes)
double calculateFitness(const std::vector<double>& genes, double startPressure, double goalPressure) {
    // double goalTimeDif = simulateRealWorldTest(startPressure,goalPressure); // perfect case real world goal, this would be the value we experimentally got based on the previously c
    
    // double geneCalculatedTime = 0;


    // double sum = 0;
    // for (double gene : genes) {
    //     sum += gene - goalTimeDif;
    // }
    // return sum;

    // would typically be gotten from real test data
    std::uniform_real_distribution<double> unif(-10,10);
    std::default_random_engine re;
    double a_random_double = unif(re);
    double actualAchievedPressure = goalPressure + a_random_double;

    return goalPressure - actualAchievedPressure;
}

// Function to perform crossover (single-point crossover)
std::pair<Individual, Individual> crossover(const Individual& parent1, const Individual& parent2) {
    int crossoverPoint = rand() % parent1.genes.size();
    Individual child1(parent1.genes.size());
    Individual child2(parent2.genes.size());

    for (int i = 0; i < parent1.genes.size(); ++i) {
        if (i < crossoverPoint) {
            child1.genes[i] = parent1.genes[i];
            child2.genes[i] = parent2.genes[i];
            // std::cout << "Fn3 " <<  child1.fitness << std::endl;
            // std::cout << "Fn4 " <<  child2.fitness << std::endl;
        } else {
            child1.genes[i] = parent2.genes[i];
            child2.genes[i] = parent1.genes[i];
        }
    }
    // std::cout << "Fn3 " <<  child1.fitness << std::endl;
    // std::cout << "Fn4 " <<  child2.fitness << std::endl;
    return std::make_pair(child1, child2);
}

// Function to perform mutation (randomly change a gene)
void mutate(Individual& individual, float mutationRate) {
    for (int i = 0; i < individual.genes.size(); ++i) {
        if (static_cast<double>(rand()) / RAND_MAX < mutationRate) {
            individual.genes[i] = (static_cast<double>(rand()) / RAND_MAX) * MAX_GENE_VAL;//rand() % 10; // Assuming genes are in range 0-9
        }
    }
}

void evaluateFitness(std::vector<Individual> &population, double startPressure, double goalPressure) {
    // Evaluate fitness
    for (Individual& individual : population) {
        individual.fitness = calculateFitness(individual.genes, startPressure,goalPressure); // need to change this to use goal value ect
    }
}

int main() {
    int populationSize = 1;
    int numGenes = 10;
    int numGenerations = 100;
    float mutationRate = 0.01;

    // Initialize population
    std::vector<Individual> population;
    for (int i = 0; i < populationSize; ++i) {
        Individual individual(numGenes);
        for (int j = 0; j < numGenes; ++j) {
            individual.genes[j] = (static_cast<double>(rand()) / RAND_MAX) * MAX_GENE_VAL; // Random genes in range 0-9
            //std::cout << individual.genes[j] << std::endl;
        }
        population.push_back(individual);
    }

    evaluateFitness(population, 100, 30);

    // Run the genetic algorithm
    for (int generation = 0; generation < numGenerations; ++generation) {

        // Selection (tournament selection)
        std::vector<Individual> newPopulation;
        for (int i = 0; i < populationSize; ++i) {
            int index1 = rand() % populationSize;
            int index2 = rand() % populationSize;
            //Individual winner = (population[index1].fitness > population[index2].fitness) ? population[index1] : population[index2];
            Individual winner = (abs(population[index1].fitness) - abs(population[index2].fitness)) > 0 ? population[index2] : population[index1]; // get the one that is closer to 0 because the fitness in our case is the err (diff) from 0
            newPopulation.push_back(winner);
        }

        // Crossover and Mutation
        for (int i = 0; i < populationSize; i += 2) {
             if(i + 1 < populationSize) {
                std::pair<Individual, Individual> children = crossover(newPopulation[i], newPopulation[i + 1]);
                mutate(children.first, mutationRate);
                mutate(children.second, mutationRate);
                // std::cout << "Fn " << newPopulation[i].fitness << std::endl;
                // std::cout << "Fn2 " <<  children.first.fitness << std::endl;
                newPopulation[i] = children.first;
                newPopulation[i + 1] = children.second;
             }
        }
        
        population = newPopulation;

        evaluateFitness(population, 100, 30);

        // Print best fitness of current generation
        double bestFitness = 0;
        for (const Individual& individual : population) {
            bestFitness = std::max(bestFitness, individual.fitness);
        }
        std::cout << "Generation " << generation + 1 << " Best Fitness: " << bestFitness << std::endl;
    }

    return 0;
}

#endif


//#define gradient_decent
#ifdef gradient_decent

#include <iostream>
#include <cmath>
#include <functional>

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <math.h>

// https://www.reddit.com/r/askmath/comments/1k5kysw/comment/mok9n9f/?context=3
// https://www.reddit.com/media?url=https%3A%2F%2Fpreview.redd.it%2Fwhat-type-of-line-is-this-and-how-can-i-make-a-formula-from-v0-f6d6iwzbniwe1.png%3Fwidth%3D640%26format%3Dpng%26auto%3Dwebp%26s%3Df9708852f49211a61e9ed69150bda683acbd733d 
// returns time
double airOutSimulationFormula(double pressure) {
    return -3.170644334432285*std::log(pressure)+16.465020074051143;
}

// returns pressure
double airOutSimulationFormulaSwapped(double time) {
    return std::exp((time-16.465020074051143)/-3.170644334432285);
}

// double simulateRealWorldTest(double startPressure, double goalPressure) {
//     return airOutSimulationFormula(goalPressure) - airOutSimulationFormula(startPressure);
// }

// double simulateRealWorldTestRandomly(double startPressure, double goalPressure) {
//     std::uniform_real_distribution<double> unif(-1,1);
//     std::default_random_engine re;
//     double a_random_double = unif(re);
//     return simulateRealWorldTest(startPressure, goalPressure) + a_random_double;
// }

double currentPressureForMysterFunction = 0;

// Placeholder for the mystery function. Replace with actual implementation.
// for mock implementation must set currentPressureForMysterFunction to current pressure beforehand!
// returns new pressure
double mystery_function(double t) {
    // First we plug in current pressure into our representation formula to get the 'current time it's already at' then we add on the valve open time t then plug it back in to get the proposed real value pressure result from opening the valve for time t
    double newPressure = airOutSimulationFormulaSwapped(airOutSimulationFormula(currentPressureForMysterFunction)+t);
    
    currentPressureForMysterFunction = newPressure;

    return newPressure;
}

// Iterative function to estimate time
double estimate_time(double start, double goal, int max_iterations = 100, double tolerance = 1e-6) {
    double t = start;
    double learning_rate = 0.1; // Tune this as needed

    for (int i = 0; i < max_iterations; ++i) {
        double result = mystery_function(t);
        double error = goal - result;

        if (std::abs(error) < tolerance) {
            break; // Converged
        }

        // Adjust time based on error — this is a heuristic update
        // Use direction of error and scale with learning rate
        double gradient = (mystery_function(t + 1e-4) - result) / 1e-4;
        if (std::abs(gradient) < 1e-8) gradient = 1; // avoid division by near-zero

        t += learning_rate * error / gradient;
    }

    return t;
}

int main() {
    double start = 100;
    double goal = 25.0; // Want mystery_function(t) ≈ 25

    // must set this value for our mock data to work properly
    currentPressureForMysterFunction = start;

    double estimated_time = estimate_time(start, goal, 2);
    
    std::cout << "Estimated time: " << estimated_time << std::endl;
    std::cout << "Function result: " << mystery_function(estimated_time) << std::endl;
}

#endif



#ifdef ai_pressure_1

// g++ src/pressureMath.cpp
// a.exe
#ifdef test_run
#include <iostream>
#include <vector>
#include <tuple>
#include <cmath>
#include <iomanip>
#endif


double normalize(double x, double min, double max) {
    return (x - min) / (max - min);
}

double denormalize(double x, double min, double max) {
    return x * (max - min) + min;
}

void AIModel::loadWeights(double _w1, double _w2, double _b) {
    w1 = _w1;
    w2 = _w2;
    b = _b;
}

// Predict time from inputs
double AIModel::predict(double start_pressure, double end_pressure, double tank_pressure) {
    start_pressure = normalize(start_pressure,0,200);
    end_pressure = normalize(end_pressure,0,200);
    tank_pressure = normalize(tank_pressure,0,200);
    double x = log(tank_pressure / (tank_pressure - end_pressure));
    double delta_p = end_pressure - start_pressure;

    double result = w1 * x + w2 * delta_p + b;

    return result;
}

void AIModel::calculateDescent(double error, double start_pressure, double end_pressure, double tank_pressure) {
    // Gradient descent updates
    start_pressure = normalize(start_pressure,0,200);
    end_pressure = normalize(end_pressure,0,200);
    tank_pressure = normalize(tank_pressure,0,200);
    double x = log(tank_pressure / (tank_pressure - end_pressure));
    double delta_p = end_pressure - start_pressure;

    w1 -= learning_rate * error * x;
    w2 -= learning_rate * error * delta_p;
    b  -= learning_rate * error;
}

double AIModel::predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure) {
    return denormalize(predict(start_pressure, end_pressure, tank_pressure), 0, 5000);
}

// Train the model with one sample
void AIModel::train(double start_pressure, double end_pressure, double tank_pressure, double actual_time) {

    double pred = predict(start_pressure, end_pressure, tank_pressure);
    double error = pred - normalize(actual_time,0,5000); //use normalized version here so that the values made by the predict are normalized
    
    calculateDescent(error, start_pressure, end_pressure, tank_pressure);
}

// Optionally: add method to print weights for debugging
void AIModel::print_weights() {
    #ifdef test_run
    std::cout << std::setprecision (5) << "Weights: w1=" << w1 << ", w2=" << w2  <<", b=" << b << std::endl;
               //<< ", w3=" << w3 << ", w4=" << w4 << ", w5=" << w5 <<", b=" << b << std::endl;
    #else
    Serial.print("Weights: ");
    Serial.print("w1=");
    Serial.print(w1,5);
    Serial.print(" w2=");
    Serial.print(w2,5);
    // Serial.print(" w3=");
    // Serial.print(w3,5);
    // Serial.print(" w4=");
    // Serial.print(w4,5);
    // Serial.print(" w5=");
    // Serial.print(w5,5);
    Serial.print(" b=");
    Serial.print(b,5);
    Serial.println();

    Serial.print("y = ");
    Serial.print(w1,5);
    Serial.print("*i + ");
    Serial.print(w2,5);
    Serial.print("*j + ");
    // Serial.print(w3,5);
    // Serial.print("*k + ");
    // Serial.print(w4,5);
    // Serial.print("*l + ");
    // Serial.print(w5,5);
    // Serial.print("*m + ");
    Serial.print(b,5);
    Serial.println();

    Serial.print("loadWeights(");
    Serial.print(w1,5);
    Serial.print(",");
    Serial.print(w2,5);
    Serial.print(",");
    // Serial.print(w3,5);
    // Serial.print(",");
    // Serial.print(w4,5);
    // Serial.print(",");
    // Serial.print(w5,5);
    // Serial.print(",");
    Serial.print(b,5);
    Serial.println(");");
    
    #endif

}


#ifdef test_run

void testPredict(AIModel model, double start_pressure, double end_pressure, double tank_pressure) {
    double time = model.predictDeNormalized(start_pressure, end_pressure, tank_pressure);
    std::cout << std::setprecision (4) << start_pressure << " to " << end_pressure << " @ " << tank_pressure << ": " << time << std::setprecision (2) << "ms (" << (time/1000) << " second" << ")" <<  std::endl;
}
#define doTraining true
int main() {
    //bool type = true;
    AIModel model;
    //if (type) {
    #if doTraining == true
        // Example training data: {start_pressure, end_pressure, tank_pressure, actual_time}
        // std::vector<std::tuple<double, double, double, double>> training_data = {
        //     {30.0, 100.0, 180.0, 3*1000},
        //     //{100.0, 90.0, 170.0, 0.2},
        //     {90.0, 130.0, 160.0, 2.5*1000},
        //     //{130.0, 30.0, 175.0, 4.1},
        //     {30.0, 90.0, 170.0, 3.1*1000},
        //     {50.0, 90.0, 170.0, 2.3*1000},
        //     {85.0, 115.0, 165.0, 2.8*1000},
        //     {85.0, 130.0, 140.0, 5*1000}
        // };

        // // up data from corvette:
        // std::vector<std::tuple<double, double, double, double>> training_data = {
        //     {33, 86, 170, 500}, {36, 78, 170, 500}, {29, 80, 170, 500}, {37, 86, 170, 500}, {86, 89, 131, 100}, {77, 81, 131, 100}, {86, 90, 131, 100}, {80, 83, 131, 100}, {81, 84, 149, 100}, {90, 94, 149, 100}, {89, 93, 149, 100}, {83, 86, 149, 100}, {93, 96, 151, 40}, {85, 88, 151, 100}, {94, 96, 151, 40}, {86, 87, 151, 100}, {87, 89, 150, 100}, {88, 92, 150, 100}, {89, 90, 151, 40}, {91, 93, 151, 40}, {90, 92, 151, 40}, {94, 96, 151, 40}, {91, 96, 152, 40}, {77, 83, 150, 100}, {76, 84, 150, 100}, {75, 79, 150, 100}, {74, 88, 151, 250}, {83, 87, 148, 100}, {83, 88, 148, 100}, {87, 92, 144, 100}, {78, 83, 148, 100}, {87, 93, 144, 100}, {88, 93, 144, 100}, {83, 89, 141, 100}, {92, 93, 141, 40}, {93, 97, 141, 40}, {94, 98, 141, 40}, {93, 96, 141, 40}, {89, 91, 141, 40}, {90, 93, 139, 40}, {92, 95, 143, 40}, {93, 99, 172, 40}, {29, 77, 158, 500}, {27, 83, 158, 500}, {28, 76, 158, 500}, {27, 86, 158, 500}, {83, 87, 127, 100}, {77, 78, 127, 100}, {77, 82, 127, 100}, {85, 91, 127, 100}, {78, 82, 133, 100}, {87, 93, 133, 100}, {82, 84, 133, 100}, {91, 92, 133, 40}, {93, 96, 134, 40}, {82, 83, 134, 100}, {84, 87, 134, 100}, {92, 94, 134, 40}, {94, 96, 134, 40}, {83, 85, 134, 100}, {87, 90, 134, 100}, {95, 100, 133, 40}, {85, 88, 133, 100}, {90, 94, 133, 100}, {94, 95, 134, 40}, {87, 91, 134, 100}, {95, 99, 137, 40}, {91, 92, 137, 40}, {92, 95, 135, 40}, {20, 85, 182, 500}, {20, 86, 182, 500}, {18, 75, 182, 500}, {30, 78, 182, 500}, {85, 90, 136, 100}, {85, 88, 136, 100}, {75, 79, 136, 100}, {79, 82, 136, 100}, {90, 94, 157, 100}, {88, 93, 157, 100}, {79, 81, 157, 100}, {82, 87, 157, 100}, {94, 96, 161, 40}, {93, 96, 161, 40}, {80, 83, 161, 100}, {86, 90, 161, 100}, {82, 88, 159, 100}, {90, 95, 159, 100}, {87, 92, 158, 100}, {95, 97, 158, 40}, {92, 95, 160, 40}, {21, 33, 168, 20}, {24, 35, 168, 20}, {27, 83, 171, 1000}, {29, 87, 171, 1000}, {25, 96, 171, 1000}, {25, 101, 171, 1000}, {96, 98, 141, 20}, {83, 90, 141, 200}, {87, 96, 141, 200}, {98, 100, 151, 20}, {89, 93, 151, 80}, {96, 97, 151, 20}, {100, 104, 155, 20}, {96, 98, 155, 20}, {93, 97, 155, 80}, {98, 100, 154, 20}, {97, 99, 154, 20}, {98, 103, 154, 20}, {100, 102, 154, 20}, {98, 103, 154, 20}, {25, 93, 158, 1000}, {27, 81, 158, 1000}, {25, 97, 157, 1000}, {28, 85, 157, 1000}, {93, 98, 124, 80}, {95, 102, 146, 80}, {98, 97, 146, 20}, {79, 87, 146, 200}, {85, 93, 146, 200}, {97, 99, 143, 20}, {92, 95, 143, 80}, {86, 93, 143, 200}, {98, 102, 143, 20}, {95, 100, 143, 80}, {93, 96, 143, 80}, {100, 102, 141, 20}, {96, 98, 141, 20}, {97, 100, 144, 20}, {23, 36, 177, 20}, {23, 36, 181, 20}, {17, 51, 181, 80}, {17, 46, 178, 80}, {10, 68, 175, 200}, {18, 49, 175, 80}, {28, 99, 171, 1000}, {28, 87, 171, 1000}, {27, 102, 171, 1000}, {30, 91, 171, 1000}, {99, 106, 134, 20}, {86, 95, 134, 200}, {90, 100, 134, 200}, {93, 97, 147, 80}, {98, 99, 147, 20}, {100, 104, 149, 20}, {96, 99, 149, 20}, {99, 102, 149, 20}, {98, 96, 146, 20}, {96, 98, 146, 20}, {94, 101, 146, 80}, {98, 100, 148, 20}, {26, 93, 160, 1000}, {25, 95, 159, 1000}, {23, 81, 159, 1000}, {27, 85, 159, 1000}, {92, 96, 147, 80}, {95, 100, 147, 80}, {80, 87, 147, 200}, {96, 98, 144, 20}, {85, 93, 144, 200}, {99, 101, 144, 20}, {98, 99, 140, 20}, {86, 95, 140, 200}, {93, 96, 140, 80}, {98, 101, 140, 20}, {94, 97, 142, 20}, {96, 98, 142, 20}, {96, 98, 142, 20}, {98, 100, 142, 20}, {97, 99, 142, 20}, {100, 102, 144, 20}, {98, 101, 144, 20}, {25, 35, 147, 20}, {27, 79, 148, 1000}, {25, 92, 148, 1000}, {26, 91, 148, 1000}, {30, 84, 148, 1000}, {92, 95, 124, 80}, {78, 84, 124, 200}, {90, 97, 124, 200}, {83, 91, 124, 200}, {94, 99, 128, 80}, {96, 97, 128, 20}, {82, 89, 128, 200}, {90, 97, 128, 200}, {99, 100, 130, 20}, {97, 98, 130, 20}, {89, 93, 130, 80}, {97, 100, 130, 20}, {98, 100, 130, 20}, {100, 101, 130, 20}, {92, 95, 131, 80}, {100, 101, 131, 20}, {99, 103, 131, 20}, {94, 96, 131, 20}, {96, 98, 130, 20}, {98, 101, 130, 20}, {97, 99, 131, 20}, {23, 35, 137, 20}, {23, 34, 137, 20}, {22, 32, 137, 20}, {23, 49, 137, 20}, {25, 34, 136, 20}, {29, 74, 140, 500}, {26, 85, 140, 500}, {27, 77, 140, 500}, {25, 84, 140, 500}, {70, 75, 131, 20}, {71, 79, 133, 20}, {72, 79, 133, 20}, {71, 76, 133, 20}, {72, 80, 133, 20}, {73, 80, 133, 20}, {71, 76, 133, 20}, {72, 80, 133, 20}, {73, 80, 132, 20}, {72, 80, 134, 20}, {73, 80, 133, 20}, {73, 80, 133, 20}, {73, 81, 134, 20}, {73, 81, 133, 20}, {74, 81, 132, 20}, {74, 81, 134, 20}, {74, 93, 135, 500}, {75, 93, 135, 500}, {73, 85, 135, 500}, {72, 85, 135, 500}, {93, 97, 119, 80}, {93, 95, 119, 80}, {85, 86, 119, 200}, {85, 91, 119, 200}, {97, 98, 127, 20}, {95, 97, 127, 80}, {86, 92, 127, 200}, {97, 97, 128, 20}, {88, 98, 128, 200}, {97, 101, 128, 20}, {89, 93, 128, 80}, {97, 100, 126, 20}, {97, 98, 126, 20}, {100, 102, 126, 20}, {93, 97, 126, 80}, {99, 102, 126, 20}, {97, 99, 125, 20}, {100, 102, 125, 20}, {96, 97, 125, 20}, {98, 100, 125, 20}, {97, 98, 125, 20}, {100, 101, 125, 20}, {98, 99, 127, 20}, {70, 75, 136, 20}, {78, 89, 138, 200}, {77, 81, 138, 200}, {77, 87, 138, 200}, {73, 96, 138, 500}, {90, 94, 128, 200}, {79, 88, 128, 200}, {84, 95, 128, 200}, {95, 97, 129, 80}, {94, 100, 129, 80}, {97, 96, 129, 20}, {87, 95, 129, 200}, {93, 98, 129, 80}, {98, 100, 129, 20}, {96, 98, 129, 20}, {94, 96, 129, 20}, {98, 99, 129, 20}, {99, 101, 129, 20}, {98, 99, 129, 20}, {95, 97, 129, 20}, {99, 101, 129, 20}, {99, 100, 130, 20}, {97, 98, 130, 20}, {100, 103, 130, 20}, {100, 102, 130, 20}, {98, 101, 130, 20}, {98, 102, 129, 20}, {20, 57, 179, 80}, {23, 35, 179, 20}, {19, 53, 181, 80}, {23, 45, 181, 20}, {17, 45, 177, 80}, {25, 37, 178, 20}
        // };

        // // down data from corvette:
        // std::vector<std::tuple<double, double, double, double>> training_data = {
        //     {96, 94, 159, 100}, {105, 92, 159, 250}, {105, 95, 159, 250}, {109, 96, 159, 250}, {94, 88, 157, 100}, {93, 88, 157, 100}, {96, 91, 157, 100}, {96, 91, 157, 100}, {88, 84, 157, 100}, {88, 85, 157, 100}, {91, 87, 157, 100}, {91, 87, 157, 100}, {84, 80, 157, 40}, {85, 82, 157, 40}, {87, 83, 157, 100}, {87, 83, 157, 100}, {82, 80, 157, 40}, {83, 81, 157, 40}, {83, 81, 157, 40}, {81, 80, 159, 40}, {81, 79, 159, 40}, {79, 76, 158, 40}, {123, 115, 160, 100}, {136, 124, 161, 250}, {137, 123, 160, 250}, {131, 115, 160, 250}, {116, 106, 160, 100}, {124, 111, 158, 100}, {122, 116, 158, 100}, {116, 104, 158, 100}, {110, 107, 158, 40}, {111, 108, 158, 100}, {115, 109, 158, 100}, {108, 115, 158, 40}, {109, 112, 158, 40}, {109, 104, 159, 40}, {111, 102, 159, 100}, {120, 98, 181, 500}, {125, 98, 181, 500}, {100, 88, 181, 500}, {100, 86, 181, 500}, {98, 83, 177, 500}, {98, 82, 178, 500}, {87, 78, 178, 500}, {87, 76, 178, 500}, {77, 74, 171, 250}, {84, 73, 171, 500}, {77, 72, 175, 250}, {82, 66, 171, 500}, {74, 69, 177, 250}, {73, 67, 177, 250}, {72, 68, 177, 250}, {66, 56, 177, 250}, {70, 63, 179, 250}, {67, 58, 179, 250}, {68, 66, 179, 250}, {56, 46, 179, 250}, {64, 58, 178, 250}, {58, 45, 178, 250}, {47, 44, 178, 100}, {67, 59, 178, 250}, {46, 42, 177, 100}, {59, 48, 177, 250}, {44, 40, 177, 100}, {60, 44, 177, 250}, {42, 36, 174, 100}, {40, 31, 174, 100}, {48, 41, 174, 100}, {45, 38, 174, 100}, {36, 23, 174, 100}, {41, 36, 174, 100}, {32, 29, 174, 40}, {38, 32, 174, 100}, {36, 33, 178, 40}, {34, 31, 173, 40}, {97, 74, 168, 1000}, {99, 67, 168, 1000}, {98, 70, 168, 1000}, {96, 75, 168, 1000}, {74, 65, 164, 500}, {68, 46, 164, 500}, {75, 69, 164, 500}, {70, 51, 164, 500}, {46, 42, 166, 200}, {66, 50, 166, 500}, {69, 47, 166, 500}, {51, 33, 166, 500}, {42, 28, 165, 200}, {28, 28, 167, 20}, {47, 36, 165, 200}, {50, 40, 165, 200}, {33, 30, 167, 80}, {29, 21, 167, 20}, {36, 30, 167, 80}, {30, 28, 168, 20}, {40, 29, 168, 200}, {30, 24, 168, 20}, {28, 24, 168, 20}, {29, 28, 168, 20}, {33, 25, 168, 80}, {35, 26, 168, 80}, {104, 98, 155, 20}, {103, 98, 154, 20}, {101, 77, 157, 1000}, {102, 74, 157, 1000}, {97, 74, 157, 1000}, {99, 68, 157, 1000}, {76, 66, 151, 500}, {74, 56, 155, 500}, {69, 42, 155, 500}, {74, 68, 155, 500}, {66, 51, 153, 500}, {43, 37, 155, 200}, {56, 35, 153, 500}, {72, 42, 155, 500}, {52, 41, 155, 200}, {37, 27, 155, 200}, {35, 29, 155, 80}, {43, 30, 155, 200}, {27, 26, 156, 20}, {30, 27, 156, 20}, {41, 30, 156, 200}, {31, 28, 156, 20}, {26, 24, 156, 20}, {27, 24, 156, 20}, {31, 29, 157, 20}, {29, 26, 157, 20}, {29, 27, 157, 20}, {97, 74, 174, 1000}, {93, 68, 174, 1000}, {95, 70, 174, 1000}, {96, 75, 174, 1000}, {73, 65, 167, 500}, {68, 49, 167, 500}, {70, 54, 167, 500}, {75, 69, 175, 500}, {50, 42, 171, 200}, {65, 52, 171, 500}, {54, 31, 171, 500}, {71, 45, 171, 500}, {42, 30, 175, 200}, {52, 41, 175, 200}, {31, 26, 175, 80}, {45, 32, 175, 200}, {31, 27, 175, 80}, {41, 29, 175, 200}, {26, 23, 177, 20}, {27, 25, 177, 20}, {33, 26, 177, 80}, {29, 29, 177, 20}, {26, 22, 181, 20}, {29, 27, 181, 20}, {36, 17, 181, 200}, {36, 17, 181, 200}, {51, 10, 178, 500}, {46, 24, 175, 200}, {68, 17, 177, 500}, {49, 25, 175, 200}, {106, 100, 147, 20}, {104, 94, 149, 20}, {99, 73, 144, 1000}, {100, 70, 144, 1000}, {100, 77, 144, 1000}, {99, 75, 144, 1000}, {74, 58, 142, 500}, {70, 52, 142, 500}, {76, 68, 142, 500}, {75, 71, 142, 500}, {59, 40, 143, 500}, {52, 38, 143, 500}, {67, 51, 143, 500}, {71, 51, 143, 500}, {41, 27, 143, 200}, {38, 29, 143, 200}, {51, 41, 143, 200}, {51, 36, 144, 200}, {28, 24, 144, 20}, {30, 26, 144, 20}, {37, 31, 144, 80}, {42, 30, 144, 200}, {27, 23, 144, 20}, {31, 28, 144, 20}, {30, 28, 144, 20}, {28, 25, 146, 20}, {35, 27, 146, 80}, {103, 97, 130, 20}, {99, 78, 134, 1000}, {101, 73, 134, 1000}, {104, 76, 134, 1000}, {100, 79, 134, 1000}, {75, 54, 136, 500}, {79, 57, 136, 1000}, {78, 40, 136, 1000}, {78, 47, 136, 1000}, {54, 28, 135, 500}, {41, 29, 136, 200}, {29, 29, 136, 20}, {48, 35, 136, 200}, {58, 31, 136, 500}, {29, 26, 136, 20}, {29, 25, 136, 20}, {35, 28, 136, 80}, {32, 30, 136, 20}, {26, 23, 137, 20}, {26, 22, 137, 20}, {28, 22, 137, 20}, {30, 28, 137, 20}, {35, 25, 137, 80}, {34, 23, 137, 80}, {32, 28, 137, 20}, {28, 25, 137, 20}, {49, 25, 136, 200}, {26, 24, 135, 20}, {34, 26, 135, 80}, {74, 69, 108, 20}, {77, 74, 131, 20}, {84, 71, 131, 80}, {84, 72, 131, 80}, {75, 71, 133, 20}, {79, 72, 133, 20}, {79, 73, 133, 20}, {76, 71, 133, 20}, {79, 72, 133, 20}, {80, 73, 133, 20}, {76, 72, 132, 20}, {80, 72, 132, 20}, {80, 74, 132, 20}, {80, 73, 134, 20}, {80, 73, 133, 20}, {80, 73, 133, 20}, {80, 73, 133, 20}, {81, 73, 133, 20}, {81, 74, 133, 20}, {81, 73, 135, 20}, {97, 92, 135, 200}, {100, 90, 135, 200}, {99, 92, 135, 200}, {100, 76, 135, 500}, {93, 83, 133, 200}, {90, 81, 133, 200}, {92, 82, 133, 200}, {79, 75, 133, 80}, {85, 80, 133, 80}, {82, 77, 133, 80}, {82, 74, 133, 80}, {76, 76, 134, 20}, {80, 79, 134, 20}, {77, 74, 134, 20}, {76, 74, 135, 20}, {79, 77, 135, 20}, {75, 73, 135, 20}, {78, 77, 135, 20}, {74, 70, 136, 20}, {77, 78, 136, 20}, {78, 74, 136, 20}, {75, 72, 134, 20}, {103, 98, 130, 20}, {113, 78, 182, 1000}, {112, 77, 182, 1000}, {97, 73, 182, 1000}, {97, 77, 182, 1000}, {74, 64, 179, 500}, {77, 72, 179, 500}, {78, 41, 179, 1000}, {78, 38, 179, 1000}, {41, 38, 175, 200}, {64, 52, 175, 500}, {39, 33, 175, 200}, {72, 51, 175, 500}, {38, 24, 173, 200}, {34, 28, 179, 80}, {53, 43, 179, 200}, {53, 20, 179, 500}, {29, 23, 178, 20}, {44, 32, 178, 200}, {32, 30, 179, 20}, {35, 26, 179, 80}, {31, 30, 179, 20}, {56, 18, 179, 500}, {27, 23, 178, 20}, {30, 28, 178, 20}, {45, 28, 177, 200}, {53, 17, 177, 500}, {28, 25, 177, 20}, {45, 25, 178, 200}, {37, 28, 180, 80}, {28, 25, 178, 20}
        // };

        // // new up data only front
        // std::vector<std::tuple<double, double, double, double>> training_data = {
        //     {25, 74, 166, 500}, {29, 71, 166, 500}, {74, 77, 128, 100}, {71, 81, 128, 250}, {77, 79, 128, 100}, {81, 88, 140, 100}, {77, 81, 140, 100}, {88, 92, 140, 100}, {81, 85, 143, 100}, {93, 94, 143, 40}, {85, 89, 143, 100}, {95, 96, 143, 40}, {89, 90, 142, 40}, {96, 99, 142, 40}, {91, 92, 142, 40}, {99, 100, 142, 10}, {99, 101, 146, 10}, {92, 95, 146, 40}, {101, 102, 146, 10}, {95, 96, 146, 10}, {96, 97, 144, 10}, {97, 98, 144, 10}, {71, 84, 157, 250}, {72, 86, 157, 250}, {84, 91, 141, 100}, {86, 92, 141, 100}, {90, 94, 144, 100}, {92, 98, 144, 100}, {95, 96, 144, 40}, {98, 101, 144, 40}, {97, 99, 144, 40}, {101, 102, 145, 10}, {99, 100, 145, 10}, {102, 104, 145, 10}, {100, 101, 145, 10}, {103, 103, 145, 10}, {101, 102, 150, 10}, {21, 30, 186, 10}, {34, 73, 183, 500}, {26, 76, 183, 500}, {73, 84, 131, 250}, {74, 79, 131, 250}, {84, 87, 148, 100}, {78, 82, 148, 100}, {87, 91, 148, 100}, {81, 85, 148, 100}, {90, 95, 148, 100}, {85, 91, 148, 100}, {96, 98, 148, 40}, {91, 96, 152, 100}, {99, 99, 152, 10}, {96, 98, 152, 40}, {98, 102, 152, 40}, {98, 99, 152, 10}, {102, 103, 153, 10}, {98, 99, 153, 10}, {100, 101, 153, 10}, {102, 102, 155, 10}, {101, 103, 155, 10}, {30, 70, 168, 500}, {24, 74, 168, 500}, {67, 77, 135, 250}, {73, 81, 135, 250}, {78, 82, 138, 100}, {75, 87, 138, 250}, {81, 85, 136, 100}, {87, 92, 136, 100}, {85, 89, 136, 100}, {90, 96, 140, 100}, {89, 92, 140, 100}, {95, 98, 139, 40}, {91, 95, 139, 100}, {96, 98, 142, 40}, {95, 97, 142, 40}, {99, 101, 140, 10}, {97, 100, 140, 40}, {100, 102, 140, 10}, {99, 101, 140, 10}, {101, 102, 139, 10}, {101, 103, 139, 10}, {102, 103, 139, 10}, {102, 104, 139, 10}, {33, 74, 183, 500}, {26, 79, 183, 500}, {72, 93, 148, 250}, {92, 94, 161, 100}, {70, 84, 148, 250}, {91, 98, 158, 100}, {78, 88, 158, 100}, {96, 100, 161, 40}, {87, 93, 161, 100}, {23, 31, 165, 10}, {23, 31, 165, 10}, {23, 31, 166, 10}, {31, 78, 165, 500}, {29, 70, 165, 500}, {75, 90, 125, 250}, {61, 79, 146, 250}, {82, 89, 146, 100}, {78, 91, 146, 250}, {88, 89, 144, 100}, {91, 97, 144, 100}, {85, 91, 140, 100}, {96, 99, 143, 40}, {90, 96, 143, 100}, {97, 101, 143, 40}, {94, 97, 142, 40}, {98, 103, 143, 40}, {97, 99, 143, 40}, {101, 103, 141, 10}, {98, 101, 143, 10}, {102, 105, 141, 10}, {100, 102, 141, 10}, {101, 103, 141, 10}, {103, 106, 141, 10}, {102, 103, 142, 10}, {102, 104, 142, 10}, {71, 83, 152, 250}, {71, 84, 152, 250}, {81, 89, 150, 100}, {82, 90, 139, 100}, {87, 92, 139, 100}, {87, 94, 145, 100}, {91, 97, 146, 100}, {92, 99, 146, 100}, {95, 96, 143, 40}, {98, 101, 144, 40}, {97, 100, 144, 40}, {99, 101, 143, 10}, {100, 103, 143, 10}, {100, 103, 143, 10}, {101, 104, 142, 10}, {102, 103, 142, 10}, {103, 105, 142, 10}, {102, 104, 143, 10}, {102, 105, 144, 10}, {103, 106, 144, 10}, {72, 87, 171, 250}, {85, 95, 152, 100}, {93, 102, 165, 100}, {73, 87, 171, 250}, {101, 104, 163, 10}, {102, 104, 164, 10}, {82, 91, 164, 100}, {103, 105, 162, 10}, {90, 98, 164, 100}, {97, 101, 163, 40}, {100, 102, 162, 10}, {102, 105, 164, 10}, {31, 63, 165, 250}, {65, 69, 157, 10}, {66, 69, 157, 10}, {26, 63, 165, 250}, {68, 71, 158, 10}, {60, 68, 158, 40}, {68, 72, 157, 10}, {66, 69, 157, 10}, {68, 71, 156, 10}, {68, 79, 158, 250}, {76, 93, 154, 250}, {91, 98, 152, 100}, {98, 101, 151, 40}, {99, 102, 149, 10}, {101, 104, 149, 10}, {102, 105, 148, 10}, {70, 78, 158, 250}, {67, 84, 149, 250}, {82, 89, 148, 100}, {88, 94, 149, 100}, {97, 102, 150, 40}, {91, 97, 150, 100}, {101, 103, 147, 10}, {102, 105, 149, 10}, {96, 99, 149, 40}, {98, 100, 148, 10}, {99, 101, 148, 10}, {101, 103, 147, 10}, {26, 73, 149, 500}, {30, 70, 149, 500}, {70, 85, 122, 250}, {62, 79, 135, 250}, {73, 87, 130, 250}, {73, 87, 133, 250}, {85, 93, 131, 100}, {86, 88, 132, 100}, {91, 98, 131, 100}, {83, 89, 131, 100}, {87, 93, 130, 100}, {94, 99, 130, 40}, {92, 98, 130, 100}, {96, 99, 133, 40}, {95, 99, 133, 40}, {97, 102, 132, 40}, {98, 100, 133, 10}, {100, 103, 132, 10}, {98, 100, 133, 10}, {99, 101, 134, 10}, {101, 104, 135, 10}, {100, 102, 134, 10}, {102, 105, 133, 10}
        // };


        // new up data rear only
        std::vector<std::tuple<double, double, double, double>> training_data = {
            {20, 77, 170, 500}, {21, 76, 170, 500}, {77, 82, 128, 100}, {76, 87, 128, 250}, {82, 87, 128, 100}, {87, 93, 140, 100}, {87, 93, 140, 100}, {93, 96, 140, 40}, {93, 96, 140, 40}, {96, 98, 143, 40}, {96, 98, 143, 40}, {98, 100, 143, 10}, {98, 99, 143, 10}, {99, 101, 143, 10}, {99, 101, 142, 10}, {101, 102, 142, 10}, {100, 102, 142, 10}, {102, 103, 142, 10}, {101, 102, 142, 10}, {73, 87, 157, 250}, {74, 88, 157, 250}, {88, 95, 141, 100}, {87, 93, 141, 100}, {95, 98, 141, 40}, {93, 97, 141, 40}, {98, 100, 144, 40}, {96, 100, 144, 40}, {100, 101, 144, 10}, {99, 101, 144, 10}, {100, 102, 145, 10}, {101, 102, 145, 10}, {101, 103, 145, 10}, {102, 104, 145, 10}, {104, 108, 150, 10}, {104, 106, 148, 10}, {22, 77, 183, 500}, {24, 81, 183, 500}, {80, 83, 131, 100}, {76, 88, 131, 250}, {81, 87, 148, 100}, {87, 91, 148, 100}, {87, 92, 148, 100}, {90, 94, 148, 100}, {92, 101, 148, 100}, {93, 96, 148, 40}, {100, 102, 148, 10}, {96, 100, 148, 40}, {101, 103, 152, 10}, {99, 101, 152, 10}, {103, 104, 152, 10}, {100, 102, 152, 10}, {104, 105, 152, 10}, {102, 104, 152, 10}, {21, 74, 168, 500}, {20, 74, 168, 500}, {74, 83, 135, 250}, {74, 83, 135, 250}, {83, 90, 138, 100}, {82, 87, 138, 100}, {90, 95, 136, 100}, {86, 91, 136, 100}, {94, 99, 136, 100}, {91, 97, 140, 100}, {99, 100, 140, 40}, {97, 100, 140, 40}, {101, 102, 140, 10}, {99, 100, 139, 10}, {102, 103, 139, 10}, {99, 101, 142, 10}, {102, 104, 142, 10}, {101, 102, 140, 10}, {103, 105, 140, 10}, {102, 103, 140, 10}, {104, 106, 140, 10}, {23, 79, 183, 500}, {23, 82, 183, 500}, {82, 85, 148, 100}, {74, 91, 148, 250}, {83, 92, 161, 100}, {89, 97, 158, 100}, {97, 99, 161, 40}, {89, 95, 161, 100}, {99, 105, 161, 40}, {22, 78, 165, 500}, {76, 96, 125, 250}, {96, 97, 146, 40}, {21, 80, 161, 500}, {94, 101, 146, 100}, {98, 99, 144, 40}, {70, 87, 144, 250}, {98, 103, 143, 40}, {100, 103, 142, 10}, {102, 105, 141, 10}, {104, 106, 142, 10}, {85, 91, 142, 100}, {88, 96, 142, 100}, {95, 99, 140, 40}, {98, 101, 141, 10}, {100, 103, 142, 10}, {102, 104, 143, 10}, {72, 86, 152, 250}, {73, 88, 150, 250}, {88, 94, 139, 100}, {94, 100, 146, 100}, {84, 92, 139, 100}, {100, 102, 146, 10}, {89, 96, 143, 100}, {99, 104, 144, 40}, {103, 104, 142, 10}, {103, 106, 143, 10}, {94, 99, 142, 40}, {97, 101, 142, 40}, {100, 103, 143, 10}, {102, 104, 143, 10}, {103, 105, 144, 10}, {74, 92, 171, 250}, {76, 93, 171, 250}, {90, 100, 152, 100}, {99, 101, 166, 10}, {89, 99, 166, 100}, {99, 103, 165, 10}, {97, 102, 166, 40}, {102, 105, 166, 10}, {101, 103, 163, 10}, {103, 106, 162, 10}, {22, 64, 165, 250}, {20, 88, 165, 500}, {59, 71, 157, 100}, {70, 75, 157, 10}, {70, 74, 157, 10}, {73, 85, 158, 250}, {71, 86, 158, 250}, {85, 94, 151, 100}, {81, 89, 151, 100}, {91, 100, 151, 100}, {85, 94, 151, 100}, {98, 100, 149, 10}, {92, 100, 149, 100}, {97, 103, 149, 40}, {98, 103, 150, 40}, {100, 104, 149, 10}, {101, 103, 149, 10}, {102, 105, 149, 10}, {104, 106, 149, 10}, {96, 100, 150, 40}, {104, 107, 147, 10}, {99, 102, 148, 10}, {104, 107, 148, 10}, {101, 104, 149, 10}, {102, 105, 148, 10}, {21, 76, 149, 500}, {22, 75, 149, 500}, {73, 85, 133, 250}, {73, 86, 133, 250}, {80, 88, 130, 100}, {85, 91, 130, 100}, {86, 93, 131, 100}, {89, 94, 131, 100}, {91, 97, 131, 100}, {92, 99, 129, 100}, {96, 101, 130, 40}, {94, 99, 130, 40}, {95, 98, 133, 40}, {99, 103, 133, 40}, {97, 101, 132, 40}, {101, 103, 133, 10}, {100, 102, 132, 10}, {102, 104, 133, 10}, {103, 105, 134, 10}, {101, 103, 134, 10}, {104, 106, 134, 10}, {101, 104, 133, 10}, {76, 102, 177, 500}, {75, 101, 177, 500}, {101, 117, 154, 250}, {97, 114, 160, 250}, {113, 122, 159, 100}, {111, 119, 161, 100}, {119, 127, 159, 100}, {126, 126, 161, 10}, {117, 124, 160, 100}, {125, 128, 160, 10}, {122, 126, 160, 40}, {126, 128, 158, 10}, {125, 127, 159, 10}, {23, 76, 162, 500}, {22, 78, 162, 500}, {74, 92, 144, 250}, {91, 93, 138, 100}, {70, 85, 138, 250}, {91, 99, 140, 100}, {82, 90, 137, 100}, {96, 101, 141, 40}, {88, 96, 141, 100}, {97, 102, 141, 40}, {94, 98, 140, 40}, {100, 102, 139, 10}, {97, 101, 141, 40}, {101, 103, 141, 10}, {22, 68, 161, 250},
        };

        // Train for several epochs
        //int i = 0;
        for (int epoch = 0; epoch < 1000*10; ++epoch) {
            for (const auto& [start, end, tank, time] : training_data) {
                //for (int epoch = 0; epoch < 1000*10*10; ++epoch) {
                //if (time >= 50) {
                    //i = i + 1;
                    model.train(start, end, tank, time);
                //}
                //}
            }
        }
        //std::cout << i << std::endl;

    #else
    //} else {
        model.loadWeights(0.08092,0.08144,-0.12231,0.08132,0.06793,-0.03788);
        //model.loadWeights(0.09092,0.09152,-0.01490,-0.00902,0.08862,-0.01865);
    //}
    #endif

    // Test prediction
    //double predicted_time2 = model.predict(std::get<0>(training_data[0]), std::get<1>(training_data[0]), std::get<2>(training_data[0]));
    
    testPredict(model,0, 64, 180);
    testPredict(model,64, 128, 180);
    std::cout << std::endl;
    testPredict(model,0, 64, 140);
    testPredict(model,64, 128, 140);
    std::cout << std::endl;
    testPredict(model,0, 64, 90);
    testPredict(model,20, 119, 120);
    std::cout << std::endl;

    testPredict(model,100, 130, 170);
    testPredict(model,90, 110, 170);
    std::cout << std::endl;

    testPredict(model,0, 130, 170);
    testPredict(model,0, 110, 170);
    std::cout << std::endl;

    testPredict(model,0, 130, 150);
    testPredict(model,0, 110, 150);
    std::cout << std::endl;


    
    model.print_weights(); // Optional: see the learned weights
}
#endif
#endif