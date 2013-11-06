/**************************************************************************
 * Copyright (C) 2004-2006 Intel Corporation
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 * 
 **************************************************************************/

/**
 * @file
 * @author Chris Weaver
 * @brief Simple class for describing system events
 */
#ifndef _FEEDERSYSTEMEVENT_
#define _FEEDERSYSTEMEVENT_
#include <string>
#include "asim/syntax.h"

enum FEEDER_SYSTEM_EVENT_TYPES
{
    NULL_FEEDER_SYSTEM_EVENT,               // MUST BE FIRST!!!
    DMA_FEEDER_SYSTEM_EVENT,
    IO_FEEDER_SYSTEM_EVENT,
    IRQ_FEEDER_SYSTEM_EVENT,
    MSR_FEEDER_SYSTEM_EVENT,
    REG_FEEDER_SYSTEM_EVENT,
    SYSTEM_EVENT_LAST                       // MUST BE LAST!!!
};

class FEEDER_SYSTEM_EVENT_CLASS
{
private:
    FEEDER_SYSTEM_EVENT_TYPES  type;

    // For DMA and IO
    UINT64                     address;
    UINT64                     size;

    // For interrupts
    IADDR_CLASS                irq_pc;

    // For register injection
    std::string                regName;
    UINT64                     regVal;

public:
    // Null constructor
    FEEDER_SYSTEM_EVENT_CLASS() :
        type( NULL_FEEDER_SYSTEM_EVENT )
    {
    }

    FEEDER_SYSTEM_EVENT_CLASS( FEEDER_SYSTEM_EVENT_TYPES type ) :
        type( type )
    {
    }

    // Constructor for DMA and IO
    FEEDER_SYSTEM_EVENT_CLASS( FEEDER_SYSTEM_EVENT_TYPES type, UINT64 address, UINT64 size ) :
        type( type ),
        address( address ),
        size( size )
    {
    }

    // Construction for irq
    FEEDER_SYSTEM_EVENT_CLASS( FEEDER_SYSTEM_EVENT_TYPES type, IADDR_CLASS irq_pc ) :
        type( type ),
        irq_pc( irq_pc )
    {
    }

    // Construction for register injection
    FEEDER_SYSTEM_EVENT_CLASS( FEEDER_SYSTEM_EVENT_TYPES type, std::string regName, UINT64 regVal ) :
        type( type ),
        regName( regName ),
        regVal( regVal )
    {
    }

    ~FEEDER_SYSTEM_EVENT_CLASS()
    {
    }

    inline UINT64                    GetAddress() const;
    inline UINT64                    GetSize()    const;
    inline FEEDER_SYSTEM_EVENT_TYPES GetType()    const;
    inline IADDR_CLASS               GetIrqPC()   const;
    inline std::string               GetRegName() const;
    inline UINT64                    GetRegVal()  const;
};


// Accessor for dma and io memory addresses
inline UINT64
FEEDER_SYSTEM_EVENT_CLASS::GetAddress() const
{
    ASSERT( (type == DMA_FEEDER_SYSTEM_EVENT || type == IO_FEEDER_SYSTEM_EVENT), "No size for non IO/DMA sys events" );
    return address;
}

// Accessor for dma and io memory size
inline UINT64
FEEDER_SYSTEM_EVENT_CLASS::GetSize() const
{
    ASSERT( (type == DMA_FEEDER_SYSTEM_EVENT || type == IO_FEEDER_SYSTEM_EVENT), "No size for non IO/DMA sys events" );
    return size;
}

inline FEEDER_SYSTEM_EVENT_TYPES
FEEDER_SYSTEM_EVENT_CLASS::GetType() const
{
    return type;
}

inline IADDR_CLASS
FEEDER_SYSTEM_EVENT_CLASS::GetIrqPC() const
{
    ASSERT( (type == IRQ_FEEDER_SYSTEM_EVENT), "IrqPc only available for interrupts" );
    return irq_pc;
}

inline std::string
FEEDER_SYSTEM_EVENT_CLASS::GetRegName() const
{
    ASSERT( (type == REG_FEEDER_SYSTEM_EVENT), "RegName only available for reg injections" );
    return regName;
}

inline UINT64
FEEDER_SYSTEM_EVENT_CLASS::GetRegVal() const
{
    ASSERT( (type == REG_FEEDER_SYSTEM_EVENT), "RegVal only available for reg injections" );
    return regVal;
}

#endif // ifndef _FEEDERSYSTEMEVENT_
