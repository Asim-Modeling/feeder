/*
 * Copyright (C) 2003-2006 Intel Corporation
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
 */

/* ipf-bundle-builder.h */
/* Mark Charney   <mark.charney@intel.com> */
/*$Id: ipf-bundle-builder.h 799 2006-10-31 12:27:52Z cjbeckma $ */


#ifndef _IPF_BUNDLE_BUILDER_H_
# define _IPF_BUNDLE_BUILDER_H_

#include <string>
#include "asim/syntax.h"
#include "synthetic-feeder.h"
#include "synth-types.h"
#include "synth-debug.h"
#include "synth-memop.h"
#include "asim/provides/ipf_raw_instruction.h"
#include "synth-thread.h"
#include "synth-params.h"

////////////////////////////////////////////////////////////////////////////
void
HandleMemoryRW(SYNTH_THREAD  thread,
               IPF_TEMPLATE_ENUM tmplt,
               SYNTH_PARAMS params);

SYNTH_INST
make_indirect_branch(SYNTH_THREAD thread,
                     UINT64      pc,
                     UINT64      target);

void
HandleBranch(SYNTH_THREAD  thread);

void
make_relative_branch(SYNTH_THREAD thread,
                     UINT64      pc,
                     UINT64      target);

bool //* returns true iff generated a long branch */
set_branch( IPF_RAW_BUNDLE& bundle, /* output */
            INT64           immed,
            bool            conditional );

void
set_branch_indirect( IPF_RAW_BUNDLE& bundle, /* output */
                     bool            conditional );

void
HandleJunkOps(SYNTH_THREAD  thread,
              IPF_RAW_BUNDLE* bundle,
              const IPF_TEMPLATE_ENUM tmplt,
              const SYNTH_IPF_ENUM* ipf_syllables);

void
set_bundle_bits(IPF_RAW_BUNDLE* bundle,
                IADDR_CLASS     pc,
                ASIM_INST       inst);

////////////////////////////////////////////////////////////////////////////

typedef enum
{
    SYNTH_BUNDLE_EVEN,
    SYNTH_BUNDLE_ODD,
    SYNTH_BUNDLE_MEMOP,
    SYNTH_BUNDLE_INVALID
} SYNTH_BUNDLE_ENUM;

class IPF_INST_BUILDER_CLASS
{
  public:

    IPF_INST_BUILDER_CLASS(SYNTH_PARAMS p) // CONS
        :params(p) 
    {
        memop_template = IPF_TEMPLATE_INVALID;
        even_template  = IPF_TEMPLATE_INVALID;
        odd_template   = IPF_TEMPLATE_INVALID;
    }

    void init(std::string even_str,
              std::string odd_str,
              std::string memop_str);

    IPF_TEMPLATE_ENUM
    refine_to_syllables(SYNTH_BUNDLE_ENUM bundle_type,
                        SYNTH_MEMOP* marray,
                        SYNTH_IPF_ENUM* ipf_syllables /* output */);

    inline SYNTH_FTYPE
    get_memop_syllable(UINT32 slot) const
    {
        ASSERTX(slot < SYLLABLES_PER_BUNDLE);
        return memop_syllables[slot];
    }

  private:
  
    SYNTH_PARAMS params;
  
    /* maps templates to slot types */
    IPF_SLOT_ENUM slot_map[IPF_TEMPLATE_LAST][SYLLABLES_PER_BUNDLE];
    
    /* these are the model syllables we want to generate */
    SYNTH_FTYPE even_syllables[SYLLABLES_PER_BUNDLE];
    SYNTH_FTYPE odd_syllables[SYLLABLES_PER_BUNDLE];
    SYNTH_FTYPE memop_syllables[SYLLABLES_PER_BUNDLE];

    // these are the templates for the above syllable arrays
    IPF_TEMPLATE_ENUM memop_template;
    IPF_TEMPLATE_ENUM even_template ;
    IPF_TEMPLATE_ENUM odd_template  ;

#define MAX_SLOTS_PER_TYPE 4

    UINT32 fop_count[SYNTH_FTYPE_LAST];

    /* maps the SYNTH_FTYPE_* to a slot type. */
    IPF_SLOT_ENUM fop_map[SYNTH_FTYPE_LAST][MAX_SLOTS_PER_TYPE];


    void
    init_op_map(void);

    void
    add_to_fop_map(SYNTH_FTYPE f,
                   IPF_SLOT_ENUM sl);
    void
    init_syllable_mapping(void);

    void
    walk_bundle_string(const char* s,
                       SYNTH_FTYPE* syllables);

    SYNTH_FTYPE
    convert_syllable_code(char c);

    void
    pick_template(list<IPF_TEMPLATE_ENUM>* tlist,
                  SYNTH_FTYPE* array);

    void
    satisfied(list<IPF_TEMPLATE_ENUM>* tlist,
              const char* str);

    SYNTH_IPF_ENUM
    fi_map(SYNTH_FTYPE f,
           IPF_SLOT_ENUM s,
           SYNTH_ETYPE event);

};


#endif
