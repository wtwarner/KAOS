// from https://github.com/GitJer/Button-debouncer/blob/main/button_debounce.cpp

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "button_debounce.h"
#include "button_debounce.pio.h"

void debounce_init(struct debounce_state_t *db, unsigned int gpio)
{
  // instantiate a pio, for now use pio0
  // TODO: if no sm in pio0 are available, try pio1
  db->pio = pio0;
  // claim a state machine
  db->sm = pio_claim_unused_sm(db->pio, false);
  // check if this is a valid sm
  if (db->sm == -1)
  {
    db->pio = pio1;
    db->sm = pio_claim_unused_sm(db->pio, false);
    printf("PIO1 sm %d\n", db->sm);
  }
  else {
    printf("PIO0 sm %d\n", db->sm);
  }
  db->valid = (db->sm != -1);
  if (!db->valid) {
    printf("PIO SM allocation invalid!\n");
    return;
  }

  // load the pio program into the pio memory
  db->offset = pio_add_program(db->pio, &button_debounce_program);
  // make a sm config
  pio_sm_config c = button_debounce_program_get_default_config(db->offset);
  // set the 'wait' pins
  sm_config_set_in_pins(&c, gpio); // for WAIT, IN
  // set the 'jmp' pins
  sm_config_set_jmp_pin(&c, gpio); // for JMP
  // set the clock divisor to set a reasonable debounce time
  // TODO: let the user set the debounce time in ms, calculate clock divisor for pio delay of 31.
  sm_config_set_clkdiv(&c, 11);
  // init the pio sm with the config
  pio_sm_init(db->pio, db->sm, db->offset, &c);
  // enable the sm
  pio_sm_set_enabled(db->pio, db->sm, true);
}

// return the value of the debounced gpio (0 is active)
unsigned int debounce_read(struct debounce_state_t *db)
{
  if (!db->valid) {
    return 1;
  }

  if (!pio_sm_is_rx_fifo_empty(db->pio, db->sm)) {
    uint32_t data = pio_sm_get(db->pio, db->sm); // Reads immediately without waiting
    return data;
  }

  return 1;
};

// if no sm was available the debounce does not work
unsigned int debunce_is_valid(struct debounce_state_t *db)
{
  return db->valid;
}
