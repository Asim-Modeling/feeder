/*
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
 */


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _VPC_TYPES_H
#define _VPC_TYPES_H

#include "iel.h"

#if defined ( WIN32 ) && !defined ( WINNT )
#define WINNT
#endif


#ifdef WINNT

#ifndef CDECL
#define CDECL __cdecl
#endif

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

/*
the following defintion is done to avoid NT include file
that needs almost all WINDOWS.H and cause collisions
*/


#define TRY __try
#define IMMEDIATE_EXIT(exit_code)  _exit(exit_code);
#define UNDERSCORE 1
#define TYPE_OBJECT 1

/* OS & Gambit memory overhead */
#define OS_GAMBIT_MEM_OVERHEAD 30*1024*1024
#define DIR_SEPARATOR_C           '\\'
#define DLL_SUFFIX                ".dll"

typedef unsigned __int64 I64;
typedef unsigned __int64 Ulong64;
typedef __int64 Slong64;
#define U64CONST(x) (x##Ui64)
#define S64CONST(x) (x##i64)


#else //LINUX


/* OS & Gambit memory overhead */
#define OS_GAMBIT_MEM_OVERHEAD 20*1024*1024
#define dllimport 0
#define dllexport 0
#define __declspec(dllexport_import)
#define CDECL
#define TRY
#define EXCEPT
#define UNDERSCORE 0
#define TYPE_OBJECT 0
#define IMMEDIATE_EXIT(exit_code)
#define DIR_SEPARATOR_C                '/'
#define DLL_SUFFIX                ".so"

//typedef long long __int64;
#define __int64 long long
typedef unsigned long long I64;

//?????
typedef unsigned long long Ulong64;
typedef long long Slong64;
#define U64CONST(x) (x##ULL)
#define S64CONST(x) (x##LL)

#endif

#ifdef _ULONGLONG_
	typedef ULONGLONG I64;
#endif /* of _ULONGLONG_ */

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)



/*DeviceIoControl variables */
typedef enum {
FSCTL_LOCK_VOLUME_t,
IOCTL_DISK_GET_DRIVE_GEOMETRY_t,
IOCTL_CDROM_GET_DRIVE_GEOMETRY_t,
IOCTL_VPCPCI_PUNMAP_t,
NON_t
} DevIoContrl_t;


typedef void (* call_back_f)(char *);

/* this enum is used for the GE stage definition. Will be held in
   the ge_stage notifier. The order of the enum is according to
   the flow in GE run time. */
typedef enum GE_stage_e
{
    GE_STAGE_TOOL_INIT,    /* each GE component initialize function is called */
    GE_STAGE_USER_SETUP_1, /* read input and close objects */
    GE_STAGE_USER_SETUP_2, /* require objects */
    GE_STAGE_USER_SETUP_3, /* internal notifiers setting */
    GE_STAGE_TOOL_CONFIG,  /* each GE component configures itself according to user setup */
    GE_STAGE_SIMUL_IDLE,   /* the user controls the simulation through SimDB */
    GE_STAGE_SIMUL_RUN,    /* EM code is simulated */
    GE_STAGE_TERMINATE,     /* Each GE component termination function is called */
	GE_STAGE_SAVE_RESTORE  /* SoftSDV is under a save/restore operation */
} GE_stage_t;

    /*!!!!! codes with @ are used by ssc api functions thus must not be changed
            without coordination !!!!*/
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

/* this enum is used to recognise the value type of any data object in GE */
typedef enum GE_value_type_t_e
{
    GE_TYPE_FIRST,
    GE_TYPE_U8,         /* 1 byte */
    GE_TYPE_U16,        /* 2 bytes */
    GE_TYPE_U32,        /* 4 bytes */
    GE_TYPE_U64,        /* 16 bytes */
    GE_TYPE_U128,       /* 32 bytes */
    GE_TYPE_S32,        /* 4 bytes integer */
    GE_TYPE_FLOAT,      /* 32 bytes */
    GE_TYPE_STRING,     /* string, maximum length limitation*/
    GE_TYPE_BYTE_ARR,   /* byte array with static size */
    GE_TYPE_LAST
} GE_value_type_t;


/* Tools ID definition */
/* the tools are enumarated according to the send algorithm order */

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 
   PLEASE DON'T ADD NEW ITEMS INTO "GE_tool_id_e" ENUM (IT REQUIRES 
   CHANGE IN THE KERNEL).
   PLEASE USE "oml_get_new_tool_id" API TO GET ID FOR 
   UNLISTED MODULES
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!*/

typedef enum GE_tool_id_e
{
    GE_NULL_TOOL_ID = 0,    /* Dummy first tool id */
    GE_GAMBIT_ID,           /* Gambit */
    GE_OS_ID,               /* Simulated OS */
    GE_SIMUL_APP_ID,        /* Simulated application */
    GE_EMERALD_ID,          /* Emerald */
    GE_SBM_ID,              /* SBM     */
    GE_DRIVER_ID,           /* Gambit Environment driver */
    GE_UTIL_ID,             /* Gambit Environment utility library */
    GE_OML_ID,              /* Object Management Library */
    GE_FP82_ID,             /* FP82 library */
    GE_SPHINX_ID,           /* Sphinx simulator */
    GE_OSIRIS_ID,           /* Osiris */

    GE_VPC_PLAT_ID,         /* General Platform */
    GE_VPC_KERNEL_ID,       /* Kernel & utilities */
    GE_VPC_BIOS_ID,
    GE_VPC_CMOS_ID,
    GE_VPC_COM_ID,
    GE_VPC_RAM_ID,
    GE_VPC_HDISK_ID,
    GE_VPC_IDE_ID,
    GE_VPC_KEYBOARD_ID,
    GE_VPC_MOUSE_ID,
    GE_VPC_PIC_ID,
    GE_VPC_PIT_ID,
    GE_VPC_RTC_ID,
    GE_VPC_SERIAL_ID,
    GE_VPC_VGA_ID,
    GE_VPC_GFX_ID,
    GE_VPC_CPU_ID,
    GE_VPC_ROM_ID,
    GE_VPC_IOAPIC_ID,
    GE_VPC_IOSAPIC_ID,
    GE_VPC_PCIproxy_ID,
    GE_VPC_HOSTIO_ID,
    GE_VPC_PCITEST_ID,
    GE_VPC_MCH_ID,
    GE_VPC_RDRAM_ID,
    GE_VPC_MDA_ID,
    GE_MTH1_ID,
    GE_MODEL_SPECIFIC_ID,
    GE_SRMGR_ID,
	GE_VPC_SCSI_ID,

    GE_VPC_DEV1_ID,
    GE_VPC_DEV2_ID,
    GE_VPC_DEV3_ID,
    GE_VPC_DEV4_ID,
    GE_VPC_DEV5_ID,
    GE_VPC_DEV6_ID,
    GE_VPC_DEV7_ID,
    GE_VPC_DEV8_ID,
    GE_VPC_DEV9_ID,
    GE_VPC_DEV10_ID,
    GE_ACPI_ID,
	/* Note: Trace generator should be the LAST one !!! */
    GE_TRCGEN_ID,
    GE_LAST_ID              /* dummy last tool id */
} GE_tool_id_t;

#define IS_VALID_TOOL_ID(tid)  (((tid) > GE_NULL_TOOL_ID) && ((tid) < GE_LAST_ID))




/* global types definitions */
#ifndef TRUE
  #define TRUE 1
  #define FALSE 0
#endif /* TRUE */

/* AM: !!! */
/* Support "GE" types (that are important to the driver also) for a while... */
/*
typedef vpc_mssg_t GE_mssg_t;
typedef vpc_mssg_reply_t GE_mssg_reply_t;
typedef vpc_exit_code_t GE_exit_code_t;
typedef vpc_driver_type_t GE_driver_type_t;
*/

typedef enum GE_mssg_e
{
    GE_MSSG_TYPE_ABORTRETRYIGNORE,/*The message contains three options: Abort, Retry, and Ignore.*/
    GE_MSSG_TYPE_INFORMATION,     /*An information message.The message has one option: OK.*/
    GE_MSSG_TYPE_ERROR,           /*An error message.The message has one option:OK*/
    GE_MSSG_TYPE_WARNING,         /*A warning message.The message has one option:OK*/
    GE_MSSG_TYPE_OKCANCEL,        /*The message contains two options: OK and Cancel*/
    GE_MSSG_TYPE_RETRYCANCEL,     /*The message contains two options: Retry and Cancel*/
    GE_MSSG_TYPE_YESNO,           /*The message contains two options: Yes and No.*/
    GE_MSSG_TYPE_YESNOCANCEL,     /*The message contains three options: Yes, No, and Cancel.*/
    GE_MSSG_TYPE_LAST
} GE_mssg_t;


/* GE_mssg_reply_t - used by the user message function to return the user input. */
typedef enum GE_mssg_reply_e
{
    GE_MSSG_REPLY_ABORT,        /*Abort option was selected.*/
    GE_MSSG_REPLY_CANCEL,   /*Cancel option was selected. */
    GE_MSSG_REPLY_IGNORE,   /*Ignore option was selected. */
    GE_MSSG_REPLY_NO,       /*No option was selected. */
    GE_MSSG_REPLY_OK,       /*OK option was selected. */
    GE_MSSG_REPLY_RETRY,        /*Retry option was selected. */
    GE_MSSG_REPLY_YES,      /*Yes option was selected. */
    GE_MSSG_REPLY_LAST
} GE_mssg_reply_t;


/* GE_exit_code_t -This enum lists all the exit codes returned by the main function */
typedef enum GE_exit_code_e
{
    GE_EXIT_NORMAL,         /*Normal program termination*/
    GE_EXIT_USR_INT,        /* User has interupted the program execution*/
    GE_EXIT_ERROR,          /*Program exit due to error */
    GE_EXIT_SIMULATION_END, /*We have reached the simulated program end */
    GE_EXIT_LAST
} GE_exit_code_t;


/* The driver_type is given to ge by the executable */
typedef enum {
    GE_TYPE_NONE,
    GE_IA_64,
    GE_IA_32,
    GE_LAST_DRIVER_TYPE
} GE_driver_type_t;








/* The GE_target_processor_t is representing the processor options for the
   target_processor notifier */
typedef enum {
    GE_PROCESSOR_EM,
    GE_PROCESSOR_ITANIUM,
    GE_PROCESSOR_MCKINLEY,
    GE_PROCESSOR_POST_MCKINLEY,
    GE_PROCESSOR_MONTECITO_IA32,
    GE_PROCESSOR_MONTECITO,
    GE_PROCESSOR_TANGLEWOOD,
    GE_PROCESSOR_P5,
    GE_PROCESSOR_P6,
    GE_PROCESSOR_P5MM,
    GE_PROCESSOR_P6MM,
    GE_PROCESSOR_P6MM_EX,
    GE_PROCESSOR_P6KATNI,
    GE_PROCESSOR_P6WMTNI,
    GE_PROCESSOR_PRESCOTT_YT,
    GE_PROCESSOR_PRESCOTT_LT,
    GE_PROCESSOR_TEJAS,
    GE_PROCESSOR_LAST
} GE_target_processor_t;

/* The GE_cache_share_hlevel_t represents the cache share hierarchy level options for the target_processor notifier */
typedef enum {
    GE_CACHE_HLEVEL_THREAD,
    GE_CACHE_HLEVEL_CORE,
    GE_CACHE_HLEVEL_CLUSTER,
    GE_CACHE_HLEVEL_SOCKET,
    GE_CACHE_HLEVEL_LAST
} GE_cache_share_hlevel_t;
    
typedef enum
{
    GE_EMRL_MODE_NO_MODE,    /* Default invocation mode before EMrl_init */
    GE_EMRL_MODE_EMPTY,      /* -emrl=s */
    GE_EMRL_MODE_BACK_END,   /* -emrl=b */
    GE_EMRL_MODE_PERFECT,    /* -emrl=p */
    GE_EMRL_MODE_NO_ICACHE,  /* -emrl=ni */
    GE_EMRL_MODE_FULL,       /* -emrl=f */
    GE_EMRL_MODE_HAZARDS,    /* -emrl=h */
    GE_EMRL_MODE_LAST
} GE_EMrl_mode_t;

typedef enum
{
/* GE_VGA_WINDOW_MAX MUST be first since it is the default */
    GE_VGA_WINDOW_MAX,
    GE_VGA_WINDOW_MIN,
    GE_VGA_WINDOW_HIDDEN
} GE_vga_window_t;


typedef enum
{
/* trash_NAT_bits : set NAT bits of */
    GE_TRASH_NAT_BITS_NONE,                /* default, no trashing              */
    GE_TRASH_NAT_BITS_STACKED_IREGS = 0x1, /* newly allocated stacked registers */
    GE_TRASH_NAT_BITS_STATIC_IREGS  = 0x2, /* static iregs upon br.call         */
    GE_TRASH_NAT_BITS_FPREGS        = 0x4, /* fp regs upon br.call              */
    GE_TRASH_NAT_BITS_LAST          = 0x8
} GE_trash_nat_bits_t;


/* used in exec_mode notifier */
typedef enum GE_exec_mode_e
{
    GE_EXEC_APP = 0,
    GE_EXEC_OS = 1
} GE_exec_mode_t;

/* EAS compatibility */
typedef enum GE_eas_comp_e
{
    GE_EAS_24 = 0,      /* eas24 */
    GE_EAS_23 = 1 ,     /* eas23 */
    GE_EAS_23_PLUS ,    /* eas23 + some acrs from eas24 */
    GE_EAS_23_PLUS_PLUS,/* eas23++ */
    GE_EAS_25,          /* eas25 */
    GE_EAS_26           /* eas26 */
} GE_eas_comp_t;

typedef enum
{
    GE_CPU_MODE_86,
    GE_CPU_MODE_REAL_BIG,
    GE_CPU_MODE_V86,
    GE_CPU_MODE_PROTECTED_16,
    GE_CPU_MODE_PROTECTED_32,
    GE_CPU_MODE_SYSTEM_MANAGEMENT,
    GE_CPU_MODE_EM,
    GE_CPU_MODE_LONG_COMPATIBLE_16,
    GE_CPU_MODE_LONG_COMPATIBLE_32,
    GE_CPU_MODE_LONG_64
} GE_cpu_mode_t;


typedef enum
{
    GE_INTERNAL_STATUS_NOT_INITIALIZED =     1,
    GE_INTERNAL_STATUS_CONFIGURED =          2,
    GE_INTERNAL_STATUS_INITIALIZED =         3,
    GE_INTERNAL_STATUS_EXECUTING =           6
} GE_internal_status_t;



/* External Interrupts: */

/* - Delivery modes */
typedef enum
{
    CPU_EXT_INTR_FIXED_DELIVERY_MODE = 0,
    CPU_EXT_INTR_LOWEST_PRIORITY_DELIVERY_MODE = 0x1,
    CPU_EXT_INTR_SMI_DELIVERY_MODE = 0x2,      /* IA32 only */
    CPU_EXT_INTR_PMI_DELIVERY_MODE = 0x2,      /* IA64 only */
    CPU_EXT_INTR_RESET_DELIVERY_MODE = 0x3,    /* IA32 only */
    CPU_EXT_INTR_NMI_DELIVERY_MODE = 0x4,
    CPU_EXT_INTR_INIT_DELIVERY_MODE = 0x5,
    CPU_EXT_INTR_START_UP_DELIVERY_MODE = 0x6, /* IA32 only */
    CPU_EXT_INTR_EXTINT_DELIVERY_MODE = 0x7    /* 8259 mode */
} Cpu_ext_intr_delivery_mode_t;

/* - Destination modes */
typedef enum
{
    CPU_EXT_INTR_PHYSICAL_DESTINATION_MODE = 0,
    CPU_EXT_INTR_LOGICAL_DESTINATION_MODE
} Cpu_ext_intr_destination_mode_t;

/* Destination shorthand */
typedef enum
{
    CPU_EXT_INTR_NO_SHORTHAND = 0,
    CPU_EXT_INTR_SELF = 0x1,
    CPU_EXT_INTR_ALL_INCLUDING_SELF = 0x2,
    CPU_EXT_INTR_ALL_EXCLUDING_SELF = 0x3
} Cpu_ext_intr_destination_shorthand_t;

/* - Constant for NMI vector */
#define CPU_EXT_INTR_NMI_VECTOR_NUM 2

typedef enum
{
    RAM_SHADOW_DISABLED = 0,
	RAM_SHADOW_READ_ONLY = 1,
	RAM_SHADOW_WRITE_ONLY = 2,
    RAM_SHADOW_READ_WRITE = 3
} RAM_shadow_t;





typedef unsigned int I32;

/*****************************************************************************
VPC Space Data Structures
*****************************************************************************/

typedef U8		*vpc_haddr_t;        // Pointer to a region of host memory
typedef char	*vpc_space_name_t; // A string that names a VPC space
typedef I64		vpc_space_id_t;     // An ID (handle) for an open VPC space


/*
 * Basic VPC data types.  We go with 64-bit values for everything.
 */
typedef I64 vpc_size_t;     // Largest "size specifier"
typedef I64 vpc_addr_t;     // An address in a VPC space
typedef I64 vpc_data_t;     // Largest amount of data that can be passed
                            // "by value" in a VPC API call


typedef struct {

    vpc_space_id_t sid;         // The VPC space in which the region resides
    vpc_addr_t addr;            // The starting address of the region
    vpc_size_t size;            // The extent of the region (in bytes)

} vpc_region_t;



/*
 * Bit flags for all access types:
 */
typedef enum {

    VPC_READ_ACCESS = 0x01,            // Read access to a VPC space
    VPC_WRITE_ACCESS = 0x02,           // Write access to a VPC space
    VPC_NO_ACCESS = 0x04               // No access

} vpc_access_types_t;

typedef unsigned vpc_access_t;

#define VPC_MAX_ACCESS_TYPE 2


/*
 * A VPC map enables a module to access directly a given region of VPC
 * space via a shared buffer in host memory.
 */
typedef struct {

    vpc_region_t region;        // The region that is mapped
    vpc_access_t access;        // The type of accesses permitted to the map
    vpc_haddr_t hptr;           // A pointer to the beginning of a host-
                                // memory buffer that maps the VPC region
} vpc_map_t;

typedef enum
{
    VPC_OS_CREATE_NEW        = 0x1, /* Creates a new file. The function fails
                                    if the specified file exists. */
    VPC_OS_CREATE_ALWAYS     = 0x2, /* Creates a new file. The function overwrites
                                    a file if exists, just like
                                    SSC_TRUNCATE_EXISTING */
    VPC_OS_OPEN_EXISTING     = 0x3, /* Opens the file. The function fails if the
                                    file does not exist. */
    VPC_OS_OPEN_ALWAYS       = 0x4, /* Opens the file, if it exists. Otherwise,
                                    the function creates the file just like
                                    SSC_CREATE_NEW. */
    VPC_OS_TRUNCATE_EXISTING = 0x5  /* Opens the file. Once opened, the file is
                                    truncated so that its size is zero bytes.
                                    The file must be opened with at least
                                    SSC_ACCESS_WRITE. The function fails if
                                    the file does not exist. */
} VPC_OS_CreateDist_t;

typedef struct _VPC_OS_TIME_FIELDS {
    unsigned long  Year;
    unsigned long  Month;
    unsigned long  Day;
    unsigned long  Hour;
    unsigned long  Minute;
    unsigned long  Second;
    unsigned long  Milliseconds;
    unsigned long  WeekDay;
} VPC_OS_TIME_FIELDS;

#define VPC_OS_MAX_VOLUMES       128
#define VPC_OS_MAX_VOLUME_NAME   512
#define VPC_OS_IO_BLOCK_SIZE     512
#define VPC_OS_ACCESS_EXIST  0x0
#define VPC_OS_ACCESS_READ   0x1  /* for OpenVolume */
#define VPC_OS_ACCESS_WRITE  0x2  /* for OpenVolume */


typedef struct _VPC_OS_FILE_INFO {
    char name[VPC_OS_MAX_VOLUME_NAME];
    unsigned directory : 1,
             readonly  : 1;
} VPC_OS_FILE_INFO;
typedef enum {
  K_CAPITAL=0x1,
  K_LMENU=0x2 ,
  K_RMENU=0x8,
  K_F10=0x10,
  K_LCONTROL=0x20,
  K_RCONTROL=0x40,
  K_INSERT=0x80,
  K_DELETE=0x100,
  K_LEFT =0x200,
  K_HOME=0x400,
  K_END=0x800,
  K_UP=0x1000,
  K_DOWN =0x2000,
  K_PRIOR=0x4000,
  K_NEXT=0x8000,
  K_RIGHT=0x10000,
  K_RSHIFT=0x20000,
  K_DIVIDE=0x40000,
  K_LSHIFT=0x80000,
  K_MENU=0x100000,
  K_ALT=0x200000,
  K_CONTROL=0x400000,
  K_SHIFT=  0x800000,
  K_NUMLOCK=0x1000000,
  K_SCROLL=0x2000000
} mod_keys;

#endif /* _VPC_TYPES_H */

#ifdef __cplusplus
};
#endif




