#pragma once

enum Protocol {
    UNKNOWN_PROTOCOL    = 0,    
    MIDI_PROTOCOL       = 1,    // MIDI OUT
    CSV_PROTOCOL        = 2,    // CONSOLE OUT
    UDP_PROTOCOL        = 3,    // NETWORK
    OSC_PROTOCOL        = 4     // NETWORK
};