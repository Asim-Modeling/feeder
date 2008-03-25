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


enum FEEDER_SYSTEM_EVENT_TYPES
{
    NULL_FEEDER_SYSTEM_EVENT,               // MUST BE FIRST!!!
    DMA_FEEDER_SYSTEM_EVENT,
    IO_FEEDER_SYSTEM_EVENT,
    IRQ_FEEDER_SYSTEM_EVENT,
    MSR_FEEDER_SYSTEM_EVENT,
    SYSTEM_EVENT_LAST                       // MUST BE LAST!!!
};

class FEEDER_SYSTEM_EVENT_CLASS
{
  private:
    FEEDER_SYSTEM_EVENT_TYPES type;

    //for DMA and IO
    UINT64 address;
    UINT64 size;

    //for interrupts
    IADDR_CLASS irq_pc;

  public:
    //null constructor
    FEEDER_SYSTEM_EVENT_CLASS()
    {
        this->type=NULL_FEEDER_SYSTEM_EVENT;
    }

    ~FEEDER_SYSTEM_EVENT_CLASS() {}
    //constructor for DMA and IO
    FEEDER_SYSTEM_EVENT_CLASS(FEEDER_SYSTEM_EVENT_TYPES type, UINT64 address, UINT64 size)
    {
        this->type=type;
        this->size=size;
        this->address=address;
    }

    //construction for irq
    FEEDER_SYSTEM_EVENT_CLASS(FEEDER_SYSTEM_EVENT_TYPES type, IADDR_CLASS irq_pc)
    {
        this->type=type;
        this->irq_pc=irq_pc;
    }

    inline UINT64 GetAddress() const;
    inline UINT64 GetSize() const;
    inline FEEDER_SYSTEM_EVENT_TYPES GetType() const;
    inline IADDR_CLASS GetIrqPC() const;

};


//accessor for dma and io memory addresses
inline UINT64 
FEEDER_SYSTEM_EVENT_CLASS::GetAddress() const
{
    ASSERT((type==DMA_FEEDER_SYSTEM_EVENT || type== IO_FEEDER_SYSTEM_EVENT),"No size for non IO/DMA sys events");
    return address;
}

//accessor for dma and io memory size
inline UINT64
FEEDER_SYSTEM_EVENT_CLASS::GetSize() const
{
    ASSERT((type==DMA_FEEDER_SYSTEM_EVENT || type== IO_FEEDER_SYSTEM_EVENT),"No size for non IO/DMA sys events");
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
    ASSERT((type==IRQ_FEEDER_SYSTEM_EVENT), "IrqPc only availabe for interrupts");
    return irq_pc;
}

#endif
