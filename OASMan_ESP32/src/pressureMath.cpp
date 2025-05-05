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
#endif


double normalize(double x, double min, double max) {
    return (x - min) / (max - min);
}

double denormalize(double x, double min, double max) {
    return x * (max - min) + min;
}

void AIModel::loadWeights(double _w1, double _w2, double _w3,double _w4,double _w5, double _b) {
    w1 = _w1;//start_pressure
    w2 = _w2;//end_pressure
    w3 = _w3;//tank_pressure
    w4 = _w4;//Pressure difference: delta_p = end_pressure - start_pressure
    w5 = _w5;//Ratio: start_pressure / tank_pressure or log versions
    b = _b;
}

// Predict time from inputs
double AIModel::predict(double start_pressure, double end_pressure, double tank_pressure, double delta_p, double ratio) {
    //delta_p can be pre-normalized since it's based off of pre-existing values
    //ratio is naturally normalized between 0 and 1 due to the nature of the values
    start_pressure = normalize(start_pressure,0,200);
    end_pressure = normalize(end_pressure,0,200);
    tank_pressure = normalize(tank_pressure,0,200);
    return w1 * start_pressure + w2 * end_pressure + w3 * tank_pressure + w4 * delta_p + w5 * ratio + b;
}

double AIModel::predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure) {
    double delta_p = normalize(end_pressure - start_pressure,-200,200);
    double ratio = start_pressure / tank_pressure;// no need to normalize, always between 0 and 1
    return denormalize(predict(start_pressure, end_pressure, tank_pressure, delta_p, ratio), 0, 5000);
}

// Train the model with one sample
void AIModel::train(double start_pressure, double end_pressure, double tank_pressure, double actual_time) {
    double delta_p = normalize(end_pressure - start_pressure,-200,200);
    double ratio = start_pressure / tank_pressure;// no need to normalize, always between 0 and 1

    double pred = predict(start_pressure, end_pressure, tank_pressure, delta_p, ratio);
    double error = pred - normalize(actual_time,0,5000); //use normalized version here so that the values made by the predict are normalized

    #ifdef test_run
    std::cout << "pred: " << denormalize(pred, 0, 5000) << " actual: " << actual_time << std::endl;
    #endif

    // Gradient descent updates
    w1 -= learning_rate * error * normalize(start_pressure,0,200);
    w2 -= learning_rate * error * normalize(end_pressure,0,200);
    w3 -= learning_rate * error * normalize(tank_pressure,0,200);
    w4 -= learning_rate * error * delta_p;
    w5 -= learning_rate * error * ratio;
    b  -= learning_rate * error;
}

// Optionally: add method to print weights for debugging
void AIModel::print_weights() {
    #ifdef test_run
    std::cout << "Weights: w1=" << w1 << ", w2=" << w2 
                << ", w3=" << w3 << ", w4=" << w4 << ", w5=" << w5 <<", b=" << b << std::endl;
    #else
    Serial.print("Weights: ");
    Serial.print("w1=");
    Serial.print(w1,5);
    Serial.print(" w2=");
    Serial.print(w2,5);
    Serial.print(" w3=");
    Serial.print(w3,5);
    Serial.print(" w4=");
    Serial.print(w4,5);
    Serial.print(" w5=");
    Serial.print(w5,5);
    Serial.print(" b=");
    Serial.print(b,5);
    Serial.println();

    Serial.print("y = ");
    Serial.print(w1,5);
    Serial.print("*i + ");
    Serial.print(w2,5);
    Serial.print("*j + ");
    Serial.print(w3,5);
    Serial.print("*k + ");
    Serial.print(w4,5);
    Serial.print("*l + ");
    Serial.print(w5,5);
    Serial.print("*m + ");
    Serial.print(b,5);
    Serial.println();

    Serial.print("loadWeights(");
    Serial.print(w1,5);
    Serial.print(",");
    Serial.print(w2,5);
    Serial.print(",");
    Serial.print(w3,5);
    Serial.print(",");
    Serial.print(w4,5);
    Serial.print(",");
    Serial.print(w5,5);
    Serial.print(",");
    Serial.print(b,5);
    Serial.println(");");
    
    #endif

}


#ifdef test_run
int main() {
    bool type = false;
    AIModel model;
    if (type) {
        // Example training data: {start_pressure, end_pressure, tank_pressure, actual_time}
        std::vector<std::tuple<double, double, double, double>> training_data = {
            {30.0, 100.0, 180.0, 3*1000},
            //{100.0, 90.0, 170.0, 0.2},
            {90.0, 130.0, 160.0, 2.5*1000},
            //{130.0, 30.0, 175.0, 4.1},
            {30.0, 90.0, 170.0, 3.1*1000},
            {50.0, 90.0, 170.0, 2.3*1000},
            {85.0, 115.0, 165.0, 2.8*1000},
            {85.0, 130.0, 140.0, 5*1000}
        };

        // Train for several epochs
        for (int epoch = 0; epoch < 500; ++epoch) {
            for (const auto& [start, end, tank, time] : training_data) {
                std::cout << tank;
                //for (int epoch = 0; epoch < 200; ++epoch) {
                    model.train(start, end, tank, time);
                //}
            }
        }

        
    } else {
        model.loadWeights(0.08092,0.08144,-0.12231,0.08132,0.06793,-0.03788);
        //model.loadWeights(0.09092,0.09152,-0.01490,-0.00902,0.08862,-0.01865);
    }

    // Test prediction
    double predicted_time = model.predictDeNormalized(50, 120, 180);
    double predicted_time2 = model.predictDeNormalized(50, 130, 180);
    //double predicted_time2 = model.predict(std::get<0>(training_data[0]), std::get<1>(training_data[0]), std::get<2>(training_data[0]));

    std::cout << "Predicted time: " << predicted_time << std::endl;
    std::cout << "Predicted time 2: " << predicted_time2 << std::endl;
    model.print_weights(); // Optional: see the learned weights
}
#endif
#endif