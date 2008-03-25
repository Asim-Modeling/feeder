/**
 *
 * @file deptable.cpp
 * @brief Tanglewood dependency table
 *
 * @author Chris Weaver, Tanglewood Architecture, MMDC, Intel Corporation
 * @date 6/21/02
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

//#include <stdio>
//#include <stdlib>
// ASIM public module
//#include "asim/provides/awin_deptable.h"
#include "deptype.h"
#include "deptable.h"
#include "asim/syntax.h"
#include "asim/mesg.h"
//constructor:
VA_TABLE::VA_TABLE()
{

}

void
VA_TABLE::INIT(UINT32 capacity, UINT32 limit, UINT32 rows, UINT32 resize, char *string)
{
    NULL_TABLE_ELEMENT=UINT32_MAX;

//default the values to:
    this->capacity=capacity;
    this->limit=limit;
    this->rows=rows;
    this->resize=resize;
    //make the data structures
    elements = new table_entry[capacity];

    head = new UINT32[rows];
    tail = new UINT32[rows];
    rowsize = new UINT32[rows];
    table_name= string;

    //setup up the table record tracking
    allocate_from=0;
    current_size=0;
    for (UINT32 i=0; i< rows; i++)
    {
        head[i]=NULL_TABLE_ELEMENT;
        tail[i]=NULL_TABLE_ELEMENT;
        rowsize[i]=0;
    }
    for (UINT32 i=0; i<capacity; i++)
    {
        elements[i].valid=0;;

    }
    trace=0;

}

bool
VA_TABLE::RESIZE(UINT32 newcapacity) //resize the number of elements to
                                        //capacity
{
  // cout <<"this is the start";
    table_entry* tmpelements;


    // trace=1;

    if(newcapacity<current_size)
    {
       cout<<"TABLE: "<<table_name<<" Cannot make the table smaller then the number of elements: "<<current_size<<" requested size: "<<newcapacity<<endl;
        return 0;
    }
    else if (newcapacity>limit)
    {
      cout<<"TABLE: "<<table_name<<" Cannot make the table bigger then the limit of: "<<limit<<" requested size: "<<newcapacity<<endl;
        return 0;
    }
    //need to allocated new memory, copy data and then fix pointers
    else
    {
        
        tmpelements = new table_entry[newcapacity];
        if(!tmpelements)
        {
	  //   cout<<"TABLE: "<<table_name<<" unable to allocated memory!!"<<endl;
            return 0;
        }
        //if memory was allocated copy old elements and delete them
        else
        {
            //don't think I can use memory copy because of the reference counted
            //elements
//            memcpy(tmpelements,elements, (sizeof(table_entry))*capacity);
            for(UINT32 i=0; i<newcapacity; i++)
            {
                if(i<capacity)
                { 
                    tmpelements[i].valid=elements[i].valid;
                    tmpelements[i].data.address=elements[i].data.address;
                    tmpelements[i].data.last_thread=elements[i].data.last_thread;
                    tmpelements[i].data.last_icount=elements[i].data.last_icount;
                    tmpelements[i].next=elements[i].next;
                    tmpelements[i].prev=elements[i].prev;
                }
                else
                {
                    tmpelements[i].valid=0;
                    tmpelements[i].data.address=0;
                    tmpelements[i].data.last_thread=0;
                    tmpelements[i].data.last_icount=0;
                    tmpelements[i].next=NULL_TABLE_ELEMENT;
                    tmpelements[i].prev=NULL_TABLE_ELEMENT;
                }
 
            }
            delete []elements;
            elements=tmpelements; 
            capacity=newcapacity;            
        }
	//	cout << "done resize \n";  
      return 1;       
    }
  
}

//find a free element for the next add.. assumes there should be one
bool
VA_TABLE::FIND_FREE_ELEMENT()
{
    for(UINT32 i=0; i< capacity; i++)
    {
       allocate_from++;

       if(allocate_from==capacity)
           allocate_from=0;

       if(elements[allocate_from].valid==0)
           return 1;
    }
    return 0;
}

void
VA_TABLE::ADD_VA(UINT64 address,UINT64 last_thread,UINT64 last_icount,UINT32 row) 
//add a dependency to the specified row
{

   //check to make sure it is a valid row
    ASSERTX(row<rows);

    //check to see if there is space
    if(current_size==capacity)
    {
        if(trace)
            cout<<"TABLE: "<<table_name<<" RESIZING TABLE!!"<<endl;

        if(!RESIZE(capacity+resize))
            ASSERT(0,"TABLE RESIZE FAILED");
    }
    else if(current_size>capacity)
       ASSERT(0,"INVALID CURRENT_SIZE!"); 
    
    
    ASSERT(FIND_FREE_ELEMENT(),"You have a memory leak somewhere, I expected to find an empty element but they were all valid!!");
    ASSERTX(row<rows);

    if(trace)
    {
        cout<<"TABLE: "<<table_name<<endl;
        cout<<"to line number: "<<row<<endl;
        cout<<"address: "<<address<<endl;
        cout<<"last_thread: "<<last_thread<<endl;
        cout<<"last_icount: "<<last_icount<<endl;
                
   }

    //copy the data--not sure how fast memcpy is, may want to replace with
    //simple assignment
    //memcpy(&elements[allocate_from].data,add_element,sizeof(dephash));
    elements[allocate_from].data.address=address;
    elements[allocate_from].data.last_thread=last_thread;
    elements[allocate_from].data.last_icount=last_icount;
        
    //setup the indexes
    //if there is nothing in the row then
    if(rowsize[row]==0)
    {
        head[row]=allocate_from;
        tail[row]=allocate_from;
        elements[allocate_from].prev=NULL_TABLE_ELEMENT;
        elements[allocate_from].next=NULL_TABLE_ELEMENT;
        elements[allocate_from].valid=1;
    }
    //else tack it on as the last element
    else if(rowsize[row])
    {
        //setup this element
        elements[allocate_from].prev=tail[row];
        elements[allocate_from].next=NULL_TABLE_ELEMENT;  
        elements[allocate_from].valid=1;
        //update the former last element
        elements[tail[row]].next=allocate_from;
        //update the tail number
        tail[row]=allocate_from;
    }
    if(trace)
        cout<<"TABLE: "<<table_name<<" added virtual address: "<<address<<" to row: "<<row<<" element number: "<<allocate_from<<endl; 
    //increment the row and overall size
    rowsize[row]++;
    current_size++;
    
}

void
VA_TABLE::REMOVE_DEP_HEAD(UINT32 row,UINT64 address) //remove the head and make sure the
                                            //UId matches what is expected
{
    //check to make sure it is a valid row
    ASSERTX(row<rows);

    //Check to make sure there is something to remove!
    ASSERT(rowsize[row],"There is nothing in that row!");
    if(trace)
        cout<<"TABLE: "<<table_name<<" removing address: "<<address<<" from row: "<<row<<endl;

    if(elements[head[row]].data.address!=address)
    {
      // cout<<"TABLE: "<<table_name<<endl;
      // cout<<"The head points to element: "<<head[row]<<endl;
      //  cout<<"The row we were given is: "<<row<<endl;
      //  cout<<"The contents of the row are: "<<endl;
      //  PRINT_ROW(row);
    }


    //check to make sure the id is correct!
    ASSERTX(elements[head[row]].data.address==address);
    //check to make sure that the previous instruction is NULL
    ASSERTX(elements[head[row]].prev==NULL_TABLE_ELEMENT);

    //null out the reference counted instruction!
    elements[head[row]].valid=0;

    //fix the links
    if(elements[head[row]].next!=NULL_TABLE_ELEMENT)
    {
        ASSERTX(rowsize[row]>1);
        head[row]=elements[head[row]].next;
        elements[head[row]].prev=NULL_TABLE_ELEMENT;

    }
    else
    { 
        ASSERTX(rowsize[row]==1);
        tail[row]=NULL_TABLE_ELEMENT;
        head[row]=NULL_TABLE_ELEMENT;
    }
    //fix the counts
    rowsize[row]--;
    current_size--;

}
void

VA_TABLE::PRINT_ROW(UINT32 row)
{
    ASSERTX(row<rows);
    UINT32 i=head[row];
    cout<<"TABLE: "<<table_name<<endl;
    while(i!=NULL_TABLE_ELEMENT)
          {
	    /*    
	for (UINT32 j=0; j<19; j++)
	  {
	      if (elements[i].data.write_cnt_td[j] !=0)
	      {
		cout<<i;
		cout<<" "<<elements[i].data.address;
		cout<<" "<<elements[i].data.last_thread;
		cout<<" "<<elements[i].data.last_icount;
		cout<<" "<<j;
	        cout<<" write "<<elements[i].data.write_cnt_td[j];
		cout << " \n";
		
	      }
	      }
	
	for (UINT32 j=0; j<19; j++)
	  {
	    if (elements[i].data.read_cnt_td[j] !=0)
	      {
		cout<<i;
		cout<<" "<<elements[i].data.address;
		cout<<" "<<elements[i].data.last_thread;
		cout<<" "<<elements[i].data.last_icount;
		cout<<" "<<j;
		cout<<" read: "<<elements[i].data.read_cnt_td[j];
		cout << " \n";
			
	      }
	  }

	for (UINT32 j=0; j<19; j++)
	  {
	    if (elements[i].data.invalid_cnt_td[j] !=0)
	      {
		cout<<i;
		cout<<" "<<elements[i].data.address;
		cout<<" "<<elements[i].data.last_thread;
		cout<<" "<<elements[i].data.last_icount;
		cout<<" "<<j;
	           cout<<" invalid "<<elements[i].data.invalid_cnt_td[j];
		    cout << " \n";
		}
		}*/

        i=elements[i].next;
    }
}


void
VA_TABLE::REMOVE_DEP_MID(UINT32 row,UINT64 address) //remove a single dependency with the
                                           //UID anywhere in the list
    
{

    ASSERTX(row<rows);

    if(trace)
        cout<<"TABLE: "<<table_name<<" removing address: "<<address<<" from row: "<<row<<endl;

    //Check to make sure there is something to remove!
    ASSERT(rowsize[row],"There is nothing in that row!");

    //find the element
    UINT32 i=head[row];
    while(i!=NULL_TABLE_ELEMENT&&
          elements[i].data.address!=address)
    {
        i=elements[i].next;
    }

    //make sure that we found the element
    if(i==NULL_TABLE_ELEMENT)
    {
        cout<<"The row is: "<<row<<endl;
        cout<<"The address is: "<<address<<endl;
        cout<<"The row size is: "<<rowsize[row]<<endl;
        cout<<"ROW CONTENTS"<<endl;
        PRINT_ROW(row);

    }
    ASSERTX(i!=NULL_TABLE_ELEMENT);

    elements[i].valid=0;

    if(rowsize[row]==1)
    {
        tail[row]=NULL_TABLE_ELEMENT;
        head[row]=NULL_TABLE_ELEMENT;  
    }
    else
    {
        //if first element 
        if(elements[i].prev==NULL_TABLE_ELEMENT)
            head[row]=elements[i].next;
        //else update the element before it
        else
            elements[elements[i].prev].next=elements[i].next;

        //if last element
        if(elements[i].next==NULL_TABLE_ELEMENT)
            tail[row]=elements[i].prev;

        //else up date element after it
        else
            elements[elements[i].next].prev= elements[i].prev;
    }
    //fix the counts
    rowsize[row]--;
    current_size--;


}

UINT64
VA_TABLE::FIND_DEP_VA(UINT32 row,UINT64 address)
{
    UINT32 i=head[row];
    //continue to look until we find the next time the register/memory location
    //is used, stop if we run out of elements or have a match that is not
    //the same instruction
    while((i!=NULL_TABLE_ELEMENT)&&
          (address!=elements[i].data.address))
    {
        i=elements[i].next;
    }
   if(i==NULL_TABLE_ELEMENT)
        return 0xFFFFFFFFFFFFFFFF;
    else
        return elements[i].data.address;
}


va_address*
VA_TABLE::FIND_DEP(UINT32 row,UINT64 address)
{
    UINT32 i=head[row];
    //continue to look until we find the next time the register/memory location
    //is used, stop if we run out of elements or have a match that is not
    //the same instruction
    while((i!=NULL_TABLE_ELEMENT)&&
          (address!=elements[i].data.address))
    {
        i=elements[i].next;
    }

    if(i==NULL_TABLE_ELEMENT)
        return 0;
    else
        return &elements[i].data;
}


