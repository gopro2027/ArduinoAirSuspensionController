
#ifndef pressureMath_h
#define pressureMath_h
#include <Arduino.h>
#ifdef parabolaLearn
void calc_parabola_vertex(double x1, double y1, double x2, double y2, double x3, double y3, double &A, double &B, double &C);
double calc_parabola_y(double A, double B, double C, double x_val);
void calculateAverageOfSamples(double *x, double *y, int sz, double &A, double &B, double &C);
#endif
#endif