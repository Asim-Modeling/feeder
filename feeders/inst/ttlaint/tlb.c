/*
 * Copyright (C) 2001-2006 Intel Corporation
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

#include "globals.h"
#include "protos.h"


int tlb_textpages_mapped = 0;


extern int was_ldq_u_count;
void tlb_atexit()
{
  if (!Quiet) 
  {
      fprintf(Aint_output, "TLB: %d pages mapped\n", tlb_textpages_mapped);
      fprintf(Aint_output, "TLB: %d ldq_u executed (converted to map_nop)\n", was_ldq_u_count);
  }
}

/* 
 * This checks to see if an address points to code space.
 */
int points_to_code_space(process_ptr process, long addr)
{ 
  segment_t *segment;
  segment = process_find_segment(process, addr);
  if(segment==NULL) return(0);
  if (segment->prot & PROT_EXEC) 
    return 1;
  else
    return 0;
}

ulong
tlb_miss(process_ptr process, ulong addr, long *paccess,
	 long vpage, long tbkey, long tbtag)
{
  struct TB_Entry *pg;
  segment_t *segment = NULL;

  /* TB MISS
   *   Check if address is private or shared.
   * If shared, look up global table. This WILL return phys_addr.
   * If private, allocate here
   */


  /* Is it a valid private addr? */
  segment = process_find_segment(process, addr);

  if (segment != NULL) {
    pg = (struct TB_Entry *) malloc(sizeof(struct TB_Entry));
    if (pg == NULL) {
      fatal("tlb_miss: cannot allocate private page table entry\n");
    }

    /*
     * Must clean 'pg' structure; otherwise, the logic below asks for
     * the value of pg->page and, depending on your OS version, you'll
     * sometimes get a NULL page and sometimes you'll get a bogus 
     * address that will cause a core dump.
     * roger 30/Aug/99
     */
    bzero((void *)pg,sizeof(struct TB_Entry));

    /* This goes in right at the beginning of the list */
    pg -> next = process->TB[tbkey];
    process->TB[tbkey] = pg;

    pg -> tag = tbtag;

    if (segment->flags & MAP_SHARED) {
      /* look for mmap() or shmat() attached memory */
      struct Shm_ds *shmds = process->Shmem_regions;
      while (shmds) {
	if ((shmds->addr <= addr) && (addr < (shmds->addr + shmds->size))) {
	  ulong pageaddr = addr & PAGE_NUMBER_MASK;
	  pg -> page = Shmseg[shmds->shmid].paddr + (pageaddr - shmds->addr) ;
	  break;
	}
	shmds = shmds->next;
      }
    }
    /* if it's not shared, or there's no attached segment, then allocate anonymous memory */
    if (! pg->page) {
      pg -> page = allocate_pages(TB_PAGESIZE);
      informative("allocating data page: VA 0x%lx  PA 0x%lx\n", addr, pg->page);
      if (pg->page == (caddr_t) (-1)) fatal("tlb_miss: cannot allocate private page\n");
    }
    pg -> textpage = NULL;

    pg->flags = segment->flags;
    pg->prot = segment->prot;

    process->TB[tbkey]->lookaside = pg;
    /* Hack to speed-up copy_addr_space at fork-time */
    process->num_pages++;
    process->num_private++;

    if (paccess)
      *paccess = pg->prot;
    return ((ulong) pg->page + TB_OFFSET(addr));
  }
  if (paccess)
    *paccess = 0;
  return (0);    
}

/*
 * Roger Nov/99
 *
 * The three functions that use the TLB structure seem to be:
 *
 *  - addr2iphys
 *  - addr2phys
 *  - addr2tbeflags
 *
 * However, they seem to have diverged into their assumptions of
 * how the TLB structure is organized. In particular, whereas addr2iphys
 * has an 'assert' statement requiring the 'lookaside' field to be set,
 * the other two do not. 
 *
 * I have decided to merge the common code of the three routines into a single
 * function, (tlb_lookup) that will look for a particular address in the TB and
 * return its corresponding page or return NULL if no page was found. On top of
 * this basic routine, I will re-write the other three.
 *
 * The downside of this common code extraction is that it will be a little slower,
 * since we have to go into 'tlb_lookup' each time. But I think the code cleanup
 * will be worth it.
 */
struct TB_Entry *
tlb_lookup(process_ptr process, long addr)
{
 /* Refer to the Local TB First */
 long vpage, tbkey, tbtag;
 struct TB_Entry *pg;
 ulong ok;

 vpage = TB_VPAGE(addr);
 tbkey = TB_KEY(vpage);
 tbtag = TB_TAG(vpage);

 pg = process->TB[tbkey];
 if ( pg ) { 
  /* 
   * first page on process's TB[] list must have a lookaside value 
   */
  if ( pg->lookaside == NULL ) {
   fatal("tlb_lookup: addr %lx = <vpage=%lx,tbkey=%lx,tbtag=%lx>\n",addr,vpage,tbkey,tbtag);
   abort();
  }

  /*
   * Is the lookaside pointer pointing to the page we are looking for?
   * If so, great; we are done and we can immediately return
   */
  if (pg->lookaside->tag == tbtag) {
   return pg->lookaside;
  }

  /*
   * Bummer! We'll have to walk down the list looking for the desired
   * page.
   */
  for ( ; pg ; pg = pg->next) {
   /*
    * Check for hit. If so, update the lookaside pointer to make the page
    * just found the "most-recently-used" page, so that future references
    * do not need to go through the linked list search
    */
   if (pg->tag == tbtag) { 
    process->TB[tbkey]->lookaside = pg;
    return pg;
   }
  }
 }

 /*
  * If we got here it means that either the TB[tbkey] bucket was empty or that
  * after searching the whole list of pages we did not find the one we
  * where looking for. Invoke the TLB miss handler to refill for us the page.
  */
 ok = tlb_miss(process, addr, NULL, vpage, tbkey, tbtag);

 /*
  * Note that 'addr' could be a totally bogus address and, therefore, tlb_miss might
  * refuse to allocate a page for it. In such a case, we have to check for success
  * before returning any value
  */
 if ( ok != 0 ) {
  assert(process->TB[tbkey] != NULL);
  assert(process->TB[tbkey]->lookaside != NULL);
  assert(process->TB[tbkey]->lookaside->tag == tbtag);
  return process->TB[tbkey]->lookaside;
 }
 else {
  return NULL;
 }
}

int
addr2tbeflags(process_ptr process, long addr)
{
 struct TB_Entry *pg;

 pg = tlb_lookup(process, addr);

 return pg ? pg->flags : 0;
}

#ifdef DO_INLINING
# ifdef __GNUC__
   inline 
# else
#  pragma inline(addr2phys)
# endif
#endif 
ulong addr2phys(process_ptr process, long addr, long *paccess)
{
 struct TB_Entry *pg;

 /*
  * Invoke the lookup routine. If the lookup fails, it will invoke by itself
  * the miss handler. However, 'pg' could still be NULL at the end of the call
  * if we are giving a bogus address to 'lookup'
  */
 pg = tlb_lookup(process, addr);

 /* 
  * If we have a page, return its protection and the translated address. Otherwise,
  * clear 'paccess' and return indicator of miss.
  */
 if ( pg ) {
  if ( paccess ) *paccess = pg->prot;
  return ((ulong)pg->page + TB_OFFSET(addr));
 }
 else {
  if ( paccess ) *paccess = 0;
  return 0;
 }
}

/* 
 * addr2iphys(process, textaddr) returns the (struct icode *) 
 * pointer corresponding to textaddr.
 */
#undef DEBUG
icode_ptr
addr2iphys(process_ptr process, long addr, long *paccess)
{
 struct TB_Entry *pg;

 pg = tlb_lookup(process, addr);

 /* now check for corresponding textpage */
 if (pg) {
  if (!pg->textpage) {
    pg->textpage = (icode_ptr) allocate_code_pages((TB_PAGESIZE >> 2) * MAX_ICODE_SIZE);
    informative("allocating inst page: VA 0x%lx  PA 0x%lx\n", addr, pg->textpage);
   
   if (pg->textpage == (icode_ptr) (-1)) fatal("addr2iphys: cannot allocate private page\n");
   tlb_textpages_mapped++;
  }
  if ( paccess ) *paccess = pg->prot;
  return (pg->textpage + ITB_OFFSET(addr));
 }

 if ( paccess ) *paccess = 0;
 return 0;
}
