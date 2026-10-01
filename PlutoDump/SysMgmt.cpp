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
SysMgmt.cpp

This file contains the sources for system management.
*/

#include "SysMgmt.h"

#include <fstream>
#include <vector>

#include <iio.h>
#include <sys/statvfs.h>

SysMgmt* SysMgmt::sInstance = nullptr;


//!************************************************************************
//! Constructor
//!************************************************************************
SysMgmt::SysMgmt()
{
}


//!************************************************************************
//! Singleton
//!
//! @returns the instance of the object
//!************************************************************************
SysMgmt* SysMgmt::getInstance()
{
    if( !sInstance )
    {
        sInstance = new SysMgmt;
    }

    return sInstance;
}


//!************************************************************************
//! Get the available memory (RAM)
//!
//! @returns Available memory in bytes
//!************************************************************************
uint64_t SysMgmt::getAvailableMemory()
{
    uint64_t availMemoryBytes = 0;
    std::ifstream meminfo( "/proc/meminfo" );
    std::string line;

    while( std::getline( meminfo, line ) )
    {
        if( line.find( "MemAvailable:" ) != std::string::npos )
        {
            std::vector<std::string> vec;
            int start = 0;
            int end = 0;

            while( ( start = line.find_first_not_of( ' ', end ) ) != std::string::npos )
            {
                end = line.find( ' ', start );
                vec.push_back( line.substr( start, end - start ) );
            }

            for( int i = 0; i < vec.size(); i++ )
            {
                uint32_t kB = 0;

                try
                {
                    kB = std::stoul( vec.at( i ) );

                    if( kB )
                    {
                        availMemoryBytes = kB * 1024;
                    }
                }
                catch(...)
                {
                    kB = 0;
                }
            }

            break;
        }
    }

    return availMemoryBytes;
}


//!************************************************************************
//! Get the libiio version
//!
//! @returns The libiio version
//!************************************************************************
SysMgmt::Version SysMgmt::getLibIioVersion()
{
    Version ver;
    iio_library_get_version( &ver.major, &ver.minor, nullptr );
    return ver;
}


//!************************************************************************
//! Get the free space on a disk
//!
//! @returns Number of bytes, -1 at error
//!************************************************************************
int64_t SysMgmt::getFreeSpace
    (
    const std::string   aPath   //!< path
    )
{
    int64_t freeSpaceBytes = -1;
    struct statvfs fiData;

    if( statvfs( aPath.c_str(), &fiData ) >= 0 )
    {
        freeSpaceBytes = fiData.f_bsize * fiData.f_bfree;
    }

    return freeSpaceBytes;
}
