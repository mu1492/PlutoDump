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
SysMgmt.h

This file contains the definitions for system management.
*/

#ifndef SysMgmt_h
#define SysMgmt_h

#include <cstdint>
#include <string>


//************************************************************************
// Class for the system management
//************************************************************************
class SysMgmt
{
    //************************************************************************
    // constants and types
    //************************************************************************
    public:
        typedef struct
        {
            uint32_t major;     //!< major field
            uint32_t minor;     //!< minor field
        }Version;


    //************************************************************************
    // functions
    //************************************************************************
    public:
        SysMgmt();

        static SysMgmt* getInstance();

        uint64_t getAvailableMemory();

        Version getLibIioVersion();

        int64_t getFreeSpace
            (
            const std::string   aPath   //!< path
            );


    //************************************************************************
    // variables
    //************************************************************************
    private:
        static SysMgmt*   sInstance;    //!< singleton
};

#endif // SysMgmt_h
