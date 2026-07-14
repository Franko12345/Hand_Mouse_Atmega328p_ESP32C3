//!
//! \file           intTrig.hpp
//! \brief          Integer (Q15) sine/cosine for the FunSAPE AVR8 Library
//! \author         Franko
//! \date           2026-07-14
//! \version        24.07
//! \copyright      license
//! \details        Fast integer sine/cosine for the ATmega328p (no FPU and no
//!                     hardware divide). The angle is given in MILLIDEGREES
//!                     (1000 = 1 degree, 360000 = full turn) as an int32_t, and
//!                     the result is Q15 fixed point (value = sin/cos * 32768,
//!                     range -32767..32767). It uses a quarter-wave sine table in
//!                     PROGMEM plus linear interpolation. Max error ~4 LSB
//!                     (~0.012%). No floating point is used.
//! \todo           Todo list
//!

// =============================================================================
// Include guard (START)
// =============================================================================

#ifndef __INT_TRIG_HPP
#define __INT_TRIG_HPP                      2604

// =============================================================================
// Dependencies
// =============================================================================

//     /////////////////    GLOBAL DEFINITIONS FILE     /////////////////     //

#include "../globalDefines.hpp"
#if !defined(__GLOBAL_DEFINES_HPP)
#   error "Global definitions file is corrupted!"
#elif __GLOBAL_DEFINES_HPP != __INT_TRIG_HPP
#   error "Version mismatch between file header and global definitions file!"
#endif

//     //////////////////     LIBRARY DEPENDENCIES     //////////////////     //

// NONE

//     ///////////////////     STANDARD C LIBRARY     ///////////////////     //

// NONE

//     ////////////////////    AVR LIBRARY FILES     ////////////////////     //

// NONE

// =============================================================================
// Undefining previous definitions
// =============================================================================

// NONE

// =============================================================================
// Constant definitions
// =============================================================================

// NONE

// =============================================================================
// New data types
// =============================================================================

// NONE

// =============================================================================
// Interrupt callback functions
// =============================================================================

// NONE

// =============================================================================
// Public functions declarations
// =============================================================================

//!
//! \brief      Integer sine (Q15).
//! \details    Computes the sine of an angle given in millidegrees using a
//!                 quarter-wave table and linear interpolation. Any angle is
//!                 accepted (it is reduced modulo a full turn, negatives
//!                 included).
//! \param[in]  angle_p     angle in millidegrees (1000 = 1 degree, 360000 = 360 degrees)
//! \return     int16_t     sine in Q15 fixed point (sin * 32768, -32767..32767)
//!
int16_t sini(
        int32_t angle_p
);

//!
//! \brief      Integer cosine (Q15).
//! \details    Computes the cosine of an angle given in millidegrees. Equivalent
//!                 to sini(angle_p + 90000).
//! \param[in]  angle_p     angle in millidegrees (1000 = 1 degree, 360000 = 360 degrees)
//! \return     int16_t     cosine in Q15 fixed point (cos * 32768, -32767..32767)
//!
int16_t cosi(
        int32_t angle_p
);

// =============================================================================
// Inlined functions definitions
// =============================================================================

// NONE

// =============================================================================
// External global variables
// =============================================================================

// NONE

// =============================================================================
// Include guard (END)
// =============================================================================

#endif  // __INT_TRIG_HPP

// =============================================================================
// END OF FILE - intTrig.hpp
// =============================================================================
