//!
//! \file           ssd1306.hpp
//! \brief          SSD1306 OLED (I2C) module interface for the FunSAPE AVR8 Library
//! \author         Franko
//! \date           2026-07-13
//! \version        24.07
//! \copyright      license
//! \details        Text-oriented driver for the SSD1306 128x64 / 128x32 OLED
//!                     display over I2C (TWI). Supports two modes, selected at
//!                     init() by whether a framebuffer is provided:
//!
//!                 DIRECT MODE (no framebuffer, ~0 SRAM): writes straight to the
//!                     GDDRAM. Over I2C the GDDRAM cannot be read back, so there
//!                     is no read-modify-write and each write replaces whole
//!                     8-pixel vertical bytes. Therefore:
//!                     - Scalable text works fully (a glyph scaled by N spans N
//!                       whole pages).
//!                     - Graphics (fillRect / drawRect / drawHLine / drawVLine)
//!                       only work over a freshly cleared background and cannot
//!                       be composed/overlapped on existing content.
//!                     - Diagonal lines and arbitrary single-pixel plotting are
//!                       NOT supported.
//!
//!                 BUFFERED MODE (framebuffer in RAM, _pages*128 bytes): drawing
//!                     composes into the buffer and display() flushes only the
//!                     changed pages in one pass. This removes flicker and lifts
//!                     the direct-mode limitations: drawPixel, drawLine (with
//!                     diagonals), and graphics/text at arbitrary Y all work.
//! \todo           Todo list
//!

// =============================================================================
// Include guard (START)
// =============================================================================

#ifndef __SSD1306_HPP
#define __SSD1306_HPP                       2604

// =============================================================================
// Dependencies
// =============================================================================

//     /////////////////    GLOBAL DEFINITIONS FILE     /////////////////     //

#include "../globalDefines.hpp"
#if !defined(__GLOBAL_DEFINES_HPP)
#   error "Global definitions file is corrupted!"
#elif __GLOBAL_DEFINES_HPP != __SSD1306_HPP
#   error "Version mismatch between file header and global definitions file!"
#endif

//     //////////////////     LIBRARY DEPENDENCIES     //////////////////     //

#include "../util/debug.hpp"
#if !defined(__DEBUG_HPP)
#   error "Header file (debug.hpp) is corrupted!"
#elif __DEBUG_HPP != __SSD1306_HPP
#   error "Version mismatch between header file and library dependency (debug.hpp)!"
#endif

#include "../util/bus.hpp"
#if !defined(__BUS_HPP)
#   error "Header file (bus.hpp) is corrupted!"
#elif __BUS_HPP != __SSD1306_HPP
#   error "Version mismatch between header file and library dependency (bus.hpp)!"
#endif

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

// NONE

// =============================================================================
// Classes
// =============================================================================

//!
//! \brief          Ssd1306 class
//! \details        Ssd1306 class.
//!
class Ssd1306
{
    // -------------------------------------------------------------------------
    // New data types ----------------------------------------------------------
public:

    //     /////////////////////    DISPLAY SIZE     ////////////////////     //

    //!
    //! \brief      Display size
    //! \details    Display resolution enumeration.
    //!
    enum class Size : uint8_t {
        SSD1306_128X64                  = 0,
        SSD1306_128X32                  = 1
    };

private:

    // NONE

protected:

    // NONE

    // -------------------------------------------------------------------------
    // Constructors ------------------------------------------------------------
public:

    //!
    //! \brief      Ssd1306 class constructor
    //! \details    Creates a Ssd1306 object.
    //!
    Ssd1306(
            void
    );

    //!
    //! \brief      Ssd1306 class destructor
    //! \details    Destroys a Ssd1306 object.
    //!
    ~Ssd1306(
            void
    );

private:

    // NONE

protected:

    // NONE

    // -------------------------------------------------------------------------
    // Methods - Inherited methods ---------------------------------------------
public:

    // NONE

private:

    // NONE

protected:

    // NONE

    // -------------------------------------------------------------------------
    // Methods - Class own methods ---------------------------------------------
public:

    //     /////////////////     CONTROL AND STATUS     /////////////////     //

    //!
    //! \brief      Initializes the device.
    //! \details    Validates the bus handler, stores it and sends the SSD1306
    //!                 power-on command sequence. The TWI peripheral must be
    //!                 initialized (e.g. twi.init(400'000, 64)) and global
    //!                 interrupts enabled (sei()) before calling this.
    //! \param[in]  busHandler_p        pointer to the \ref Bus handler object
    //! \param[in]  size_p              display resolution
    //! \param[in]  address_p          I2C slave address (0x3C or 0x3D)
    //! \return true
    //! \return false
    //!
    bool_t init(
            Bus *busHandler_p,
            const Size size_p           = Size::SSD1306_128X64,
            cuint8_t address_p          = 0x3C
    );

    //!
    //! \brief      Clears the whole display and homes the text cursor.
    //! \details    Writes zeros to every page and resets the cursor to (0, 0).
    //! \return true
    //! \return false
    //!
    bool_t clearScreen(
            void
    );

    //!
    //! \brief      Turns the display on or off.
    //! \details    Sends the SSD1306 display-on/off command.
    //! \param[in]  state_p            \ref Activation value (ON / OFF)
    //! \return true
    //! \return false
    //!
    bool_t setDisplayState(
            const Activation state_p
    );

    //!
    //! \brief      Sets the display contrast.
    //! \details    Sends the SSD1306 contrast command (0-255).
    //! \param[in]  contrast_p         contrast value (0-255)
    //! \return true
    //! \return false
    //!
    bool_t setContrast(
            cuint8_t contrast_p
    );

    //!
    //! \brief      Rotates the whole display 180 degrees (upside down).
    //! \details    Uses the SSD1306 hardware segment-remap and COM-scan
    //!                 direction commands. Call it right after init(), or
    //!                 clearScreen() and redraw afterwards, since existing
    //!                 content is remapped when the orientation changes.
    //! \param[in]  enable_p           true = upside down, false = normal
    //! \return true
    //! \return false
    //!
    bool_t setUpsideDown(
            cbool_t enable_p
    );

    //!
    //! \brief      Returns the last error.
    //! \details    Returns the error code of the last operation.
    //! \return     Error              Return info
    //!
    Error getLastError(
            void
    );

    //     ////////////////////////     TEXT     ////////////////////////     //

    //!
    //! \brief      Sets the text scale factor.
    //! \details    Integer scale (1..4). A character scaled by N occupies N
    //!                 pages (8*N pixels) tall and 6*N pixels wide.
    //! \param[in]  size_p             scale factor (1-4)
    //! \return true
    //! \return false
    //!
    bool_t setTextSize(
            cuint8_t size_p
    );

    //!
    //! \brief      Positions the text cursor.
    //! \details    Sets the cursor at column x (in pixels, 0-127) and page
    //!                 (0-7 for 128x64, 0-3 for 128x32).
    //! \param[in]  x_p                horizontal position in pixels
    //! \param[in]  page_p             vertical position in pages
    //! \return true
    //! \return false
    //!
    bool_t setCursor(
            cuint8_t x_p,
            cuint8_t page_p
    );

    //!
    //! \brief      Redirects stdio (printf) to this display.
    //! \details    After this call, printf() output is written to the OLED.
    //!                 Only one stdio device can own stdout at a time.
    //! \return true
    //! \return false
    //!
    bool_t stdio(
            void
    );

    //!
    //! \brief      Prints a single character at the cursor.
    //! \details    Handles '\n' (advances to the next text line). Advances the
    //!                 cursor by 6*textSize pixels; wraps at the right edge.
    //! \param[in]  character_p        character to print
    //! \return true
    //! \return false
    //!
    bool_t print(
            cchar_t character_p
    );

    //!
    //! \brief      Prints a null-terminated string at the cursor.
    //! \details    Long description
    //! \param[in]  string_p           string to print
    //! \return true
    //! \return false
    //!
    bool_t print(
            cchar_t *string_p
    );

    //     //////////////////////     GRAPHICS     //////////////////////     //
    //     NOTE: no framebuffer -> these write whole 8-pixel vertical bytes.
    //     Draw over a freshly cleared background; they do NOT compose with
    //     existing content, and diagonals / arbitrary pixels are unsupported.

    //!
    //! \brief      Fills a solid rectangular block (page-aligned vertically).
    //! \details    Writes full 0xFF (or 0x00) bytes. Height is given in pages
    //!                 (multiples of 8 pixels), so this is always exact.
    //! \param[in]  x_p                left column in pixels (0-127)
    //! \param[in]  page_p             top page
    //! \param[in]  width_p            width in pixels
    //! \param[in]  heightPages_p      height in pages
    //! \param[in]  on_p               true = lit (0xFF), false = clear (0x00)
    //! \return true
    //! \return false
    //!
    bool_t fillRect(
            cuint8_t x_p,
            cuint8_t page_p,
            cuint8_t width_p,
            cuint8_t heightPages_p,
            cbool_t on_p                = true
    );

    //!
    //! \brief      Draws a 1-pixel rectangle outline (page-aligned vertically).
    //! \details    Best used on a freshly cleared region. Vertical bounds are
    //!                 the top of page_p and the bottom of the last page.
    //! \param[in]  x_p                left column in pixels (0-127)
    //! \param[in]  page_p             top page
    //! \param[in]  width_p            width in pixels
    //! \param[in]  heightPages_p      height in pages
    //! \return true
    //! \return false
    //!
    bool_t drawRect(
            cuint8_t x_p,
            cuint8_t page_p,
            cuint8_t width_p,
            cuint8_t heightPages_p
    );

    //!
    //! \brief      Draws a horizontal line (best on a cleared background).
    //! \details    y_p is in pixels; the line occupies a single bit within its
    //!                 page. Writing it clears the other 7 pixels of the touched
    //!                 page-columns (no read-modify-write over I2C).
    //! \param[in]  x_p                start column in pixels
    //! \param[in]  y_p                vertical position in pixels (0-63)
    //! \param[in]  width_p            length in pixels
    //! \return true
    //! \return false
    //!
    bool_t drawHLine(
            cuint8_t x_p,
            cuint8_t y_p,
            cuint8_t width_p
    );

    //!
    //! \brief      Draws a vertical line (best on a cleared background).
    //! \details    Writes whole page-columns spanning y_p..y_p+height_p-1.
    //! \param[in]  x_p                column in pixels (0-127)
    //! \param[in]  y_p                start row in pixels (0-63)
    //! \param[in]  height_p           length in pixels
    //! \return true
    //! \return false
    //!
    bool_t drawVLine(
            cuint8_t x_p,
            cuint8_t y_p,
            cuint8_t height_p
    );

private:

    //!
    //! \brief      Sends a single command byte (control byte 0x00).
    //! \details    Long description
    //! \param[in]  command_p          command byte
    //! \return true
    //! \return false
    //!
    bool_t _writeCommand(
            uint8_t command_p
    );

    //!
    //! \brief      Sends a list of command bytes from PROGMEM.
    //! \details    Long description
    //! \param[in]  commands_p         pointer to PROGMEM command array
    //! \param[in]  size_p             number of command bytes
    //! \return true
    //! \return false
    //!
    bool_t _writeCommandList(
            const uint8_t *commands_p,
            uint8_t size_p
    );

    //!
    //! \brief      Sends a data buffer (control byte 0x40), chunked.
    //! \details    Fragments the transfer to fit the TWI internal buffer.
    //! \param[in]  data_p             pointer to data bytes
    //! \param[in]  size_p             number of data bytes
    //! \return true
    //! \return false
    //!
    bool_t _writeData(
            cuint8_t *data_p,
            uint8_t size_p
    );

    //!
    //! \brief      Sets the GDDRAM page/column address (page-addressing mode).
    //! \details    Long description
    //! \param[in]  page_p             page (0-7)
    //! \param[in]  x_p                column in pixels (0-127)
    //! \return true
    //! \return false
    //!
    bool_t _setAddress(
            uint8_t page_p,
            uint8_t x_p
    );

    //!
    //! \brief      Renders one glyph at the current cursor using _textSize.
    //! \details    Long description
    //! \param[in]  character_p        character to render
    //! \return true
    //! \return false
    //!
    bool_t _writeChar(
            char_t character_p
    );

    //!
    //! \brief      Checks whether the device is initialized.
    //! \details    Sets _lastError to NOT_INITIALIZED when it is not.
    //! \return true
    //! \return false
    //!
    bool_t _isInitialized(
            void
    );

protected:

    // NONE

    // -------------------------------------------------------------------------
    // Properties --------------------------------------------------------------
public:

    // NONE

private:

    //     /////////////////     DEVICE BUS HANDLER     /////////////////     //

    Bus             *_busHandler;

    //     /////////////////     CONTROL AND STATUS     /////////////////     //

    bool_t          _busHandlerSet              : 1;
    bool_t          _initialized                : 1;
    Error           _lastError;

    //     ///////////////     HARDWARE CONFIGURATION     ///////////////     //

    uint8_t         _deviceAddress              : 7;
    Size            _size;
    uint8_t         _pages;                             // 8 (128x64) or 4 (128x32)

    //     //////////////////     TEXT CURSOR STATE     /////////////////     //

    uint8_t         _cursorX;                           // 0 to 127 (pixels)
    uint8_t         _cursorPage                 : 3;    // 0 to 7
    uint8_t         _textSize                   : 3;    // 1 to 4

protected:

    // NONE

}; // class Ssd1306

// =============================================================================
// Inlined class functions
// =============================================================================

// NONE

// =============================================================================
// External global variables
// =============================================================================

// NONE

// =============================================================================
// Include guard (END)
// =============================================================================

#endif  // __SSD1306_HPP

// =============================================================================
// END OF FILE - ssd1306.hpp
// =============================================================================
