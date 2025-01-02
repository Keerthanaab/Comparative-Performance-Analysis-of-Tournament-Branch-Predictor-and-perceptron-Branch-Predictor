#include "predictor.h"
#include <cstring> // For memset

//defines the maximum global history register size
#define k 28
//defines the total number of perceptrons which will be indexed with an instruction address
#define percep_num  64

//minimum and maximum values for perceptron
#define LOWER_LIMIT -128
#define UPPER_LIMIT 127

//array size for the weight of perceptrons
int weights[percep_num * (k + 1)];

//global history register
long Globalhistory_bits;
//index bit and output of the perceptron
int index_perceptron, y;


bool output_prediction;

void PREDICTOR::init_predictor()
{
    //initializing the weights and Globalhistory_bits values to 0
    std::memset(weights, 0, sizeof(weights));
    Globalhistory_bits = 0;
}

bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os)
{
    output_prediction = false;

    //index  bits with instruction_addr and perceptron value //total index bits
    index_perceptron = br->instruction_addr & (percep_num - 1);
    // printf("instr add= %0d , percepnum-1= %0d index_1 = %0d\n",br->instruction_addr,percep_num, index_perceptron);
    //  value of y is calculated based on bias weight (first weight) and total weight of the perceptron
    y = weights[index_perceptron * (k + 1)];
    //printf("%d\n",y);
    for (int i = 1; i <= k; i++) {
    // Extract the bit corresponding to the i-th position in Globalhistory_bits
    int bit = (Globalhistory_bits >> (i - 1)) & 1;  // Shifting Globalhistory_bits to get the i-th bit (either 0 or 1)

    // Determine the value of x based on the Globalhistory_bits bit
    int x = (bit == 1) ? 1 : -1;
    //printf("x = %d\n", x);
    // Calculate the weight index and update y
    int weightIndex = index_perceptron * (k + 1) + i;
    //printf("%d\n",weightIndex);
    y += weights[weightIndex] * x;


    }
    //printf("w_y = %d\n",y);
    output_prediction = (y >= 0) ? true : false;
     //printf("prediction = %d\n",prediction);
    return output_prediction;
}




void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken)
{
    int t = taken ? 1 : -1;
    if (taken != output_prediction) {
    // Check if the first weight (bias) is within the allowed limits and update it if necessary
        int biasWeightIndex = index_perceptron * (k + 1);
                if (weights[biasWeightIndex] > LOWER_LIMIT && weights[biasWeightIndex] < UPPER_LIMIT) {
                            weights[biasWeightIndex] += t;
                                }

for (int i = 1; i <= k; ++i) {
    // Shift Globalhistory_bits and isolate the i-th bit to determine if it's 1 or 0
    int ghrBit = (Globalhistory_bits >> (i - 1)) & 1;  // Extract the i-th bit of Globalhistory_bits

    // Determine the value of x based on the extracted Globalhistory_bits bit and the 'taken' status
    int x = (ghrBit == taken) ? 1 : -1;

    // Calculate the weight index for the i-th perceptron weight
    int percep_index = index_perceptron * (k + 1) + i;

    // Check if the weight is within the valid limits and update it
    if (weights[percep_index] > LOWER_LIMIT && weights[percep_index] < UPPER_LIMIT) {
        weights[percep_index] += x;
    }
}

    }

    //updates the Globalhistory_bits values and checks for taken bits we have 28 bits
    Globalhistory_bits = ((Globalhistory_bits << 1) | (taken ? 1L : 0L)) & ((1L << k) - 1);
}



/*
i	GHR & Mask	x	Weight Index	y
1	0b00001 & 0b00001 = 1	1	1	0 + 0 * 1 = 0
2	0b00001 & 0b00010 = 0	-1	2	0 + 0 * -1 = 0
3	0b00001 & 0b00100 = 0	-1	3	0 + 0 * -1 = 0
4	0b00001 & 0b01000 = 0	-1	4	0 + 0 * -1 = 0
5	0b00001 & 0b10000 = 0	-1	5	0 + 0 * -1 = 0
*/