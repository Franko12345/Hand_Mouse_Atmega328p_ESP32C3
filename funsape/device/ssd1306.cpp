//!
//! \file           ssd1306.cpp
//! \brief          SSD1306 OLED (I2C) module interface for the FunSAPE AVR8 Library
//! \author         Franko
//! \date           2026-07-13
//! \version        24.07
//! \copyright      license
//! \details        Text-oriented driver for the SSD1306 128x64 / 128x32 OLED
//!                     display over I2C (TWI), using direct GDDRAM writes (no
//!                     framebuffer). See header for feature limitations.
//! \todo           Todo list
//!

// =============================================================================
// System file dependencies
// =============================================================================

#include "ssd1306.hpp"
#if !defined(__SSD1306_HPP)
#   error "Header file is corrupted!"
#elif __SSD1306_HPP != 2604
#   error "Version mismatch between source and header files!"
#endif

#include <stdio.h>
#include <avr/pgmspace.h>

// =============================================================================
// File exclusive - Constants
// =============================================================================

//     ////////////////////     I2C CONTROL BYTES     ///////////////////     //

#define SSD1306_CONTROL_COMMAND         0x00
#define SSD1306_CONTROL_DATA            0x40

//     //////////////////////     COMMAND SET     //////////////////////     //

#define SSD1306_SET_CONTRAST            0x81
#define SSD1306_DISPLAY_ALL_ON_RESUME   0xA4
#define SSD1306_NORMAL_DISPLAY          0xA6
#define SSD1306_INVERT_DISPLAY          0xA7
#define SSD1306_DISPLAY_OFF             0xAE
#define SSD1306_DISPLAY_ON              0xAF
#define SSD1306_SET_DISPLAY_OFFSET      0xD3
#define SSD1306_SET_COM_PINS            0xDA
#define SSD1306_SET_VCOM_DETECT         0xDB
#define SSD1306_SET_DISPLAY_CLOCK_DIV   0xD5
#define SSD1306_SET_PRECHARGE           0xD9
#define SSD1306_SET_MULTIPLEX           0xA8
#define SSD1306_SET_LOW_COLUMN          0x00
#define SSD1306_SET_HIGH_COLUMN         0x10
#define SSD1306_SET_START_LINE          0x40
#define SSD1306_MEMORY_MODE             0x20
#define SSD1306_SET_PAGE_ADDRESS        0xB0
#define SSD1306_COM_SCAN_INC            0xC0
#define SSD1306_COM_SCAN_DEC            0xC8
#define SSD1306_SEG_REMAP               0xA0
#define SSD1306_CHARGE_PUMP             0x8D

//     ////////////////////     DRIVER CONSTANTS     ///////////////////     //

cuint8_t constSsd1306Width              = 128;      // display width in pixels
cuint8_t constSsd1306CellWidth          = 6;        // 5 glyph columns + 1 spacing
cuint8_t constSsd1306FontFirst          = 0x20;     // first supported ASCII char
cuint8_t constSsd1306FontLast           = 0x7E;     // last supported ASCII char
cuint8_t constSsd1306DataChunk          = 16;       // max data bytes per TWI write
cuint8_t constSsd1306TextSizeMax        = 4;        // max text scale factor

// =============================================================================
// File exclusive - New data types
// =============================================================================

// NONE

// =============================================================================
// Static function declarations
// =============================================================================

static int ssd1306WriteStd(char character_p, FILE *stream_p);

// =============================================================================
// File exclusive - Global variables
// =============================================================================

//     ////////////////     POWER-ON COMMAND SEQUENCE     ///////////////     //
//     Size-dependent commands (multiplex and COM pins) are sent separately.

static const uint8_t constSsd1306InitSequence[] PROGMEM = {
    SSD1306_DISPLAY_OFF,
    SSD1306_SET_DISPLAY_CLOCK_DIV,      0x80,
    SSD1306_SET_DISPLAY_OFFSET,         0x00,
    SSD1306_SET_START_LINE |            0x00,
    SSD1306_CHARGE_PUMP,                0x14,
    SSD1306_MEMORY_MODE,                0x02,       // page addressing mode
    SSD1306_SEG_REMAP |                 0x01,
    SSD1306_COM_SCAN_DEC,
    SSD1306_SET_PRECHARGE,              0xF1,
    SSD1306_SET_VCOM_DETECT,            0x40,
    SSD1306_DISPLAY_ALL_ON_RESUME,
    SSD1306_NORMAL_DISPLAY,
};

//     /////////////////////     5x7 ASCII FONT     ////////////////////     //
//     Column-major, 5 columns per glyph, bit 0 = top row. Covers 0x20-0x7E.

static const uint8_t constSsd1306Font[][5] PROGMEM = {
    {0x00, 0x00, 0x00, 0x00, 0x00},     // 0x20 ' '
    {0x00, 0x00, 0x5F, 0x00, 0x00},     // 0x21 '!'
    {0x00, 0x07, 0x00, 0x07, 0x00},     // 0x22 '"'
    {0x14, 0x7F, 0x14, 0x7F, 0x14},     // 0x23 '#'
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},     // 0x24 '$'
    {0x23, 0x13, 0x08, 0x64, 0x62},     // 0x25 '%'
    {0x36, 0x49, 0x55, 0x22, 0x50},     // 0x26 '&'
    {0x00, 0x05, 0x03, 0x00, 0x00},     // 0x27 '''
    {0x00, 0x1C, 0x22, 0x41, 0x00},     // 0x28 '('
    {0x00, 0x41, 0x22, 0x1C, 0x00},     // 0x29 ')'
    {0x14, 0x08, 0x3E, 0x08, 0x14},     // 0x2A '*'
    {0x08, 0x08, 0x3E, 0x08, 0x08},     // 0x2B '+'
    {0x00, 0x50, 0x30, 0x00, 0x00},     // 0x2C ','
    {0x08, 0x08, 0x08, 0x08, 0x08},     // 0x2D '-'
    {0x00, 0x60, 0x60, 0x00, 0x00},     // 0x2E '.'
    {0x20, 0x10, 0x08, 0x04, 0x02},     // 0x2F '/'
    {0x3E, 0x51, 0x49, 0x45, 0x3E},     // 0x30 '0'
    {0x00, 0x42, 0x7F, 0x40, 0x00},     // 0x31 '1'
    {0x42, 0x61, 0x51, 0x49, 0x46},     // 0x32 '2'
    {0x21, 0x41, 0x45, 0x4B, 0x31},     // 0x33 '3'
    {0x18, 0x14, 0x12, 0x7F, 0x10},     // 0x34 '4'
    {0x27, 0x45, 0x45, 0x45, 0x39},     // 0x35 '5'
    {0x3C, 0x4A, 0x49, 0x49, 0x30},     // 0x36 '6'
    {0x01, 0x71, 0x09, 0x05, 0x03},     // 0x37 '7'
    {0x36, 0x49, 0x49, 0x49, 0x36},     // 0x38 '8'
    {0x06, 0x49, 0x49, 0x29, 0x1E},     // 0x39 '9'
    {0x00, 0x36, 0x36, 0x00, 0x00},     // 0x3A ':'
    {0x00, 0x56, 0x36, 0x00, 0x00},     // 0x3B ';'
    {0x08, 0x14, 0x22, 0x41, 0x00},     // 0x3C '<'
    {0x14, 0x14, 0x14, 0x14, 0x14},     // 0x3D '='
    {0x00, 0x41, 0x22, 0x14, 0x08},     // 0x3E '>'
    {0x02, 0x01, 0x51, 0x09, 0x06},     // 0x3F '?'
    {0x32, 0x49, 0x79, 0x41, 0x3E},     // 0x40 '@'
    {0x7E, 0x11, 0x11, 0x11, 0x7E},     // 0x41 'A'
    {0x7F, 0x49, 0x49, 0x49, 0x36},     // 0x42 'B'
    {0x3E, 0x41, 0x41, 0x41, 0x22},     // 0x43 'C'
    {0x7F, 0x41, 0x41, 0x22, 0x1C},     // 0x44 'D'
    {0x7F, 0x49, 0x49, 0x49, 0x41},     // 0x45 'E'
    {0x7F, 0x09, 0x09, 0x09, 0x01},     // 0x46 'F'
    {0x3E, 0x41, 0x49, 0x49, 0x7A},     // 0x47 'G'
    {0x7F, 0x08, 0x08, 0x08, 0x7F},     // 0x48 'H'
    {0x00, 0x41, 0x7F, 0x41, 0x00},     // 0x49 'I'
    {0x20, 0x40, 0x41, 0x3F, 0x01},     // 0x4A 'J'
    {0x7F, 0x08, 0x14, 0x22, 0x41},     // 0x4B 'K'
    {0x7F, 0x40, 0x40, 0x40, 0x40},     // 0x4C 'L'
    {0x7F, 0x02, 0x0C, 0x02, 0x7F},     // 0x4D 'M'
    {0x7F, 0x04, 0x08, 0x10, 0x7F},     // 0x4E 'N'
    {0x3E, 0x41, 0x41, 0x41, 0x3E},     // 0x4F 'O'
    {0x7F, 0x09, 0x09, 0x09, 0x06},     // 0x50 'P'
    {0x3E, 0x41, 0x51, 0x21, 0x5E},     // 0x51 'Q'
    {0x7F, 0x09, 0x19, 0x29, 0x46},     // 0x52 'R'
    {0x46, 0x49, 0x49, 0x49, 0x31},     // 0x53 'S'
    {0x01, 0x01, 0x7F, 0x01, 0x01},     // 0x54 'T'
    {0x3F, 0x40, 0x40, 0x40, 0x3F},     // 0x55 'U'
    {0x1F, 0x20, 0x40, 0x20, 0x1F},     // 0x56 'V'
    {0x3F, 0x40, 0x38, 0x40, 0x3F},     // 0x57 'W'
    {0x63, 0x14, 0x08, 0x14, 0x63},     // 0x58 'X'
    {0x07, 0x08, 0x70, 0x08, 0x07},     // 0x59 'Y'
    {0x61, 0x51, 0x49, 0x45, 0x43},     // 0x5A 'Z'
    {0x00, 0x7F, 0x41, 0x41, 0x00},     // 0x5B '['
    {0x02, 0x04, 0x08, 0x10, 0x20},     // 0x5C '\'
    {0x00, 0x41, 0x41, 0x7F, 0x00},     // 0x5D ']'
    {0x04, 0x02, 0x01, 0x02, 0x04},     // 0x5E '^'
    {0x40, 0x40, 0x40, 0x40, 0x40},     // 0x5F '_'
    {0x00, 0x01, 0x02, 0x04, 0x00},     // 0x60 '`'
    {0x20, 0x54, 0x54, 0x54, 0x78},     // 0x61 'a'
    {0x7F, 0x48, 0x44, 0x44, 0x38},     // 0x62 'b'
    {0x38, 0x44, 0x44, 0x44, 0x20},     // 0x63 'c'
    {0x38, 0x44, 0x44, 0x48, 0x7F},     // 0x64 'd'
    {0x38, 0x54, 0x54, 0x54, 0x18},     // 0x65 'e'
    {0x08, 0x7E, 0x09, 0x01, 0x02},     // 0x66 'f'
    {0x0C, 0x52, 0x52, 0x52, 0x3E},     // 0x67 'g'
    {0x7F, 0x08, 0x04, 0x04, 0x78},     // 0x68 'h'
    {0x00, 0x44, 0x7D, 0x40, 0x00},     // 0x69 'i'
    {0x20, 0x40, 0x44, 0x3D, 0x00},     // 0x6A 'j'
    {0x7F, 0x10, 0x28, 0x44, 0x00},     // 0x6B 'k'
    {0x00, 0x41, 0x7F, 0x40, 0x00},     // 0x6C 'l'
    {0x7C, 0x04, 0x18, 0x04, 0x78},     // 0x6D 'm'
    {0x7C, 0x08, 0x04, 0x04, 0x78},     // 0x6E 'n'
    {0x38, 0x44, 0x44, 0x44, 0x38},     // 0x6F 'o'
    {0x7C, 0x14, 0x14, 0x14, 0x08},     // 0x70 'p'
    {0x08, 0x14, 0x14, 0x18, 0x7C},     // 0x71 'q'
    {0x7C, 0x08, 0x04, 0x04, 0x08},     // 0x72 'r'
    {0x48, 0x54, 0x54, 0x54, 0x20},     // 0x73 's'
    {0x04, 0x3F, 0x44, 0x40, 0x20},     // 0x74 't'
    {0x3C, 0x40, 0x40, 0x20, 0x7C},     // 0x75 'u'
    {0x1C, 0x20, 0x40, 0x20, 0x1C},     // 0x76 'v'
    {0x3C, 0x40, 0x30, 0x40, 0x3C},     // 0x77 'w'
    {0x44, 0x28, 0x10, 0x28, 0x44},     // 0x78 'x'
    {0x0C, 0x50, 0x50, 0x50, 0x3C},     // 0x79 'y'
    {0x44, 0x64, 0x54, 0x4C, 0x44},     // 0x7A 'z'
    {0x00, 0x08, 0x36, 0x41, 0x00},     // 0x7B '{'
    {0x00, 0x00, 0x7F, 0x00, 0x00},     // 0x7C '|'
    {0x00, 0x41, 0x36, 0x08, 0x00},     // 0x7D '}'
    {0x08, 0x04, 0x08, 0x10, 0x08},     // 0x7E '~'
};

FILE ssd1306Stream                      = {0};
Ssd1306 *defaultSsd1306Display          = nullptr;

// =============================================================================
// File exclusive - Macro-functions
// =============================================================================

// NONE

// =============================================================================
// Class constructors
// =============================================================================

Ssd1306::Ssd1306(void)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::Ssd1306(void)"), Debug::CodeIndex::SSD1306_MODULE);

    fdev_setup_stream(&ssd1306Stream, ssd1306WriteStd, nullptr, _FDEV_SETUP_WRITE);

    // Reset data members
    this->_busHandler                   = nullptr;
    this->_busHandlerSet                = false;
    this->_initialized                  = false;
    this->_deviceAddress                = 0x3C;
    this->_size                         = Size::SSD1306_128X64;
    this->_pages                        = 8;
    this->_cursorX                      = 0;
    this->_cursorPage                   = 0;
    this->_textSize                     = 1;

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return;
}

Ssd1306::~Ssd1306(void)
{
    // Returns successfully
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return;
}

// =============================================================================
// Class public methods
// =============================================================================

//     ///////////////////     CONTROL AND STATUS     ///////////////////     //

bool_t Ssd1306::init(Bus *busHandler_p, const Size size_p, cuint8_t address_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::init(Bus *, const Size, cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Updates data members
    this->_busHandlerSet                = false;
    this->_initialized                  = false;

    // Check function arguments for errors
    if(!isPointerValid(busHandler_p)) {
        // Returns error
        this->_lastError = Error::BUS_HANDLER_POINTER_NULL;
        debugMessage(Error::BUS_HANDLER_POINTER_NULL, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    } else if(busHandler_p->getBusType() != Bus::BusType::TWI) {
        // Returns error
        this->_lastError = Error::BUS_HANDLER_NOT_SUPPORTED;
        debugMessage(Error::BUS_HANDLER_NOT_SUPPORTED, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Updates data members
    this->_busHandler                   = busHandler_p;
    this->_busHandlerSet                = true;
    this->_deviceAddress                = address_p;
    this->_size                         = size_p;
    this->_pages                        = (size_p == Size::SSD1306_128X64) ? 8 : 4;
    this->_cursorX                      = 0;
    this->_cursorPage                   = 0;
    this->_textSize                     = 1;

    // Local variables
    uint8_t auxMultiplex                = (size_p == Size::SSD1306_128X64) ? 0x3F : 0x1F;
    uint8_t auxComPins                  = (size_p == Size::SSD1306_128X64) ? 0x12 : 0x02;

    // Sends power-on command sequence
    if(!this->_writeCommandList(constSsd1306InitSequence, sizeof(constSsd1306InitSequence))) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if(!this->_writeCommand(SSD1306_SET_MULTIPLEX) || !this->_writeCommand(auxMultiplex)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if(!this->_writeCommand(SSD1306_SET_COM_PINS) || !this->_writeCommand(auxComPins)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if(!this->_writeCommand(SSD1306_SET_CONTRAST) || !this->_writeCommand(0xCF)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if(!this->_writeCommand(SSD1306_DISPLAY_ON)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Marks device as initialized and clears the display
    this->_initialized                  = true;
    if(!this->clearScreen()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::clearScreen(void)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::clearScreen(void)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables
    uint8_t auxBuffer[constSsd1306DataChunk];

    // Prepares a blank chunk
    for(uint8_t i = 0; i < constSsd1306DataChunk; i++) {
        auxBuffer[i] = 0x00;
    }

    // Writes zeros to every page
    for(uint8_t page = 0; page < this->_pages; page++) {
        if(!this->_setAddress(page, 0)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        uint8_t remaining = constSsd1306Width;
        while(remaining > 0) {
            uint8_t chunk = (remaining > constSsd1306DataChunk) ? constSsd1306DataChunk : remaining;
            if(!this->_writeData(auxBuffer, chunk)) {
                // Returns error
                debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
                return false;
            }
            remaining -= chunk;
        }
    }

    // Homes the text cursor
    this->_cursorX                      = 0;
    this->_cursorPage                   = 0;

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::setDisplayState(const Activation state_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::setDisplayState(const Activation)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Sends command
    if(!this->_writeCommand((state_p == Activation::ON) ? SSD1306_DISPLAY_ON : SSD1306_DISPLAY_OFF)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::setContrast(cuint8_t contrast_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::setContrast(cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Sends command
    if(!this->_writeCommand(SSD1306_SET_CONTRAST) || !this->_writeCommand(contrast_p)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::setUpsideDown(cbool_t enable_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::setUpsideDown(cbool_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables
    uint8_t auxSegRemap                 = (enable_p) ? SSD1306_SEG_REMAP : (SSD1306_SEG_REMAP | 0x01);
    uint8_t auxComScan                  = (enable_p) ? SSD1306_COM_SCAN_INC : SSD1306_COM_SCAN_DEC;

    // Sends the orientation commands
    if(!this->_writeCommand(auxSegRemap) || !this->_writeCommand(auxComScan)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

Error Ssd1306::getLastError(void)
{
    // Returns last error
    return this->_lastError;
}

//     //////////////////////////     TEXT     /////////////////////////     //

bool_t Ssd1306::setTextSize(cuint8_t size_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::setTextSize(cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check function arguments for errors
    if((size_p == 0) || (size_p > constSsd1306TextSizeMax)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Updates data members
    this->_textSize                     = size_p;

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::setCursor(cuint8_t x_p, cuint8_t page_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::setCursor(cuint8_t, cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if((x_p >= constSsd1306Width) || (page_p >= this->_pages)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Updates data members
    this->_cursorX                      = x_p;
    this->_cursorPage                   = page_p;

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::stdio(void)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::stdio(void)"), Debug::CodeIndex::SSD1306_MODULE);

    stdin = stdout = stderr             = &ssd1306Stream;
    defaultSsd1306Display               = this;

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::print(cchar_t character_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::print(cchar_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    if(character_p == '\n') {
        this->_cursorX                  = 0;
        this->_cursorPage               += this->_textSize;
        if((this->_cursorPage + this->_textSize) > this->_pages) {
            this->_cursorPage           = 0;
        }
    } else {
        if(!this->_writeChar(character_p)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::print(cchar_t *string_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::print(cchar_t *)"), Debug::CodeIndex::SSD1306_MODULE);

    // Local variables
    uint16_t i                          = 0;

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if(!isPointerValid(string_p)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_POINTER_NULL;
        debugMessage(Error::ARGUMENT_POINTER_NULL, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    while(string_p[i]) {
        if(!this->print(string_p[i])) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        i++;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

//     ////////////////////////     GRAPHICS     ///////////////////////     //

bool_t Ssd1306::fillRect(cuint8_t x_p, cuint8_t page_p, cuint8_t width_p, cuint8_t heightPages_p, cbool_t on_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::fillRect(cuint8_t, cuint8_t, cuint8_t, cuint8_t, cbool_t)"),
            Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if((x_p >= constSsd1306Width) || (page_p >= this->_pages) || (width_p == 0) || (heightPages_p == 0)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables (clamps region to the display bounds)
    uint8_t auxBuffer[constSsd1306DataChunk];
    uint8_t auxValue                    = (on_p) ? 0xFF : 0x00;
    uint8_t auxWidth                    = width_p;
    uint8_t auxLastPage                 = page_p + heightPages_p;

    if((x_p + auxWidth) > constSsd1306Width) {
        auxWidth = constSsd1306Width - x_p;
    }
    if(auxLastPage > this->_pages) {
        auxLastPage = this->_pages;
    }
    for(uint8_t i = 0; i < constSsd1306DataChunk; i++) {
        auxBuffer[i] = auxValue;
    }

    // Writes the block page by page
    for(uint8_t page = page_p; page < auxLastPage; page++) {
        if(!this->_setAddress(page, x_p)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        uint8_t remaining = auxWidth;
        while(remaining > 0) {
            uint8_t chunk = (remaining > constSsd1306DataChunk) ? constSsd1306DataChunk : remaining;
            if(!this->_writeData(auxBuffer, chunk)) {
                // Returns error
                debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
                return false;
            }
            remaining -= chunk;
        }
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::drawRect(cuint8_t x_p, cuint8_t page_p, cuint8_t width_p, cuint8_t heightPages_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::drawRect(cuint8_t, cuint8_t, cuint8_t, cuint8_t)"),
            Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if((x_p >= constSsd1306Width) || (page_p >= this->_pages) || (width_p < 2) || (heightPages_p == 0)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables (clamps region to the display bounds)
    uint8_t auxBuffer[constSsd1306DataChunk];
    uint8_t auxWidth                    = width_p;
    uint8_t auxLastPage                 = page_p + heightPages_p;

    if((x_p + auxWidth) > constSsd1306Width) {
        auxWidth = constSsd1306Width - x_p;
    }
    if(auxLastPage > this->_pages) {
        auxLastPage = this->_pages;
    }

    // Writes the outline page by page (whole page-columns, so corners are correct)
    for(uint8_t page = page_p; page < auxLastPage; page++) {
        bool_t auxIsTopPage             = (page == page_p);
        bool_t auxIsBottomPage          = (page == (uint8_t)(auxLastPage - 1));
        uint8_t auxIndex                = 0;

        if(!this->_setAddress(page, x_p)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        for(uint8_t i = 0; i < auxWidth; i++) {
            uint8_t auxByte             = 0x00;
            if((i == 0) || (i == (uint8_t)(auxWidth - 1))) {
                auxByte                 = 0xFF;                 // left / right side
            } else {
                if(auxIsTopPage) {
                    auxByte             |= 0x01;                // top edge
                }
                if(auxIsBottomPage) {
                    auxByte             |= 0x80;                // bottom edge
                }
            }
            auxBuffer[auxIndex++]       = auxByte;
            if(auxIndex == constSsd1306DataChunk) {
                if(!this->_writeData(auxBuffer, auxIndex)) {
                    // Returns error
                    debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
                    return false;
                }
                auxIndex                = 0;
            }
        }
        if(auxIndex > 0) {
            if(!this->_writeData(auxBuffer, auxIndex)) {
                // Returns error
                debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
                return false;
            }
        }
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::drawHLine(cuint8_t x_p, cuint8_t y_p, cuint8_t width_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::drawHLine(cuint8_t, cuint8_t, cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if((x_p >= constSsd1306Width) || (y_p >= (uint8_t)(this->_pages * 8)) || (width_p == 0)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables
    uint8_t auxBuffer[constSsd1306DataChunk];
    uint8_t auxMask                     = (1 << (y_p % 8));
    uint8_t auxWidth                    = width_p;

    if((x_p + auxWidth) > constSsd1306Width) {
        auxWidth = constSsd1306Width - x_p;
    }
    for(uint8_t i = 0; i < constSsd1306DataChunk; i++) {
        auxBuffer[i] = auxMask;
    }

    // Writes the line
    if(!this->_setAddress(y_p / 8, x_p)) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    uint8_t remaining                   = auxWidth;
    while(remaining > 0) {
        uint8_t chunk = (remaining > constSsd1306DataChunk) ? constSsd1306DataChunk : remaining;
        if(!this->_writeData(auxBuffer, chunk)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        remaining -= chunk;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

bool_t Ssd1306::drawVLine(cuint8_t x_p, cuint8_t y_p, cuint8_t height_p)
{
    // Mark passage for debugging purpose
    debugMark(PSTR("Ssd1306::drawVLine(cuint8_t, cuint8_t, cuint8_t)"), Debug::CodeIndex::SSD1306_MODULE);

    // Check for errors
    if(!this->_isInitialized()) {
        // Returns error
        debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }
    if((x_p >= constSsd1306Width) || (y_p >= (uint8_t)(this->_pages * 8)) || (height_p == 0)) {
        // Returns error
        this->_lastError = Error::ARGUMENT_VALUE_INVALID;
        debugMessage(Error::ARGUMENT_VALUE_INVALID, Debug::CodeIndex::SSD1306_MODULE);
        return false;
    }

    // Local variables (clamps to the display bottom)
    uint8_t auxEndY                     = y_p + height_p - 1;
    if(auxEndY >= (uint8_t)(this->_pages * 8)) {
        auxEndY = (this->_pages * 8) - 1;
    }

    // Writes one masked byte per affected page
    for(uint8_t page = (y_p / 8); page <= (auxEndY / 8); page++) {
        uint8_t auxPageTop              = page * 8;
        uint8_t auxLow                  = (y_p > auxPageTop) ? (y_p - auxPageTop) : 0;
        uint8_t auxHigh                 = (auxEndY < (uint8_t)(auxPageTop + 7)) ? (auxEndY - auxPageTop) : 7;
        uint8_t auxMask                 = 0x00;

        for(uint8_t b = auxLow; b <= auxHigh; b++) {
            auxMask |= (1 << b);
        }
        if(!this->_setAddress(page, x_p)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
        if(!this->_writeData(&auxMask, 1)) {
            // Returns error
            debugMessage(this->_lastError, Debug::CodeIndex::SSD1306_MODULE);
            return false;
        }
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    debugMessage(Error::NONE, Debug::CodeIndex::SSD1306_MODULE);
    return true;
}

// =============================================================================
// Class private methods
// =============================================================================

bool_t Ssd1306::_writeCommand(uint8_t command_p)
{
    // Local variables
    uint8_t auxCommand                  = command_p;

    // Selects device and sends the command byte (control byte 0x00)
    if(!this->_busHandler->setDevice(this->_deviceAddress)) {
        // Returns error
        this->_lastError = this->_busHandler->getLastError();
        return false;
    }
    if(!this->_busHandler->writeReg(SSD1306_CONTROL_COMMAND, &auxCommand, 1)) {
        // Returns error
        this->_lastError = this->_busHandler->getLastError();
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

bool_t Ssd1306::_writeCommandList(const uint8_t *commands_p, uint8_t size_p)
{
    // Sends each command byte read from PROGMEM
    for(uint8_t i = 0; i < size_p; i++) {
        if(!this->_writeCommand(pgm_read_byte(&commands_p[i]))) {
            // Returns error
            return false;
        }
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

bool_t Ssd1306::_writeData(cuint8_t *data_p, uint8_t size_p)
{
    // Local variables
    uint8_t auxOffset                   = 0;

    // Selects device
    if(!this->_busHandler->setDevice(this->_deviceAddress)) {
        // Returns error
        this->_lastError = this->_busHandler->getLastError();
        return false;
    }

    // Sends data in chunks (control byte 0x40) to respect the TWI buffer size
    while(auxOffset < size_p) {
        uint8_t chunk = size_p - auxOffset;
        if(chunk > constSsd1306DataChunk) {
            chunk = constSsd1306DataChunk;
        }
        if(!this->_busHandler->writeReg(SSD1306_CONTROL_DATA, &data_p[auxOffset], chunk)) {
            // Returns error
            this->_lastError = this->_busHandler->getLastError();
            return false;
        }
        auxOffset += chunk;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

bool_t Ssd1306::_setAddress(uint8_t page_p, uint8_t x_p)
{
    // Sets page and column pointers (page addressing mode)
    if(!this->_writeCommand(SSD1306_SET_PAGE_ADDRESS | (page_p & 0x07))) {
        return false;
    }
    if(!this->_writeCommand(SSD1306_SET_LOW_COLUMN | (x_p & 0x0F))) {
        return false;
    }
    if(!this->_writeCommand(SSD1306_SET_HIGH_COLUMN | ((x_p >> 4) & 0x0F))) {
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

bool_t Ssd1306::_writeChar(char_t character_p)
{
    // Local variables
    uint8_t auxBuffer[constSsd1306CellWidth * constSsd1306TextSizeMax];   // up to 6*4
    uint8_t auxScale                    = this->_textSize;
    uint8_t auxCode                     = (uint8_t)character_p;
    uint8_t auxIndex                    = 0;

    // Maps unsupported characters to a blank space
    if((auxCode < constSsd1306FontFirst) || (auxCode > constSsd1306FontLast)) {
        auxCode = constSsd1306FontFirst;
    }
    auxIndex = auxCode - constSsd1306FontFirst;

    // Wraps to the next line when the glyph does not fit horizontally
    if((this->_cursorX + (constSsd1306CellWidth * auxScale)) > constSsd1306Width) {
        this->_cursorX                  = 0;
        this->_cursorPage               += auxScale;
    }
    if((this->_cursorPage + auxScale) > this->_pages) {
        this->_cursorPage               = 0;
    }

    // Renders the glyph, one page-row at a time
    for(uint8_t pg = 0; pg < auxScale; pg++) {
        uint8_t auxBufferSize           = 0;

        if(!this->_setAddress(this->_cursorPage + pg, this->_cursorX)) {
            return false;
        }
        for(uint8_t col = 0; col < constSsd1306CellWidth; col++) {
            uint8_t auxSourceCol        = 0x00;
            uint8_t auxScaledByte       = 0x00;

            if(col < 5) {
                auxSourceCol = pgm_read_byte(&constSsd1306Font[auxIndex][col]);
            }
            // Vertically expands the source column into this output page byte
            for(uint8_t b = 0; b < 8; b++) {
                uint8_t auxSourceRow = ((pg * 8) + b) / auxScale;
                if((auxSourceCol >> auxSourceRow) & 0x01) {
                    auxScaledByte |= (1 << b);
                }
            }
            // Horizontally repeats the scaled column auxScale times
            for(uint8_t rep = 0; rep < auxScale; rep++) {
                auxBuffer[auxBufferSize++] = auxScaledByte;
            }
        }
        if(!this->_writeData(auxBuffer, auxBufferSize)) {
            return false;
        }
    }

    // Advances the text cursor
    this->_cursorX                      += (constSsd1306CellWidth * auxScale);

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

bool_t Ssd1306::_isInitialized(void)
{
    // Check for errors
    if(!this->_initialized) {
        // Returns error
        this->_lastError = Error::NOT_INITIALIZED;
        return false;
    }

    // Returns successfully
    this->_lastError = Error::NONE;
    return true;
}

// =============================================================================
// Class protected methods
// =============================================================================

// NONE

// =============================================================================
// Static functions definitions
// =============================================================================

static int ssd1306WriteStd(char character_p, FILE *stream_p)
{
    (void)stream_p;

    defaultSsd1306Display->print(character_p);

    return 0;
}

// =============================================================================
// General public functions definitions
// =============================================================================

// NONE

// =============================================================================
// Interrupt callback functions
// =============================================================================

// NONE

// =============================================================================
// Interruption handlers
// =============================================================================

// NONE

// =============================================================================
// END OF FILE - ssd1306.cpp
// =============================================================================
