/*
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
 */

#ifndef _SYNTH_PARAMS_H_
#define _SYNTH_PARAMS_H_

#include <string>
#include "asim/provides/instfeeder_implementation.h"
#include "synth-headers.h"

class SYNTH_PARAMS_CLASS
{

    private:      

      void get_numeric_values_per_thread(string param, UINT32 nThreads, UINT32* valueArray);
      void get_numeric_values_per_thread(string param, UINT32 nThreads, UINT64* valueArray);
      void get_string_values_per_thread(string param, UINT32 nThreads, string* valueArray);
      
      UINT32* synth_sharing_pct;
      UINT32* synth_pointer_chasing;
      UINT32* synth_depends_on_load;
      UINT32* synth_dependent_range_start;
      UINT32* synth_dependent_range_stop;
      UINT64* synth_data_offset;
      UINT32* synth_locality;
      UINT32* synth_history_size;
      UINT32* synth_load_pct;
      UINT32* synth_atomic_pct;
      UINT32* synth_acquire_pct;
      UINT32* synth_release_pct;
      string* synth_lfetch_hint;
      UINT32* synth_fixed_spacing;
      UINT32* synth_random_threshold;
      UINT32* synth_memop_frequency;
      UINT32* synth_data_pattern;
      UINT32* synth_data_stride;
      UINT32* synth_fixed_size_refs;
      UINT32* synth_data_alignment;
      UINT32* synth_branch_off_path_pct;
      UINT32* synth_branch_bundle_in_routine;
      UINT32* synth_routine_offset;
      UINT32* synth_static_syllabes_per_routine;
      UINT32* synth_dynamic_syllabes_per_routine;
      UINT32* synth_num_instr_to_commit;
      UINT32* synth_synch_hold_delay;
      UINT32* synth_synch_retry_delay;
      UINT32* synth_synchronization;
      string* synth_memop_bundle;
      string* synth_even_bundle;
      string* synth_odd_bundle;

    public:    
    
      void init_params();
      
      void dump_params();
      
      UINT32 getSynth_Sharing_Pct(UINT32 threadNum);
      UINT32 getSynth_Pointer_Chasing(UINT32 threadNum);
      UINT32 getSynth_Depends_On_Load(UINT32 threadNum);
      UINT32 getSynth_Dependent_Range_Start(UINT32 threadNum);
      UINT32 getSynth_Dependent_Range_Stop(UINT32 threadNum);
      UINT64 getSynth_Data_Offset(UINT32 threadNum);
      UINT32 getSynth_Locality(UINT32 threadNum);
      UINT32 getSynth_History_Size(UINT32 threadNum);
      UINT32 getSynth_Load_Pct(UINT32 threadNum);
      UINT32 getSynth_Atomic_Pct(UINT32 threadNum);
      UINT32 getSynth_Acquire_Pct(UINT32 threadNum);
      UINT32 getSynth_Release_Pct(UINT32 threadNum);
      string getSynth_Lfetch_Hint(UINT32 threadNum);
      UINT32 getSynth_Fixed_Spacing(UINT32 threadNum);
      UINT32 getSynth_Random_Threshold(UINT32 threadNum);
      UINT32 getSynth_Memop_Frequency(UINT32 threadNum);
      UINT32 getSynth_Data_Pattern(UINT32 threadNum);
      UINT32 getSynth_Data_Stride(UINT32 threadNum);
      UINT32 getSynth_Fixed_Size_Refs(UINT32 threadNum);
      UINT32 getSynth_Data_Alignment(UINT32 threadNum);
      UINT32 getSynth_Branch_Off_Path_Pct(UINT32 threadNum);
      UINT32 getSynth_Branch_Bundle_In_Routine(UINT32 threadNum);
      UINT32 getSynth_Routine_Offset(UINT32 threadNum);
      UINT32 getSynth_Static_Syllabes_Per_Routine(UINT32 threadNum);
      UINT32 getSynth_Dynamic_Syllabes_Per_Routine(UINT32 threadNum);
      UINT32 getSynth_Num_Instr_To_Commit(UINT32 threadNum);
      UINT32 getSynth_Synch_Hold_Delay(UINT32 threadNum);
      UINT32 getSynth_Synch_Retry_Delay(UINT32 threadNum);
      UINT32 getSynth_Synchronization(UINT32 threadNum);
      string getSynth_Memop_Bundle(UINT32 threadNum);
      string getSynth_Even_Bundle(UINT32 threadNum);
      string getSynth_Odd_Bundle(UINT32 threadNum);
            
};

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Sharing_Pct(UINT32 threadNum)
{
    return synth_sharing_pct[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Pointer_Chasing(UINT32 threadNum)
{
    return synth_pointer_chasing[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Depends_On_Load(UINT32 threadNum)
{
    return synth_depends_on_load[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Dependent_Range_Start(UINT32 threadNum)
{
    return synth_dependent_range_start[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Dependent_Range_Stop(UINT32 threadNum)
{
    return synth_dependent_range_stop[threadNum];
}

inline UINT64
SYNTH_PARAMS_CLASS::getSynth_Data_Offset(UINT32 threadNum)
{
    return synth_data_offset[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Locality(UINT32 threadNum)
{
    return synth_locality[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_History_Size(UINT32 threadNum)
{
    return synth_history_size[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Load_Pct(UINT32 threadNum)
{
    return synth_load_pct[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Atomic_Pct(UINT32 threadNum)
{
    return synth_atomic_pct[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Acquire_Pct(UINT32 threadNum)
{
    return synth_acquire_pct[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Release_Pct(UINT32 threadNum)
{
    return synth_release_pct[threadNum];
}

inline string
SYNTH_PARAMS_CLASS::getSynth_Lfetch_Hint(UINT32 threadNum)
{
    return synth_lfetch_hint[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Fixed_Spacing(UINT32 threadNum)
{
    return synth_fixed_spacing[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Random_Threshold(UINT32 threadNum)
{
    return synth_random_threshold[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Memop_Frequency(UINT32 threadNum)
{
    return synth_memop_frequency[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Data_Pattern(UINT32 threadNum)
{
    return synth_data_pattern[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Data_Stride(UINT32 threadNum)
{
    return synth_data_stride[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Fixed_Size_Refs(UINT32 threadNum)
{
    return synth_fixed_size_refs[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Data_Alignment(UINT32 threadNum)
{
    return synth_data_alignment[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Branch_Off_Path_Pct(UINT32 threadNum)
{
    return synth_branch_off_path_pct[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Branch_Bundle_In_Routine(UINT32 threadNum)
{
    return synth_branch_bundle_in_routine[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Routine_Offset(UINT32 threadNum)
{
    return synth_routine_offset[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Static_Syllabes_Per_Routine(UINT32 threadNum)
{
    return synth_static_syllabes_per_routine[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Dynamic_Syllabes_Per_Routine(UINT32 threadNum)
{
    return synth_dynamic_syllabes_per_routine[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Num_Instr_To_Commit(UINT32 threadNum)
{
    return synth_num_instr_to_commit[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Synch_Hold_Delay(UINT32 threadNum)
{
    return synth_synch_hold_delay[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Synch_Retry_Delay(UINT32 threadNum)
{
    return synth_synch_retry_delay[threadNum];
}

inline UINT32
SYNTH_PARAMS_CLASS::getSynth_Synchronization(UINT32 threadNum)
{
    return synth_synchronization[threadNum];
}

inline string
SYNTH_PARAMS_CLASS::getSynth_Memop_Bundle(UINT32 threadNum)
{
    return synth_memop_bundle[threadNum];
}

inline string
SYNTH_PARAMS_CLASS::getSynth_Even_Bundle(UINT32 threadNum)
{
    return synth_even_bundle[threadNum];
}

inline string
SYNTH_PARAMS_CLASS::getSynth_Odd_Bundle(UINT32 threadNum)
{
    return synth_odd_bundle[threadNum];
}

typedef SYNTH_PARAMS_CLASS* SYNTH_PARAMS;

#endif

