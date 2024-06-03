#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ------------ //
// LCDJunoG_cs1 //
// ------------ //

#define LCDJunoG_cs1_wrap_target 0

static const uint16_t LCDJunoG_cs1_program_instructions[] = {
    //     .wrap_target
    (uint16_t)pio_encode_wait_gpio(0, JUNO_CS1),
    (uint16_t)pio_encode_wait_gpio(1, JUNO_CS1),
    (uint16_t)pio_encode_in(pio_pins, 32), // read all 32 pins.
    (uint16_t)pio_encode_push(false, true),
    //     .wrap
};

const uint8_t LCDJunoG_cs1_program_length = sizeof(LCDJunoG_cs1_program_instructions) / sizeof(uint16_t);
const uint8_t LCDJunoG_cs1_wrap = LCDJunoG_cs1_program_length - 1; // wrap is the last instruction.

#if !PICO_NO_HARDWARE
static const struct pio_program LCDJunoG_cs1_program = {
    .instructions = LCDJunoG_cs1_program_instructions,
    .length = LCDJunoG_cs1_program_length,
    .origin = -1,
};

static inline pio_sm_config LCDJunoG_cs1_program_get_default_config(uint offset)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + LCDJunoG_cs1_wrap_target, offset + LCDJunoG_cs1_wrap);
    return c;
}
#endif

// ------------ //
// LCDJunoG_cs2 //
// ------------ //

#define LCDJunoG_cs2_wrap_target 0

static const uint16_t LCDJunoG_cs2_program_instructions[] = {
    //     .wrap_target
    (uint16_t)pio_encode_wait_gpio(0, JUNO_CS2),
    (uint16_t)pio_encode_wait_gpio(1, JUNO_CS2),
    (uint16_t)pio_encode_in(pio_pins, 32), // read all 32 pins.
    (uint16_t)pio_encode_push(false, true),
    //     .wrap
};

const uint8_t LCDJunoG_cs2_program_length = sizeof(LCDJunoG_cs2_program_instructions) / sizeof(uint16_t);
const uint8_t LCDJunoG_cs2_wrap = LCDJunoG_cs2_program_length - 1; // wrap is the last instruction.

#if !PICO_NO_HARDWARE
static const struct pio_program LCDJunoG_cs2_program = {
    .instructions = LCDJunoG_cs2_program_instructions,
    .length = LCDJunoG_cs2_program_length,
    .origin = -1,
};

static inline pio_sm_config LCDJunoG_cs2_program_get_default_config(uint offset)
{
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + LCDJunoG_cs2_wrap_target, offset + LCDJunoG_cs2_wrap);
    return c;
}
#endif
