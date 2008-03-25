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


#include "synth-params.h"

void
SYNTH_PARAMS_CLASS::init_params()
{
  
    // SYNTH_SHARING_PCT
    synth_sharing_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_SHARING_PCT, SYNTH_THREADS, synth_sharing_pct);
  
    // SYNTH_POINTER_CHASING
    synth_pointer_chasing = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_POINTER_CHASING, SYNTH_THREADS, synth_pointer_chasing);
    
    // SYNTH_DEPENDS_ON_LOAD
    synth_depends_on_load = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DEPENDS_ON_LOAD, SYNTH_THREADS, synth_depends_on_load);
    
    // SYNTH_DEPENDENT_RANGE_START
    synth_dependent_range_start = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DEPENDENT_RANGE_START, SYNTH_THREADS, synth_dependent_range_start);
    
    // SYNTH_DEPENDENT_RANGE_STOP
    synth_dependent_range_stop = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DEPENDENT_RANGE_STOP, SYNTH_THREADS, synth_dependent_range_stop);
      
    // SYNTH_DATA_OFFSET
    synth_data_offset = new UINT64[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DATA_OFFSET, SYNTH_THREADS, synth_data_offset);

    // SYNTH_LOCALITY
    synth_locality = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_LOCALITY, SYNTH_THREADS, synth_locality);

    // SYNTH_HISTORY_SIZE
    synth_history_size = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_HISTORY_SIZE, SYNTH_THREADS, synth_history_size);    
    
    // SYNTH_LOAD_PCT
    synth_load_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_LOAD_PCT, SYNTH_THREADS, synth_load_pct);
    
    // SYNTH_ATOMIC_PCT
    synth_atomic_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_ATOMIC_PCT, SYNTH_THREADS, synth_atomic_pct);
    
    // SYNTH_ACQUIRE_PCT
    synth_acquire_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_ACQUIRE_PCT, SYNTH_THREADS, synth_acquire_pct);
    
    // SYNTH_RELEASE_PCT
    synth_release_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_RELEASE_PCT, SYNTH_THREADS, synth_release_pct);
    
    // SYNTH_LFETCH_HINT
    synth_lfetch_hint = new string[SYNTH_THREADS];
    get_string_values_per_thread(SYNTH_LFETCH_HINT, SYNTH_THREADS, synth_lfetch_hint);
    
    // SYNTH_FIXED_SPACING
    synth_fixed_spacing = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_FIXED_SPACING , SYNTH_THREADS, synth_fixed_spacing);
    
    // SYNTH_RANDOM_THRESHOLD
    synth_random_threshold = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_RANDOM_THRESHOLD , SYNTH_THREADS, synth_random_threshold);
    
    // SYNTH_MEMOP_FREQUENCY
    synth_memop_frequency = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_MEMOP_FREQUENCY , SYNTH_THREADS, synth_memop_frequency);
    
    // SYNTH_DATA_PATTERN
    synth_data_pattern = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DATA_PATTERN , SYNTH_THREADS, synth_data_pattern);
    
    // SYNTH_DATA_STRIDE
    synth_data_stride = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DATA_STRIDE , SYNTH_THREADS, synth_data_stride);
    
    // SYNTH_FIXED_SIZE_REFS
    synth_fixed_size_refs = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_FIXED_SIZE_REFS , SYNTH_THREADS, synth_fixed_size_refs);
    
    // SYNTH_DATA_ALIGNMENT
    synth_data_alignment = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DATA_ALIGNMENT, SYNTH_THREADS, synth_data_alignment);
    
    // SYNTH_BRANCH_OFF_PATH_PCT
    synth_branch_off_path_pct = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_BRANCH_OFF_PATH_PCT, SYNTH_THREADS, synth_branch_off_path_pct);
    
    // SYNTH_BRANCH_BUNDLE_IN_ROUTINE
    synth_branch_bundle_in_routine = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_BRANCH_BUNDLE_IN_ROUTINE, SYNTH_THREADS, synth_branch_bundle_in_routine);
    
    // SYNTH_ROUTINE_OFFSET
    synth_routine_offset = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_ROUTINE_OFFSET, SYNTH_THREADS, synth_routine_offset);

    // SYNTH_STATIC_SYLLABLES_PER_ROUTINE
    synth_static_syllabes_per_routine = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_STATIC_SYLLABLES_PER_ROUTINE, SYNTH_THREADS, synth_static_syllabes_per_routine);
       
    // SYNTH_DYNAMIC_SYLLABLES_PER_ROUTINE
    synth_dynamic_syllabes_per_routine = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_DYNAMIC_SYLLABLES_PER_ROUTINE, SYNTH_THREADS, synth_dynamic_syllabes_per_routine);

    // SYNTH_NUM_INS_TO_COMMIT
    synth_num_instr_to_commit = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_NUM_INS_TO_COMMIT, SYNTH_THREADS, synth_num_instr_to_commit);
    
    // SYNTH_SYNCH_HOLD_DELAY
    synth_synch_hold_delay = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_SYNCH_HOLD_DELAY, SYNTH_THREADS, synth_synch_hold_delay);

    // SYNTH_SYNCH_RETRY_DELAY    
    synth_synch_retry_delay = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_SYNCH_RETRY_DELAY, SYNTH_THREADS, synth_synch_retry_delay);
    
    // SYNTH_SYNCHRONIZATION
    synth_synchronization = new UINT32[SYNTH_THREADS];
    get_numeric_values_per_thread(SYNTH_SYNCHRONIZATION, SYNTH_THREADS, synth_synchronization);    
    
    // SYNTH_MEMOP_BUNDLE
    synth_memop_bundle = new string[SYNTH_THREADS];
    get_string_values_per_thread(SYNTH_MEMOP_BUNDLE, SYNTH_THREADS, synth_memop_bundle);
    
    // SYNTH_EVEN_BUNDLE
    synth_even_bundle = new string[SYNTH_THREADS];
    get_string_values_per_thread(SYNTH_EVEN_BUNDLE, SYNTH_THREADS, synth_even_bundle);

    // SYNTH_ODD_BUNDLE
    synth_odd_bundle = new string[SYNTH_THREADS];
    get_string_values_per_thread(SYNTH_ODD_BUNDLE, SYNTH_THREADS, synth_odd_bundle);
    
    if(SYNTH_PRINT) dump_params();
        
}

void
SYNTH_PARAMS_CLASS::dump_params()
{
    
    cout << "DUMPING THREAD CONFIGURATIONS: " << endl;
    for(UINT32 i = 0; i < SYNTH_THREADS; i++)
    {
        cout << "THREAD " << i << endl;
        cout << "synth_sharing_pct: " << synth_sharing_pct[i] << endl;
        cout << "synth_pointer_chasing: " << synth_pointer_chasing[i] << endl;
        cout << "synth_depends_on_load: " << synth_depends_on_load[i] << endl;
        cout << "synth_dependent_range_start: " << synth_dependent_range_start[i] << endl;
        cout << "synth_dependent_range_stop: " << synth_dependent_range_stop[i] << endl;
        cout << "synth_data_offset: " << synth_data_offset[i] << endl;
        cout << "synth_locality: " << synth_locality[i] << endl;
        cout << "synth_history_size: " << synth_history_size[i] << endl;
        cout << "synth_load_pct: " << synth_load_pct[i] << endl;
        cout << "synth_atomic_pct: " << synth_atomic_pct[i] << endl;
        cout << "synth_acquire_pct: " << synth_acquire_pct[i] << endl;
        cout << "synth_release_pct: " << synth_release_pct[i] << endl;
        cout << "synth_lfetch_hint: " << synth_lfetch_hint[i] << endl;
        cout << "synth_fixed_spacing: " << synth_fixed_spacing[i] << endl;
        cout << "synth_random_threshold: " << synth_random_threshold[i] << endl;
        cout << "synth_memop_frequency: " << synth_memop_frequency[i] << endl;
        cout << "synth_data_pattern: " << synth_data_pattern[i] << endl;
        cout << "synth_data_stride: " << synth_data_stride[i] << endl;
        cout << "synth_fixed_size_refs: " << synth_fixed_size_refs[i] << endl;
        cout << "synth_data_alignment: " << synth_data_alignment[i] << endl;
        cout << "synth_branch_off_path_pct: " << synth_branch_off_path_pct[i] << endl;
        cout << "synth_branch_bundle_in_routine: " << synth_branch_bundle_in_routine[i] << endl;
        cout << "synth_routine_offset: " << synth_routine_offset[i] << endl;
        cout << "synth_static_syllabes_per_routine: " << synth_static_syllabes_per_routine[i] << endl;
        cout << "synth_num_inst_to_commit: " << synth_num_instr_to_commit[i] << endl;
        cout << "synth_dynamic_syllabes_per_routine: " << synth_dynamic_syllabes_per_routine[i] << endl;
        cout << "synth_memop_bundle: " << synth_memop_bundle[i] << endl;
        cout << "synth_even_bundle: " << synth_even_bundle[i] << endl;
        cout << "synth_odd_bundle: " << synth_odd_bundle[i] << endl;
        cout << "synth_synch_hold_delay: " << synth_synch_hold_delay[i] << endl;
        cout << "synth_synch_retry_delay: " << synth_synch_retry_delay[i] << endl;
        cout << "synth_synchronization: " << synth_synchronization[i] << endl;
        
        cout << endl;
        
    }
    
}


void 
SYNTH_PARAMS_CLASS::get_numeric_values_per_thread(string param, UINT32 nThreads, UINT32* valueArray)
{
    UINT32 begin, end, i;
    string numbers("0123456789");
    
    i = 0;
    begin = 0;
    end = 0;
    begin = param.find_first_of(numbers, end);
    end = param.find_first_not_of(numbers, begin);
    while((i < nThreads) && (begin != string::npos))
    {
        valueArray[i] = strtol(param.substr(begin, end - begin).c_str(),NULL,0);
        begin = param.find_first_of(numbers, end);
        end = param.find_first_not_of(numbers, begin);        
        i++;
    }
    
    i = i - 1;
    UINT32 last = i;
    while(i < nThreads)
    {
        valueArray[i] = valueArray[last];
        i++;
    }
        
}

void 
SYNTH_PARAMS_CLASS::get_numeric_values_per_thread(string param, UINT32 nThreads, UINT64* valueArray)
{
    UINT32 begin, end, i;
    string numbers("0123456789");
    
    i = 0;
    begin = 0;
    end = 0;
    begin = param.find_first_of(numbers, end);
    end = param.find_first_not_of(numbers, begin);
    while((i < nThreads) && (begin != string::npos))
    {
        valueArray[i] = strtoul(param.substr(begin, end - begin).c_str(),NULL,0);
        begin = param.find_first_of(numbers, end);
        end = param.find_first_not_of(numbers, begin);        
        i++;
    }
    
    i = i - 1;
    UINT32 last = i;
    while(i < nThreads)
    {
        valueArray[i] = valueArray[last];
        i++;
    }
        
}

void 
SYNTH_PARAMS_CLASS::get_string_values_per_thread(string param, UINT32 nThreads, string* valueArray)
{
    UINT32 begin, end, i;
    string ws(" \t");
    
    i = 0;
    begin = 0;
    end = 0;
    begin = param.find_first_not_of(ws, end);
    end = param.find_first_of(ws, begin);
    while((i < nThreads) && (begin != string::npos))
    {
        valueArray[i] = param.substr(begin, end - begin);
        begin = param.find_first_not_of(ws, end);
        end = param.find_first_of(ws, begin);        
        i++;
    }
    
    i = i - 1;
    UINT32 last = i;
    while(i < nThreads)
    {
        valueArray[i] = valueArray[last];
        i++;
    }
    
}

