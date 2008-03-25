/**
 *
 * @file awindow.h
 * @brief Header file for Tanglewood Retire 
 *
 * @author Chris Weaver, Tanglewood Architecture, MMDC, Intel Corporation
 * @date 7/4/02
 *
 * Copyright (C) 2002-2006 Intel Corporation
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

#ifndef _va_deptable_
#define _va_deptable_


#include "deptype.h"


/*
 * Class ASIM_AWINDOW
 *
 * This is the class used to generate a variable size window of instructions
 * to do post processing on 
 *
 */
struct table_entry{
    va_address data;
    UINT32 next;
    UINT32 prev;
    bool valid;
};



class VA_TABLE 
{
  private:
    //since we are using arrays instead of pointers, use UINT32_MAX 
    //to mark NULL edges.
    UINT32  NULL_TABLE_ELEMENT;
    bool trace;
    table_entry* elements; //the pointer to the array of elements

    UINT64 allocate_from; //the number to add of the add new entries to- a
                          //variable is kept so that hopefully the next free
                          //element will be be close to the just used.
    char*   table_name;
    UINT32* head; //the index of the heads
    UINT32* tail; //the index of the tails
    UINT32* rowsize; //the pointer to the row size array

    UINT32 capacity; //the current capacity of the table
    UINT32 current_size; //the number of elements actually being used
    UINT32 limit; //how large the table can grow to before an error
    UINT32 rows; //the number of rows in the table
    UINT32 resize; //how many elements to add on a resize

  public:
    
    VA_TABLE();
    void INIT(UINT32 capacity, UINT32 limit, UINT32 rows, UINT32 resize, char * string);
    bool RESIZE(UINT32 newcapacity); //resize the number of elements to capacity
    void ADD_VA(UINT64 address,UINT64 last_thread,UINT64 last_icount, UINT32 row); //add a dependency to the specified row

    void REMOVE_DEP_HEAD(UINT32 row,UINT64 uid); //remove the head and make sure the
                                            //UId matches what is expected

    void REMOVE_DEP_MID(UINT32 row,UINT64 uid); //remove a single dependency with the
                                           //UID anywhere in the list
    
    bool FIND_FREE_ELEMENT();
    va_address* FIND_DEP(UINT32 row,UINT64 address);
    UINT64 FIND_DEP_VA(UINT32 row,UINT64 address);
    void PRINT_ROW(UINT32 row);
    void trace_off()
        {
            trace=0;
        }
    
    void trace_on()
        {
            trace=1;
        }

    UINT32 table_capacity()
        {
            return capacity;
        }
};
#endif 
