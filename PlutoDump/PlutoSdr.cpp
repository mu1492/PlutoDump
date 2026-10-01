///////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2024 Mihai Ursu                                                 //
//                                                                               //
// This program is free software; you can redistribute it and/or modify          //
// it under the terms of the GNU General Public License as published by          //
// the Free Software Foundation as version 3 of the License, or                  //
// (at your option) any later version.                                           //
//                                                                               //
// This program is distributed in the hope that it will be useful,               //
// but WITHOUT ANY WARRANTY; without even the implied warranty of                //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                  //
// GNU General Public License V3 for more details.                               //
//                                                                               //
// You should have received a copy of the GNU General Public License             //
// along with this program. If not, see <http://www.gnu.org/licenses/>.          //
///////////////////////////////////////////////////////////////////////////////////

/*
PlutoSdr.cpp

This file contains the sources for the Pluto SDR.
*/

#include "PlutoSdr.h"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

#include <stdio.h>


//!************************************************************************
//! Constructor
//!************************************************************************
PlutoSdr::PlutoSdr
    (
    std::string aUri        //!< URI
    )
    : mUri( aUri )
    // context
    , mIioContext( nullptr )
    // devices
    , mPhyDev( nullptr )
    , mRxDev( nullptr )
    , mXadcDev( nullptr )
    // channels
    , mRxPhyChan( nullptr )
    , mRxLoChan( nullptr )
    , mTxLoChan( nullptr )
    , mTemperatureChan( nullptr )
    , mXadcTemperatureChan( nullptr )
    , mRx0_I( nullptr )
    , mRx0_Q( nullptr )
    , mRxBuf( nullptr )
    // settings
    , mRxBandwidth( 0 )
    , mRxSamplingFrequency( 0 )
    , mRxLoFrequency( 0 )
    , mRxHwGainDb( 0 )
    , mRxRssi( 0 )
    , mRxTrackingBbDcEnabled( true )
    , mRxTrackingQuadratureEnabled( true )
    , mRxTrackingRfDcEnabled( true )
    // init status
    , mInitialized( false )
{
    memset( &mRxSamplingFrequencyParams, 0, sizeof( mRxSamplingFrequencyParams ) );
    memset( &mRxLoFrequencyParams, 0, sizeof( mRxLoFrequencyParams ) );

    mIioContext = iio_create_context_from_uri( mUri.c_str() );
    bool status = nullptr != mIioContext;

    if( status )
    {
        status = iio_context_get_devices_count( mIioContext ) > 0;
    }

    if( status )
    {
        uint32_t ctxAttributesCount = iio_context_get_attrs_count( mIioContext );

        for( uint32_t i = 0; i < ctxAttributesCount; i++ )
        {
            std::string attrNameStr;
            std::string attrValueStr;
            const char* attrNameCstr = attrNameStr.c_str();
            const char* attrValueCstr = attrValueStr.c_str();

            if( 0 == iio_context_get_attr( mIioContext, i, &attrNameCstr, &attrValueCstr ) )
            {
                attrNameStr = attrNameCstr;
                attrValueStr = attrValueCstr;
                mCtxAttributesMap[attrNameStr] = attrValueStr;
            }
        }

        status = extractContextAttributes();
    }

    // devices: phy, Rx streaming, xADC
    if( status )
    {
        mPhyDev = iio_context_find_device( mIioContext, AD9361_PHY_DEV_STR.c_str() );
        status = nullptr != mPhyDev;        
    }

    if( status )
    {
        mRxDev = iio_context_find_device( mIioContext, AD9361_STREAMING_DEV_STR.c_str() );
        status = nullptr != mRxDev;
    }   

    if( status )
    {
        mXadcDev = iio_context_find_device( mIioContext, XADC_DEV_STR.c_str() );
        status = nullptr != mXadcDev;
    }

    // channels: Rx phy, Rx LO, Tx LO
    if( status )
    {
        const bool IS_OUTPUT_CH = false;
        mRxPhyChan = iio_device_find_channel( mPhyDev, "voltage0", IS_OUTPUT_CH );
        status = nullptr != mRxPhyChan;
    }

    if( status )
    {
        const bool IS_OUTPUT_CH = true;
        mRxLoChan = iio_device_find_channel( mPhyDev, "altvoltage0", IS_OUTPUT_CH );
        status = nullptr != mRxLoChan;
    }

    if( status )
    {
        const bool IS_OUTPUT_CH = true;
        mTxLoChan = iio_device_find_channel( mPhyDev, "altvoltage1", IS_OUTPUT_CH );
        status = nullptr != mTxLoChan;
    }

    // channels: temperature
    if( status )
    {
        const bool IS_OUTPUT_CH = false;
        mTemperatureChan = iio_device_find_channel( mPhyDev, "temp0", IS_OUTPUT_CH );
        status = nullptr != mTemperatureChan;
    }

    if( status )
    {
        const bool IS_OUTPUT_CH = false;
        mXadcTemperatureChan = iio_device_find_channel( mXadcDev, "temp0", IS_OUTPUT_CH );
        status = nullptr != mXadcTemperatureChan;
    }

    // channels: Rx streaming
    if( status )
    {
        const bool IS_OUTPUT_CH = false;
        mRx0_I = iio_device_find_channel( mRxDev, "voltage0", IS_OUTPUT_CH );
        status = nullptr != mRx0_I;
    }

    if( status )
    {
        const bool IS_OUTPUT_CH = false;
        mRx0_Q = iio_device_find_channel( mRxDev, "voltage1", IS_OUTPUT_CH );
        status = nullptr != mRx0_I;
    }

    if( status )
    {
        iio_channel_enable( mRx0_I );
        iio_channel_enable( mRx0_Q );
    }

    if( status )
    {
        const bool IS_CYCLIC_BUFFER = false;
        mRxBuf = iio_device_create_buffer( mRxDev, RX_BUF_SAMPLES_COUNT, IS_CYCLIC_BUFFER );
        status = nullptr != mRxBuf;
    }


    if( status )
    {
        status = getRxBandwidthParams();
    }

    if( status )
    {
        status = getRxSamplingFrequencyParams();
    }

    if( status )
    {
        status = getRxLoFrequencyParams();
    }

    if( status )
    {
        status = setRxGainControlMode( "manual" );
    }

    if( status )
    {
        status = getRxHwGainParams();
    }

    if( status )
    {
        status = getRxGainControlModeParams();
    }

    mInitialized = status;
}


//!************************************************************************
//! Destructor
//!************************************************************************
PlutoSdr::~PlutoSdr()
{
    if( mRxBuf )
    {
        iio_buffer_destroy( mRxBuf );
    }

    if( mRx0_I )
    {
        iio_channel_disable( mRx0_I );
    }
    if( mRx0_Q )
    {
        iio_channel_disable( mRx0_Q );
    }

    if( mRxLoChan )
    {
        iio_channel_disable( mRxLoChan );
    }
    if( mRxPhyChan )
    {
        iio_channel_disable( mRxPhyChan );
    }

    if( mTxLoChan )
    {
        iio_channel_disable( mTxLoChan );
    }

    if( mTemperatureChan )
    {
        iio_channel_disable( mTemperatureChan );
    }
    if( mXadcTemperatureChan )
    {
        iio_channel_disable( mXadcTemperatureChan );
    }


    if( mIioContext )
    {
        iio_context_destroy( mIioContext );
    }
}


//!************************************************************************
//! Disable power for all Tx modules of AD936x
//!
//! @returns true if fully disabling Tx power can be done
//!************************************************************************
bool PlutoSdr::disableAd9361TxFull()
{
    uint8_t reg = 0;
    bool status = readAd9361Register( AD9361_REG_TX_EN_FILTER_CTRL, reg );

    if( status )
    {
        const uint8_t DISABLE_TX_BOTH_MASK = 0xc0;
        reg &= ~DISABLE_TX_BOTH_MASK;
        status = writeAd9361Register( AD9361_REG_TX_EN_FILTER_CTRL, reg );
    }

    if( status )
    {
        const uint8_t TX_SYNTH_PWRDOWN_MASK = 0x1f;
        status = writeAd9361Register( AD9361_REG_TX_SYNTH_PWRDOWN, TX_SYNTH_PWRDOWN_MASK );
    }

    if( status )
    {
        const uint8_t TX_ANALOG_PWRDOWN_MASK = 0xff;
        status = writeAd9361Register( AD9361_REG_TX_ANALOG_PWRDOWN, TX_ANALOG_PWRDOWN_MASK );
    }

    if( status )
    {
        status = readAd9361Register( AD9361_REG_ANALOG_PWRDOWN, reg );

        if( status )
        {
            const uint8_t ANALOG_PWRDOWN_TX_ONLY_MASK = 0x1f;
            reg |= ANALOG_PWRDOWN_TX_ONLY_MASK;
            status = writeAd9361Register( AD9361_REG_ANALOG_PWRDOWN, reg );
        }
    }

    return status;
}


//!************************************************************************
//! Extract a selected list of context attributes
//!
//! @returns true if the context attributes can be extracted
//!************************************************************************
bool PlutoSdr::extractContextAttributes()
{
    memset( &mCtxAttributes.fwVersion, 0, sizeof( mCtxAttributes.fwVersion ) );
    const uint8_t FILLED_IN_FIELDS = 5;
    uint8_t fieldsCtr = 0;
    std::map<std::string, std::string>::iterator it;

    for( it = mCtxAttributesMap.begin(); it != mCtxAttributesMap.end(); it++ )
    {
        if( "hw_model" == it->first )
        {
            mCtxAttributes.hwRev = extractHwRev( it->second );
            fieldsCtr++;
        }
        else if( "hw_serial" == it->first )
        {
            mCtxAttributes.hwSerial = it->second;
            fieldsCtr++;
        }
        else if( "fw_version" == it->first )
        {
            mCtxAttributes.fwVersion = extractFwVersion( it->second );
            fieldsCtr++;
        }
        else if( "ad9361-phy,model" == it->first )
        {
            mCtxAttributes.ad936xModel = it->second;
            fieldsCtr++;
        }
        else if( "uri" == it->first )
        {
            mCtxAttributes.uri = it->second;;
            fieldsCtr++;
        }
    }

    return FILLED_IN_FIELDS == fieldsCtr;
}


//!************************************************************************
//! Extract a double value from a string, based on a substring index
//!
//! @returns true if a double value can be extracted
//!************************************************************************
bool PlutoSdr::extractDouble
    (
    const std::string   aString,    //!< string
    const size_t        aIndex,     //!< substring index
    double&             aValue      //!< value
    )
{
    aValue = 0;
    const size_t STR_SIZE = aString.size();
    bool status = STR_SIZE > 0;

    if( status )
    {
        std::vector<std::string> vec;
        int start = 0;
        int end = 0;

        while( ( start = aString.find_first_not_of( ' ', end ) ) != std::string::npos )
        {
            end = aString.find( ' ', start );
            vec.push_back( aString.substr( start, end - start ) );
        }

        status = ( vec.size() > aIndex );

        if( status )
        {
            aValue = std::atof( vec.at( aIndex ).c_str() );
        }
    }

    return status;
}


//!************************************************************************
//! Extract the firmware version (e.g. 0.38) running on PlutoSDR
//!
//! @returns The firmware version
//!************************************************************************
PlutoSdr::Version PlutoSdr::extractFwVersion
    (
    const std::string   aString     //!< string
    )
{
    Version version;
    memset( &version, 0, sizeof( version ) );

    int start = 0;
    int end = 0;
    std::vector<std::string> vec;

    while( ( start = aString.find_first_not_of( '.', end ) ) != std::string::npos )
    {
        end = aString.find( '.', start );
        vec.push_back( aString.substr( start, end - start ) );
    }

    if( vec.size() >= 1 )
    {
        if( vec.at( 0 )[0] == 'v'
         || vec.at( 0 )[0] == 'V' )
        {
            const size_t V0_LEN = vec.at( 0 ).size();
            version.major = std::atoi( vec.at( 0 ).substr( 1, V0_LEN - 1 ).c_str() );
        }
        else
        {
            version.major = std::atoi( vec.at( 0 ).c_str() );
        }
    }

    if( vec.size() >= 2 )
    {
        version.minor = std::atoi( vec.at( 1 ).c_str() );
    }

    return version;
}


//!************************************************************************
//! Extract the PlutoSDR hardware revision (e.g. "B", "C")
//!
//! @returns The string with hardware revision
//!************************************************************************
std::string PlutoSdr::extractHwRev
    (
    const std::string   aString     //!< string
    )
{
    std::string rev;
    int start = 0;
    int end = 0;

    while( ( start = aString.find_first_not_of( ' ', end ) ) != std::string::npos )
    {
        end = aString.find( ' ', start );
        std::string crtSubstring = aString.substr( start, end - start );
        std::string comparisonPartStr = crtSubstring.substr( 0, 4 );

        if( "Rev." == comparisonPartStr
         || "rev." == comparisonPartStr )
        {
            rev = crtSubstring.substr( 4, 1 );
        }
    }

    return rev;
}


//!************************************************************************
//! Extract an integer from a string, based on a substring index
//!
//! @returns true if an integer can be extracted
//!************************************************************************
bool PlutoSdr::extractInteger
    (
    const std::string   aString,    //!< string
    const size_t        aIndex,     //!< substring index
    int64_t&            aValue      //!< value
    )
{
    aValue = 0;
    const size_t STR_SIZE = aString.size();
    bool status = STR_SIZE > 0;

    if( status )
    {
        std::vector<std::string> vec;
        int start = 0;
        int end = 0;

        while( ( start = aString.find_first_not_of( ' ', end ) ) != std::string::npos )
        {
            end = aString.find( ' ', start );
            vec.push_back( aString.substr( start, end - start ) );
        }

        status = ( vec.size() > aIndex );

        if( status )
        {            
            aValue = static_cast<int64_t>( std::atof( vec.at( aIndex ).c_str() ) );
        }
    }

    return status;
}


//!************************************************************************
//! Extract an integer range from a string
//!
//! @returns true if a range can be extracted
//!************************************************************************
bool PlutoSdr::extractIntegerRange
    (
    std::string                 aString,    //!< string
    PlutoSdr::IntegerRange&     aRange      //!< range
    )
{
    memset( &aRange, 0, sizeof( aRange ) );
    const size_t STR_SIZE = aString.size();
    bool status = STR_SIZE > 2;

    if( status )
    {
        if( '[' == aString.at( 0 )
         && ']' == aString.at( STR_SIZE - 1 ) )
        {
            aString = aString.substr( 1, aString.size() - 2 );
        }

        std::vector<std::string> vec;
        int start = 0;
        int end = 0;

        while( ( start = aString.find_first_not_of( ' ', end ) ) != std::string::npos )
        {
            end = aString.find( ' ', start );
            vec.push_back( aString.substr( start, end - start ) );
        }

        status = ( 3 == vec.size() );

        if( status )
        {
            aRange.min  = std::stoll( vec.at( 0 ).c_str() );
            aRange.step = std::stoll( vec.at( 1 ).c_str() );
            aRange.max  = std::stoll( vec.at( 2 ).c_str() );

            status = ( aRange.min < aRange.max
                    && aRange.step < ( aRange.max - aRange.min ) );
        }
    }

    return status;
}


//!************************************************************************
//! Get the AD936x model (e.g. "ad9361", "ad9364")
//!
//! @returns The string with the AD936x model
//!************************************************************************
std::string PlutoSdr::getAd9361Model() const
{
    return mCtxAttributes.ad936xModel;
}


//!************************************************************************
//! Get the AD9361 chip revision (e.g. 2)
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getAd9361ProductRev
    (
    uint8_t& aProductRev        //!< product revision
    )
{
    aProductRev = 0;
    bool status = readAd9361Register( AD9361_REG_PRODUCT_ID, aProductRev );

    if( status )
    {
        const uint8_t REV_MASK = 0x07;
        aProductRev &= REV_MASK;
    }

    return status;
}


//!************************************************************************
//! Get the temperature [C] measured internally by AD9361
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getAd9361Temperature
    (
    double& aTemperature        //!< temperature [C]
    ) const
{
    aTemperature = -99;
    long long temperatureMilliC = 0;
    bool status = ( 0 == iio_channel_attr_read_longlong( mTemperatureChan, "input", &temperatureMilliC ) );

    if( status )
    {
        aTemperature = temperatureMilliC / 1000.0;
    }

    return status;
}


//!************************************************************************
//! Get the firmware version of PlutoSDR (e.g. 0.38)
//!
//! @returns The firmware version
//!************************************************************************
PlutoSdr::Version PlutoSdr::getFwVersion() const
{
    return mCtxAttributes.fwVersion;
}


//!************************************************************************
//! Get the hardware revision of PlutoSDR (e.g. "B", "C")
//!
//! @returns The string with hardware revision
//!************************************************************************
std::string PlutoSdr::getHwRevision() const
{
    return mCtxAttributes.hwRev;
}


//!************************************************************************
//! Get the hardware serial of PlutoSDR, a 34 hex digits string
//!
//! @returns The string with hardware serial
//!************************************************************************
std::string PlutoSdr::getHwSerial() const
{
    return mCtxAttributes.hwSerial;
}


//!************************************************************************
//! Get the Rx bandwidth [Hz]
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxBandwidth
    (
    int64_t& aBandwidth     //!< bandwidth [Hz]
    )
{
    mRxBandwidth = 0;
    char rfBandwidthString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rf_bandwidth", rfBandwidthString, sizeof( rfBandwidthString ) ) > 0 );

    if( status )
    {
        mRxBandwidth = std::stoll( rfBandwidthString );
    }

    aBandwidth = mRxBandwidth;

    return status;
}


//!************************************************************************
//! Get the Rx bandwidth parameters
//!
//! @returns true if the parameters can be read
//!************************************************************************
bool PlutoSdr::getRxBandwidthParams()
{
    char rfBandwidthAvailableString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rf_bandwidth_available", rfBandwidthAvailableString, sizeof( rfBandwidthAvailableString ) ) > 2 );

    if( status )
    {
        std::string rawStr = rfBandwidthAvailableString;
        status = extractIntegerRange( rawStr, mRxBandwidthParams );
    }

    return status;
}


//!************************************************************************
//! Get the Rx bandwidth range
//!
//! @returns The range for Rx bandwidth
//!************************************************************************
PlutoSdr::IntegerRange PlutoSdr::getRxBandwidthRange() const
{
    return  mRxBandwidthParams;
}



//!************************************************************************
//! Get the Rx gain control mode parameters
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxGainControlModeParams()
{
    mRxGainControlModeVec.clear();
    char gainCtrlModeAvailableString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "gain_control_mode_available", gainCtrlModeAvailableString, sizeof( gainCtrlModeAvailableString ) ) > 2 );

    if( status )
    {
        std::string rawStr = gainCtrlModeAvailableString;
        int start = 0;
        int end = 0;

        while( ( start = rawStr.find_first_not_of( ' ', end ) ) != std::string::npos )
        {
            end = rawStr.find( ' ', start );
            mRxGainControlModeVec.push_back( rawStr.substr( start, end - start ) );
        }

        status = mRxGainControlModeVec.size() > 0;
    }

    return status;
}


//!************************************************************************
//! Get the Rx hardware gain [dB]
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxHwGain
    (
    int64_t& aHwGainDb      //!< hardware gain [dB]
    )
{
    mRxHwGainDb = 0;
    char hwGainString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "hardwaregain", hwGainString, sizeof( hwGainString ) ) > 0 );

    if( status )
    {
        std::string rawStr = hwGainString;
        status = extractInteger( rawStr, 0, mRxHwGainDb );
    }

    aHwGainDb = mRxHwGainDb;

    return status;
}


//!************************************************************************
//! Get the Rx hardware gain parameters
//!
//! @returns true if the parameters can be read
//!************************************************************************
bool PlutoSdr::getRxHwGainParams()
{
    char hwGainAvailableString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "hardwaregain_available", hwGainAvailableString, sizeof( hwGainAvailableString ) ) > 2 );

    if( status )
    {
        std::string rawStr = hwGainAvailableString;
        status = extractIntegerRange( rawStr, mRxHwGainDbParams );
    }

    return status;
}


//!************************************************************************
//! Get the Rx LO frequency [Hz]
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxLoFrequency
    (
    int64_t& aFrequency     //!< frequency [Hz]
    )
{
    mRxLoFrequency = 0;
    char freqString[256];
    bool status = ( iio_channel_attr_read( mRxLoChan, "frequency", freqString, sizeof( freqString ) ) > 0 );

    if( status )
    {
        mRxLoFrequency = std::stoll( freqString );
    }

    aFrequency = mRxLoFrequency;

    return status;
}


//!************************************************************************
//! Get the Rx LO frequency parameters
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxLoFrequencyParams()
{
    char freqAvailableString[256];
    bool status = ( iio_channel_attr_read( mRxLoChan, "frequency_available", freqAvailableString, sizeof( freqAvailableString ) ) > 2 );

    if( status )
    {
        std::string rawStr = freqAvailableString;
        status = extractIntegerRange( rawStr, mRxLoFrequencyParams );
    }

    return status;
}


//!************************************************************************
//! Get the Rx LO frequency range
//!
//! @returns The range for Rx LO frequency
//!************************************************************************
PlutoSdr::IntegerRange PlutoSdr::getRxLoFrequencyRange() const
{
    return mRxLoFrequencyParams;
}


//!************************************************************************
//! Get the Rx selected port
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxPortSelect
    (
    std::string& aRfPort    //!< RF port
    ) const
{
    char rfPortString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rf_port_select", rfPortString, sizeof( rfPortString ) ) > 0 );

    if( status )
    {
        aRfPort = rfPortString;
    }

    return status;
}


//!************************************************************************
//! Get the Rx selected port parameters
//!
//! @returns true if the parameters can be read
//!************************************************************************
bool PlutoSdr::getRxPortSelectParams()
{
    mRxPortSelectVec.clear();
    char rfPortSelectString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rf_port_select_available", rfPortSelectString, sizeof( rfPortSelectString ) ) > 2 );

    if( status )
    {
        std::string rawStr = rfPortSelectString;
        int start = 0;
        int end = 0;

        while( ( start = rawStr.find_first_not_of( ' ', end ) ) != std::string::npos )
        {
            end = rawStr.find( ' ', start );
            mRxPortSelectVec.push_back( rawStr.substr( start, end - start ) );
        }

        status = mRxPortSelectVec.size() > 0;
    }

    return status;
}


//!************************************************************************
//! Get the Rx RSSI [dB]
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxRssi
    (
    double& aRssi       //!< RSSI [dB]
    )
{
    mRxRssi = 0;
    char rssiString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rssi", rssiString, sizeof( rssiString ) ) > 0 );

    if( status )
    {
        std::string rawStr = rssiString;
        status = extractDouble( rawStr, 0, mRxRssi );
    }

    aRssi = mRxRssi;

    return status;
}


//!************************************************************************
//! Get the Rx sampling frequency [Hz]
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxSamplingFrequency
    (
    int64_t& aFrequency     //!< sampling frequency [Hz]
    )
{
    mRxSamplingFrequency = 0;
    char samplingFreqString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "sampling_frequency", samplingFreqString, sizeof( samplingFreqString ) ) > 0 );

    if( status )
    {
        mRxSamplingFrequency = std::stoll( samplingFreqString );
    }

    aFrequency = mRxSamplingFrequency;

    return status;
}


//!************************************************************************
//! Get the Rx sampling frequency parameters
//!
//! @returns true if the parameters can be read
//!************************************************************************
bool PlutoSdr::getRxSamplingFrequencyParams()
{
    char samplingFreqAvailableString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "sampling_frequency_available", samplingFreqAvailableString, sizeof( samplingFreqAvailableString ) ) > 2 );

    if( status )
    {
        std::string rawStr = samplingFreqAvailableString;
        status = extractIntegerRange( rawStr, mRxSamplingFrequencyParams );
    }

    return status;
}


//!************************************************************************
//! Get the Rx sampling frequency range
//!
//! @returns The range for Rx sampling frequency
//!************************************************************************
PlutoSdr::IntegerRange PlutoSdr::getRxSamplingFrequencyRange() const
{
    return mRxSamplingFrequencyParams;
}


//!************************************************************************
//! Get the status of baseband DC tracking
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxTrackingBbDc
    (
    bool& aEnable       //!< true if tracking enabled
    )
{
    aEnable = true;
    char trackingString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "bb_dc_offset_tracking_en", trackingString, sizeof( trackingString ) ) > 0 );

    if( status )
    {
        int trackingValue = std::atoi( trackingString );
        aEnable = ( 0 != trackingValue );
    }

    return status;
}


//!************************************************************************
//! Get the status of quadrature (IQ) tracking
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxTrackingQuadrature
    (
    bool& aEnable       //!< true if tracking enabled
    )
{
    aEnable = true;
    char trackingString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "quadrature_tracking_en", trackingString, sizeof( trackingString ) ) > 0 );

    if( status )
    {
        int trackingValue = std::atoi( trackingString );
        aEnable = ( 0 != trackingValue );
    }

    return status;
}


//!************************************************************************
//! Get the status of RF DC tracking
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getRxTrackingRfDc
    (
    bool& aEnable       //!< true if tracking enabled
    )
{
    aEnable = true;
    char trackingString[256];
    bool status = ( iio_channel_attr_read( mRxPhyChan, "rf_dc_offset_tracking_en", trackingString, sizeof( trackingString ) ) > 0 );

    if( status )
    {
        int trackingValue = std::atoi( trackingString );
        aEnable = ( 0 != trackingValue );
    }

    return status;
}


//!************************************************************************
//! Get the status of Tx LO power
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getTxLoPower
    (
    bool& aEnable       //!< true if power is enabled
    ) const
{
    aEnable = true;
    long long isPowerDown = 0;
    bool status = ( 0 == iio_channel_attr_read_longlong( mTxLoChan, "powerdown", &isPowerDown ) );

    if( status )
    {
        aEnable = !isPowerDown;
    }

    return status;
}


//!************************************************************************
//! Get the temperature [C] from the ADC sensor
//!
//! @returns true if the parameter can be read
//!************************************************************************
bool PlutoSdr::getXadcTemperature
    (
    double& aTemperature        //!< temperature [C]
    ) const
{
    aTemperature = -99;
    long long offset = 0;
    long long raw = 0;
    long long scale = 0;
    bool status = ( 0 == iio_channel_attr_read_longlong( mXadcTemperatureChan, "offset", &offset ) );

    if( status )
    {
        status = ( 0 == iio_channel_attr_read_longlong( mXadcTemperatureChan, "raw", &raw ) );
    }

    if( status )
    {
        status = ( 0 == iio_channel_attr_read_longlong( mXadcTemperatureChan, "scale", &scale ) );
    }

    if( status )
    {
        aTemperature = ( raw + offset ) * scale / 1000.0;
    }

    return status;
}


//!************************************************************************
//! Check if PlutoSDR is initialized
//!
//! @returns true if PlutoSDR is initialized
//!************************************************************************
bool PlutoSdr::isInitialized() const
{
    return mInitialized;
}


//!************************************************************************
//! Read a byte from a register
//!
//! @returns true if the value can be read
//!************************************************************************
bool PlutoSdr::readAd9361Register
    (
    const uint16_t aAddress,    //!< register address
    uint8_t&       aValue       //!< read value
    )
{
    uint32_t readValue = 0;
    bool status = ( 0 == iio_device_reg_read( mPhyDev, aAddress, &readValue ) );
    aValue = status ? ( readValue & 0xff ) : 0;
    return status;
}


//!************************************************************************
//! Set the Rx bandwidth [Hz]
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxBandwidth
    (
    const int64_t aBandwidth    //!< bandwidth [Hz]
    )
{
    bool status = ( aBandwidth >= mRxBandwidthParams.min
                 && aBandwidth <= mRxBandwidthParams.max );

    if( status )
    {
        status = ( 0 == iio_channel_attr_write_longlong( mRxPhyChan, "rf_bandwidth", aBandwidth ) );
    }

    if( status )
    {
        mRxBandwidth = aBandwidth;
    }

    return status;
}


//!************************************************************************
//! Set the Rx gain control mode
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxGainControlMode
    (
    const std::string aMode     //!< gain control mode
    )
{
    bool status = ( iio_channel_attr_write( mRxPhyChan, "gain_control_mode", aMode.c_str() ) > 0 );
    return status;
}


//!************************************************************************
//! Set the Rx hardware gain [dB]
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxHwGain
    (
    const int64_t& aHwGainDb    //!< hardware gain [dB]
    )
{
    bool status = ( aHwGainDb >= mRxHwGainDbParams.min
                 && aHwGainDb <= mRxHwGainDbParams.max );

    if( status )
    {
        status = ( 0 == iio_channel_attr_write_longlong( mRxPhyChan, "hardwaregain", aHwGainDb ) );
    }

    if( status )
    {
        mRxHwGainDb = aHwGainDb;
    }

    return status;
}


//!************************************************************************
//! Set the Rx LO frequency [Hz]
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxLoFrequency
    (
    const int64_t aFrequency    //!< frequency [Hz]
    )
{
    bool status = ( aFrequency >= mRxLoFrequencyParams.min
                 && aFrequency <= mRxLoFrequencyParams.max );

    if( status )
    {
        status = ( 0 == iio_channel_attr_write_longlong( mRxLoChan, "frequency", aFrequency ) );
    }

    if( status )
    {
        mRxLoFrequency = aFrequency;
    }

    return status;
}


//!************************************************************************
//! Set the Rx selected port
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxPortSelect
    (
    const std::string aRfPort   //!< RF port
    )
{
    bool status = ( iio_channel_attr_write( mRxPhyChan, "rf_port_select", aRfPort.c_str() ) > 0 );
    return status;
}


//!************************************************************************
//! Set the Rx sampling frequency [Hz]
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxSamplingFrequency
    (
    const int64_t aFrequency    //!< frequency [Hz]
    )
{
    bool status = ( aFrequency >= mRxSamplingFrequencyParams.min
                 && aFrequency <= mRxSamplingFrequencyParams.max );

    if( status )
    {
        status = ( 0 == iio_channel_attr_write_longlong( mRxPhyChan, "sampling_frequency", aFrequency ) );
    }

    if( status )
    {
        mRxSamplingFrequency = aFrequency;
    }

    return status;
}


//!************************************************************************
//! Set tracking for Rx baseband DC
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxTrackingBbDc
    (
    const bool aEnable      //!< true for enabling tracking
    )
{
    bool status = ( iio_channel_attr_write( mRxPhyChan, "bb_dc_offset_tracking_en", aEnable ? "1" : "0" ) > 0 );
    return status;
}


//!************************************************************************
//! Set tracking for Rx quadrature (IQ)
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxTrackingQuadrature
    (
    const bool aEnable      //!< true for enabling tracking
    )
{
    bool status = ( iio_channel_attr_write( mRxPhyChan, "quadrature_tracking_en", aEnable ? "1" : "0" ) > 0 );
    return status;
}


//!************************************************************************
//! Set tracking for Rx RF DC
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setRxTrackingRfDc
    (
    const bool aEnable      //!< true for enabling tracking
    )
{
    bool status = ( iio_channel_attr_write( mRxPhyChan, "rf_dc_offset_tracking_en", aEnable ? "1" : "0" ) > 0 );
    return status;
}


//!************************************************************************
//! Enable or disable the Tx LO power
//!
//! @returns true if the setting can be applied
//!************************************************************************
bool PlutoSdr::setTxLoPower
    (
    const bool aEnable      //!< true for enabling power
    )
{
    bool status = ( 0 == iio_channel_attr_write_longlong( mTxLoChan, "powerdown", aEnable ? 0 : 1 ) );
    return status;
}


//!************************************************************************
//! Start Rx streaming
//!
//! @returns nothing
//!************************************************************************
void PlutoSdr::startRxStreaming
    (
    const int64_t aFreeSpaceBytes      //!< free space [B]
    )
{
    bool isDataValid = aFreeSpaceBytes > 0;
    mRxDataVec.clear();

    const double ONE_MB = 1048576.0;
    double freeSpaceMegaBytes = aFreeSpaceBytes / ONE_MB;

    const size_t ONE_BUF_TRUNK_MB = 8;
    uint32_t bufTrunkCount = freeSpaceMegaBytes / ONE_BUF_TRUNK_MB;

    bool isRunningLocal = ( "local:" == mCtxAttributes.uri );

    if( isRunningLocal )
    {
        const size_t BUF_TRUNK_MAX = 25;

        if( bufTrunkCount > BUF_TRUNK_MAX )
        {
            bufTrunkCount = BUF_TRUNK_MAX;
        }
    }

    const double MAX_SECONDS_DUMP = ONE_BUF_TRUNK_MB * bufTrunkCount / ( 4.0e-6 * mRxSamplingFrequency );
    double desiredSecondsDump = MAX_SECONDS_DUMP;
    bool useMaxDuration = false;
    char answer[64] = "";
    int retScanf = 0;
    printf( "\n\n With current settings, maximum duration of saved data would be %.3lf seconds", MAX_SECONDS_DUMP );

    do{
        printf( "\n Do you want to use the entire duration? [yes / no] " );
        retScanf = scanf( "%s", answer );
        useMaxDuration = ( 'y' == tolower( answer[0] ) );
    }while( 'y' != tolower( answer[0] ) && 'n' != tolower( answer[0] ) );

    if( !useMaxDuration )
    {
        do{
            printf( "\n Enter the maximum desired duration (< %.3lf) [s] = ", MAX_SECONDS_DUMP );
            retScanf = scanf( "%lf", &desiredSecondsDump );
        }while( desiredSecondsDump <= 0 || desiredSecondsDump >= MAX_SECONDS_DUMP );

        bufTrunkCount = floor( desiredSecondsDump * 4.e-6 * mRxSamplingFrequency / ONE_BUF_TRUNK_MB );
        desiredSecondsDump = ONE_BUF_TRUNK_MB * bufTrunkCount / ( 4.0e-6 * mRxSamplingFrequency );        
    }

    printf( "\n Estimated duration for dumping data = %.3lf [s]", desiredSecondsDump );
    printf( "\n Buf # = %lu", bufTrunkCount );

    if( isDataValid )
    {
        printf( "\n Start Rx streaming.. " );
        fflush( stdout );
        ssize_t sampleSize = iio_device_get_sample_size( mRxDev );

        if( sampleSize <= 0 )
        {
            isDataValid = false;
            printf( "\n Error getting the sample size.\n" );
        }
    }

    if( isRunningLocal )
    {
        if( isDataValid )
        {
            for( size_t iter = 0; iter < bufTrunkCount; iter++ )
            {
                ssize_t rxBytesCount = iio_buffer_refill( mRxBuf );

                if( rxBytesCount <= 0 )
                {
                    isDataValid = false;
                    printf( "\n Error refilling Rx buffer.\n" );
                    break;
                }

                uint8_t* dataBuf;

                for( dataBuf = static_cast< uint8_t* >( iio_buffer_first( mRxBuf, mRx0_I ) );
                     dataBuf < static_cast< uint8_t* >( iio_buffer_end( mRxBuf ) );
                     dataBuf += iio_buffer_step( mRxBuf ) )
                {
                    mRxDataVec.push_back( ( reinterpret_cast< int16_t* >( dataBuf ) )[0] );
                    mRxDataVec.push_back( ( reinterpret_cast< int16_t* >( dataBuf ) )[1] );
                }
            }

            printf( "done." );
        }

        if( isDataValid )
        {
            std::ofstream binFile( "file.dat", std::ios::binary );

            if( binFile.is_open() )
            {
                printf( "\n Dumping data to mass storage.. " );
                fflush( stdout );

                for( size_t i = 0; i < mRxDataVec.size(); i++ )
                {
                    binFile.write( reinterpret_cast< char* >( &mRxDataVec.at( i ) ), sizeof( int16_t ) );
                }

                binFile.close();
                printf( "done." );
            }
            else
            {
                printf( "\n Could not create file on mass storage." );
            }
        }
        else
        {
            printf( "\n Data was NOT dumped to mass storage." );
        }
    }
    else // dump from host
    {
        if( isDataValid )
        {
            std::ofstream binFile( "file.dat", std::ios::binary );

            if( binFile.is_open() )
            {
                for( size_t iter = 0; iter < bufTrunkCount; iter++ )
                {
                    ssize_t rxBytesCount = iio_buffer_refill( mRxBuf );

                    if( rxBytesCount <= 0 )
                    {
                        printf( "\n Error refilling Rx buffer.\n" );
                        break;
                    }

                    uint8_t* dataBuf;

                    for( dataBuf = static_cast< uint8_t* >( iio_buffer_first( mRxBuf, mRx0_I ) );
                         dataBuf < static_cast< uint8_t* >( iio_buffer_end( mRxBuf ) );
                         dataBuf += iio_buffer_step( mRxBuf ) )
                    {
                        int16_t dataI = ( reinterpret_cast< int16_t* >( dataBuf ) )[0];
                        int16_t dataQ = ( reinterpret_cast< int16_t* >( dataBuf ) )[1];

                        binFile.write( reinterpret_cast< char* >( &dataI ), sizeof( dataI ) );
                        binFile.write( reinterpret_cast< char* >( &dataQ ), sizeof( dataQ ) );
                    }
                }

                binFile.close();
            }
            else
            {
                printf( "\n Could not create file on mass storage." );
            }
        }
        else
        {
            printf( "\n Data was NOT dumped to mass storage." );
        }

        printf( "done." );
    }
}


//!************************************************************************
//! Write a byte to a register
//!
//! @returns true if the value can be written
//!************************************************************************
bool PlutoSdr::writeAd9361Register
    (
    const uint16_t aAddress,    //!< register address
    const uint8_t  aValue       //!< value to write
    )
{
    return ( 0 == iio_device_reg_write( mPhyDev, aAddress, aValue ) );
}
