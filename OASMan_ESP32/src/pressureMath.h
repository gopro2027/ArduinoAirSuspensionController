
#ifndef pressureMath_h
#define pressureMath_h
// #define test_run
#ifdef test_run
#else
#include <Arduino.h>
#endif

class AIModel
{
    double learning_rate = 0.00075;
    double predict(double start_pressure, double end_pressure, double tank_pressure);
    void calculateDescent(double error, double start_pressure, double end_pressure, double tank_pressure);

public:
    // Weights for each input
    double w1 = 0.1, w2 = 0.1, b = 0.0;
    // double w3 = -0.1, w4 = 0.1, w5 = 0.1; // test weights
    bool up = true;
    void loadWeights(double _w1, double _w2, double _b);
    double predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure);
    void train(double start_pressure, double end_pressure, double tank_pressure, double actual_time);
    void print_weights();
};

#endif