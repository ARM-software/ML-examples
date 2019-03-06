#ifndef __NN_H__
#define __NN_H__

#include "mbed.h"
#include "arm_math.h"
#include "parameter.h"
#include "weights.h"
#include "arm_nnfunctions.h"

void run_nn(q7_t* input_data, q7_t* output_data);

#endif
