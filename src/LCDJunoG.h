/*
 * Copyright (c) 2022 Eddi De Pieri
 * Most code borrowed by Pico-DMX by Jostein Løwer 
 *
 * SPDX-License-Identifier: BSD-3-Clause
 * 
 * Description: 
 * Roland Juno G LCD Emulator
 */

#ifndef LCD_JUNOG_H
#define LCD_JUNOG_H

#if defined(ARDUINO_ARCH_MBED)
  #include <dma.h>
  #include <pio.h>
#else
  #ifdef ARDUINO
    #include <Arduino.h>
  #endif
  #include "hardware/dma.h"
  #include "hardware/pio.h"
#endif

class LCDJunoG
{
    uint _pin;

public:
    /*
    private properties that are declared public so the interrupt handler has access
    */
    volatile uint32_t *_buf;
    volatile PIO _pio;
    volatile uint _sm;
    volatile uint _dma_chan;
    volatile uint _cs;
    volatile bool _prgm_loaded = false;

    volatile unsigned long _last_packet_timestamp=0;
    void (*_cb)(LCDJunoG*);
    /*
        All different return codes for the DMX class. Only the SUCCESS
        Return code guarantees that the DMX output instance was properly configured
        and is ready to run
    */
    enum return_code
    {
        SUCCESS = 0,

        // There were no available state machines left in the
        // pio instance.
        ERR_NO_SM_AVAILABLE = -1,

        // There is not enough program memory left in the PIO to fit
        // The DMX PIO program
        ERR_INSUFFICIENT_PRGM_MEM = -2
    };

    /*
       Starts a new DMX input instance. 
       
       Param: pin
       Any valid GPIO pin on the Pico

       Param: pio
       defaults to pio0. pio0 can run up to 3
       DMX input instances. If you really need more, you can
       run 3 more on pio1  

       Param: cs
       The chip number (1 or 2).
    */

    return_code begin(uint pin, PIO pio = pio0, uint cs = 1);

    /*
        Read the selected channels from .begin(...) into a buffer.
        Method call blocks until the selected channels have been received

        Param: buffer
        A pointer to the location where the channels should be received 
        The buffer should have a max length of
        513 bytes (1 byte start code + 512 bytes frame). For ordinary
        DMX data frames, the start code should be 0x00.
    */
    void read(volatile uint32_t *buffer);

    /*
        Start async read process. This should only be called once.
        From then on, the buffer will always contain the latest DMX data.
        If you want to be notified whenever a new DMX frame has been received,
        provide a callback function that will be called without arguments.
    */
    void read_async(volatile uint32_t *buffer, void (*inputUpdatedCallback)(LCDJunoG* instance) = nullptr);

    int get_capture_index();

    /*
        Get the timestamp (like millis()) from the moment the latest dmx packet was received.
        May be used to detect if the dmx signal has stopped coming in.
    */
    unsigned long latest_packet_timestamp();

    /*
        Get the pin this instance is listening on
    */
    uint pin();


    /*
        De-inits the DMX input instance. Releases PIO resources. 
        The instance can safely be destroyed after this method is called
    */
    void end();
};

#endif
