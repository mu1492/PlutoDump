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
PlutoSdr.h

This file contains the definitions for the Pluto SDR.
*/

#ifndef PlutoSdr_h
#define PlutoSdr_h

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <iio.h>


//************************************************************************
// Class for handling Pluto SDR
//************************************************************************
class PlutoSdr
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef struct
        {
            uint8_t major;              //!< major field
            uint8_t minor;              //!< minor field
        }Version;

        typedef struct
        {
            std::string hwRev;          //!< hardware revision, e.g. "B", "C"
            std::string hwSerial;       //!< hardware serial, a 34 hex digits string
            Version     fwVersion;      //!< firmware version, e.g. 0.38
            std::string ad936xModel;    //!< AD936x model, e.g. "ad9361", "ad9364"
            std::string uri;            //!< URI
        }ContextAttributes;

        typedef struct
        {
            int64_t min;                //!< lower limit
            int64_t step;               //!< step
            int64_t max;                //!< upper limit
        }IntegerRange;

        typedef enum : uint16_t
        {
            AD9361_REG_TX_EN_FILTER_CTRL    = 0x0002,
            AD9361_REG_RX_EN_FILTER_CTRL    = 0x0003,
            AD9361_REG_INPUT_SELECT         = 0x0004,
            AD9361_REG_PRODUCT_ID           = 0x0037,
            AD9361_REG_TX_SYNTH_PWRDOWN     = 0x0051,
            AD9361_REG_TX_ANALOG_PWRDOWN    = 0x0056,
            AD9361_REG_ANALOG_PWRDOWN       = 0x0057
        }Ad9361_Reg;

    private:
        const std::string AD9361_PHY_DEV_STR = "ad9361-phy";            //!< AD9361 phy device string
        const std::string AD9361_STREAMING_DEV_STR = "cf-ad9361-lpc";   //!< AD9361 streaming device string
        const std::string XADC_DEV_STR = "xadc";                        //!< xADC device string

        static const size_t RX_BUF_SAMPLES_COUNT = 2 * 1024 * 1024;     //!< number of samples in Rx buffer


    //************************************************************************
    // functions
    //************************************************************************
    public:
        PlutoSdr
            (
            std::string aUri            //!< URI
            );

        ~PlutoSdr();

        bool disableAd9361TxFull();

        std::string getAd9361Model() const;

        bool getAd9361ProductRev
            (
            uint8_t& aProductRev        //!< product revision
            );

        bool getAd9361Temperature
            (
            double& aTemperature        //!< temperature [C]
            ) const;


        Version getFwVersion() const;

        std::string getHwRevision() const;

        std::string getHwSerial() const;


        bool getRxBandwidth
            (
            int64_t& aFrequency         //!< frequency [Hz]
            );

        bool getRxBandwidthParams();

        IntegerRange getRxBandwidthRange() const;


        bool getRxGainControlModeParams();


        bool getRxHwGain
            (
            int64_t& aHwGainDb          //!< hardware gain [dB]
            );

        bool getRxHwGainParams();


        bool getRxLoFrequency
            (
            int64_t& aFrequency         //!< frequency [Hz]
            );

        bool getRxLoFrequencyParams();

        IntegerRange getRxLoFrequencyRange() const;


        bool getRxPortSelect
            (
            std::string& aRfPort        //!< RF port
            ) const;

        bool getRxPortSelectParams();


        bool getRxRssi
            (
            double& aRssi               //!< RSSI [dB]
            );


        bool getRxSamplingFrequency
            (
            int64_t& aFrequency         //!< sampling frequency [Hz]
            );

        bool getRxSamplingFrequencyParams();

        IntegerRange getRxSamplingFrequencyRange() const;


        bool getRxTrackingBbDc
            (
            bool& aEnable               //!< true if tracking enabled
            );

        bool getRxTrackingQuadrature
            (
            bool& aEnable               //!< true if tracking enabled
            );

        bool getRxTrackingRfDc
            (
            bool& aEnable               //!< true if tracking enabled
            );


        bool getTxLoPower
            (
            bool& aEnable               //!< true if power is enabled
            ) const;


        bool getXadcTemperature
            (
            double& aTemperature        //!< temperature [C]
            ) const;


        bool isInitialized() const;


        bool readAd9361Register
            (
            const uint16_t aAddress,    //!< register address
            uint8_t&       aValue       //!< read value
            );


        bool setRxBandwidth
            (
            const int64_t aBandwidth    //!< bandwidth [Hz]
            );

        bool setRxGainControlMode
            (
            const std::string aMode     //!< gain control mode
            );

        bool setRxHwGain
            (
            const int64_t& aHwGainDb    //!< hardware gain [dB]
            );

        bool setRxLoFrequency
            (
            const int64_t aFrequency    //!< frequency [Hz]
            );

        bool setRxPortSelect
            (
            const std::string aRfPort   //!< RF port
            );

        bool setRxSamplingFrequency
            (
            const int64_t aFrequency    //!< frequency [Hz]
            );


        bool setRxTrackingBbDc
            (
            const bool aEnable          //!< true for enabling tracking
            );

        bool setRxTrackingQuadrature
            (
            const bool aEnable          //!< true for enabling tracking
            );

        bool setRxTrackingRfDc
            (
            const bool aEnable          //!< true for enabling tracking
            );


        bool setTxLoPower
            (
            const bool aEnable          //!< true for enabling power
            );


        void startRxStreaming
            (
            const int64_t aFreeSpaceBytes  //!< buffer space [B]
            );


        bool writeAd9361Register
            (
            const uint16_t aAddress,    //!< register address
            const uint8_t  aValue       //!< value to write
            );

    private:
        bool extractContextAttributes();

        bool extractDouble
            (
            const std::string           aString,    //!< string
            const size_t                aIndex,     //!< substring index
            double&                     aValue      //!< value
            );

        Version extractFwVersion
            (
            const std::string           aString     //!< string
            );

        std::string extractHwRev
            (
            const std::string           aString     //!< string
            );

        bool extractInteger
            (
            const std::string           aString,    //!< string
            const size_t                aIndex,     //!< substring index
            int64_t&                    aValue      //!< value
            );

        bool extractIntegerRange
            (
            std::string                 aString,    //!< string
            PlutoSdr::IntegerRange&     aRange      //!< range
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        std::string             mUri;                               //!< URI string

        struct iio_context*     mIioContext;                        //!< IIO context

        std::map<std::string, std::string> mCtxAttributesMap;       //!< map with context attributes
        ContextAttributes       mCtxAttributes;                     //!< context attributes

        struct iio_device*      mPhyDev;                            //!< AD9361 phy device
        struct iio_device*      mRxDev;                             //!< AD9361 Rx streaming device
        struct iio_device*      mXadcDev;                           //!< xADC device

        struct iio_channel*     mRxPhyChan;                         //!< AD9361 Rx phy channel
        struct iio_channel*     mRxLoChan;                          //!< AD9361 Rx LO channel
        struct iio_channel*     mTxLoChan;                          //!< AD9361 Tx LO channel

        struct iio_channel*     mTemperatureChan;                   //!< AD9361 temperature channel
        struct iio_channel*     mXadcTemperatureChan;               //!< xADC temperature channel

        struct iio_channel*     mRx0_I;                             //!< Rx I channel
        struct iio_channel*     mRx0_Q;                             //!< Rx Q channel

        struct iio_buffer*      mRxBuf;                             //!< Rx data buffer

        std::vector<std::string> mRxGainControlModeVec;             //!< vector with Rx gain control modes

        std::vector<std::string> mRxPortSelectVec;                  //!< vector with Rx selected ports

        IntegerRange            mRxBandwidthParams;                 //!< Rx bandwidth parameters
        int64_t                 mRxBandwidth;                       //!< Rx bandwidth [Hz]

        IntegerRange            mRxSamplingFrequencyParams;         //!< Rx sampling frequency parameters
        int64_t                 mRxSamplingFrequency;               //!< Rx sampling frequency [Hz]

        IntegerRange            mRxLoFrequencyParams;               //!< Rx LO frequency parameters
        int64_t                 mRxLoFrequency;                     //!< Rx LO frequency [Hz]

        IntegerRange            mRxHwGainDbParams;                  //!< Rx hardware gain parameters
        int64_t                 mRxHwGainDb;                        //!< Rx hardware gain [dB]

        double                  mRxRssi;                            //!< Rx RSSI [dB]

        bool                    mRxTrackingBbDcEnabled;             //!< Rx baseband DC tracking status
        bool                    mRxTrackingQuadratureEnabled;       //!< Rx quadrature (IQ) tracking status
        bool                    mRxTrackingRfDcEnabled;             //!< Rx RF DC tracking status

        bool                    mInitialized;                       //!< PlutoSDR initialization status

        std::vector<int16_t>    mRxDataVec;                         //!< vector with Rx data
};

#endif // PlutoSdr_h
