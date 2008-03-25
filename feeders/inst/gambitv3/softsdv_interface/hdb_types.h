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


#ifndef _HDB_TYPES_H
#define _HDB_TYPES_H

extern "C" {
#define HDB_ADDR_t  U64
/* To avoid redefinition of EX86_vaddr_t use the CPP macro EX86_VADDR_T */
#ifndef EX86_VADDR_T
#define EX86_VADDR_T
typedef U64 EX86_vaddr_t;    /* type of virtual address */
#endif
typedef U64             EX86_paddr_t;    /* type of physical address */

typedef U32 x86_vaddr_t;

typedef U32 x86_paddr_t;

typedef U8  *hptr_t;

#define MAX_S8  ((S8)0x7f)
#define MAX_S16 ((S16)0x7fff)
#define MAX_S32 ((S32)0x7fffffff)

#define MAX_U8  ((U8)0xff)
#define MAX_U16 ((U16)0xffff)
#define MAX_U32 ((U32)0xffffffff)
#define U64_ZERO_CONST    {0x0L , 0x0L}
const U64 hdb_u64_zero = {U64_ZERO_CONST};
#define U64_ZERO hdb_u64_zero

typedef U32             EX86instr; /* simulate CPU instr - 32 bit wide */

typedef void           *HPTR_t;    /* host pointer */

typedef char *general_pointer;

typedef struct
{
  general_pointer prev;
  general_pointer next;
} dbllink_t;

typedef void (*Function_t)();
typedef struct{
        U64  val;
        U128  fp_val;
        char *name;
        int  size;
        int  base;
        int  mode;
        int  curp;
        U64  params[10];
        Function_t print_func;
        Function_t set_func;
} parse_type;
typedef enum
{
    GE_BRKPT_FETCH = 1,
    GE_BRKPT_READ_DATA,
    GE_BRKPT_WRITE_DATA,
    GE_DEBUG_STEP,
    CPU_DEBUG_FS_STEP_INSTRUCTION = GE_DEBUG_STEP,
    GE_DEBUG_SELECT_CPU,
    CPU_DEBUG_FS_SELECT_CPU = GE_DEBUG_SELECT_CPU,
    CPU_DEBUG_LAST_FS_BREAKPOINT_TYPE = GE_DEBUG_SELECT_CPU,
    CPU_DEBUG_FIRST_PS_BREAKPOINT_TYPE,
    CPU_DEBUG_PS_FETCH = CPU_DEBUG_FIRST_PS_BREAKPOINT_TYPE,
    CPU_DEBUG_PS_READ_ACCESS,
    CPU_DEBUG_PS_WRITE_ACCESS,
    CPU_DEBUG_PS_STEP_CYCLE,
    CPU_DEBUG_PS_STEP_INSTRUCTION,
    CPU_DEBUG_PS_STEP_FS_INSTRUCTION,
    CPU_DEBUG_US_SEGMENT_SIZE,
    CPU_DEBUG_US_SEGMENT_RATIO,
    CPU_DEBUG_PS_SELECT_CPU,
    CPU_DEBUG_LAST_PS_BREAKPOINT_TYPE = CPU_DEBUG_PS_SELECT_CPU,
    CPU_DEBUG_FIRST_IO_PORT_BRKPT,
    CPU_DEBUG_IO_PORT_BRKPT_BEFORE_READ = CPU_DEBUG_FIRST_IO_PORT_BRKPT,
    CPU_DEBUG_IO_PORT_BRKPT_BEFORE_READ_ALL,
    CPU_DEBUG_IO_PORT_BRKPT_AFTER_READ,
    CPU_DEBUG_IO_PORT_BRKPT_AFTER_READ_ALL,
    CPU_DEBUG_IO_PORT_BRKPT_BEFORE_WRITE,
    CPU_DEBUG_IO_PORT_BRKPT_BEFORE_WRITE_ALL,
    CPU_DEBUG_IO_PORT_BRKPT_AFTER_WRITE,
    CPU_DEBUG_IO_PORT_BRKPT_AFTER_WRITE_ALL,
    CPU_DEBUG_LAST_IO_PORT_BRKPT = CPU_DEBUG_IO_PORT_BRKPT_AFTER_WRITE_ALL
} GE_brkpt_type_t;
typedef unsigned OML_handle_t;

#define GE_ERR_DISPLACEMENT 1000
typedef enum GE_stat_t_e
{
    GE_OK,     /* must be zero */
    GE_STAT_FIRST =         2000,
    GE_ERR_NULL_POINTER,
    GE_ERR_BUFF_TOO_SHORT,  /*@*/
    GE_ERR_INVALID_CONFIG,
    GE_ERR_INVALID_USAGE,
    GE_ERR_MISSING_FILE,
    GE_ERR_INTERNAL,
    GE_NEXT_IS_PARAM,
    GE_THREAD_TERMINATION,

    GE_ERR_LIB_LOAD_FAILED,
    GE_ERR_FUNC_LOAD_FAILED,
    GE_NOT_COMMON_FLAG,


    /* Debug Assist error codes */
    GE_DEBUG_ERR_MALLOC,
    GE_DEBUG_ERR_FLUSH_FAILED,
    GE_DEBUG_ERR_INV_TOOL_LIST,
    GE_DEBUG_ERR_OUT_OF_RANGE,
    GE_DEBUG_ERR_INVALID_TOOL,


    /* GE Malloc return value */
    GE_MALLOC_FAILURE,

    GE_ERR_CANT_BE_CALLED_NOW,
    GE_ERR_ACTION_FAILED,
    /*  all OML internal error code. The order is important since the code is an index
    to the error messages array err_code_2_str. New codes must be added at the same
    entry of the message in the err_code_2_str array.
    new error codes must be added LAST   */

    GE_OML_ERR_FIRST = GE_STAT_FIRST+GE_ERR_DISPLACEMENT,
    OML_ERR_CLIENT_NOT_REGISTER,
    OML_ERR_CLIENT_RE_REGISTER,
    OML_ERR_CLIENT_NOT_ACTIVE,
    OML_ERR_HANDLING_PROBLEM,
    OML_ERR_INVALID_HNDL,       /*@*/
    OML_ERR_INVALID_TOOL,       /*@*/
    OML_ERR_INVALID_GE_STAGE,   /*@*/
    OML_ERR_INVALID_ORDER_ID,
    OML_ERR_INVALID_TOKEN_ID,
    OML_ERR_INVALID_SEQUENCE,
    OML_ERR_INVALID_MODE,
    OML_ERR_INVALID_FILTER,
    OML_ERR_INVALID_PROPERTY,
    OML_ERR_INVALID_NAME_ORDER,
    OML_ERR_INTERNAL,
    OML_ERR_NULL_FUNC,
    OML_ERR_NULL_POINTER,
    OML_ERR_NO_INIT,            /*@*/
    OML_ERR_NOT_LOCAL,
    OML_ERR_NO_MORE_USER_OBJECTS,
    OML_ERR_NO_NEXT,
    OML_ERR_NOT_OWNER,          /*@*/
    OML_ERR_NOT_ITEM,           /*@*/
    OML_ERR_NOT_IN_FAMILY,
    OML_ERR_OBJ_CLOSED,         /*@*/
    OML_ERR_OBJ_NOT_OPENED,     /*@*/
    OML_ERR_OBJ_NOT_AVAILABLE,  /*@*/
    OML_ERR_OBJ_NOT_ACTIVE,     /*@*/
    OML_ERR_OBJ_NOT_READY,
    OML_ERR_OBJ_RE_OPEN,
    OML_ERR_OBJ_UNDER_TREATMENT,/*@*/
    OML_ERR_OWNER_NOT_READY,
    OML_ERR_OUT_OF_MEMORY,
    OML_ERR_OUT_OF_RANGE,
    OML_ERR_STRING_ADDR,
    OML_ERR_SECOND_INIT_CALL,
    OML_ERR_TOO_MANY_TOKENS,
    OML_ERR_WRONG_CLASS,        /*@*/
    OML_ERR_WRONG_SIZE,         /*@*/
    OML_ERR_NO_OWNER,           /*@*/
    OML_ERR_OWNER_FAILURE,      /*@*/
    OML_NOTE_SAME_VALUE,
    OML_ERR_WRONG_TYPE,
    OML_ERR_ODB_LOAD_FAILURE,
    OML_ERR_ODB_NAME_COLLISION,
    OML_ERR_INVALID_NAME,
    GE_OML_ERR_LAST,

    /* trace generator errors */
    TRCGEN_ERR_FIRST = GE_OML_ERR_FIRST+GE_ERR_DISPLACEMENT,
    TRCGEN_ERR_NO_TDD,
    TRCGEN_ERR_NO_MEMORY,
    TRCGEN_ERR_OPEN_ERROR,
    TRCGEN_ERR_NOT_FOUND,
    TRCGEN_ERR_REQ_FAILED,
    TRCGEN_ERR_REG_FAILED,
    TRCGEN_ERR_BAD_FORM,
    TRCGEN_ERR_BAD_EVENT,
    TRCGEN_ERR_BAD_ITEM,
    TRCGEN_ERR_TOO_MANY_STREAMS,
    TRCGEN_ERR_OBJ_NOT_AVAILABLE,
    TRCGEN_ERR_WRITE_ERROR,
    TRCGEN_ERR_TOO_MANY_TIMERS,
    TRCGEN_ERR_LAST,
    TRCGEN_ERR_SHARED_MEMORY,
    TRCGEN_ERR_NO_EVENT,
    TRCGEN_ERR_NO_MUTEX,
    TRCGEN_ERR_BUFFERS,

    /* Gambit Driver errors */
    GB_DRIVER_ERR_FIRST = TRCGEN_ERR_FIRST+GE_ERR_DISPLACEMENT,
    GB_DRIVER_ERR_LOAD_FAILED,
    GB_DRIVER_ERR_IP_SETTING_FAILED,
    GB_DRIVER_ERR_CONTEXT_FAILED,
    GB_DRIVER_ERR_LAST,

    GE_ERR_STAT_LAST

} GE_stat_t;

typedef enum
{
        HDB_MEMORY_ACCESS_READ = 1,
        HDB_MEMORY_ACCESS_WRITE,
        HDB_MEMORY_ACCESS_EXECUTE,
}HDB_memory_access_t;


typedef struct HDB_reg_info_s
{
    char name[20];
        char oml_name[50];
        unsigned int size;
    unsigned int offset;
}HDB_reg_info_t;

/*********** MACROs ************************************************/

/* returns x aligned to sz (sz must be power of 2) */
#define ALIGN(x, sz)  ((x) & ~(sz)+1)

/* returns TRUE iff x is aligend to sz (sz must be power of 2) */
#define IS_ALIGNED_BY_SZ(x, sz)  (!((x) & ((sz)-1)))

/* returns the absolute value */
//#define ABS(x) (abs(x))

/* return the max/min value */
//#define MAX(a,b) ((a)<(b) ? (b) : (a))
//#define MIN(a,b) ((a)>(b) ? (b) : (a))

/* returns the i'th byte of 32 bits operand */
#define BX(U32, i, j) (((U32>>i*8) & 0xff) << j*8)

/* returns the mask of the specified bit */
#ifndef BIT
#define BIT(nn) ((unsigned)1<<(nn))
#endif

/* the following macro return TRUE  iff  theirs 2 64 bit arguments are equal */
#define EQ_64(l1, h1, l2, h2)  ((((l1) ^ (l2))  |  ((h1) ^ (h2))) == 0)
#define EQU64(arg1,arg2) EQ_64((arg1).low_w, (arg1).high_w, (arg2).low_w, (arg2).high_w)

/* the following macro retruns TRUE  iff  (s1) <= (s2) */
#define LEU64(s1, s2) \
    (((s1).high_w < (s2).high_w) || (((s1).high_w == (s2).high_w) && ((s1).low_w <= (s2).low_w)))

/* the following macro retruns TRUE  iff  (s1) < (s2) */
#define LU64(s1, s2) \
        (((s1).high_w < (s2).high_w) || \
         (((s1).high_w == (s2).high_w) && ((s1).low_w < (s2).low_w)))

/* the following macro retruns TRUE  iff  (s1) > (s2), s2 is 32 bit operand */
#define GU64_32(arg1, arg2)  CMPGU_64((arg1).low_w, (arg1).high_w, (arg2), 0)

/* the following macro increments the given 64 bit argument */
#define INCU64(arg) ((arg).low_w += 1, (arg).high_w += (arg).low_w == 0, (arg))

/* the following macro adds y to x, using a temporary U32 t */
#define ADDU64(x, y, t) ((t) = (x).low_w, (x).low_w += (y).low_w, (x).high_w += (y).high_w + ((x).low_w < (t)), (x))

/* the following macro adds y to x, where y=U64 and x=U32 */
#define ADD2U64(x, y) ((x).low_w+=(y), (x).high_w += ((x).low_w < (y)), (x))

/* the following macro subs y from x, where x=U64 and y=U32 */
#define SUB4U64(x, y) ( (x).high_w -= ((x).low_w < (y)),(x).low_w-=(y), (x))

/* the following macro subs y from x, using a temporary U32 t */
#define SUBU64(x,y,t) \
 ((t)=(x).low_w,(x).low_w-=(y).low_w, (x).high_w -= ((y).high_w+((x).low_w > (t))), (x))

/* the following macro shifts the U64 number x, y bits to the right */
#define SHRU64(x, y) \
        (x.low_w = ((y) < 32) ? ((x.low_w >> (y)) | (x.high_w << (32-(y)))) : \
                   (x.high_w >> ((y)-32)), \
         x.high_w = ((y) < 32) ? (x.high_w >> (y)) : 0, x)

/* the following macro shifts the U64 number x, y bits to the left */
#define SHLU64(x, y) \
        (x.high_w = ((y) < 32) ? ((x.high_w << (y)) | (x.low_w >> (32-(y)))) : \
                    (x.low_w << ((y)-32)), \
         x.low_w = ((y) < 32) ? (x.low_w << (y)) : 0, x)

/* assigns 0 into the given U64 arg */
#define ZU64(arg) ((arg).low_w = (arg).high_w = 0)

/* The following macro return 1 if x > y */
#define BIGGERU64(x,y) (((x).high_w > (y).high_w) || (((x).high_w == (y).high_w) && ((x).low_w > (y).low_w)))

/* x >> y */
#define SHR64(x,y) ((((y) > 0) ? \
                    (((y) < 32) ? \
                     ((x).low_w = ((x).low_w >> (y)) | ((x).high_w << (32-(y))),\
                      (x).high_w >>= (y)) : \
                     ((x).low_w = (x).high_w << ((y)-32), (x).high_w = 0)) : \
                     (((y) == 0) ? (1) : \
                      ((-(y) < 32) ? \
                       ((x).high_w = ((x).high_w << -(y)) | ((x).low_w >> (32+(y))),\
                        (x).low_w <<= -(y)) : \
                       ((x).high_w = (x).low_w >> (-(y)-32), (x).low_w = 0)))), \
                    (x))

/* x << y */
#define SHL64(x,y) SHR64((x),-(y))

#define SEXT_S32_TO_S64(s64, s32) \
             (s64.low_w = s32, \
               s64.high_w = ((s32 & 0x80000000) ? -1 : 0))

#define CMPE_64(s1l, s1h, s2l, s2h) (((s1l) == (s2l)) && ((s1h) == (s2h)))
#define CMPNE_64(s1l, s1h, s2l, s2h) (!CMPE_64(s1l, s1h, s2l, s2h))
#define CMPGU_64(s1l, s1h, s2l, s2h) (((s1h) > (s2h)) || (((s1h) == (s2h)) && ((s1l) > (s2l))))
#define CMPLU_64(s1l, s1h, s2l, s2h) (((s1h) < (s2h)) || (((s1h) == (s2h)) && ((s1l) < (s2l))))
#define CMPGEU_64(s1l, s1h, s2l, s2h) (((s1h) > (s2h)) || (((s1h) == (s2h)) && ((s1l) >= (s2l))))
#define CMPGE_64(s1l, s1h, s2l, s2h) (((long)(s1h) > (long)(s2h)) || (((s1h) == (s2h)) && ((s1l) >= (s2l))))
#define CMPLEU_64(s1l, s1h, s2l, s2h) (((s1h) < (s2h)) || (((s1h) == (s2h)) && ((s1l) <= (s2l))))
#define CMPG_64(s1l, s1h, s2l, s2h) (((long)(s1h) > (long)(s2h)) || (((s1h) == (s2h)) && ((s1l) > (s2l))))
#define CMPL_64(s1l, s1h, s2l, s2h) (((long)(s1h) < (long)(s2h)) || (((s1h) == (s2h)) && ((s1l) < (s2l))))
#define CMPGE_64(s1l, s1h, s2l, s2h) (((long)(s1h) > (long)(s2h)) || (((s1h) == (s2h)) && ((s1l) >= (s2l))))
#define CMPLE_64(s1l, s1h, s2l, s2h) (((long)(s1h) < (long)(s2h)) || (((s1h) == (s2h)) && ((s1l) <= (s2l))))

extern void reverse_buf(char *buf, int n);
#define SWAP(var) reverse_buf((char *)&(var), sizeof(var))

#ifdef _BIG__ENDIAN
#define SIGN_BIT_SET(f) ((f).low_w & 0x80000000)
#define HostToNetworkShort(x) (x)
#define HostToNetworkLong(x) (x)
#define REVERSE(var)    reverse_buf((char *)&(var), sizeof(var))
#define REVERSE_BUF(buf,n) {\
                int i1;\
                for(i1=0;i1<n;i1++)\
                    reverse_buf((char*)&(buf[i1*4]),4);\
               }
#define CHANGE_U64_WORDS(v) {U32 __tmp32;__tmp32=v.low_w; v.low_w=v.high_w;v.high_w=__tmp32;}
#define LOW_WORD   1
#define HIGH_WORD  0
#else
#define SIGN_BIT_SET(f) ((f).w[2] & 0x8000)

#define HostToNetworkShort(x)   LE_HtoN_short((unsigned short)x)
#define HostToNetworkLong(x)    LE_HtoN_long((unsigned long)x)
#define REVERSE(var)
#define REVERSE_BUF(buf,n)
#define CHANGE_U64_WORDS(v)
#define LOW_WORD   0
#define HIGH_WORD  1
#endif

#if defined(_BIG__ENDIAN) || defined(HPUX)

#define LOW_WORD   1
#define HIGH_WORD  0

#else /* LITTLE ENDIAN */

#define LOW_WORD   0
#define HIGH_WORD  1

#endif

#define high_w      dw[HIGH_WORD]
#define low_w       dw[LOW_WORD]

#define U64_ZERO_CONST    {0x0L , 0x0L}

#ifndef HPUX
/* this is a little endian definition!!! */
#define NATVAL_CONST {(U32)0x0, (U32)0x00000000, (U32)0x7ffe, (U32)0x0}
#define EXACT_FP_NATVAL_CONST {(U32)0x0, (U32)0x00000000, (U32)0x1fffe}

#define EAS23_NATVAL_CONST {(U32)0x0, (U32)0x20000000, (U32)0x7fff, (U32)0x0}
#define EAS23_EXACT_FP_NATVAL_CONST {(U32)0x0, (U32)0x20000000, (U32)0x1ffff}
#endif /* HPUX */


#define HDB_NOT_INIT 0
#define HDB_INIT     1
#define HDB_DONE     5

#define HDB_MODE_PHYSICAL   0x0001
#define HDB_MODE_CODE       0x0002
#define HDB_MODE_SYMBOL     0x0004
#define HDB_MODE_VIRTUAL    0x0008
#define HDB_MODE_TLB_ONLY   0x0010
#define HDB_MODE_SEG_AND_OFF 0x0020
#define HDB_MODE_SEG_AND_REG 0x0040
#define FUNC
#define HDB_ADDR_t  U64
#define CHECK_FOR_ERROR if(hdb_error) HDB_RESUME(9);
#define MAX_NAME_LENGTH   128
#define HDB_MAX_BRKS      1000
#define HDB_VIEW_CPU      32


}
#endif
