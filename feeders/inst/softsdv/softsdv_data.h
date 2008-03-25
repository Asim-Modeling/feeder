/**************************************************************************
 * Copyright (C) 2005-2006 Intel Corporation
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
 * @author Michael Adler
 * @brief SoftSDV architecture independent data passed from SoftSDV
 */

#ifndef _SOFTSDV_DATA_H
#define _SOFTSDV_DATA_H

#include "asim/memory_reference.h"

//-------------------------------------------------------------------------
//
// Structures here must have the same layout on both the SoftSDV and Asim
// sides.  They may be compiled in different modes (32 vs 64 bit).  Force
// the structure layout to be the same in each.
//
//-------------------------------------------------------------------------
#pragma pack(push,4)


//-------------------------------------------------------------------------
//
// Memory reference descriptor.
//
//-------------------------------------------------------------------------

//
// SOFTSDV_MEM_REF_SOURCE --
//
// Each memory reference is tagged by SoftSDV with the source of the
// access.
//
enum SOFTSDV_MEM_REF_SOURCE
{
    SDV_MEM_REF_UNKNOWN,
    SDV_MEM_REF_INSTRUCTION,    // Standard explicit reference in instruction
    SDV_MEM_REF_DESCRIPTOR,
    SDV_MEM_REF_TSS             // TSS (task state segment)
};


//
// ASIM_SOFTSDV_MEM_ACCESS_CLASS --
//
// Descriptor for a single access region.  SoftSDV has reasonably tight
// restrictions on the size of a region, making this data structure a
// bit simpler.  No access can be larger than 4k (x86 REP instructions
// are implemented as one iteration per execution of the instruction,
// which branches to itself).  Given the small maximum reference size,
// regions can touch at most 2 physical pages.
//

class ASIM_SOFTSDV_MEM_ACCESS_CLASS
{
  public:
    ASIM_SOFTSDV_MEM_ACCESS_CLASS()
    {
        ASSERTX(sizeof(valueStorageOffset) >= sizeof(PTR_SIZED_UINT));
        Reset();
    };

    ~ASIM_SOFTSDV_MEM_ACCESS_CLASS() {};

    // Set/Get the virtual address of the reference
    void SetVA(UINT64 newVA) { baseVA = newVA; };
    UINT64 GetVA(void) const { return baseVA; };

    // Set/Get the physical address of the Nth page.
    void SetPA(UINT64 newPA, UINT32 pageIdx);
    UINT64 GetNPages(void) const { return nPages; };
    UINT64 GetPA(UINT32 pageIdx) const { ASSERTX(pageIdx < nPages); return pages[pageIdx]; };

    // Helper function so callers can compute PAs without page offsets
    UINT64 PageSize(void) const { return PAGESIZE; };
    UINT64 PageMask(void) const { return ~(PAGESIZE - 1); };

    // Does the reference contain the VA range?
    bool ContainsVA(const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion) const;

    // Translate from a physical to a virtual range?  If the physical
    // range is not found the size of the returned virtual reference is 0.
    MEMORY_VIRTUAL_REFERENCE_CLASS VTranslate(UINT64 pa, UINT32 size) const;

    // Get the physical address of of a portion of the reference.
    // The output region (vNext) will hold the size of the remainder
    // that is on a different physical page.
    UINT64 PTranslate(
        const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
        MEMORY_VIRTUAL_REFERENCE_CLASS& vNext) const;
    UINT64 PTranslate(UINT64 va) const;

    // Set/Get the size of the reference
    UINT32 GetSize(void) const { return nBytes; };
    void SetSize(UINT64 size);
    // Used when the virtual size is allowed to span more than 2 pages.
    //  This is primarly used when the region is describing merged REP
    //  instructions.
    void SetSize_NOCHECK(UINT64 size);

    // Set/Get the source of the reference
    void SetRefSource(SOFTSDV_MEM_REF_SOURCE s);
    SOFTSDV_MEM_REF_SOURCE GetRefSource(void) const { return refSource; };

    // Set a portion of the value, offset bytes from the beginning of the ref.
    void SetValue(const void *v, UINT32 size, UINT32 offset);
    bool HasValue(void) const { return valueSet; };
    const unsigned char *GetValue(void) const;
    unsigned char *GetValue(void);

    // Allocate storage for the value.  Storage is allocated outside this
    // class because the maximum individual reference size is large (about
    // 512 bytes) but the sum of the reference sizes for an instruction
    // is only a bit larger than the maximum individual reference size.
    void AllocValueStorage(void *storage);

    void Reset(void);

  private:
    void *GetValueStorage(void) const;

    UINT64 baseVA;
    UINT64 pages[2];

    //
    // A relative pointer to the storage for the value.  Use relative pointers
    // since the ring buffer may be allocated at different addresses in each
    // process.
    //
    // Use a UINT64 so the data structure is the same size on 32 and 64 bit
    // machines in case data is shared across varying sizes.
    //
    UINT64 valueStorageOffset;

    UINT32 nPages;
    UINT32 nBytes;

    SOFTSDV_MEM_REF_SOURCE refSource : 8;

    bool valueSet;

    enum
    {
        PAGESIZE = 4096         // Assumed page size.
    };
}
__attribute__((aligned));


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::Reset(void)
{
    baseVA = 0;
    valueStorageOffset = 0;
    nPages = 0;
    nBytes = 0;
    refSource = SDV_MEM_REF_UNKNOWN;
    valueSet = false;
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::SetPA(UINT64 newPA, UINT32 pageIdx)
{
    ASSERTX(pageIdx < 2);

    pages[pageIdx] = newPA & PageMask();

    if (pageIdx >= nPages)
    {
        nPages = pageIdx + 1;
    }
};


inline bool
ASIM_SOFTSDV_MEM_ACCESS_CLASS::ContainsVA(
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion) const
{
    UINT64 va = vRegion.GetVA();
    UINT64 size = vRegion.GetNBytes();

    return (va >= baseVA) && (va + size <= baseVA + nBytes);
};


inline MEMORY_VIRTUAL_REFERENCE_CLASS
ASIM_SOFTSDV_MEM_ACCESS_CLASS::VTranslate(
    UINT64 pa,
    UINT32 size) const
{
    UINT64 maskedPA = pa & PageMask();

    for (UINT32 i = 0; i < nPages; i++)
    {
        if (maskedPA == pages[i])
        {
            //
            // Found the right page.  The equivalent virtual address is
            // computed with 3 terms:  the VA of the first page in the
            // virtual region, the offset to this physical page, and
            // the offset within the page.
            //
            UINT64 va = (baseVA & PageMask()) + i * PageSize() + (pa & ~PageMask());
            MEMORY_VIRTUAL_REFERENCE_CLASS vRef(va, size);
            if (ContainsVA(vRef))
            {
                return vRef;
            }
            else
            {
                return MEMORY_VIRTUAL_REFERENCE_CLASS();
            }
        }
    }

    return MEMORY_VIRTUAL_REFERENCE_CLASS();
};


inline UINT64
ASIM_SOFTSDV_MEM_ACCESS_CLASS::PTranslate(
    const MEMORY_VIRTUAL_REFERENCE_CLASS& vRegion,
    MEMORY_VIRTUAL_REFERENCE_CLASS& vNext) const
{
    if (nPages == 0)
    {
        // No translation available
        vNext = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
        return 0;
    }

    UINT64 va = vRegion.GetVA();
    UINT64 size = vRegion.GetNBytes();

    ASSERTX((va >= baseVA) &&
            (va + size <= baseVA + nBytes) &&
            (nPages > 0));

    UINT64 offset = va - baseVA;

    UINT64 pageMask = PageMask();

    // If only one page in the reference the answer is easy...
    if (nPages == 1)
    {
        vNext = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
        return pages[0] + (va & ~pageMask);
    }

    if ((va & pageMask) != (baseVA & pageMask))
    {
        // The requested VA begins on the second page
        vNext = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
        return pages[1] + (va & ~pageMask);
    }

    // Reference has two pages and the requested VA is on the first page.
    // Limit the size of the reference to the first page.
    UINT32 maxSize = PAGESIZE - (va & ~pageMask);
    if (maxSize < size)
    {
        vNext = MEMORY_VIRTUAL_REFERENCE_CLASS(va + maxSize, size - maxSize);
    }
    else
    {
        vNext = MEMORY_VIRTUAL_REFERENCE_CLASS(0, 0);
    }
    return pages[0] + (va & ~pageMask);
};


inline UINT64
ASIM_SOFTSDV_MEM_ACCESS_CLASS::PTranslate(UINT64 va) const
{
    MEMORY_VIRTUAL_REFERENCE_CLASS vRegion(va, 1);
    MEMORY_VIRTUAL_REFERENCE_CLASS vNext(0, 0);

    return PTranslate(vRegion, vNext);
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::SetSize(UINT64 size)
{
    ASSERTX(size < PAGESIZE);
    nBytes = size;
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::SetSize_NOCHECK(UINT64 size)
{
    nBytes = size;
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::SetRefSource(SOFTSDV_MEM_REF_SOURCE s)
{
    ASSERTX(refSource == SDV_MEM_REF_UNKNOWN);
    refSource = s;
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::AllocValueStorage(
    void *storage)
{
    ASSERTX((nBytes > 0) && (valueStorageOffset == 0));

    // Use relative pointer in case the shared memory buffer is mapped at
    // different locations in the Asim and SoftSDV processes.
    valueStorageOffset = PTR_SIZED_UINT(storage) -
                         PTR_SIZED_UINT(&valueStorageOffset);
};


inline void *
ASIM_SOFTSDV_MEM_ACCESS_CLASS::GetValueStorage(void) const
{
    ASSERTX(valueStorageOffset != 0);

    // Use relative pointer in case the shared memory buffer is mapped at
    // different locations in the Asim and SoftSDV processes.
    return (void *)(PTR_SIZED_UINT(valueStorageOffset) +
                    PTR_SIZED_UINT(&valueStorageOffset));
};


inline void
ASIM_SOFTSDV_MEM_ACCESS_CLASS::SetValue(
    const void *v,
    UINT32 size,
    UINT32 offset)
{
    ASSERTX(size + offset <= nBytes);
    valueSet = true;
    UINT8 *storage = (UINT8 *)GetValueStorage();
    memcpy(&storage[offset], v, size);
};


inline const unsigned char *
ASIM_SOFTSDV_MEM_ACCESS_CLASS::GetValue(void) const
{
    ASSERTX(HasValue());
    return (unsigned char *)GetValueStorage();
};


inline unsigned char *
ASIM_SOFTSDV_MEM_ACCESS_CLASS::GetValue(void)
{
    ASSERTX(HasValue());
    return (unsigned char *)GetValueStorage();
};



//-------------------------------------------------------------------------
//
// ASIM_SOFTSDV_MEM_REFS_CLASS --
//
// All memory references in an instruction.  All ISA-specific instructions
// are derived from this class to describe an individual instruction's
// references.  Parameters contorl the maximum number of load and store
// references per instruction and the maximum number of bytes in each
// reference.
//
//-------------------------------------------------------------------------

template<int maxLoads, int maxStores, int maxRefSize>
class ASIM_SOFTSDV_MEM_REFS_CLASS
{
  public:
    void Reset(void);

    typedef class ASIM_SOFTSDV_MEM_ACCESS_CLASS *ASIM_SOFTSDV_MEM_ACCESS;
    typedef const class ASIM_SOFTSDV_MEM_ACCESS_CLASS *CONST_ASIM_SOFTSDV_MEM_ACCESS;

    // ActiveLoad -- called on the SoftSDV side to get the handle for a
    //    load reference.  The call returns a pointer to the reference
    //    descriptor and notes that the descriptor is active.
    ASIM_SOFTSDV_MEM_ACCESS ActivateLoad(UINT32 idx, SOFTSDV_MEM_REF_SOURCE refSrc);

    // DeactivateLoad -- used only on the SoftSDV side when it merges
    //    two loads into a single reference.  Only the load with the higest
    //    index may be deactivated.  Loads may only be deactivated if they
    //    have no value backing storage allocated.
    void DeactivateLoad(UINT32 idx);

    // NRefs / GetRef -- a method for getting either a load or a store
    //    reference, specifying to get loads or stores using a bool passed
    //    as a parameter.  This allows for writing code that handles both
    //    types without special copies for loads and stores.
    UINT32 NRefs(bool getLoad) const;
    ASIM_SOFTSDV_MEM_ACCESS GetRef(bool getLoad, UINT32 idx);
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetRef(bool getLoad, UINT32 idx) const;

    // NLoads / GetLoad -- used on the Asim side to figure out how
    //    many load descriptors are in use and read them.
    UINT32 NLoads(void) const { return nActiveLoads; };
    ASIM_SOFTSDV_MEM_ACCESS GetLoad(UINT32 idx);
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetLoad(UINT32 idx) const;

    // AllocLoadStorage --
    //     Most memory references are to small regions.  A few may be to large
    //     ones.  The largest reference is too large to have backing storage for
    //     the loaded value stored along with the load descriptor.  Once a virtual
    //     region is chosen, call AllocLoadStorage() to allocate the space for
    //     storing the value.
    bool AllocLoadStorage(UINT32 idx);

    // Equivalent functions for stores.
    ASIM_SOFTSDV_MEM_ACCESS ActivateStore(UINT32 idx, SOFTSDV_MEM_REF_SOURCE refSrc);
    UINT32 NStores(void) const { return nActiveStores; };
    ASIM_SOFTSDV_MEM_ACCESS GetStore(UINT32 idx);
    const ASIM_SOFTSDV_MEM_ACCESS_CLASS *GetStore(UINT32 idx) const;
    bool AllocStoreStorage(UINT32 idx);

    ASIM_SOFTSDV_MEM_REFS_CLASS& operator=(const ASIM_SOFTSDV_MEM_REFS_CLASS& src);

  private:
    ASIM_SOFTSDV_MEM_ACCESS_CLASS loads[maxLoads];
    ASIM_SOFTSDV_MEM_ACCESS_CLASS stores[maxStores];

    UINT32 nActiveLoads;
    UINT32 nActiveStores;

    UINT32 refUsed;
    UINT8 refData[maxRefSize];
}
__attribute__((aligned));


template<int maxLoads, int maxStores, int maxRefSize>
inline UINT32
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::NRefs(bool getLoad) const
{
    return getLoad ? NLoads() : NStores();
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetRef(bool getLoad, UINT32 idx)
{
    return getLoad ? GetLoad(idx) : GetStore(idx);
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::CONST_ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetRef(bool getLoad, UINT32 idx) const
{
    return getLoad ? GetLoad(idx) : GetStore(idx);
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetLoad(UINT32 idx)
{
    ASSERTX(idx < nActiveLoads);
    return &loads[idx];
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::CONST_ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetLoad(UINT32 idx) const
{
    ASSERTX(idx < nActiveLoads);
    return &loads[idx];
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ActivateLoad(
    UINT32 idx,
    SOFTSDV_MEM_REF_SOURCE refSrc)
{
    ASSERTX(idx < maxLoads);
    ASSERT(idx == nActiveLoads, "Must activate memory refs in order");

    nActiveLoads += 1;
    loads[idx].Reset();
    loads[idx].SetRefSource(refSrc);
    return &loads[idx];
};

template<int maxLoads, int maxStores, int maxRefSize>
inline void
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::DeactivateLoad(UINT32 idx)
{
    ASSERT(idx == nActiveLoads - 1, "May only deactivate load with largest index");
    ASSERTX(! loads[idx].HasValue());

    nActiveLoads -= 1;
    loads[idx].Reset();
};

template<int maxLoads, int maxStores, int maxRefSize>
inline bool
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::AllocLoadStorage(UINT32 idx)
{
    ASSERTX(idx < nActiveLoads);

    UINT32 size = loads[idx].GetSize();
    ASSERTX(size != 0);
    
    if (refUsed + size > sizeof(refData))
    {
        return false;
    }

    loads[idx].AllocValueStorage(&refData[refUsed]);
    refUsed += size;
    return true;
};


template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetStore(UINT32 idx)
{
    ASSERTX(idx < nActiveStores);
    return &stores[idx];
};

template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::CONST_ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::GetStore(UINT32 idx) const
{
    ASSERTX(idx < nActiveStores);
    return &stores[idx];
};

template<int maxLoads, int maxStores, int maxRefSize>
inline typename ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ASIM_SOFTSDV_MEM_ACCESS
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::ActivateStore(
    UINT32 idx,
    SOFTSDV_MEM_REF_SOURCE refSrc)
{
    ASSERTX(idx < maxStores);
    ASSERT(idx == nActiveStores, "Must activate memory refs in order");

    nActiveStores += 1;
    stores[idx].Reset();
    stores[idx].SetRefSource(refSrc);
    return &stores[idx];
};

template<int maxLoads, int maxStores, int maxRefSize>
inline bool
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::AllocStoreStorage(UINT32 idx)
{
    ASSERTX(idx < nActiveStores);

    UINT32 size = stores[idx].GetSize();
    ASSERTX(size != 0);
    
    if (refUsed + size > sizeof(refData))
    {
        return false;
    }

    stores[idx].AllocValueStorage(&refData[refUsed]);
    refUsed += size;
    return true;
};


template<int maxLoads, int maxStores, int maxRefSize>
inline void
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::Reset(void)
{
    //
    // Just reset the counters of active references.  Delay the Reset() call
    // of the reference descriptors until they are actually used, since
    // often some of the descriptors are unused in an instruction.
    //
    nActiveLoads = 0;
    nActiveStores = 0;
    refUsed = 0;
}


template<int maxLoads, int maxStores, int maxRefSize>
inline ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize> &
ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>::operator=(
    const ASIM_SOFTSDV_MEM_REFS_CLASS<maxLoads, maxStores, maxRefSize>& src)
{
    //
    // Smart copy to copy only active data...
    //

    nActiveLoads = src.nActiveLoads;
    for (unsigned int i = 0; i < nActiveLoads; i++)
    {
        loads[i] = src.loads[i];
    }

    nActiveStores = src.nActiveStores;
    for (unsigned int i = 0; i < nActiveStores; i++)
    {
        stores[i] = src.stores[i];
    }

    refUsed = src.refUsed;
    if (refUsed != 0)
    {
        memcpy(refData, src.refData, refUsed);
    }

    return *this;
}



//-------------------------------------------------------------------------
//
// Register descriptor.
//
//-------------------------------------------------------------------------

//
// The values of all architecturally visible registers are passed every
// time a group of registers changes every time there is unusual control
// flow (e.g. interrupts).  During initialization Asim and SoftSDV
// agree on the placement of registers within this array.
//

typedef class ASIM_SOFTSDV_REG_INFO_CLASS *ASIM_SOFTSDV_REG_INFO;

class ASIM_SOFTSDV_REG_INFO_CLASS
{
  public:
    ASIM_SOFTSDV_REG_INFO_CLASS() { Reset(); };

    void Reset(void) { uid = 0; nRegisters = 0; };

    // Number of registers available
    UINT32 NRegisters(void) const { return nRegisters; };

    // Set/Get register values
    void AddRegister(const ARCH_REGISTER_CLASS &reg);
    const ARCH_REGISTER_CLASS* GetRegister(UINT32 n) const;

    // Each register state passed to Asim has a unique identifier...
    void SetUid(UINT64 newUid) { uid = newUid; };
    UINT64 GetUid(void) const { return uid; };

    // Registers are for SoftSDV CPU number...
    void SetCpuNum(UINT64 newCpuNum) { cpuNum = newCpuNum; };
    UINT64 GetCpuNum(void) const { return cpuNum; };

    //
    // Smart copy operator avoids copying unused portions of the value array.
    //
    ASIM_SOFTSDV_REG_INFO_CLASS& operator=(const ASIM_SOFTSDV_REG_INFO_CLASS& src);

  private:
    ARCH_REGISTER_CLASS registers[MAX_SOFTSDV_REGS];

    UINT64 uid;
    UINT64 cpuNum;

    UINT32 nRegisters;
}
__attribute__((aligned));


inline void
ASIM_SOFTSDV_REG_INFO_CLASS::AddRegister(const ARCH_REGISTER_CLASS &reg)
{
    ASSERTX(nRegisters < MAX_SOFTSDV_REGS);
    registers[nRegisters] = reg;
    nRegisters += 1;
};


inline const ARCH_REGISTER_CLASS*
ASIM_SOFTSDV_REG_INFO_CLASS::GetRegister(UINT32 n) const
{
    ASSERTX(n < NRegisters());
    return &registers[n];
};


inline ASIM_SOFTSDV_REG_INFO_CLASS &
ASIM_SOFTSDV_REG_INFO_CLASS::operator=(const ASIM_SOFTSDV_REG_INFO_CLASS& src)
{
    uid = src.uid;
    cpuNum = src.cpuNum;
    nRegisters = src.nRegisters;

    for (UINT32 i = 0; i < nRegisters; i++)
    {
        registers[i] = src.registers[i];
    }

    return *this;
};

#pragma pack(pop)

#endif // _SOFTSDV_DATA_H
