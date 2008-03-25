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

/* 
 * Routines for loading executables and shared objects
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <time.h>
#include <sys/utsname.h>
#include <string.h>
#include </usr/include/assert.h>

#if defined(__LANGUAGE_C__) || defined(__cplusplus)
#else
#error bozo compilers apply here
#endif

#include <elf_abi.h>
#include <elf_mips.h>

#include "icode.h"
#include "opcodes.h"
#include "globals.h"

#define USE_ATOM_OUTPUT 0

/* prototypes for functions */

#include "protos.h"

#include "bfd.h"
#include "obstack.h"
extern long bfd_elf_hash(const char *);

static loader_initialized = 0;
static sqrt_notification = 0;

static void decode_opnum(icode_ptr picode, unsigned instr);
#if USE_ATOM_OUTPUT
static void read_bb_info(process_ptr process, const char *Objname);
#endif

static const char *loader_target = "ecoff-littlealpha";

static void loader_init ()
{
  bfd_init();

}



int emulate_sbin_loader = 0; 

#define MAX_N_OBJECT_FILES 256
typedef struct elf_hash_s {
  Elf32_Word nbuckets;
  Elf32_Word nchains;
  Elf32_Word *bucket;
  Elf32_Word *chain;
} elf_hash_t;

typedef struct gotinfo_s {
  Elf32_Word local_gotno;
  Elf32_Word gotsym;
} gotinfo_t;

typedef struct object_file_s {
  const char *filename;
  char loaded[MAX_NPROCS];

  int flags; /* copy of abfd->flags */
  int n_libs_needed; 
  Elf32_Lib *liblist;

  const char *dynstr;
  Elf32_Sym *dynsym;
  elf_hash_t hash;

  gotinfo_t *gotinfo;
  int ngots;
  bfd *abfd;
  Elf32_Addr stack_end;
  Elf32_Addr brk;

  /* other info from .dynamic section */
  Elf32_Addr dt_pltgot;
  Elf32_Addr dt_init;
  Elf32_Addr dt_fini;
  Elf32_Word dt_mips_time_stamp;
  Elf32_Word dt_mips_ichecksum;
} object_file_t;

object_file_t object_file[MAX_N_OBJECT_FILES];
int n_object_files = 0;

object_file_t *
object_file_add(const char *name)
{
  int i;
  for (i = 0; i < n_object_files; i++) {
    if (strcmp(object_file[i].filename, name) == 0)
      return &object_file[i];
  }
  object_file[n_object_files].filename = name;
  return &object_file[n_object_files++];
} 

object_file_t *
object_file_get(const char *name)
{
  int i;
  for (i = 0; i < n_object_files; i++) {
    if (strcmp(object_file[i].filename, name) == 0)
      return &object_file[i];
  }
  return NULL;
} 

int 
object_file_open(object_file_t *pof)
{
  bfd *abfd;
  if (pof->abfd)
    return -1;

  if ((abfd = pof->abfd = bfd_openr(pof->filename, loader_target)) == NULL) {
    char msg[1024];
    sprintf(msg, "object_file_open(%s)", pof->filename);
    bfd_perror(msg);
    return -1;
  } 
  if (!bfd_check_format(pof->abfd, bfd_object))
    bfd_perror("load_object_file - check_format");

  /* save the flags */
  pof->flags = abfd->flags;

  if (emulate_sbin_loader) {
    if (!pof->dynstr) {
      asection *dynstr_section = bfd_get_section_by_name(abfd, ".dynstr");
      if (dynstr_section != NULL) {
	int size = bfd_section_size(abfd, dynstr_section);
	pof->dynstr = malloc(size);
        if (pof->dynstr == NULL)
           fatal("failed to allocate pof->dynstr");
	bfd_get_section_contents(abfd, dynstr_section, (char*)pof->dynstr, 0, size);
      }
    }
    if (!pof->dynsym){
      asection *dynsym_section = bfd_get_section_by_name(abfd, ".dynsym");
      if (dynsym_section != NULL) {
	int size = bfd_section_size(abfd, dynsym_section);
	pof->dynsym = (Elf32_Sym *)malloc(size);
        if (pof->dynsym == NULL)
           fatal("failed to allocate pof->dynsym");
	bfd_get_section_contents(abfd, dynsym_section, (char*)pof->dynsym, 0, size);
      }
    }

    if (!pof->hash.nbuckets) {
      asection *hash_section = bfd_get_section_by_name(abfd, ".hash");
      if (hash_section != NULL) {
	int size = bfd_section_size(abfd, hash_section);
	Elf32_Word *rawhash = (Elf32_Word *)malloc(size);
        if (rawhash == NULL)
           fatal("failed to allocate rawhash");
	bfd_get_section_contents(abfd, hash_section, (char*)rawhash, 0, size);
	pof->hash.nbuckets = rawhash[0];
	pof->hash.nchains = rawhash[1];
	pof->hash.bucket = &rawhash[2];
	pof->hash.chain = &rawhash[2+pof->hash.nbuckets];
      }
    }
  }

  return 0;
}

void
object_file_close(object_file_t *pof)
{
  bfd_close(pof->abfd);
  pof->abfd = NULL;
}

const char *object_file_dynstr(object_file_t *pof, Elf32_Word dynstrIndex)
{
  if (pof->dynstr)
    return pof->dynstr + dynstrIndex;
  else 
    return NULL;
}

Elf32_Sym *object_file_dynsym_from_index(object_file_t *pof, Elf32_Word dynsymIndex)
{
  if (pof->dynsym)
    return pof->dynsym + dynsymIndex;
  else 
    return NULL;
}

/*
 * r2r: this is pulled right out of libbfd:elf.c
 *      frankly, I have no idea why the ECOFF format that we are trying to
 *      read here would use an ELF hash table in the .hash section, but
 *      to here is the code to generate the hash index anyway;
 *      if we don't define this function ourselves, we end up with a missing
 *      symbol if libbfd is built without ELF support (likely on Alpha);
 */
/* Standard ELF hash function.  Do not change this function; you will
   cause invalid hash tables to be generated.  */
unsigned long
aint_bfd_elf_hash (namearg)
     const char *namearg;
{
  const unsigned char *name = (const unsigned char *) namearg;
  unsigned long h = 0; 
  unsigned long g;
  int ch;

  while ((ch = *name++) != '\0')
    {
      h = (h << 4) + ch;
      if ((g = (h & 0xf0000000)) != 0)
        {
          h ^= g >> 24;
          /* The ELF ABI says `h &= ~g', but this is equivalent in
             this case and on some machines one insn instead of two.  */
          h ^= g;
        }
    }
  return h;
}

Elf32_Sym *object_file_dynsym_from_name(object_file_t *pof, const char *name)
{
  Elf32_Word nbuckets = pof->hash.nbuckets;
  Elf32_Word *bucket = pof->hash.bucket;
  Elf32_Word *chain = pof->hash.chain;
  long h = aint_bfd_elf_hash(name);
  int b = h % nbuckets;
  int i = bucket[b]; 
  Elf32_Sym *sym = object_file_dynsym_from_index(pof, i);
  const char *symname = object_file_dynstr(pof, sym->st_name);
  if (strcmp(symname, name) == 0) {
    return sym;
  } else {
    while ((i = chain[i]) != STN_UNDEF) {
      sym = object_file_dynsym_from_index(pof, i); 
      symname = object_file_dynstr(pof, sym->st_name);
      if (strcmp(symname, name) == 0) 
	return sym;
    }
    return NULL;
  }
}



void ofse_fcn (bfd *abfd, asection *section, object_file_t *pof)
{
  if (section->vma == 0)
    return;
  if (pof->stack_end == 0)
    pof->stack_end = section->vma;
  if (section->vma < pof->stack_end)
    pof->stack_end = section->vma;
  if (0)
  fprintf(Aint_output, "sectionName=%s flags=%x section_start=%lx section_end=%lx\n", 
          section->name, section->flags, section->vma, section->vma + bfd_section_size(abfd, section));
  if (section->flags & (SEC_ALLOC)) {
     ulong section_end = section->vma + bfd_section_size(abfd, section);
     if (pof->brk < section_end)
        pof->brk = section_end;
  }
}

void
object_file_info(const char *objfile_name, 
		 ulong *pstack_end, 
		 int *pdynamic,
                 ulong *brk)
{
  object_file_t *pof = object_file_get(objfile_name);
  if (!pof) {
    pof = object_file_add(objfile_name);
    if(object_file_open(pof)==-1){
      fprintf(Aint_output, "Error couldn't open object file: %s.\n",objfile_name);
      exit(1);
    }
    pof->brk = 0;
    bfd_map_over_sections(pof->abfd, 
			  (void (*)(bfd*,asection*,void*))ofse_fcn,
			  (void*)pof);
    object_file_close(pof);
  }
  *pstack_end = pof->stack_end;
  *pdynamic = (pof->flags & DYNAMIC);
  *brk = pof->brk;
}


void basic_block_boundaries_from_symtab(process_ptr process, object_file_t *pof);
void object_file_load(process_t *process, object_file_t *pof);

/* load_section is funky because it is called from bfd_map_over_sections() */

struct ofl_state {
  object_file_t *pof;
  process_ptr process;
};

static void load_section(bfd *abfd, asection *section, struct ofl_state *);
static void load_code_section(process_ptr process, object_file_t *pof, asection *section);
static void load_liblist_section(process_ptr process, object_file_t *pof, asection *section);

static void load_dynamic_section(process_ptr process, object_file_t *pof, asection *section);
static void adjust_got_section(process_ptr process, object_file_t *pof);

static void analyze_code_segment(process_ptr process, ulong start, ulong size);


/* load_object_file (process, filename)
   uses BFD to read the executable to which filename points
   and readies process for execution.

   load_object_file also parses all the instructions contained in filename.
   */
void load_object_file(process_t *process, const char *exename)
{
  object_file_t *pof;
  bfd *abfd;
  if (!loader_initialized)
    loader_init();

  pof = object_file_add(exename);
  object_file_open(pof);
  abfd = pof->abfd;
  assert(abfd->flags & EXEC_P);
  
  process->entry_point = abfd->start_address;
  process->initial_gp = bfd_ecoff_get_gp_value(abfd);

  object_file_load(process, pof);

  if (emulate_sbin_loader)
    adjust_got_section(process, pof);

  object_file_close(pof);

  informative("load_object_file: entry point is %lx\n", process->entry_point);
  informative("load_object_file: gp is %lx\n", process->initial_gp);
  informative("load_object_file: found %d instructions\n", process->n_instructions);

}

void load_library(process_t *process, const char *libname)
{
  object_file_t *pof;
  bfd *abfd;
  if (!loader_initialized)
    loader_init();

  pof = object_file_add(libname);
  object_file_open(pof);
  abfd = pof->abfd;
  assert(abfd->flags & EXEC_P);
  
  object_file_load(process, pof);

  informative("load_library: entry point is %lx\n", abfd->start_address);
  informative("load_library: gp is %lx\n", bfd_ecoff_get_gp_value(abfd));

  object_file_close(pof);

}

void object_file_load(process_t *process, object_file_t *pof)
{
  bfd *abfd = pof->abfd;
  struct ofl_state ofl;
  ofl.pof = pof;
  ofl.process = process;

  informative("load_object_file: (%s)\n", pof->filename);
  informative("load_object_file: flags are %lx\n", abfd->flags);

  bfd_map_over_sections(abfd, 
			(void (*)(bfd *, asection *, void *))load_section,
			(void *)&ofl);
  

  pof->loaded[process->pid] = 1;

  if (emulate_sbin_loader) {
    int i;
    for (i = 0; i < pof->n_libs_needed; i++) {
      char filename[1024];
      const char *name = object_file_dynstr(pof, pof->liblist[i].l_name);
      object_file_t *lpof;

      sprintf(filename, "/usr/shlib/%s", name);
      lpof = object_file_add(strdup(filename));

      if (!lpof->loaded[process->pid]) {
	load_library(process, lpof->filename);
      }
    }
  }

  if (aint_parse_basic_blocks) {
    basic_block_boundaries_from_symtab(process, pof);
  }
#if USE_ATOM_OUTPUT
  read_bb_info(process, exename);
#endif

}

static void load_code_section(process_ptr process, object_file_t *pof, asection *section)
{
  ulong start = section->vma;
  ulong size = section->_raw_size;
  decode_text_region(process, start, size);

}




static void load_section(bfd *abfd, asection *section, struct ofl_state *state)
{
  object_file_t *pof = state->pof;
  process_ptr process = state->process;
  flagword flags = section->flags;
  ulong start = section->vma;
  ulong size = section->_raw_size;

  if (flags & SEC_NEVER_LOAD) {
    /* do nothing */
    informative("load_section(%s: start=0x%lx, size=%ld, flags=0x%x -- do nothing \n",
		section->name, start, size, flags);
  } else if (flags & SEC_ALLOC) {
    int prot = PROT_READ;

    informative("load_section(%s: start=0x%lx, size=%ld, flags=0x%x\n",
		section->name, start, size, flags);

    if (!(flags & SEC_READONLY) && (flags & SEC_DATA))
      prot |= PROT_WRITE;
    if (flags & SEC_CODE) {
      prot |= PROT_EXEC;
    }

    process_add_segment(process, start, size, prot, 
			(flags & SEC_CODE) ? MAP_SHARED : MAP_PRIVATE);
    if (strcmp(section->name, ".liblist") == 0)
      load_liblist_section(process, pof, section);
    if (strcmp(section->name, ".dynamic") == 0)
      load_dynamic_section(process, pof, section);

    if (flags & (SEC_LOAD|SEC_HAS_CONTENTS)) {
      char *buf = malloc(size);
      if (buf == NULL)
         fatal("failed to allocate buf in load_section");
      if (!bfd_get_section_contents(abfd, section, buf, 0, size)) {
	bfd_perror("load_section - get_section_contents");
      }
      /* now copy the contents */
      memcpy_to_object_space(process, start, buf, size);
      free(buf);
      if (flags & SEC_CODE) {
	load_code_section(process, pof, section);
	if (0) analyze_code_segment(process, section->vma, section->_raw_size);
      }
    } else {
      /* zero-filled section: do nothing */
    }
  }
}



int segment_contains(segment_t *segment, ulong addr)
{
  return ((segment->start <= addr) && (addr <= segment->end));
}

segment_t *
process_add_segment(process_ptr process, ulong start, ulong size, int prot, int flags)
{
  int i;
  int n_segments = process->n_segments;
  int is_new = 0;
  segment_t *segments = process->segments; 
  segment_t *segment = NULL;
  ulong rounded_start = BASE2ROUNDDOWN(start, (ulong)TB_PAGESIZE);
  ulong rounded_end = BASE2ROUNDUP(start+size, (ulong)TB_PAGESIZE)-(ulong)1;
  ulong rounded_size = rounded_end - rounded_start+(ulong)1;

  informative("process_add_segment [%lx, %lx] [%lx]\n", 
	      (ulong)start, (ulong)(start+size), flags);
  informative(" \t => [%lx, %lx, %lx]\n", 
	      rounded_start, rounded_end, rounded_size);

  /* check for existing segment */
  for (i = 0; i < n_segments; i++) {
    if (   segment_contains(&segments[i], rounded_start)
	|| segment_contains(&segments[i], rounded_end)) {
      segment = &segments[i];
      break;
    }
  }
  if (segment == NULL) {
    is_new = 1;
    n_segments++;
    segments = realloc(segments, n_segments * sizeof(segment_t));
    if (segments == NULL)
      fatal("process_add_segment failed to realloc process->segments\n");
    process->n_segments = n_segments;
    process->segments = segments;
    segment = &segments[n_segments-1];
    segment->start = rounded_start;
    segment->end = rounded_end;
    segment->size = rounded_size;
    segment->prot = prot;
    segment->flags = flags;
  } else {
    if (segment->start > rounded_start)
      segment->start = rounded_start;
    if (segment->end < rounded_end)
      segment->end = rounded_end;
    segment->size = 
      (ulong)((ulong)segment->end - (ulong)segment->start + (ulong)1);
  }
  informative(" \t => [%lx, %lx, %lx] (%s segment %d)\n", 
	      segment->start, segment->end, segment->size, 
	      (is_new ? "new" : "extended"), i);
  segment->prot |= prot;
  return segment;
}

segment_t *process_find_segment(process_ptr process, ulong addr)
{
  int i;
  int n_segments = process->n_segments;
  segment_t *segments = process->segments; 
  /* check for containing segment */
  for (i = 0; i < n_segments; i++) {
    if (segment_contains(&segments[i], addr)) {
      return &segments[i];
    }
  }
  /* no access */
  return NULL;
}


#define ALIGNED(v_, type_) ((((long)(v_)) & (sizeof(type_)-1)) == 0)

void memcpy_from_object_space(process_ptr process, char *aint_ptr, ulong object_ptr, size_t size)
{
  size_t i;
  /* optimize this more later */
  if (ALIGNED(size, long) && ALIGNED(object_ptr, long) && ALIGNED(aint_ptr, long)) {
    for (i=0; i<size; i += sizeof(long)) {
      *(long*)(aint_ptr + i) = * (long *) addr2phys(process, object_ptr + i, NULL);
    }
  } else if (ALIGNED(size, int) && ALIGNED(object_ptr, int) && ALIGNED(aint_ptr, int)) {
    for (i=0; i<size; i += sizeof(int)) {
      *(int*)(aint_ptr + i) = * (int *) addr2phys(process, object_ptr + i, NULL);
    }
  } else {
    for (i=0; i<size; i += sizeof(char)) {
      *(char*)(aint_ptr + i) = * (char *) addr2phys(process, object_ptr + i, NULL);
    }
  }
}

void memcpy_to_object_space(process_ptr process, ulong object_ptr, const void *aint_vptr, size_t size)
{
  const char *aint_ptr = (const char *)aint_vptr;
  size_t i;
  register long *phys;
  /* optimize this more later */
  if (ALIGNED(size, long) && ALIGNED(object_ptr, long) && ALIGNED(aint_ptr, long)) {
    for (i=0; i<size; i += sizeof(long)) {
      phys = (long *) addr2phys(process, object_ptr + i, NULL);
      * phys = * (long*) (aint_ptr + i);
    }
  } else if (ALIGNED(size, int) && ALIGNED(object_ptr, int) && ALIGNED(aint_ptr, int)) {
    for (i=0; i<size; i += sizeof(int)) {
      * (int *) addr2phys(process, object_ptr + i, NULL) = * (int*) (aint_ptr + i);
    }
  } else {
    for (i=0; i<size; i += sizeof(char)) {
      * (char *) addr2phys(process, object_ptr + i, NULL) = * (char*) (aint_ptr + i);
    }
  }
}


size_t strlen_from_object_space(process_ptr process, ulong object_ptr)
{
  size_t i;
  size_t len = 0;
  /* optimize this later */
  for (i=0; ; i++) {
    char c = * (char *) addr2phys(process, object_ptr + i, NULL);
    if (c == '\0') 
      break;
    else
      len++;
  }
  return len;
}

char *strdup_from_object_space(process_ptr process, ulong object_ptr)
{
  size_t i;
  size_t size = 1024;
  char *aint_ptr = malloc(size);
  size_t len = 0;
  if (aint_ptr == NULL)
    fatal("strdup_from_object_space failed to allocate %d bytes\n", size);

  /* optimize this later */
  for (i=0; ; i++) {
    char c = * (char *) addr2phys(process, object_ptr + i, NULL);
    aint_ptr[i] = c;
    if (c == '\0') {
      break;
    } else {
      len ++;
      if (len == size) {
	size *= 2;
	aint_ptr = realloc(aint_ptr, size);
	if (aint_ptr == NULL)
	  fatal("strdup_from_object_space failed to %d bytes\n", size);
      }
    }
  }
  return aint_ptr;
}


void strcpy_from_object_space(process_ptr process, char *aint_ptr, ulong object_ptr)
{
  long i;
  /* optimize this later */
  for (i=0; ; i++) {
    char c = * (char *) addr2phys(process, object_ptr + i, NULL);
    aint_ptr[i] = c;
    if (c == '\0') break;
  }
}

void strcpy_to_object_space(process_ptr process, ulong object_ptr, const char *aint_ptr)
{
  long i;
  /* optimize this later */
  for (i=0; ; i++) {
    char c = aint_ptr[i];
    * (char *) addr2phys(process, object_ptr + i, NULL) = c;
    if (c == '\0') break;
  }
}




int zap_gp_updates = 0;

/* Decode the given instruction: determine the aint opnum for the inst. */

#define FP_REDIRECT(iarg) ((iarg==ZERO_REGISTER) ? (iarg):(iarg) + N_INT_LOGICALS)

static
#ifdef __GNUC__
  inline
#else
#  pragma inline(decode_opnum_group_entry)
#endif
int decode_opnum_group_entry(group_t *group, int function)
{
  int i;
  for (i=0; group[i].opnum != 0; i++) {
    if (group[i].opkey == function)
      return group[i].opnum;
  }
  return default_opnum;
}

static void
decode_opnum(icode_ptr picode, unsigned instr)
{
  int opcode;
  union alpha_instruction uinst;

  picode->instr = instr;

  uinst = * (union alpha_instruction *) &instr;

  opcode = uinst.common.opcode;

  /* Now, use the opcode and other fields to zero in on the actual
   * instruction.
   */

  switch (opcode) {
  case op_call_pal:
    picode->opnum = decode_opnum_group_entry(pal_group, uinst.pal_format.function);
    break;
  case op_inta:
    picode->opnum = decode_opnum_group_entry(inta_group, uinst.o_format.function);
    break;
  case op_intl:
    picode->opnum = decode_opnum_group_entry(intl_group, uinst.o_format.function);
    break;
  case op_intm:
    picode->opnum = decode_opnum_group_entry(intm_group, uinst.o_format.function);
    break;
  case op_ints:
    picode->opnum = decode_opnum_group_entry(ints_group, uinst.o_format.function);
    break;
  case op_fltv:
    picode->opnum = decode_opnum_group_entry(fltv_group, uinst.f_format.function);
    break;
  case op_flti:
    picode->opnum = decode_opnum_group_entry(flti_group, uinst.f_format.function);
    break;
  case op_fltl:
    picode->opnum = decode_opnum_group_entry(fltl_group, uinst.f_format.function);
    break;
  case op_opc14:
    picode->opnum = decode_opnum_group_entry(opc14_group, uinst.f_format.function);
    break;
  case op_intmisc:
    picode->opnum = decode_opnum_group_entry(intmisc_group, uinst.o_format.function);
    break;
#ifdef NEW_MVI
  case op_opc04:
    picode->opnum = decode_opnum_group_entry(opc04_group, uinst.f_format.function);
    break;
#endif

  case op_misc:
    picode->opnum = decode_opnum_group_entry(misc_group, 0xFFFF&uinst.m_format.memory_displacement);
    break;

  case op_jsr:
    picode->opnum = decode_opnum_group_entry(jsr_group, uinst.j_format.function);
    break;

  default:
    picode->opnum = decode_opnum_group_entry(defgroup, uinst.common.opcode);
  }
}

/* Decode an instruction: store information in an icode struct */

/*
 * Roger: Set this to 1 if you want a dump of every instruction being decoded
 */
#define DEBUG_DECODE 0

void
decode_instr(process_ptr process, ulong textaddr, unsigned instr, 
	     icode_ptr picode, icode_ptr next_picode)
{
  PFPI simfunc;

  int opcode, opnum;
  union alpha_instruction uinst;

  picode->next = next_picode;
  picode->addr = textaddr;

  uinst.word = instr;

  /* fix up universal no-ops here */
  if (   uinst.o_format.ra == 31
      && uinst.o_format.opcode == 0xb)
    picode->iflags |= E_WAS_LDQ_U;

  if (   uinst.o_format.ra == 31
      && uinst.o_format.opcode == 0xb
      && aint_recognize_unop) {
    picode->opnum = nop_opn;
    picode->iflags |= E_WAS_LDQ_U;
    uinst.f_format.opcode = op_fltl;
    uinst.f_format.function = fltl_cpys;
    uinst.f_format.fa = 31;
    uinst.f_format.fb = 31;
    uinst.f_format.fc = 31;
    if (0)
      fprintf(Aint_output, "LOADER: unop pc=%lx instr=%x new instr=%x\n", picode->addr, instr, uinst.word);
    instr = uinst.word;
    *(int *)addr2phys(process, picode->addr, NULL) = instr;
  }

  picode->instr = instr;
  picode->markers = 0;  /* clear all markers in this inst */

 retry_decode:

  picode->dest = MaxArgs;
  picode->args[RA] = picode->args[RB] = picode->args[RC] = ZERO_REGISTER;

  /* Get the aint operation number */

  opcode = uinst.common.opcode;
  decode_opnum(picode, instr);
  opnum = picode->opnum;

  /* At this time, we know the opnum, so we can access information stored
   * in the op_desc desc_table. Information desired is the function pointer,
   * the instruction type etc.
   */
  picode->func = desc_table[opnum].func;
  /* iflags is initially zero, and may have E_BB_ set from previous picodes */
  picode->iflags |= desc_table[picode->opnum].iflags;
  assert(picode->func != NULL);
  
  /*
   * Decode instruction arguments
   */

  switch (OpFormat[opcode]) {
  case PAL:
    picode->immed = uinst.pal_format.function;
    break;

  case Reserved:
    break;

  case Memory:
    picode->immed = uinst.m_format.memory_displacement;
    /*
     * Sign-extend the immed field if opcode is not op_misc
     */
    if (opcode != op_misc) {
      picode->immed = (picode->immed | ((picode->immed>>15) ? ~0xffff : 0));
      if (opnum == ldah_opn) picode->immed = picode->immed << 16;
    }
    picode->args[RB] = uinst.m_format.rb;
    /*
     * If this is a load, redirect loads into the zero reg.
     * Also, the m_format is being used to handle j_format,
     * so look for those...
     */
    switch(uinst.m_format.opcode) {
    case op_ldf: 
    case op_ldg: 
    case op_lds: 
    case op_ldt: 
      picode->args[RA] = FP_REDIRECT(uinst.m_format.ra);

#ifndef NEW_TTL    
      picode->dest = RA;    /* this is not convenient to ttl stores! */
#endif      
      
#ifdef NEW_TTL
      /* Federico Ardanaz Notes:
       * Currently we use allways opcode 0x20 to ttl loads and 0x21 to stores
       * in /usr/sys/.../inst.h we can find     
       * #define op_ldf 0x20
       * #define op_ldg	0x21
       * So our stores are patched inside the general load brach !
       * I think this is a little problem due to the fact that 
       * in load branch we initialize picode->dest = RA and this have
       * little sense to a store!
       * To overcome this we put this statment iside #ifndef...
       */
       
      if ( uinst.m_format.opcode != 0x21 )          /* if it isnt't a ttl store */
          picode->dest = RA;      
          
      if (( uinst.m_format.opcode == 0x20 )||( uinst.m_format.opcode == 0x21 ))
       patch_vaxfp_insn(process,picode,textaddr,uinst);

#endif

      break;
    case op_stf:
    case op_stg:
    case op_sts:
    case op_stt:
      picode->args[RA] = FP_REDIRECT(uinst.m_format.ra);
      break;
    case op_ldq_u: 
      if ( (picode->iflags & E_WAS_LDQ_U) != 0) {
	  picode->args[RB] = ZERO_REGISTER;
	  picode->iflags &= ~(E_READ);
      }
    case op_ldbu:
    case op_ldwu:
    case op_ldl: 
    case op_ldq: 
    case op_pal1b: 
    case op_ldl_l: 
    case op_ldq_l: 
    case op_stl_c:
    case op_stq_c:
    case op_jsr:
      picode->dest = RA;
      picode->args[RA] = uinst.m_format.ra;
      break;
    case op_lda:
    case op_ldah:
      if (zap_gp_updates && uinst.m_format.ra == GP) {
	/* convert it to a noop */
	uinst.f_format.opcode = op_fltl;
	uinst.f_format.function = fltl_cpys;
	uinst.f_format.fa = 31;
	uinst.f_format.fb = 31;
	uinst.f_format.fc = 31;
	instr = uinst.word;
	zap_gp_updates--;

	picode->instr = instr;
	*(int *)addr2phys(process, picode->addr, NULL) = instr;

	picode->iflags = 0;

	goto retry_decode;

      }	
      picode->dest = RA;
      picode->args[RA] = uinst.m_format.ra;
      break;
    default:
      picode->args[RA] = uinst.m_format.ra;
      break;
    }
    break;

  case Operate:
#ifdef USE_FAST_FUNC
    picode -> op_form = uinst.o_format.form;
#endif
    if (!uinst.o_format.form) {
      if (opnum == ftois_opn || opnum == ftoit_opn)
 	/* ftoiX: source is an FP register */
 	picode->args[RA] = FP_REDIRECT(uinst.o_format.ra);
      else
 	picode->args[RA] = uinst.o_format.ra;
      picode->args[RB] = uinst.o_format.rb;
      picode->args[RC] = uinst.o_format.rc;
      picode->literal = 0xffff;
      picode->immed = uinst.o_format.function;
      /* fprintf(Aint_output, "<%d> r%d, r%d, r%d\n", picode->immed, picode->args[RA],
	 picode->args[RB], picode->args[RC]); */
    }
    else {
      picode->args[RA] = uinst.l_format.ra;
      picode->args[RB] = ZERO_REGISTER;
      picode->args[RC] = uinst.o_format.rc;
      picode->literal = uinst.l_format.literal;
      picode->immed = uinst.l_format.function;
      picode->iflags |= E_LITERAL;
      /* fprintf(Aint_output, "<%d> r%d, #0x%x, r%d\n", picode->immed,
	 picode->args[RA], picode->literal, picode->args[RC]); */
    }
    picode->dest = RC;
    break;
  case Branch: {
    ulong targetaddr;
    switch (uinst.common.opcode) {
    case op_jsr: {
      int hint = uinst.j_format.hint; /* too small => not very useful -Jamey 11/6/97 */
      picode->immed = hint;
      picode->dest = RA;
      picode->args[RA] = uinst.j_format.ra;
      picode->args[RB] = uinst.j_format.rb;
      targetaddr = findJSRTargetPC(picode->addr);
      if (0) fprintf(Aint_output, "JSR pc=%lx targetaddr=%lx\n", picode->addr, targetaddr);
    } break;
    default:
      picode->immed = uinst.b_format.branch_displacement;
      picode->immed = (picode->immed | ((picode->immed >> 20) ? ~0x1fffff : 0));
      targetaddr = picode->addr + (picode->immed<<2) + 4;
    }

    if (replace_sqrt_function_calls)
    if (   (targetaddr >= sqrt_function_addr && targetaddr < sqrt_function_addr + 64)
	|| (targetaddr >= f_sqrt_function_addr && targetaddr < f_sqrt_function_addr + 64)
	|| (targetaddr >= sqrtf_function_addr && targetaddr < sqrtf_function_addr + 64)
	|| (targetaddr >= f_sqrtf_function_addr && targetaddr < f_sqrtf_function_addr + 64)){
      /* replace the jsr with a sqrtt instruction, 
	 then branch to top of switch to cause it to be decoded properly. */
      if (!sqrt_notification) {
	warning("Replacing call to sqrt() with sqrtt instruction at pc=%lx (targetaddr=%lx)\n"
	       "\t No further notices will be given.\n",
	       picode->addr, targetaddr);
	sqrt_notification=1;
      }
      uinst.f_format.opcode = op_opc14;
      if (   (targetaddr >= sqrtf_function_addr && targetaddr < sqrtf_function_addr + 64)
	  || (targetaddr >= f_sqrtf_function_addr && targetaddr < f_sqrtf_function_addr + 64))
	uinst.f_format.function = opc14_sqrts;
      else
	uinst.f_format.function = opc14_sqrtt;

      /* Eric: changed 0 to 31 to avoid artificial dependencies */
      uinst.f_format.fa = 31;	
      uinst.f_format.fb = 16;
      uinst.f_format.fc = 0;
      instr = uinst.word;
      zap_gp_updates = 2;

      picode->instr = instr;
      *(int *)addr2phys(process, picode->addr, NULL) = instr;
      picode->iflags = 0;

      goto retry_decode;
    }
    if (process_find_segment(process, targetaddr)) {
      picode->target = addr2iphys(process, targetaddr, NULL);
    } else {
      /* this instruction doesn't branch to a mapped location, so make it loop to itself */
      picode->target = picode;
    }
    switch (uinst.b_format.opcode) {
    case op_br:
    case op_bsr:
      picode->args[RA] = uinst.b_format.ra;
      picode->dest = RA;
      break;
    default:
      picode->args[RA] = (picode->iflags & E_FLOAT) ? FP_REDIRECT(uinst.b_format.ra): uinst.b_format.ra;
      break;
    }
    if (aint_parse_basic_blocks) {
      picode->iflags |= E_BB_LAST;
      picode->target->iflags |= E_BB_FIRST;
      if (next_picode != NULL)
	next_picode->iflags |= E_BB_FIRST;
    }
    /* fprintf(Aint_output, "r%d, 0x%x\n", picode->args[RA], picode->immed); */

#ifdef NEW_TLDS
    /*
     * Now that this instruction is fully decoded, check if we have to patch it!
     */
    if ( is_call_to_tlds(targetaddr) != -1 ) {
      fprintf(Aint_output, "TLDS BSR pc=%lx targetaddr=%lx\n", picode->addr, targetaddr);

      tlds_patch_binary(process,picode,targetaddr);
    }
#endif

  } break;
  case Float:
    if (opcode == op_opc14 && zap_bogus_ldgp_after_sqrtt) {
      /* @@@@ assume this is sqrt */
      zap_gp_updates = 2; /* in case we have a bogusly patched executable */
    }
    if (opnum == itofs_opn || opnum == itoff_opn || opnum == itoft_opn)
      /* source is an integer register */
      picode->args[RA] = uinst.f_format.fa;
    else
      picode->args[RA] = FP_REDIRECT(uinst.f_format.fa);
    picode->args[RB] = FP_REDIRECT(uinst.f_format.fb);
    picode->args[RC] = FP_REDIRECT(uinst.f_format.fc);
    picode->immed = uinst.f_format.function;
    picode->dest = RC;
    picode->iflags |= E_FLOAT;
    if ( DEBUG_DECODE ) fprintf(Aint_output, "x%x FLOAT <%d> f%d, f%d, f%d\n", picode->addr, picode->immed, picode->args[RA],
       picode->args[RB], picode->args[RC]); 
#ifdef NEW_TTL
    if ( is_vaxfp_insn(uinst,textaddr) ) {
      patch_vaxfp_insn(process,picode,textaddr,uinst);
    }
#endif
    break;
  default:
    break;
  }

  /*
   * WARNING WARNING WARNING 
   * WARNING WARNING WARNING 
   * WARNING WARNING WARNING 
   * WARNING WARNING WARNING 
   *
   * Under TARANTULA and TLDS, the input picode and the final picode will most likely
   * be different. Hence, from this point onward in the code, avoid using local
   * variables!! ALWAYS ACCESS picode fields !!!!
   */

  if ((picode->iflags & (E_READ)) && picode->args[RA] == ZERO_REGISTER) {
   picode->iflags |= E_PREFETCH;
  }

#if !USE_ATOM_OUTPUT
  if (!aint_parse_basic_blocks)
    picode->iflags |= E_BB_FIRST|E_BB_LAST;
#endif

  if (0)
	fprintf(Aint_output, 
		"DECODE: %lx: \t %x \t %s\n",
		picode->addr, 
		instr, 
		desc_table[picode->opnum].opname);

  /*
   * Check to see if this is the address of the sim_user
   * procedure entry, and if so, insert a call to the sim_user
   * event
   */
  if ( sim_user_addr != 0 && picode->addr == sim_user_addr ) {
    warning("sim_user event not handled (addr=%lx)\n", sim_user_addr);
  }

  simfunc = (PFPI) NULL;
  if (picode->iflags & E_READ)		simfunc = event_read;
  else if ((picode->iflags & (E_WRITE|E_CACHEOP)) == (E_WRITE|E_CACHEOP)) {
    simfunc = event_wh64;
  }
  else if (picode->iflags & E_WRITE)	simfunc = event_write;
  else if (picode->iflags & E_LD_L)	simfunc = event_load_locked;
  else if (picode->iflags & E_ST_C)	simfunc = event_store_conditional;
  else if (picode->iflags & E_BARRIER)	simfunc = event_memory_barrier;
  else if (Trace_option & TRACE_INST)	simfunc = event_inst; 

  if (Trace_option & TRACE_REFS) {
   picode->trace_private = (Trace_option & TRACE_PRIVATE) ? 1 : 0;
   picode->trace_shared = (Trace_option & TRACE_SHARED) ? 1 : 0;
  }

  /* 
   * for all the load and store ops, record size and ufunc in icode 
   */
  if (picode->iflags & (E_MEM_REF|E_LOCK)) {
   switch (picode->opnum) {
      case ldb_opn:
      case stb_opn:
	picode->size = 1;
	break;
      case ldw_opn:
      case stw_opn:
	picode->size = 2;
	break;
      case ldl_opn:
      case stl_opn:
      case ldl_l_opn:
      case stl_c_opn:
      case ldf_opn:
      case lds_opn:
      case stf_opn:
      case sts_opn:
	picode->size = 4;
	break;
      case ldq_u_opn:
      case ldq_opn:
      case ldg_opn:
      case ldt_opn:
      case pal1b_opn:	/* HW_LD */
      case stq_u_opn:
      case stq_opn:
      case stg_opn:
      case stt_opn:
      case ldq_l_opn:
      case stq_c_opn:
	picode->size = 8;
	break;
#ifdef NEW_TTL
      case ttl_vldt_opn:
      case ttl_vstt_opn:
      case ttl_vldq_opn:
      case ttl_vstq_opn:
	  case ttl_vgathq_opn:
	  case ttl_vgatht_opn:
	  case ttl_vscatq_opn:
	  case ttl_vscatt_opn:
	  case ttl_vncgathq_opn:
	  case ttl_vncgatht_opn:
	  case ttl_vncscatq_opn:
	  case ttl_vncscatt_opn:
	  /* prefetching ones */
      case ttl_vldpf_opn:
      case ttl_vstpf_opn:
	  case ttl_vgathpf_opn:
	  case ttl_vscatpf_opn:
	  case ttl_vncgathpf_opn:
	  case ttl_vncscatpf_opn:
	  
	printf("TTL Setting size = 8\n");
	picode->size = 8;
	break;
    
      case ttl_vlds_opn:
      case ttl_vsts_opn:
      case ttl_vldl_opn:
      case ttl_vstl_opn:
	  case ttl_vgathl_opn:
	  case ttl_vgaths_opn:
	  case ttl_vscatl_opn:
	  case ttl_vscats_opn:
	  case ttl_vncgathl_opn:
	  case ttl_vncgaths_opn:
	  case ttl_vncscatl_opn:
	  case ttl_vncscats_opn:
	printf("TTL Setting size = 4\n");
	picode->size = 4;
	break;
#endif

      default:
	break;
   }
  }

  if (picode->iflags & E_BRANCH) {
    zap_gp_updates = 0;
  }

  if (simfunc) {
    /* we recover the original func by looking at desc_table[picode->opnum].func; -Jamey 1/7/97*/
    picode->func = simfunc;
    picode->iflags |= E_SPECIAL;
  }
}

void
decode_text_region(process_ptr process, ulong start, size_t size)
{
  size_t i;
  icode_ptr picode = addr2iphys(process, start, NULL);
  informative("decode_text_region: startaddr=%lx, size=%ld, picode=%p\n", start, size, picode);
  process->n_instructions += size/sizeof(int);

  if (!Quiet) fprintf(Aint_output,"MY decode_text_region size = %ld\n",size);
  for (i = 0; i < size; i += sizeof(int)) {
    ulong textaddr = start + i;
    ulong nextaddr = textaddr + sizeof(unsigned int);
    unsigned instr = *(unsigned int *)addr2phys(process, textaddr, NULL);
    icode_ptr next_picode = addr2iphys(process, nextaddr, NULL);
    if (next_picode == NULL)
      next_picode = picode;

    if (picode)
      decode_instr(process, textaddr, instr, picode, next_picode);

    /* get ready for the next instruction */
    picode = next_picode;
  }
  if (!Quiet) fprintf(Aint_output,"MY decode_text_region DONE\n");
}


unsigned int picode_instr(thread_ptr pthread, icode_ptr picode)
{
 if (picode != NULL)
   return picode->instr;
 else
   return 0xdeadbeef;
}


void
load_dynamic_section(process_ptr process, object_file_t *pof, asection *section)
{
  bfd *abfd = pof->abfd;
  gotinfo_t tgotinfo[1024];
  Elf32_Word local_gotno_count = 0;
  Elf32_Word gotsym_count = 0;
  int size = bfd_section_size(abfd, section);
  Elf32_Dyn *dyncontents = (Elf32_Dyn*)malloc(size);
  Elf32_Dyn *dyn;
  Elf32_Word i;

  bfd_get_section_contents(abfd, section, (char*)dyncontents, 0, size);

  for (dyn = dyncontents; 
       dyn->d_tag != DT_NULL ; 
       dyn++) {
    switch (dyn->d_tag) {
    case DT_PLTGOT:
      pof->dt_pltgot = dyn->d_un.d_ptr;
      break;
    case DT_INIT:
      pof->dt_init = dyn->d_un.d_ptr;
      break;
    case DT_FINI:
      pof->dt_init = dyn->d_un.d_ptr;
      break;
    case DT_MIPS_TIME_STAMP:
      pof->dt_mips_time_stamp = dyn->d_un.d_val;
      break;
    case DT_MIPS_ICHECKSUM:
      pof->dt_mips_ichecksum = dyn->d_un.d_val;
      break;
    case DT_MIPS_LOCAL_GOTNO:
      tgotinfo[local_gotno_count++].local_gotno = dyn->d_un.d_val;
      break;
    case DT_MIPS_GOTSYM:
      tgotinfo[gotsym_count++].gotsym = dyn->d_un.d_val;
      break;
    }
  }
  free(dyncontents);
  pof->gotinfo = (gotinfo_t *)malloc(local_gotno_count * sizeof(gotinfo_t));
  for (i = 0; i < local_gotno_count; i++) {
    pof->gotinfo[i] = tgotinfo[i];
  }
  pof->ngots = local_gotno_count;
}

void 
load_liblist_section(process_ptr process, object_file_t *pof, asection *section)
{
  bfd *abfd = pof->abfd;
  int size = bfd_section_size(abfd, section);
  Elf32_Lib *libs = (Elf32_Lib *)malloc(size);
  int nlibs = size/sizeof(Elf32_Lib);

  bfd_get_section_contents(abfd, section, libs, 0, size);
  /* record each library for later loading */
  fprintf(Aint_output, "load_liblist_section: %d libs needed for file %s\n",
	  nlibs, pof->filename);
  pof->liblist = libs;
  pof->n_libs_needed = nlibs;
}

Elf32_Sym *
resolve_symbol(object_file_t *pof, Elf32_Sym *sym)
{
  int i;
  for (i = 0; i < pof->n_libs_needed; i++) {
    const char *name = 
      object_file_dynstr(pof, pof->liblist[i].l_name);
    char filename[1024];
    object_file_t *rpof;
    sprintf(filename, "/usr/shlib/%s", name);
    rpof = object_file_get(filename);
    {
      const char *name = object_file_dynstr(pof, sym->st_name);
      Elf32_Sym *defsym;
      defsym = object_file_dynsym_from_name(rpof, name);
      if (defsym && defsym->st_value != 0) {
	fprintf(Aint_output, "resolve_symbol: found def for %s in %s\n",
		name, rpof->filename);
	return defsym;
      }
    }
  }
  return NULL;
}

static void adjust_got_section(process_ptr process, object_file_t *pof)
{
  /* object space address */
  ulong object_gotbase = pof->dt_pltgot;
  long nGot = pof->ngots;
  long i = 1;			/* skip over first NULL GOT entry */
  Elf32_Word n;

  fprintf(Aint_output, "GOT: %ld GOT in %s\n", nGot, pof->filename);
  for (n = 0; n < nGot; n++) {
    gotinfo_t gotinfo = pof->gotinfo[n];
    fprintf(Aint_output, ".GOT[%d]: \t {%d, %d}\n", 
	   n, gotinfo.local_gotno, gotinfo.gotsym);
    for (i = gotinfo.local_gotno; 1; i++) {
      Elf32_Addr *pgoti = 
	(Elf32_Addr *)addr2phys(process, object_gotbase + i*sizeof(Elf32_Addr), NULL);
      Elf32_Addr goti = *pgoti;
      if (goti == 0 && i >= gotinfo.local_gotno) {
	fprintf(Aint_output, "    end of GOT[%d]\n", n);
	fprintf(Aint_output, "\t%12lx \t %ld\n", goti,
	       i - gotinfo.local_gotno);
	break;
      }
      if (i == gotinfo.local_gotno) {
	fprintf(Aint_output, "    externals for GOT[%d]:\n", n);
      }
      if (i < gotinfo.local_gotno) {
	fprintf(Aint_output, "\t%12lx \t %ld\n", goti,
	       i - gotinfo.local_gotno);
      } else {
	Elf32_Sym *sym = 
	  object_file_dynsym_from_index(pof, (i - gotinfo.local_gotno) + gotinfo.gotsym);
	fprintf(Aint_output, "\t%12lx \t %ld \t %12lx %s\n", 
	       goti, 
	       i - gotinfo.local_gotno,
	       sym->st_value,
	       object_file_dynstr(pof, sym->st_name));
	if (1 || sym->st_value == 0) {
	  /* needs resolving */
	  Elf32_Sym *defsym = resolve_symbol(pof, sym);
	  if (defsym) {
	    fprintf(Aint_output, "\t found definition: %lx\n", defsym->st_value);
	    *pgoti = defsym->st_value;
	  }
	}
      }
    }
    i++;			/* skip over the NULL entry before starting the next GOT */
  }
}



/* 
 * Reads the text section of the object file and creates the linked list
 * of icode structures
 */

#if USE_ATOM_OUTPUT
static void
read_bb_info(process_ptr process, const char *Objname)
{

  icode_ptr picode;
  char bbname[80];
  char bbline[80];
  FILE *Fbb;
  long bbaddr;
  int bbinst, bbcode;

  bbline[0]=0;
  strcpy(bbname, Objname);
  Fbb = fopen(strcat(bbname, ".bb"), "rt");
  if (Fbb == NULL) {
    char command[2048];
    sprintf(command, "atom %s %s > %s",
	    Objname,
	    "/udir/pvega/AINT/src/bbdump.inst.o",
	    bbname);
    fprintf(Aint_output, "BBDUMP: %s ... ", command);
    system(command);
    fprintf(Aint_output, "done\n");
    Fbb = fopen(bbname, "rt");
    assert(Fbb != NULL);
  }

  while (fgets(bbline, 80, Fbb) != NULL) {
    sscanf(bbline, "%lx%x%d", &bbaddr, &bbinst, &bbcode);
    picode = addr2iphys(process, bbaddr, NULL);
    if (bbcode & BB_FIRST) 
      picode->iflags |= E_BB_FIRST;
    if (bbcode & BB_LAST) 
      picode->iflags |= E_BB_LAST;
  }
  fclose(Fbb);
}
#endif



/* 
 * analyze_code_segment(process, start, size):
 *    - look for conditional mov/store opportunities 
 */
static void analyze_code_segment(process_ptr process, ulong start, ulong size)
{
  size_t i;
  thread_ptr pthread = &Threads[process->pid];
  informative("analyze_code_segment: startaddr=%lx, size=%ld",
	      start, size);
  process->n_instructions += size/sizeof(int);
  for (i = 0; i < size; i += sizeof(int)) {
    ulong textaddr = start + i;
    icode_ptr picode = addr2iphys(process, textaddr, NULL);
    if (picode->iflags & E_CONDBR) {
      if (picode->target->addr == textaddr + 8) {
	icode_ptr picode_op = picode->next;
	if (picode_op->iflags & (E_WRITE|E_READ|E_ST_C|E_LD_L)) {
	  fprintf(Aint_output, "    found conditional %s\n",
		  (picode_op->iflags & (E_WRITE|E_ST_C)) ? "WRITE" : "READ");
	  fprintf(Aint_output, "    %16lx %8x \t %s $%d, %lx\n", 
		  textaddr, picode_instr(pthread, picode), desc_table[picode->opnum].opname,
		  picode->args[RA], picode->target->addr);
	  fprintf(Aint_output, "    %16lx %8x \t %s $%d, %ld($%d)\n", 
		  textaddr+4, picode_instr(pthread, picode), 
		  desc_table[picode_op->opnum].opname,
		  picode_op->args[RA], picode_op->immed, picode_op->args[RB]);
	} else if ((picode_op->iflags & E_BRANCH) == 0) {
	  int uses_lit;
	  uses_lit = ((picode->iflags & E_LITERAL) != 0);
	  fprintf(Aint_output, "    found conditional OP\n");
	  fprintf(Aint_output, "    %16lx %8x \t %s $%d, %lx\n", 
		  textaddr, picode_instr(pthread, picode), desc_table[picode->opnum].opname,
		  picode->args[RA], picode->target->addr);
	  fprintf(Aint_output, "    %16lx %8x \t %s $%d, %s%ld, $%d\n", 
		  textaddr+4, picode_instr(pthread, picode), 
		  desc_table[picode_op->opnum].opname,
		  picode_op->args[RA], 
		  (uses_lit ? "" : "$"),
		  (uses_lit ? picode->literal : (ulong)picode_op->args[RB]),
		  picode_op->args[RC]);
	}
      }
    }
  }

}



extern char *xmalloc(size_t);

int aint_symbol_compare(aint_symbol_t *sym1, aint_symbol_t *sym2)
{
  if (sym1->base < sym2->base)
    return -1;
  else if (sym1->base > sym2->base)
    return 1;
  else
    return 0;
}

void basic_block_boundaries_from_symtab(process_ptr process, object_file_t *pof)
{
  bfd *abfd = pof->abfd;
  long storage_needed = bfd_get_symtab_upper_bound(abfd);
  process->symtab = NULL;
  process->nsymbols = 0;
  if (storage_needed > 0) {
    asymbol **symtab = (asymbol**)xmalloc(storage_needed);
    long number_of_symbols = 
      bfd_canonicalize_symtab(abfd, symtab);
    if (number_of_symbols < 0) {
      bfd_perror("basic_block_boundaries_from_symtab");
    } else {
      long i;
      long nchars = 0;
      long nprocsyms = 0;
      informative("basic_block_boundaries_from_symtab: %ld symbols\n", 
		  number_of_symbols);
      for (i = 0; i < number_of_symbols; i++) {
	asymbol *sym = symtab[i];
	ulong value = bfd_asymbol_value(sym);
	if (sym->section->flags & SEC_CODE) {
	  icode_ptr picode = addr2iphys(process, value, NULL);
	  icode_ptr prev_picode = addr2iphys(process, value-4, NULL);
	  if (!picode || picode->addr != value)
	    continue;
	  if (0 && Verbose)
	    fprintf(Aint_output, "BBBFS:    %12lx  %12lx \t%s %s\n", 
		    (prev_picode ? value-4 : 0),
		    value,
		    ((sym->flags & BSF_LOCAL) 
		     ? "L"
		     : ((sym->flags & BSF_GLOBAL) 
			? "X"
			: " ")), 
		    sym->name);
	  
	  picode->iflags |= E_BB_FIRST;
	  if (prev_picode) prev_picode->iflags |= E_BB_LAST;
	  nchars += strlen(sym->name) + 1; /* save room for \0 */
	  nprocsyms++;
	}
      }
      
      /* now create the name table */
      {
	char *chars = malloc(nchars);
	char *pchars = chars;
	int symnum = 0;
	aint_symbol_t *aint_symtab = calloc(nprocsyms+1, sizeof(aint_symbol_t));
        if (aint_symtab == NULL)
           fatal("failed to allocate aint_symtab");
	for (i = 0; i < number_of_symbols; i++) {
	  asymbol *sym = symtab[i];
	  ulong value = bfd_asymbol_value(sym);
	  if (sym->section->flags & SEC_CODE) {
	    int len = strlen(sym->name);
	    icode_ptr picode = addr2iphys(process, value, NULL);
	    if (!picode || picode->addr != value)
	      continue;
	    strcpy(pchars, sym->name);
	    pchars[len] = '\0';
	    aint_symtab[symnum].base = value;
	    aint_symtab[symnum].name = pchars;
	    symnum++;
	    pchars += len + 1;
	  }
	}

	/* guard symbol */
	aint_symtab[symnum].base = (ulong)-1;
	aint_symtab[symnum].name = "(nosymbol)";

	qsort(aint_symtab, nprocsyms, sizeof(aint_symbol_t), (void*)aint_symbol_compare); 

	process->symtab = aint_symtab;
	process->nsymbols = nprocsyms;

      }
    }
  } else if (storage_needed == 0) {
    printf("basic_block_boundaries_from_symtab: no symbols\n");
  } else {
    printf("basic_block_boundaries_from_symtab: symbol table problem\n");
    bfd_perror("basic_block_boundaries_from_symtab");
  } 
}


static int lastsymno[MAX_NPROCS];

const char *aint_lookup_symbol_name(process_ptr process, ulong addr)
{
   int pid = process->pid;
   int i = lastsymno[pid];
   aint_symbol_t *symtab = process->symtab;
   int nsymbols = process->nsymbols;
   if (nsymbols > 0) {
      if (addr < symtab[0].base) {
         return symtab[0].name;
      }
      if (addr >= symtab[nsymbols-1].base) {
         return symtab[nsymbols-1].name;
      }

      if (addr < symtab[i].base)
         i = 0;

      while (i < nsymbols) {
         if (addr >= symtab[i].base && addr < symtab[i+1].base)
            break;
         i++;
      }
      lastsymno[pid] = i;
      return symtab[i].name;
   } else {
      return "<nosymbol>";
   }
}




typedef struct {
  long fpcr;       /* The Floating point control register */
  RegType Reg[TOTAL_PHYSICALS];
  Physical_RegNum FirstFreePhysicalRegister; /* free list linked through Reg array */
  Physical_RegNum FirstFreeVectorPhysicalRegister; /* free list linked through Reg array */
  Physical_RegNum RegMap[TOTAL_LOGICALS];

  ulong pc;
  state_t runstate;    /* status of thread (runnable, sleeping etc. */
  int tid;             /* thread id assigned by aint */

  aint_time_t time;    /*simulated time for this thread */
  aint_time_t cpu_time;    /* accumulated cpu time for this thread */

  int exitcode;
  int semval;        /* waiting for this semaphore value */
  int terrno;           /* most recent */

  int signal;      
  int sigblocked;    /* bit i=1 if signal i currently blocked */
  int sigpending;    /* bit i=1 if signal i currently pending */

  long unique; /* process unique value used by rduniq and wruniq */
  long prio;   /* used for getprio/setprio syscalls */

  thread_ptr wthread;     /* thread waiting on us */

} snapshot_thread_t;

void snapshot_threads(process_ptr process, FILE *out)
{
  int i;
  int nthreads = process->thread_count;
  thread_ptr pthread = process->threads;
  fprintf(out, "nthreads=%d\n", nthreads); fflush(out);
  for (i = 0; i < nthreads; i++) {
    snapshot_thread_t st;
    st.fpcr = pthread->fpcr;
    memcpy(&st.Reg, pthread->Reg, TOTAL_PHYSICALS*sizeof(RegType));
    st.FirstFreePhysicalRegister = pthread->FirstFreePhysicalRegister;
    st.FirstFreeVectorPhysicalRegister = pthread->FirstFreeVectorPhysicalRegister;
    memcpy(&st.RegMap, pthread->RegMap, TOTAL_LOGICALS*sizeof(Physical_RegNum));
    if (pthread->next_fetch_picode != NULL)
      st.pc = pthread->next_fetch_picode->addr;
    else
      st.pc = 0;
    informative("snapshot_thread: tid=%d pc=%lx\n", i, st.pc);
    st.runstate = pthread->runstate;
    st.tid = pthread->tid;
    st.time = pthread->time;
    st.cpu_time = pthread->cpu_time;
    st.exitcode = pthread->exitcode;
    st.semval = pthread->semval;
    st.terrno = pthread->terrno;
    st.signal = pthread->signal;
    st.sigblocked = pthread->sigblocked;
    st.sigpending = pthread->sigpending;
    st.unique = pthread->unique;
    st.prio = pthread->prio;
    if (pthread->wthread != NULL)
      warning("Trying to snapshot a thread with a wait on it.\n");
    fprintf(out, "thread%d\n", i); fflush(out);
    fwrite(&st, sizeof(st), 1, out);

    pthread = pthread->tsibling;
  }
}


void restore_threads(process_ptr process, FILE *in)
{
  int i;
  int tid;
  int nthreads;
  thread_ptr pthread = process->threads;
  thread_ptr last_thread = NULL;
  if (fscanf(in, "nthreads=%d\n", &nthreads) != 1)
    fatal("restore_threads failed to read nthreads");
  process->thread_count = nthreads;
  fprintf(Aint_output, "restore_threads: Nthreads=%d\n", Nthreads);
  Nthreads += (nthreads-1);
  fprintf(Aint_output, "\t now: Nthreads=%d\n", Nthreads);

  for (i = 0; i < nthreads; i++) {
    snapshot_thread_t st;
    /* now get a new thread */
    if (i > 0) {
      event_ptr pevent;
      INLINE_DEQUEUE(&Free_q, pthread);
      NEW_ITEM(Event_free, sizeof(event_t), pevent, "init_main_thread");
      init_thread(pthread, pevent);
    }
    if (fscanf(in, "thread%d\n", &tid) != 1 || tid != i)
      fatal("restore_threads failed to read thread number");
    fread(&st, sizeof(st), 1, in);

    pthread->fpcr = st.fpcr;
    memcpy(pthread->Reg, &st.Reg, TOTAL_PHYSICALS*sizeof(RegType));
    pthread->FirstFreePhysicalRegister = st.FirstFreePhysicalRegister;
    memcpy(pthread->RegMap, &st.RegMap, TOTAL_LOGICALS*sizeof(Physical_RegNum));
    informative("restore_thread: tid=%d pc=%lx\n", i, st.pc);
    print_reg_state(i);
    if (st.pc != 0)
      pthread->next_fetch_picode = addr2iphys(process, st.pc, NULL);
    else
      pthread->next_fetch_picode = NULL;
    pthread->runstate = st.runstate;
    if (st.tid != pthread->tid)
      warning("need to fix up tid mismatches\n");
    /* pthread->tid = st.tid; */
    pthread->time = st.time;
    pthread->cpu_time = st.cpu_time;
    pthread->exitcode = st.exitcode;
    pthread->semval = st.semval;
    pthread->terrno = st.terrno;
    pthread->signal = st.signal;
    pthread->sigblocked = st.sigblocked;
    pthread->sigpending = st.sigpending;
    pthread->unique = st.unique;
    pthread->prio = st.prio;

    /* now link it into the process structure */
    pthread->process = process;
    if (last_thread != NULL) {
      last_thread->tsibling = pthread;
    }
    last_thread = pthread;
  }
}


typedef struct {
  ulong vpage;
  int flags;
  int prot;
  int hastextpage;
  struct TB_Entry *tbe;
} snapshot_page_descriptor_t;

int spd_compare(snapshot_page_descriptor_t *spd1, snapshot_page_descriptor_t *spd2)
{
  if (spd1->vpage < spd2->vpage)
    return -1;
  if (spd1->vpage > spd2->vpage)
    return 1;
  else
    return 0;
}

void snapshot_pages(process_ptr process, FILE *out)
{
  int tbn;
  size_t spdn = 0;
  size_t npages = process->num_pages;
  snapshot_page_descriptor_t *spd = malloc(npages * sizeof(snapshot_page_descriptor_t));
  for (tbn = 0; tbn < TB_SIZE; tbn++) {
    struct TB_Entry *tbe;
    for (tbe = process->TB[tbn]; tbe != NULL; tbe = tbe->next) {
      ulong tag = tbe->tag;
      ulong vpage = (tag << (TB_LISTIDX_LENGTH + TB_OFFSET_LENGTH)) | (tbn << TB_OFFSET_LENGTH);
      spd[spdn].vpage = vpage;
      spd[spdn].flags = tbe->flags;
      spd[spdn].prot = tbe->prot;
      spd[spdn].hastextpage = (tbe->textpage != NULL);
      spd[spdn].tbe = tbe;
      spdn++;
    }
  }
  qsort(spd, npages, sizeof(snapshot_page_descriptor_t), (int (*)(const void *,const void*))&spd_compare);

  /* write the number of pages */
  fprintf(out, "npages=%d\n", npages); fflush(out);
  /* first write the addresses */
  if (fwrite(spd, sizeof(snapshot_page_descriptor_t), npages, out) != npages)
    fatal("snapshot_pages: failed to write page descriptors, errno=%d\n", errno);
  /* then write the data */
  for (spdn = 0; spdn < npages; spdn++) {
    char buf[128];
    fprintf(Aint_output, " snapshot %12lx flags=%d prot=%d\n", 
	    spd[spdn].vpage, spd[spdn].flags, spd[spdn].prot);
    sprintf(buf, "vpage %16lx\n", spd[spdn].vpage);
    if (fwrite(buf, 128, 1, out) != 1)
      fatal("snapshot_pages: failed to write page descriptor %lx, errno=%d\n", spd[spdn].vpage, errno);
    if (fwrite(spd[spdn].tbe->page, TB_PAGESIZE, 1, out) != 1)
      fatal("snapshot_pages: failed to write page %lx, errno=%d\n", spd[spdn].vpage, errno);
  }
}


void restore_pages(process_ptr process, FILE *in)
{
  size_t spdn = 0;
  size_t npages = process->num_pages;
  snapshot_page_descriptor_t *spd;

  /* read the number of pages */
  if (fscanf(in, "npages=%d\n", &npages) != 1) 
    fatal("restore_pages failed to read npages");
  spd = malloc(npages * sizeof(snapshot_page_descriptor_t));
  /* first read the addresses */
  if (fread(spd, sizeof(snapshot_page_descriptor_t), npages, in) != npages)
    fatal("restore_pages: failed to read %d page descriptors: errno=%d\n", npages, errno);
  /* then read the data */
  for (spdn = 0; spdn < npages; spdn++) {
    char buf[128];
    ulong va = 0;
    void *page = (void*)addr2phys(process, spd[spdn].vpage, NULL);
    informative("restore_pages: reading vpage %lx\n", spd[spdn].vpage);

    if (fread(buf, 128, 1, in) != 1)
      fatal("restore_pages: failed to read page descriptor %lx, errno=%d\n", spd[spdn].vpage, errno);
    if (   (sscanf(buf, "vpage %16lx\n", &va) != 1)
	|| (va != spd[spdn].vpage))
      fatal("error reading page %lx: errno=%d va=%lx\n", spd[spdn].vpage, errno, va);
    if (fread(page, TB_PAGESIZE, 1, in) != 1)
      fatal("restore_pages: failed to read page %lx, errno=%d\n", va, errno);
    if (spd[spdn].hastextpage) {
      segment_t *segment = process_find_segment(process, spd[spdn].vpage);
      fprintf(Aint_output, "restore_pages: %lx has text page\n", spd[spdn].vpage);
      segment->prot |= PROT_EXEC;
      decode_text_region(process, spd[spdn].vpage, TB_PAGESIZE);
    } else {
      segment_t *segment = process_find_segment(process, spd[spdn].vpage);
      segment->prot &= ~PROT_EXEC;
    }
  }
}


typedef struct {
  int argc;
  int pid;
  int num_pages;
  int num_private;
  ulong Unsp_Shmat_Current;
  int thread_count;
  aint_time_t process_time;	/* simulated time for this process */
  aint_time_t process_cpu;	/* accumulated cpu time for this process */
  aint_time_t child_cpu;        /* accumulated cpu time for finished children */

  state_t runstate;
  int is_zombie;		/* =1 if process is almost dead */

  int n_segments;
  int n_instructions;
  ulong entry_point;		/* entry point for this process (virtual address) */
  ulong initial_gp;		/* GP value for this process upon entry */
  ulong stack_end;
  ulong brk;
    
  long nsymbols;

} snapshot_process_t;

void snapshot_symtab(process_t *process, FILE *out)
{
  int nsymbols = process->nsymbols;
  aint_symbol_t *symtab = process->symtab;
  int nchars = 0;
  int i;
  const char *symnames = process->symtab[0].name;
  for (i = 0; i < nsymbols; i++) {
    nchars += (strlen(symtab[i].name) + 1); /* name and null character */
    if (symnames > symtab[i].name)
      symnames = symtab[i].name;
  }

  fprintf(out, "symtab: nsymbols=%d nchars=%d\n", nsymbols, nchars);
  for (i = 0; i < nsymbols; i++) {
    fprintf(out, "%lx:%d\n", symtab[i].base, symtab[i].name - symnames);
  }
  fflush(out);
  fwrite(symnames, nchars, 1, out);
}

void restore_symtab(process_t *process, FILE *in)
{
  int nsymbols;
  aint_symbol_t *symtab;
  int nchars;
  int i;
  char *symnames;

  if (fscanf(in, "symtab: nsymbols=%d nchars=%d\n", &nsymbols, &nchars) != 2)
    fatal("restore_symtab failed to read nsymbols\n");
  process->nsymbols = nsymbols;
  symtab = process->symtab = realloc(process->symtab, (nsymbols+1) * sizeof(aint_symbol_t));
  symnames = malloc(nchars);
  for (i = 0; i < nsymbols; i++) {
    int offset;
    if (fscanf(in, "%lx:%d\n", &symtab[i].base, &offset) != 2)
      fatal("restore_symtab failed to read symbol %d\n", i);
    symtab[i].name = symnames+offset;
  }
  fread(symnames, nchars, 1, in);
}


void snapshot_shm(process_ptr process, FILE *out)
{
  int i;
  struct Shm_ds *shmds;
  int nshmds = 0;
  for (shmds = process->Shmem_regions; shmds != NULL; shmds = shmds->next) {
    nshmds++;
  }
  if (nshmds) 
    warning("snapshot_shm: nshmds=%d\n", nshmds);
}

void restore_shm(process_ptr process, FILE *in)
{
}

void snapshot_fds(process_ptr process, FILE *out)
{
  int vfd;
  for (vfd = 3; vfd < MAX_FDNUM; vfd++) {
    if (process->fd[vfd] != -1) {
      warning("open file descriptor %d", process->fd[vfd]);
    }
  }
}

void restore_fds(process_ptr process, FILE *in)
{
}

void snapshot_process(process_ptr process, const char *filename)
{
  FILE *out = fopen(filename, "w");
  char resolved_objname[PATH_MAX];
  snapshot_process_t sp;
  

  fprintf(out, "#!AintSnapshot-1.0\n");
  /* might be nice to get an abolute pathname here, but realpath() is not available in DU3.2 */
  strcpy(resolved_objname, ProcessArgv[process->pid][0]);
  fprintf(out, "Objname=%s\n", resolved_objname);
  sp.argc = ProcessArgc[process->pid];
  sp.pid = process->pid;
  sp.num_pages = process->num_pages;
  sp.num_private = process->num_private;
  sp.Unsp_Shmat_Current = process->Unsp_Shmat_Current;
  sp.thread_count = process->thread_count;
  sp.process_time = process->process_time;
  sp.process_cpu = process->process_cpu;
  sp.child_cpu = process->child_cpu;
  sp.runstate = process->runstate;
  sp.is_zombie = process->is_zombie;
  sp.n_segments = process->n_segments;
  sp.n_instructions = process->n_instructions;
  sp.entry_point = process->entry_point;
  sp.initial_gp = process->initial_gp;
  sp.stack_end = process->stack_end;
  sp.brk = process->brk;
  sp.nsymbols = process->nsymbols;
  
  fwrite(&sp, sizeof(sp), 1, out);
  fwrite(process->segments, sizeof(segment_t), process->n_segments, out);
  snapshot_symtab(process, out);
  snapshot_fds(process, out);
  snapshot_pages(process, out);
  snapshot_threads(process, out);
  snapshot_subst(out);
  fclose(out);
}

char *read_snapshot_objname(const char *filename)
{
  char resolved_objname[PATH_MAX];
  FILE *in = fopen(filename, "r");
  if (in == NULL) {
    fatal("Failed to open snapshot file '%s': %s\n", 
	  filename, sys_errlist[errno]);
  }
  if (fscanf(in, "#!AintSnapshot-1.0\n") != 0)
    fatal("%d is not a valid snapshot file\n", filename); 
  if (fscanf(in, "Objname=%s", resolved_objname) != 1)
    fatal("Failed to read objname from valid snapshot file\n", filename); 
  fclose(in);
  return strdup(resolved_objname);
}



void restore_process(process_ptr process, const char *filename)
{
  FILE *in = fopen(filename, "r");
  char resolved_objname[PATH_MAX];
  snapshot_process_t sp;

  if (fscanf(in, "#!AintSnapshot-1.0\n") != 0)
    fatal("restore_process: %s is not a valid snapshot file.\n", filename);
  if (fscanf(in, "Objname=%s\n", resolved_objname) != 1)
    fatal("restore_process: could not read objname from snapshot file %s.\n", filename);

  fread(&sp, sizeof(sp), 1, in);

  /* process->pid = sp.pid; */
  process->num_pages = sp.num_pages;
  process->num_private = sp.num_private;
  process->Unsp_Shmat_Current = sp.Unsp_Shmat_Current;
  process->thread_count = sp.thread_count;
  process->process_time = sp.process_time;
  process->process_cpu = sp.process_cpu;
  process->child_cpu = sp.child_cpu;
  process->runstate = sp.runstate;
  process->is_zombie = sp.is_zombie;
  process->n_segments = sp.n_segments;
  process->n_instructions = sp.n_instructions;
  process->entry_point = sp.entry_point;
  process->initial_gp = sp.initial_gp;
  process->stack_end = sp.stack_end;
  process->brk = sp.brk;
  process->nsymbols = sp.nsymbols;
  
  process->segments = realloc(process->segments, process->n_segments*sizeof(segment_t));
  fread(process->segments, sizeof(segment_t), process->n_segments, in);
  /* fix this later */
  { int i;
    for (i = 0; i < process->n_segments; i++) {
      process->segments[i].prot &= ~(long)PROT_EXEC;
    }
  }
  restore_symtab(process, in);
  restore_fds(process, in);
  restore_pages(process, in);
  restore_threads(process, in);
  restore_subst(in);
  fclose(in);
}
