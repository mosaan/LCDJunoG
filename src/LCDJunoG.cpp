/*
 * Copyright (c) 2021 Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "LCDJunoG.h"
#include "LCDJunoG.pio.h"
#include "constants.h"

#if defined(ARDUINO_ARCH_MBED)
  #include <clocks.h>
  #include <irq.h>
  #include <Arduino.h> // REMOVE ME
#else
  #include "pico/time.h"
  #include "hardware/clocks.h"
  #include "hardware/irq.h"
#endif

/*
This array tells the interrupt handler which instance has interrupted.
The interrupt handler has only the ints0 register to go on, so this array needs as many spots as there are DMA channels. 
*/
#define NUM_DMA_CHANS 12
volatile LCDJunoG *active_inputs[NUM_DMA_CHANS] = {nullptr};

LCDJunoG::return_code LCDJunoG::begin(uint pin, PIO pio, uint cs)
{
    if(!_prgm_loaded) {
        /* 
        Attempt to load the PIO assembly program into the PIO program memory
        */

        if (cs == 1) {
            if (!pio_can_add_program(pio, &LCDJunoG_cs1_program))
            {
                return ERR_INSUFFICIENT_PRGM_MEM;
            }
            _prgm_offset = pio_add_program(pio, &LCDJunoG_cs1_program);

        } else if (cs == 2) {
            if (!pio_can_add_program(pio, &LCDJunoG_cs2_program))
            {
                return ERR_INSUFFICIENT_PRGM_MEM;
            }
            _prgm_offset = pio_add_program(pio, &LCDJunoG_cs2_program);
        }
        _prgm_loaded = true;
    }

    /* 
    Attempt to claim an unused State Machine into the PIO program memory
    */

    int sm = pio_claim_unused_sm(pio, false);
    if (sm == -1)
    {
        return ERR_NO_SM_AVAILABLE;
    }

    // Set this pin's GPIO function (connect PIO to the pad)
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 8, false);
    pio_sm_set_pindirs_with_mask(pio, sm, 0, 1 << RS_PIN_RELATIVE_POSITION );
    {
        uint32_t pin_offset;
        // init Data Pins
        for ( pin_offset = 0; pin_offset < 8; pin_offset++ )
        {
            pio_gpio_init( pio, pin + pin_offset );
        }
        // init RS Pin
        pio_gpio_init( pio, JUNO_RS);
    }

    //eddi- gpio_pull_up(pin);

    // Generate the default PIO state machine config provided by pioasm
    pio_sm_config sm_conf;
    if (cs == 1) 
        sm_conf = LCDJunoG_cs1_program_get_default_config(_prgm_offset);
    else if (cs == 2) 
        sm_conf = LCDJunoG_cs2_program_get_default_config(_prgm_offset);
    sm_config_set_in_pins(&sm_conf, pin); // for WAIT, IN
    sm_config_set_jmp_pin(&sm_conf, pin); // for JMP

    // Setup the side-set pins for the PIO state machine
    // Shift to right, autopush disabled
    sm_config_set_in_shift(&sm_conf, true, false, 32);
    // Deeper FIFO as we're not doing any TX
    sm_config_set_fifo_join(&sm_conf, PIO_FIFO_JOIN_RX);

    // Load our configuration, jump to the start of the program and run the State Machine
    pio_sm_init(pio, sm, _prgm_offset, &sm_conf);

    _pio = pio;
    _sm = sm;
    _cs = cs;
    _pin = pin;
    _buf = nullptr;
    _cb = nullptr;

    _dma_chan = dma_claim_unused_channel(true);

    if (active_inputs[_dma_chan] != nullptr) {
        return ERR_NO_SM_AVAILABLE;
    }
    active_inputs[_dma_chan] = this;

    return SUCCESS;
}

void LCDJunoG::read(volatile uint32_t *buffer)
{
    if(_buf==nullptr) {
        read_async(buffer);
    }
    unsigned long start = _last_packet_timestamp;
    while(_last_packet_timestamp == start) {
        tight_loop_contents();
    }
}

void lcdjunog_dma_handler() {
    for(int i=0;i<NUM_DMA_CHANS;i++) {
        if(active_inputs[i]!=nullptr && (dma_hw->ints0 & (1u<<i))) {
            dma_hw->ints0 = 1u << i;
            volatile LCDJunoG *instance = active_inputs[i];

            dma_channel_set_write_addr(i, instance->_buf, true);
            pio_sm_exec(instance->_pio, instance->_sm, pio_encode_jmp(instance->_prgm_offset));
            pio_sm_clear_fifos(instance->_pio, instance->_sm);

#ifdef ARDUINO
            instance->_last_packet_timestamp = millis();
#else
            instance->_last_packet_timestamp = to_ms_since_boot(get_absolute_time());
#endif
            // Trigger the callback if we have one
            if (instance->_cb != nullptr) {
                (*(instance->_cb))((LCDJunoG*)instance);
            }
        }
    }
}

void LCDJunoG::read_async(volatile uint32_t *buffer, void (*inputUpdatedCallback)(LCDJunoG*)) {

    _buf = buffer;
    if (inputUpdatedCallback!=nullptr) {
        _cb = inputUpdatedCallback;
    }

    pio_sm_set_enabled(_pio, _sm, false);

    // Reset the PIO state machine to a consistent state. Clear the buffers and registers
    pio_sm_restart(_pio, _sm);

    //setup dma
    dma_channel_config cfg = dma_channel_get_default_config(_dma_chan);

    // Reading from constant address, writing to incrementing byte addresses
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);

    // Pace transfers based on DREQ_PIO0_RX0 (or whichever pio and sm we are using)
    channel_config_set_dreq(&cfg, pio_get_dreq(_pio, _sm, false));

    //channel_config_set_ring(&cfg, true, 5);
    dma_channel_configure(
        _dma_chan, 
        &cfg,
        NULL,    // dst
        &_pio->rxf[_sm],  // src. Direct read access to the RX FIFO for this state machine
        12 * 123,
        false
    );

    dma_channel_set_irq0_enabled(_dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, lcdjunog_dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);

    //aaand start!
    dma_channel_set_write_addr(_dma_chan, buffer, true);
    pio_sm_exec(_pio, _sm, pio_encode_jmp(_prgm_offset));
    pio_sm_clear_fifos(_pio, _sm);
#ifdef ARDUINO
    _last_packet_timestamp = millis();
#else
    _last_packet_timestamp = to_ms_since_boot(get_absolute_time());
#endif

    pio_sm_set_enabled(_pio, _sm, true);
}

unsigned long LCDJunoG::latest_packet_timestamp() {
    return _last_packet_timestamp;
}

uint LCDJunoG::pin() {
    return _pin;
}

void LCDJunoG::end()
{
    // Stop the PIO state machine
    pio_sm_set_enabled(_pio, _sm, false);

    // Remove the PIO program from the PIO program memory
    uint pio_id = pio_get_index(_pio);
    bool inuse = false;
    for(uint i=0;i<NUM_DMA_CHANS;i++) {
        if(i==_dma_chan) {
            continue;
        }
        if(pio_id == pio_get_index(active_inputs[i]->_pio)) {
            inuse = true;
            break;
        }
    }
    if(!inuse) {
        _prgm_loaded = false;
        if (_cs == 1)
            pio_remove_program(_pio, &LCDJunoG_cs1_program, _prgm_offset);
        else if (_cs == 2)
            pio_remove_program(_pio, &LCDJunoG_cs2_program, _prgm_offset);

        _prgm_offset=0;
    }

    // Unclaim the sm
    pio_sm_unclaim(_pio, _sm);

    dma_channel_unclaim(_dma_chan);
    active_inputs[_dma_chan] = nullptr;

    _buf = nullptr;
}
