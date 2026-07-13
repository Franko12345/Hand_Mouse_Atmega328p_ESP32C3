//!
//! \file           bus.hpp
//! \brief          Bus Handler support to Funsape AVR8 Library
//! \author         Leandro Schwarz (bladabuska+funsapeavr8lib@gmail.com)
//! \date           2024-05-08
//! \version        24.05
//! \copyright      license
//! \details        This file provides a virtual class to handle a digital
//!                     communication bus for the Funsape AVR8 Library
//!

// =============================================================================
// Include guard (START)
// =============================================================================

#ifndef __BUS_HPP
#define __BUS_HPP                   2604

// =============================================================================
// Dependencies
// =============================================================================

//     /////////////////     GLOBAL DEFINITIONS FILE    /////////////////     //
#include "../globalDefines.hpp"
#if !defined(__GLOBAL_DEFINES_HPP)
#   error "Global definitions file is corrupted!"
#elif __GLOBAL_DEFINES_HPP != __BUS_HPP
#   error "Version mismatch between file header and global definitions file!"
#endif

//     //////////////////     LIBRARY DEPENDENCIES     //////////////////     //
#include "debug.hpp"
#if !defined(__DEBUG_HPP)
#   error "Header file (debug.hpp) is corrupted!"
#elif __DEBUG_HPP != __BUS_HPP
#   error "Version mismatch between header file and library dependency (debug.hpp)!"
#endif

// #include "../peripheral/funsapeLibGpio.hpp"
// #if !defined(__FUNSAPE_LIB_GPIO_HPP)
// #   error "Header file (funsapeLibGpio.hpp) is corrupted!"
// #elif __FUNSAPE_LIB_GPIO_HPP != __BUS_HPP
// #   error "Version mismatch between header file and library dependency (funsapeLibGpio.hpp)!"
// #endif

// =============================================================================
// Undefining previous definitions
// =============================================================================

// NONE

// =============================================================================
// Constant definitions
// =============================================================================


// =============================================================================
// New data types
// =============================================================================

// NONE

// =============================================================================
// Interrupt callback functions
// =============================================================================

// NONE

// =============================================================================
// Bus Class
// =============================================================================

//!
//! \brief          Bus class
//! \details        Bus class
//!
class Bus
{
    // -------------------------------------------------------------------------
    // New data types ----------------------------------------------------------
public:
    //     //////////////////////     Bus Type    ///////////////////////     //
    //!
    //! \brief      Bus type enumeration
    //! \details    Bus type options associated with the bus.
    //!
    enum class BusType {
        NONE                            = 0,    //!< Bus is not initialized
        OWI                             = 1,    //!< One Wire Interface
        PARALLEL                        = 2,    //!< Simple parallel bitbang interface
        SERIAL                          = 3,    //!< Simple serial bitbang interface
        SPI                             = 4,    //!< Serial Peripheral Interface
        TWI                             = 5,    //!< Two Wire Interface (I2C)
        UART                            = 6,    //!< Universal Sync./Async. Receiver Transmitter
    };

private:
    // NONE

protected:
    // NONE

    // -------------------------------------------------------------------------
    // Operator overloading ----------------------------------------------------
public:
    // NONE

private:
    // NONE

protected:
    // NONE

    // -------------------------------------------------------------------------
    // Constructors ------------------------------------------------------------
public:
    // NONE

    // -------------------------------------------------------------------------
    // Methods - Inherited methods ---------------------------------------------
public:
    // NONE

protected:
    // NONE

    // -------------------------------------------------------------------------
    // Methods - Class own methods ---------------------------------------------
public:
    //     ////////////////////    DATA TRANSFER     ////////////////////     //
    //!
    //! \brief          Reads data from an address.
    //! \details        This function reads a block of data from the given
    //!                     register address.
    //! \param[in]      reg_p           register address
    //! \param[out]     buffData_p      pointer to data vector
    //! \param[in]      buffSize_p      number of data elements to read
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t readReg(
            cuint8_t reg_p,
            uint8_t *buffData_p,
            cuint16_t buffSize_p
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::readReg(cuint8_t, uint8_t *, cuint16_t)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    //!
    //! \brief          Sends (exchange) data to the bus.
    //! \details        This function exchanges (sends and receives) a block of
    //!                     data to the bus. The data to be sent is read from
    //!                     the data vector, and the data received is stored at
    //!                     the same vector, overwriting its data.
    //! \param[in,out]  buffData_p      pointer to data vector to exchange
    //! \param[in]      buffSize_p      number of data elements to exchange
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t sendData(
            uint8_t *buffData_p,
            cuint16_t buffSize_p
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::sendData(uint8_t *, cuint16_t)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    //!
    //! \brief          Sends (exchange) data to the bus.
    //! \details        This function exchanges (sends and receives) a block of
    //!                     data to the bus. The data to be sent and the
    //!                     received data are stored in separate data vectors.
    //! \param[in]      txBuffData_p    pointer to the tx data vector
    //! \param[out]     rxBuffData_p    pointer to the rx data vector
    //! \param[in]      buffSize_p      number of data elements to exchange
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t sendData(
            cuint8_t *txBuffData_p,
            uint8_t *rxBuffData_p,
            cuint16_t buffSize_p
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::sendData(cuint8_t *, uint8_t *, cuint16_t)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    //!
    //! \brief          Writes data at an address.
    //! \details        This function writes a block of data at the given
    //!                     register address.
    //! \param[in]      reg_p           register address
    //! \param[in]      buffData_p      pointer to data vector
    //! \param[in]      buffSize_p      number of data elements to write
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t writeReg(
            cuint8_t reg_p,
            cuint8_t *buffData_p,
            cuint16_t buffSize_p
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::writeReg(cuint8_t, cuint8_t *, cuint16_t)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    //     ////////////////     TWI PROTOCOL METHODS     ////////////////     //
    //!
    //! \brief          Sets the device slave address.
    //! \details        This function sets the device slave address.
    //! \param[in]      address_p           slave address
    //! \param[in]      useLongAddress_p    use 10-bits slave address
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t setDevice(
            cuint16_t address_p,
            cbool_t useLongAddress_p = false
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::setDevice(cuint16_t, cbool_t)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    //     ////////////////     SPI PROTOCOL METHODS     ////////////////     //
    //!
    //! \brief          Sets the device slave select functions.
    //! \details        This function sets the device slave select functions.
    //! \param[in]      actFunc_p       pointer to slave select function
    //! \param[in]      deactFunc_p     pointer to slave release function
    //! \return         bool_t          True on success / False on failure
    //!
    virtual bool_t setDevice(
            void (* actFunc_p)(void),
            void (* deactFunc_p)(void)
    ) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::setDevice(void *(void), void *(void))"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns error
        this->_lastError = Error::FEATURE_NOT_SUPPORTED;
        debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
        return false;
    };

    // !
    // ! \brief          Sets the device slave select Gpio pin.
    // ! \details        This function sets the device slave select Gpio pin.
    // ! \param[in]      csPin_p         pointer Gpio slave select pin object
    // ! \return         bool_t          True on success / False on failure
    // !
    // virtual bool_t setDevice(
    //         const Gpio *csPin_p
    // ) {
    //     // Mark passage for debugging purpose
    // debugMark(PSTR("Bus::setDevice(const Gpio *)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

    //     // Returns error
    //     this->_lastError = Error::FEATURE_NOT_SUPPORTED;
    //     debugMessage(Error::FEATURE_NOT_SUPPORTED, Debug::CodeIndex::BUS_HANDLER_MODULE);
    //     return false;
    // };

    //     /////////////////     CONTROL AND STATUS     /////////////////     //
    //!
    //! \brief          Retuns the bus type.
    //! \details        This function returns the bus type interface.
    //! \return         BusType         Bus type
    //!
    virtual Bus::BusType getBusType(void) {
        // Mark passage for debugging purpose
        debugMark(PSTR("Bus::getBusType(void)"), Debug::CodeIndex::BUS_HANDLER_MODULE);

        // Returns default handler
        return BusType::NONE;
    }

    //!
    //! \brief          Retuns the last error.
    //! \details        This function returns the error code associated with the
    //!                     last operation.
    //! \return         Error           Error code
    //!
    virtual Error getLastError(void) {
        // Returns last error
        return this->_lastError;
    }

private:
    // NONE

protected:
    // NONE

    // -------------------------------------------------------------------------
    // Properties --------------------------------------------------------------
public:
    // NONE

private:
    //     /////////////////     CONTROL AND STATUS     /////////////////     //
    Error           _lastError;

protected:
    // NONE

}; // class Bus

// =============================================================================
// Bus - Class overloading operators
// =============================================================================

// NONE

// =============================================================================
// Global variables
// =============================================================================

// -----------------------------------------------------------------------------
// Externally defined global variables -----------------------------------------

// NONE

// -----------------------------------------------------------------------------
// Internally defined global variables -----------------------------------------

// NONE

// =============================================================================
// Bus - Class inline function definitions
// =============================================================================

// NONE

// =============================================================================
// General public functions declarations
// =============================================================================

// NONE

// =============================================================================
// General inline functions definitions
// =============================================================================

// NONE

// =============================================================================
// External default objects
// =============================================================================

// NONE

#endif  // __BUS_HPP

// =============================================================================
// END OF FILE
// =============================================================================
