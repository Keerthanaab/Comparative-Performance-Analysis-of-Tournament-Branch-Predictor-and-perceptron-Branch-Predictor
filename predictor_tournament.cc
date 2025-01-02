#include "predictor.h"
#include <iostream> // For debugging output
#include <cstdint>  // For fixed-width integer types

// Memory allocations for predictor tables
uint32_t* local_hist  = new uint32_t[4096]();  // Local history table
uint32_t* local_predt  = new uint32_t[4096]();  // Local prediction table
uint32_t* global_hist = new uint32_t[4096]();  // Global prediction table
uint32_t* choice_predt = new uint32_t[4096]();  // Choice predictor table

// Masks and variables
uint32_t PC_mask = 0xFFF;    // Mask for 4096 entries
uint32_t LH_mask = 0xFFF;    // Mask for 4096 entries
uint32_t LP_mask = 0x1F;     // 5-bit counters
uint32_t path_mask = 0xFFF;  // Mask for 12 bits of path history

uint32_t path_history = 0; // Initialize path history to 0
uint32_t PC_bits = 0;
uint32_t LH_bits = 0;
uint32_t LP_bits = 0;
uint32_t GP_bits = 0;
uint32_t choice_index = 0;
uint32_t choice = 0;

bool prediction = false;
uint32_t local_prediction = 0;
uint32_t global_prediction = 0;

// Get prediction from the branch predictor
bool PREDICTOR::get_prediction(const branch_record_c* br, const op_state_c* os) {
    if (!br->is_conditional) {
        return true; // Always predict true for unconditional branches
    }

    // PC bits generation
    PC_bits = (br->instruction_addr >> 2) & PC_mask;

    // Local history table and prediction
    LH_bits = LH_mask & local_hist[PC_bits];

    LP_bits = LP_mask & local_predt[LH_bits];
    local_prediction = (LP_bits < 16) ? false : true; // 5-bit threshold is 16

    // Global history and choice
    choice_index = (path_history & path_mask);


    GP_bits = global_hist[choice_index];
    GP_bits = GP_bits & 0x1F; // 5-bit counter
    global_prediction = GP_bits >= 16; // 5-bit threshold is 16

    choice = choice_predt[choice_index];
    choice = choice & 0x1F; // 5-bit counter

    // Combine predictions using choice predictor
    prediction = (choice < 16) ? global_prediction : local_prediction;

    return prediction;
}

// Update the branch predictor
void PREDICTOR::update_predictor(const branch_record_c* br, const op_state_c* os, bool taken) {
    if (!br->is_conditional) {
        return;
    }

    // Update local history table
    local_hist[PC_bits] = ((local_hist[PC_bits] << 1) | (taken ? 1 : 0)) & LH_mask;

    if (taken) {
        if (local_predt[LH_bits] < 31) { // 5-bit counter max value is 31
            local_predt[LH_bits]++;
        }
    } else {
        if (local_predt[LH_bits] > 0) {
            local_predt[LH_bits]--;
        }
    }

    // Update global prediction table
    if (taken) {
        if (GP_bits < 31) { // 5-bit counter max value is 31
            global_hist[choice_index]++;
        }
    } else {
        if (GP_bits > 0) {
            global_hist[choice_index]--;
        }
    }

    // Update choice predictor
    if (choice < 16) {
        if (global_prediction != taken && local_prediction == taken) {
            choice++;
        }
    } else {
        if (global_prediction == taken && local_prediction != taken) {
            choice--;
        }
    }

    choice_predt[choice_index] = choice;

    // Update path history
    path_history = ((path_history << 1) | (taken ? 1 : 0)) & path_mask;
}
