// from https://github.com/GitJer/Button-debouncer/blob/main/button_debounce.cpp
#pragma once

#include "hardware/pio.h"

struct debounce_state_t {
    // the pio instance 
    PIO pio;
    // the state machine
    unsigned int sm;
    // the location of the pio program in the memory
    unsigned int  offset;
    // indicator if the sm is valid
    unsigned int  valid;
};
#ifdef __cplusplus
extern "C" {
#endif
void debounce_init(struct debounce_state_t *db, unsigned int gpio);
unsigned int  debounce_read(struct debounce_state_t *db);
unsigned int  debunce_is_valid(struct debounce_state_t *db);
#ifdef __cplusplus
};
#endif