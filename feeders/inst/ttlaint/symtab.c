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
 * Routines for reading the object file feader information
 */


#include <stdio.h>
#include <filehdr.h>
#include <aouthdr.h>
#include <scnhdr.h>
#include <syms.h>
#include <errno.h>
#include <ldfcn.h>


#include "icode.h"
#include "globals.h"
#include "protos.h"

void
read_hdrs(const char *objfile)
{
  FILE *Fobj; /* file descriptor for object file */
  struct filehdr Fhdr;

  int i, magic;
  ulong Gp_value;
  ulong Data_size;
  ulong Pdata_size, Pdata_start;
  ulong Rdata_size, Rdata_start;
  ulong Rconst_size, Rconst_start;
  ulong Bss_size, Bss_start;
  ulong Text_size, Text_start, Text_end;
  struct aouthdr ahdr;
  struct scnhdr shdr;

  Fobj = fopen(objfile, "r");
  if (Fobj == NULL) {
    perror(objfile);
    exit(1);
  }

  fseek(Fobj, 0, SEEK_SET);
  if (fread(&Fhdr, sizeof(struct filehdr), 1, Fobj) < 1)
    fatal("read_hdrs: could not read file header\n");

  fprintf(Aint_output, "\n");
  fprintf(Aint_output, "F_Magic(%s) = %x\n", objfile, Fhdr.f_magic);
  fprintf(Aint_output, "F_Flags(%s) = %x\n", objfile, Fhdr.f_flags);

  fseek(Fobj, sizeof(struct filehdr), SEEK_SET);
  if (fread(&ahdr, sizeof(struct aouthdr), 1, Fobj) < 1)
    fatal("read_hdrs: could not read optional header\n");

  magic = ahdr.magic;
  if (magic != OMAGIC && magic != NMAGIC && magic != ZMAGIC) {
    error("\n\"%s\" is not an alpha COFF executable file.\n",
	  objfile);
    fatal("read_hdrs: bad magic number (0%o)\n", ahdr.magic);
  }
  fprintf(Aint_output, "Magic(%s) = %x\n", objfile, magic);

  Data_start = ahdr.data_start;
  Data_size = ahdr.dsize;

#ifdef SYMTAB_VERBOSE
  printf("Ahdr: data start 0x%lx size 0x%lx\n", Data_start, Data_size);
#endif

  Bss_size = ahdr.bsize;
  Bss_start = ahdr.bss_start;

#ifdef SYMTAB_VERBOSE
  printf("Ahdr: bss start 0x%lx size 0x%lx\n", Bss_start, Bss_size);
#endif

  Gp_value = ahdr.gp_value;

  /* Seek to the beginning of the first section header.
   * The file header comes first, followed by the optional header
   * (This is the aouthdr). The size of the aouthdr is giben in
   * Fhdr.f_opthdr.
   */

  fseek(Fobj, sizeof(struct filehdr) +Fhdr.f_opthdr, SEEK_SET);

  /* loop through the section headers */
  for (i = 0;  i < Fhdr.f_nscns; i++) {
#ifdef SYMTAB_VERBOSE
    printf("loop iter %d / %d\n", i, Fhdr.f_nscns);
#endif
    if (fread(&shdr, sizeof(struct scnhdr), 1, Fobj) < 1)
      fatal("read_hdrs: could not read section header %d\n", i);

    switch (shdr.s_flags) {
    case STYP_TEXT:
      Text_seek = shdr.s_scnptr;
      Text_start = shdr.s_vaddr;
      Text_size = shdr.s_size / 4;
      /* there is a null routine after the supposed end of text */
      Text_size += 10;
      Text_end = Text_start + Text_size*4;
      break;

    case STYP_PDATA:
      Pdata_start = shdr.s_vaddr;
      Pdata_size  = shdr.s_size;
      Pdata_seek = shdr.s_scnptr;
      break;

    case STYP_RDATA:
      /* the .rdata section is sometimes placed before the text
       * section instead of being contiguous with the .data section
       */
      Rdata_start = shdr.s_vaddr;
      Rdata_size = shdr.s_size;
      Rdata_seek = shdr.s_scnptr;
      break;

    case STYP_RCONST:
      Rconst_start = shdr.s_vaddr;
      Rconst_size = shdr.s_size;
      Rconst_seek = shdr.s_scnptr;
      break;

    case STYP_DATA:
      Data_size = shdr.s_size;
#ifdef SYMTAB_VERBOSE
      printf("Data scnhdr: got size 0x%lx\n", Data_size);
#endif
      Data_seek = shdr.s_scnptr;
      break;

    case STYP_SDATA:
      Sdata_seek = shdr.s_scnptr;
      Data_size += shdr.s_size;
      break;

    case STYP_LITA:
    case STYP_LIT8:
    case STYP_LIT4:
    case STYP_XDATA:
      Data_size += shdr.s_size;
      break;

    case STYP_BSS:
      Bss_start = shdr.s_vaddr;
      Bss_size = shdr.s_size;
#ifdef SYMTAB_VERBOSE
      printf("secn hdr: bss start 0x%lx size 0x%lx\n", Bss_start, Bss_size);
#endif

      break;

    case STYP_SBSS:
      break;
    }
  }
  fclose(Fobj);
}


/*
 * Look up the load-time address of symbol_name in objfile
 */
void
find_proc_addrs(char *objfile, const char **symbol_names, unsigned long *symbol_addrs)
{
  static LDFILE *ldptr = NULL;
  int symindex;
  
  /* initialize the address vector */
  const char **sns = symbol_names;
  unsigned long *sas = symbol_addrs;
  while (*sns != NULL) {
    *sas = -1;
    sns++;
    sas++;
  }

  if ( ldptr == NULL ) {
    ldptr = ldopen(objfile, NULL);
    if ( ldptr == NULL ) {
      perror("ldopen");
      return;
    }
  }

  if ( ldtbseek(ldptr) != SUCCESS ) {
    return;
  }
  
  for (symindex = 0; ; symindex++ ) {
    int status;
    SYMR duh_symbol;
    pSYMR symbol = &duh_symbol;
    status = ldtbread(ldptr, symindex, symbol);

    if ( status != SUCCESS ) {
      return;
    } else {
      char *name = ldgetname(ldptr, symbol); 
      sns = symbol_names;
      sas = symbol_addrs;
      while (*sns != NULL) {
	if ( strcmp(name, *sns) == 0 ) {
	  if (0) fprintf(Aint_output, "find_symbol_name: sns=%s name=%s sc=%d st=%d pc=%lx\n",
			 *sns, name, symbol->sc, symbol->st, symbol->value);
	  if ( symbol -> sc == scText && symbol -> st == stProc ) {
	    *sas =  symbol->value;
	  }
	}
	sns++;
	sas++;
      }
    }
  }
}




/* 
 * Find jsr targets the hard way (using atom)
 */

typedef struct {
  ulong pc;
  ulong targetpc;
  char *name;
} JSRTargetInfo;

JSRTargetInfo *jsrTargetInfo = NULL;
#define INITIAL_N_TARGETS 1024
int nTargets = 0;

void initJSRTargets(char *objfile)
{
  char *aintdir = NULL;
  /* (r2r): we don't use this functionality of AINT and it generates
   * an error message when environment variable AINTDIR is set
   * (because sometimes you might also be running another version
   * of AINT that does care).
   * -- disabled --
   */
  return;
 /*******/
  if ((aintdir = getenv("AINTDIR")) != NULL) {
    char cmd[1024];
    FILE *in;
    sprintf(cmd, "atom %s %s/jsrtarget.inst.c %s/jsrtarget.anal.c\n",
	    objfile, aintdir, aintdir);
    fprintf(Aint_output, "Using atom to find JSR targets of %s\n", objfile);
    in = popen(cmd, "r");
    if (in != NULL) {
      int i = 0;
      int nconverted;
      ulong pc, targetpc;
      char line[1024];
      char name[1024];

      nTargets = INITIAL_N_TARGETS;
      jsrTargetInfo = realloc(jsrTargetInfo, nTargets*sizeof(JSRTargetInfo));
      if (jsrTargetInfo == NULL)
         fatal("failed to realloc jsrTargetInfo to size %ld\n",
               nTargets*sizeof(JSRTargetInfo));
      while(fgets(line, sizeof(line), in) != NULL) {
	if (sscanf(line, "jsr: pc=%lx targetpc=%lx tname=%1024s\n",
		   &pc, &targetpc, name) == 3) {
	  if (i >= nTargets) {
	    nTargets *= 2;
	    jsrTargetInfo = realloc(jsrTargetInfo, nTargets*sizeof(JSRTargetInfo));
            if (jsrTargetInfo == NULL)
               fatal("failed to realloc jsrTargetInfo to size %ld\n",
                     nTargets*sizeof(JSRTargetInfo));
	  }
	  if (0) fprintf(Aint_output, "jsr: pc=%lx targetpc=%lx tname=%s\n", pc, targetpc, name);
	  jsrTargetInfo[i].pc = pc;
	  jsrTargetInfo[i].targetpc = targetpc;
	  i++;
	} else {
	  fprintf(Aint_output, "erroneous line: %s\n", line);
	}
      }
      fprintf(Aint_output, "read %d lines\n", i);
      pclose(in);
      nTargets = i;
    } else {
      warning("failed to open pipe: %s", sys_errlist[errno]);
    }
  }
}

ulong findJSRTargetPC(ulong pc)
{
  static lastFindIndex = 0;
  int i;
retry:
  for (i = lastFindIndex; i < nTargets; i++) {
    if (jsrTargetInfo[i].pc == pc) {
      lastFindIndex = i;
      return jsrTargetInfo[i].targetpc;
    }
  }
  if (lastFindIndex != 0) {
    /* try again */
    lastFindIndex = 0;
    goto retry;
  }
  return 0;
}
