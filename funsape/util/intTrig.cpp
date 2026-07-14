//!
//! \file           intTrig.cpp
//! \brief          Integer (Q15) sine/cosine for the FunSAPE AVR8 Library
//! \author         Franko
//! \date           2026-07-14
//! \version        24.07
//! \copyright      license
//! \details        See intTrig.hpp. Quarter-wave sine table (Q15) in PROGMEM plus
//!                     linear interpolation; input in millidegrees, output Q15.
//! \todo           Todo list
//!

// =============================================================================
// System file dependencies
// =============================================================================

#include "intTrig.hpp"
#if !defined(__INT_TRIG_HPP)
#   error "Header file is corrupted!"
#elif __INT_TRIG_HPP != 2604
#   error "Version mismatch between source and header files!"
#endif

#include <avr/pgmspace.h>

// =============================================================================
// File exclusive - Constants
// =============================================================================

//     Converts a first-quadrant angle in millidegrees [0..90000] into a table
//     position in Q8 over 128 intervals:  pos = (angle * SCALE) >> 16, with
//     SCALE = round(32768 / 90000 * 65536) = 23860. This avoids any division.
cuint16_t constIntTrigScale             = 23860;

cint32_t constIntTrigQuarterTurn        = 90000L;   //  90 degrees, in millidegrees
cint32_t constIntTrigHalfTurn           = 180000L;  // 180 degrees, in millidegrees
cint32_t constIntTrigFullTurn           = 360000L;  // 360 degrees, in millidegrees

//     Quarter-wave sine table, Q15 (value = sin(k * 90/128 degrees) * 32768).
//     129 entries (k = 0..128), 258 bytes in flash, 0 bytes in RAM.
static const int16_t constSinTable[129] PROGMEM = {
         0,    402,    804,   1206,   1608,   2009,   2410,   2811,
      3212,   3612,   4011,   4410,   4808,   5205,   5602,   5998,
      6393,   6786,   7179,   7571,   7962,   8351,   8739,   9126,
      9512,   9896,  10278,  10659,  11039,  11417,  11793,  12167,
     12539,  12910,  13279,  13645,  14010,  14372,  14732,  15090,
     15446,  15800,  16151,  16499,  16846,  17189,  17530,  17869,
     18204,  18537,  18868,  19195,  19519,  19841,  20159,  20475,
     20787,  21096,  21403,  21705,  22005,  22301,  22594,  22884,
     23170,  23452,  23731,  24007,  24279,  24547,  24811,  25072,
     25329,  25582,  25832,  26077,  26319,  26556,  26790,  27019,
     27245,  27466,  27683,  27896,  28105,  28310,  28510,  28706,
     28898,  29085,  29268,  29447,  29621,  29791,  29956,  30117,
     30273,  30424,  30571,  30714,  30852,  30985,  31113,  31237,
     31356,  31470,  31580,  31685,  31785,  31880,  31971,  32057,
     32137,  32213,  32285,  32351,  32412,  32469,  32521,  32567,
     32609,  32646,  32678,  32705,  32728,  32745,  32757,  32765,
     32767,
};

// =============================================================================
// Public functions definitions
// =============================================================================

int16_t sini(int32_t angle_p)
{
    // Local variables
    int32_t aux                         = angle_p % constIntTrigFullTurn;
    int8_t auxSign                      = 1;

    // Range reduction to [0, 360000)
    if(aux < 0) {
        aux += constIntTrigFullTurn;
    }
    // Sign from the half-circle (sin is negative in [180, 360) degrees)
    if(aux >= constIntTrigHalfTurn) {
        aux -= constIntTrigHalfTurn;
        auxSign = -1;
    }
    // Reflect into the first quadrant [0, 90000]  (sin(180 - x) = sin(x))
    if(aux > constIntTrigQuarterTurn) {
        aux = constIntTrigHalfTurn - aux;
    }

    // Table position in Q8 over 128 intervals (no division); auxPos <= 32766,
    // so auxIndex <= 127 and constSinTable[auxIndex + 1] never reads past the end
    uint16_t auxPos                     = (uint16_t)(((uint32_t)aux * constIntTrigScale) >> 16);
    uint8_t auxIndex                    = (uint8_t)(auxPos >> 8);
    int16_t auxValue;

    if(auxIndex >= 128) {
        // Defensive edge guard (not reached with the constants above)
        auxValue = 32767;
    } else {
        uint8_t auxFrac                 = (uint8_t)(auxPos & 0x00FF);
        int16_t auxBase                 = (int16_t)pgm_read_word(&constSinTable[auxIndex]);
        int16_t auxNext                 = (int16_t)pgm_read_word(&constSinTable[auxIndex + 1]);
        auxValue = auxBase + (int16_t)(((int32_t)(auxNext - auxBase) * auxFrac) >> 8);
    }

    return (auxSign > 0) ? auxValue : (int16_t)(-auxValue);
}

int16_t cosi(int32_t angle_p)
{
    // cos(x) = sin(x + 90 degrees)
    return sini(angle_p + constIntTrigQuarterTurn);
}

// =============================================================================
// END OF FILE - intTrig.cpp
// =============================================================================
