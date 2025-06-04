
#ifndef pressureMath_h
#define pressureMath_h
//#define test_run
#ifdef test_run
#else
#include <Arduino.h>
#endif

#define ai_pressure_1

#ifdef parabolaLearn
void calc_parabola_vertex(double x1, double y1, double x2, double y2, double x3, double y3, double &A, double &B, double &C);
double calc_parabola_y(double A, double B, double C, double x_val);
void calculateAverageOfSamples(double *x, double *y, int sz, double &A, double &B, double &C);
#endif

#ifdef ai_pressure_1
class AIModel {
    double learning_rate = 0.00075;
    double predict(double start_pressure, double end_pressure, double tank_pressure);
    void calculateDescent(double error, double start_pressure, double end_pressure, double tank_pressure);
public:
    // Weights for each input
    double w1 = 0.1, w2 = 0.1, b = 0.0;
    //double w3 = -0.1, w4 = 0.1, w5 = 0.1; // test weights
    bool up = true;
    void loadWeights(double _w1, double _w2, double _b);
    double predictDeNormalized(double start_pressure, double end_pressure, double tank_pressure);
    void train(double start_pressure, double end_pressure, double tank_pressure, double actual_time);
    void print_weights();
};
#endif

#endif