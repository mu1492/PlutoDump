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
main.cpp

This file contains the main source for PlutoSDR dump.
*/

#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <stdio.h>

#include "SysMgmt.h"
#include "PlutoSdr.h"


//!************************************************************************
//! Main application
//!
//! @returns: exit code
//!************************************************************************
int main
    (
    int     argc,   //!< argument count
    char*   argv[]  //!< argument vector
    )
{
    if( argc != 2 )
    {
        printf( "\nUsage is: %s [uri]"
                "\ne.g. %s local:"
                "\n     or"
                "\n     %s usb:1.23.4",
                argv[0], argv[0], argv[0] );

        printf( "\n\nFor more information please use:"
                "\n iio_info --help"
                "\n iio_info --scan" );
    }
    else
    {
        PlutoSdr pluto = PlutoSdr( argv[1] );

        if( !pluto.isInitialized() )
        {
            printf( "\n Error - cannot initialize PlutoSDR on %s", argv[1] );
        }
        else
        {
            SysMgmt* sysMgmtInstance = SysMgmt::getInstance();

            SysMgmt::Version libiioVer = sysMgmtInstance->getLibIioVersion();
            printf( "\n LibIIO v%d.%d", libiioVer.major, libiioVer.minor );

            printf( "\n PlutoSDR rev. %s", pluto.getHwRevision().c_str() );
            printf( "\n PlutoSDR hw serial %s", pluto.getHwSerial().c_str() );
            printf( "\n PlutoSDR model %s", pluto.getAd9361Model().c_str() );
            PlutoSdr::Version fwVer = pluto.getFwVersion();
            printf( "\n PlutoSDR fw %u.%u", fwVer.major, fwVer.minor );

            uint8_t ad9361ProductRev = 0;
            pluto.getAd9361ProductRev( ad9361ProductRev );
            printf( "\n AD9361 product rev. %u", ad9361ProductRev );

            pluto.disableAd9361TxFull();

            printf( "\n" );
            int retScanf = 0;
            PlutoSdr::IntegerRange crtRange;
            memset( &crtRange, 0, sizeof( crtRange ) );

            int64_t rxLoFreq = 101.5e6;
            crtRange = pluto.getRxLoFrequencyRange();

            do{
                printf( "\n\n Enter Rx LO freq, in [%.6lf ... %.6lf] [MHz] = ", crtRange.min / 1.e6, crtRange.max / 1.e6 );
                double rxLoFreqDbl = 0;
                retScanf = scanf( "%lf", &rxLoFreqDbl );
                rxLoFreq = rxLoFreqDbl * 1.e6;
            }while( rxLoFreq < crtRange.min || rxLoFreq > crtRange.max );

            pluto.setRxLoFrequency( rxLoFreq );
            pluto.getRxLoFrequency( rxLoFreq );
            printf( " Rx LO freq = %.6lf MHz", rxLoFreq / 1.e6 );

            int64_t rxSampFreq = 2.1e6;
            crtRange = pluto.getRxSamplingFrequencyRange();

            do{
                printf( "\n\n Enter Rx sampling freq, in [%.6lf ... %.6lf] [MHz] = ", crtRange.min / 1.e6, crtRange.max / 1.e6 );
                double rxSampFreqDbl = 0;
                retScanf = scanf( "%lf", &rxSampFreqDbl );
                rxSampFreq = rxSampFreqDbl * 1.e6;
            }while( rxSampFreq < crtRange.min || rxSampFreq > crtRange.max );

            pluto.setRxSamplingFrequency( rxSampFreq );
            pluto.getRxSamplingFrequency( rxSampFreq );
            printf( " Rx sampling freq = %.6lf MHz", rxSampFreq / 1.e6 );

            int64_t rxBw = 270e3;
            crtRange = pluto.getRxBandwidthRange();

            do{
                printf( "\n\n Enter Rx BW, in [%.3lf ... %.3lf] [kHz] = ", crtRange.min / 1.e3, crtRange.max / 1.e3 );
                double rxBwDbl = 0;
                retScanf = scanf( "%lf", &rxBwDbl );
                rxBw = rxBwDbl * 1.e3;
            }while( rxBw < crtRange.min || rxBw > crtRange.max );

            pluto.setRxBandwidth( rxBw );
            pluto.getRxBandwidth( rxBw );
            printf( " Rx BW = %.3lf kHz", rxBw / 1.e3 );


            printf( "\n" );
            int64_t gainDb = 20;
            pluto.setRxHwGain( gainDb );
            pluto.getRxHwGain( gainDb );
            printf( "\n Rx gain [dB] = %lld", gainDb );

            double rssi = 0;
            pluto.getRxRssi( rssi );
            printf( "\n Rx RSSI [dB] = %.2lf", rssi );

            pluto.setRxTrackingBbDc( true ); // *** always set -> true ***
            pluto.setRxTrackingRfDc( true );
            pluto.setRxTrackingQuadrature( true );

            pluto.startRxStreaming( sysMgmtInstance->getFreeSpace( "." ) );
        }
    }

    printf( "\n" );
    return 0;
}
