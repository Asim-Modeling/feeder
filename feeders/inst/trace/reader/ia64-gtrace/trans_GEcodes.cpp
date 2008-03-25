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

/**
 * @file
 * @author ??
 * @brief GTrace codes for additional info
 */

// generic
#include <stdlib.h>

struct code_map_typ {
  char * code;
  int tag;
} unsorted_code_table[] = {
  { "ACCESSES", 3136   }, 
  { "ACTIVE", 3041   }, 
  { "ACTIVE_CPU", 4148   }, 
  { "ADDRESS", 3080   }, 
  { "ADVISE", 4227   }, 
  { "ALIAS", 4078   }, 
  { "ALIGNMENTFIXUPCOUNT", 3155   }, 
  { "APCBYPASSCOUNT", 3231   }, 
  { "API_CNT", 4226   }, 
  { "ARCH", 3005   }, 
  { "ARGC", 3332   }, 
  { "ARGV", 3331   }, 
  { "ASYNC_IO_DELAY", 4077   }, 
  { "BASE", 3113   }, 
  { "BASE_CODE", 3314   }, 
  { "BASE_DATA", 3315   }, 
  { "BIOS", 4029   }, 
  { "BIOS_EMUL", 4213   }, 
  { "BRANCH", 3071   }, 
  { "BRANCH_HINT", 3072   }, 
  { "BRANCH_TAKEN", 3249   }, 
  { "BSP_NUM", 4207   }, 
  { "BUNDLE_BYTES", 4106   }, 
  { "BYTEWORDEMULATIONCOUNT", 3158   }, 
  { "CACHE", 3062   }, 
  { "CACHEIOCOUNT", 3218   }, 
  { "CACHEREADCOUNT", 3217   }, 
  { "CACHETRANSITIONCOUNT", 3213   }, 
  { "CALL", 3261   }, 
  { "CCCOPYREADNOWAIT", 3199   }, 
  { "CCCOPYREADNOWAITMISS", 3200   }, 
  { "CCCOPYREADWAIT", 3198   }, 
  { "CCCOPYREADWAITMISS", 3201   }, 
  { "CCDATAFLUSHES", 3208   }, 
  { "CCDATAPAGES", 3209   }, 
  { "CCFASTMDLREADNOTPOSSIBLE", 3188   }, 
  { "CCFASTMDLREADNOWAIT", 3185   }, 
  { "CCFASTMDLREADRESOURCEMISS", 3187   }, 
  { "CCFASTMDLREADWAIT", 3186   }, 
  { "CCFASTREADNOTPOSSIBLE", 3184   }, 
  { "CCFASTREADNOWAIT", 3181   }, 
  { "CCFASTREADRESOURCEMISS", 3183   }, 
  { "CCFASTREADWAIT", 3182   }, 
  { "CCLAZYWRITEIOS", 3207   }, 
  { "CCMAPDATANOWAIT", 3189   }, 
  { "CCMAPDATANOWAITMISS", 3191   }, 
  { "CCMAPDATAWAIT", 3190   }, 
  { "CCMAPDATAWAITMISS", 3192   }, 
  { "CCMDLREADNOWAIT", 3202   }, 
  { "CCMDLREADNOWAITMISS", 3204   }, 
  { "CCMDLREADWAIT", 3203   }, 
  { "CCMDLREADWAITMISS", 3205   }, 
  { "CCPINMAPPEDDATACOUNT", 3193   }, 
  { "CCPINREADNOWAIT", 3194   }, 
  { "CCPINREADNOWAITMISS", 3196   }, 
  { "CCPINREADWAIT", 3195   }, 
  { "CCPINREADWAITMISS", 3197   }, 
  { "CCREADAHEADIOS", 3206   }, 
  { "CEXIT", 3262   }, 
  { "CFM", 3034   }, 
  { "CHANGE", 3272   }, 
  { "CLASS_DATA", 3001   }, 
  { "CLASS_EVENT", 3002   }, 
  { "CLASS_NOTIFIER", 3003   }, 
  { "CLOOP", 3265   }, 
  { "CODE_BRK", 3276   }, 
  { "CODE_BRK_ADDR", 3084   }, 
  { "CODE_FETCH", 3256   }, 
  { "COMPLETED", 4102   }, 
  { "COND", 3260   }, 
  { "CONFIG", 4027   }, 
  { "COPYONWRITECOUNT", 3211   }, 
  { "CPL_CHANGE", 3243   }, 
  { "CPU_MODE", 3022   }, 
  { "CPU_MODE_CHANGE", 3244   }, 
  { "CPU_SHUTDOWN", 4193   }, 
  { "CR", 3024   }, 
  { "CREATE", 3100   }, 
  { "CTOP", 3266   }, 
  { "CTRL_C", 3304   }, 
  { "CURRENT", 3114   }, 
  { "CURR_USED", 3044   }, 
  { "CYCLE_COUNT", 3059   }, 
  { "DEBUGGING", 3008   }, 
  { "DEBUG_MODE", 3283   }, 
  { "DELETE", 3102   }, 
  { "DELIVERY_MODE", 4198   }, 
  { "DELTA_IN", 4005   }, 
  { "DELTA_OUT", 4006   }, 
  { "DEMANDZEROCOUNT", 3214   }, 
  { "DESTINATION", 4196   }, 
  { "DESTINATION_MODE", 4197   }, 
  { "DIRECT", 3269   }, 
  { "DIRTYPAGESWRITECOUNT", 3219   }, 
  { "DIRTYWRITEIOCOUNT", 3220   }, 
  { "DISK", 4100   }, 
  { "DPCBYPASSCOUNT", 3230   }, 
  { "DPCCOUNT", 3228   }, 
  { "DPCREQUESTRATE", 3229   }, 
  { "DPCTIME", 3225   }, 
  { "DRIVER_TYPE", 3952   }, 
  { "EMRL_MODE", 4004   }, 
  { "ENTRIES_NUM", 3148   }, 
  { "ENVP", 3333   }, 
  { "EPROCESS", 3104   }, 
  { "ETHREAD", 3106   }, 
  { "EXCEPTIONDISPATCHCOUNT", 3156   }, 
  { "EXECUTE", 4109   }, 
  { "EXEC_MODE", 3282   }, 
  { "EXEC_STOP", 3279   }, 
  { "EXT_INTR_MESSAGE", 4195   }, 
  { "FB_START_PADDR", 4080   }, 
  { "FETCH", 3274   }, 
  { "FILE", 3287   }, 
  { "FILE_ALIAS", 4075   }, 
  { "FIXED", 3007   }, 
  { "FLOATINGEMULATIONCOUNT", 3157   }, 
  { "FLUSH", 3275   }, 
  { "FULL_PATH", 3094   }, 
  { "GAMBIT_RC", 3338   }, 
  { "GAMBIT_SIZE", 3116   }, 
  { "GB_IO_FUNCS", 3318   }, 
  { "GB_MEM", 4217   }, 
  { "GB_MEM1", 4228   }, 
  { "GE", 3011   }, 
  { "GERR", 3359   }, 
  { "GE_MEM", 4216   }, 
  { "GIN", 3357   }, 
  { "GOUT", 3358   }, 
  { "HINT", 3077   }, 
  { "HITS", 3144   }, 
  { "HPTRS_CHUNK", 4224   }, 
  { "IA", 3267   }, 
  { "IDLETIME", 3227   }, 
  { "IL", 3016   }, 
  { "IMAGE", 3088   }, 
  { "IMAGE_SIZE", 3098   }, 
  { "IMAGE_TYPE", 3099   }, 
  { "IMPLICIT_MEMORY_ACCESS", 4151   }, 
  { "INDIRECT", 3270   }, 
  { "INITVALUE", 4039   }, 
  { "INST", 3040   }, 
  { "INSTRUCTION", 4105   }, 
  { "INST_ALL_VALUES", 4186   }, 
  { "INST_DB", 4225   }, 
  { "INST_ID", 3076   }, 
  { "INST_MODE", 4115   }, 
  { "INTERRUPTION", 3245   }, 
  { "INTERRUPTTIME", 3226   }, 
  { "INVALIDS", 3146   }, 
  { "IO", 3012   }, 
  { "IOOTHEROPERATIONCOUNT", 3164   }, 
  { "IOOTHERTRANSFERCOUNT", 3161   }, 
  { "IOREADOPERATIONCOUNT", 3162   }, 
  { "IOREADTRANSFERCOUNT", 3159   }, 
  { "IOWRITEOPERATIONCOUNT", 3163   }, 
  { "IOWRITETRANSFERCOUNT", 3160   }, 
  { "IO_BASEADDRESS", 4074   }, 
  { "I_COUNT", 3119   }, 
  { "KECONTEXTSWITCHES", 3165   }, 
  { "KEFIRSTLEVELTBFILLS", 3166   }, 
  { "KEINTERRUPTCOUNT", 3167   }, 
  { "KERNELTIME", 3224   }, 
  { "KESECONDLEVELTBFILLS", 3168   }, 
  { "KESYSTEMCALLS", 3169   }, 
  { "KEY_DOWN_MSG_RATE", 4083   }, 
  { "LAST_BASE", 4208   }, 
  { "LAST_EVENT_IL", 3112   }, 
  { "LOAD", 3092   }, 
  { "LOAD_BASE", 3095   }, 
  { "LOAD_COUNT", 3097   }, 
  { "LOCAL_FD", 3288   }, 
  { "MAPPEDPAGESWRITECOUNT", 3221   }, 
  { "MAPPEDWRITEIOCOUNT", 3222   }, 
  { "MARK", 3091   }, 
  { "MAXIMUMWORKINGSETSIZE", 3236   }, 
  { "MAX_MEM", 3301   }, 
  { "MEMORY", 3125   }, 
  { "MEMORY_ACCESS", 3073   }, 
  { "MEMORY_MAPPED_IO", 4041   }, 
  { "MINIMUMWORKINGSETSIZE", 3235   }, 
  { "MISSES", 3142   }, 
  { "MMAVAILABLEPAGES", 3170   }, 
  { "MMINFOCOUNTERS", 3180   }, 
  { "MMPAGEDPOOLPAGE", 3176   }, 
  { "MMPEAKCOMMITMENT", 3173   }, 
  { "MMSYSTEMCACHEPAGE", 3175   }, 
  { "MMSYSTEMCACHEWS", 3232   }, 
  { "MMSYSTEMCODEPAGE", 3174   }, 
  { "MMSYSTEMDRIVERPAGE", 3177   }, 
  { "MMTOTALCOMMITLIMIT", 3172   }, 
  { "MMTOTALCOMMITTEDPAGES", 3171   }, 
  { "MMTOTALSYSTEMCODEPAGES", 3178   }, 
  { "MMTOTALSYSTEMDRIVERPAGES", 3179   }, 
  { "MM_ACTIVE", 4191   }, 
  { "MM_DEBUG", 4194   }, 
  { "MOUSE_MOVE_MSG_RATE", 4082   }, 
  { "MOVE_TO_BR", 3075   }, 
  { "MP", 4116   }, 
  { "MP_DEBUG", 4209   }, 
  { "NANOS_PER_INST", 4076   }, 
  { "NOISY", 3296   }, 
  { "NONE", 3257   }, 
  { "NON_ACCESS", 3081   }, 
  { "NOT_TAKEN", 3259   }, 
  { "NO_SIGNON", 3294   }, 
  { "NO_WARN", 3295   }, 
  { "NUM_OF_SIMULATED_CPUS", 4200   }, 
  { "OS", 3009   }, 
  { "PAGEFAULTCOUNT", 3210   }, 
  { "PAGEREADCOUNT", 3215   }, 
  { "PAGEREADIOCOUNT", 3216   }, 
  { "PEAKWORKINGSETSIZE", 3234   }, 
  { "PHYS_PAGE", 4222   }, 
  { "PORT", 3117   }, 
  { "PREDICATE", 3074   }, 
  { "PROCESS", 3089   }, 
  { "PROCESSID", 3103   }, 
  { "PROCESS_ID", 3096   }, 
  { "RANDOM_SCHEDULING", 4203   }, 
  { "RANDOM_TIME_SLICE", 4202   }, 
  { "RC_INFO", 4028   }, 
  { "READ", 3238   }, 
  { "READS", 3138   }, 
  { "REAL_NAME", 4079   }, 
  { "RECOVER", 3141   }, 
  { "REDRAW_DELAY", 4081   }, 
  { "REGISTER", 3017   }, 
  { "REGISTERS", 3083   }, 
  { "REPLACES", 3147   }, 
  { "REP_COUNT", 3929   }, 
  { "REP_STEPPING", 3928   }, 
  { "RET", 3268   }, 
  { "RFI", 3246   }, 
  { "RSE_READ", 4152   }, 
  { "RSE_WRITE", 4153   }, 
  { "SERR", 3362   }, 
  { "SET", 3273   }, 
  { "SIMUL", 4215   }, 
  { "SIMULATION_START", 3953   }, 
  { "SIMULATOR_NAME", 3316   }, 
  { "SIMUL_PROG", 3286   }, 
  { "SIMUL_PROG_ILP", 3111   }, 
  { "SIMUL_PROG_NAME", 3110   }, 
  { "SIM_APP_IO_FUNCS", 3319   }, 
  { "SIN", 3360   }, 
  { "SIZE", 3042   }, 
  { "SOFT_LIT", 4206   }, 
  { "SOUT", 3361   }, 
  { "SSC_INFO", 4034   }, 
  { "STAGE", 3281   }, 
  { "START", 4210   }, 
  { "STARTADDRESS", 4038   }, 
  { "STARTED", 4101   }, 
  { "STAT", 3014   }, 
  { "STDALL", 3337   }, 
  { "STDERR", 3336   }, 
  { "STDIN", 3334   }, 
  { "STDOUT", 3335   }, 
  { "STOP", 4211   }, 
  { "STRING_OP", 3931   }, 
  { "STUB", 3307   }, 
  { "SWITCH", 3101   }, 
  { "SWITCH_ACTIVE_CPU", 4192   }, 
  { "SYSTEMCONTEXTSWITCHINFORMATION", 3152   }, 
  { "SYSTEMEXCEPTIONINFORMATION", 3149   }, 
  { "SYSTEMFILECACHEINFORMATION", 3154   }, 
  { "SYSTEMINTERRUPTINFORMATION", 3153   }, 
  { "SYSTEMPERFORMANCEINFORMATION", 3150   }, 
  { "SYSTEMPROCESSORPERFORMANCEINFORMATION", 3151   }, 
  { "TAG", 3079   }, 
  { "TAKEN", 3258   }, 
  { "TARGET", 3078   }, 
  { "TARGET_PROCESSOR", 3299   }, 
  { "TERMINATE", 3280   }, 
  { "THREAD", 3090   }, 
  { "THREADID", 3105   }, 
  { "TIMER", 3109   }, 
  { "TIME_OUT", 3941   }, 
  { "TIME_SLICE", 4201   }, 
  { "TLB", 3021   }, 
  { "TLB_ACCESS", 4003   }, 
  { "TLB_INFO", 3020   }, 
  { "TLB_STAT", 3128   }, 
  { "TOTAL_FREE", 4218   }, 
  { "TOTAL_MALLOC", 4220   }, 
  { "TOTAL_POOL", 4219   }, 
  { "TOTAL_VALLOC", 4221   }, 
  { "TRACE", 3285   }, 
  { "TRACE_BUFFER", 3108   }, 
  { "TRACE_MODE_B", 3326   }, 
  { "TRACE_MODE_D", 3329   }, 
  { "TRACE_MODE_F", 4190   }, 
  { "TRACE_MODE_I", 4107   }, 
  { "TRACE_MODE_M", 3330   }, 
  { "TRACE_MODE_PRED", 4189   }, 
  { "TRACE_MODE_R", 4111   }, 
  { "TRACE_MODE_S", 3328   }, 
  { "TRACE_MODE_W", 4112   }, 
  { "TRANSITIONCOUNT", 3212   }, 
  { "TRANS_CHUNK", 4223   }, 
  { "TRCGEN_SIZE", 3115   }, 
  { "TRC_DEF", 3323   }, 
  { "TRC_ON", 3325   }, 
  { "TRC_OPEN", 3324   }, 
  { "TYPE", 4040   }, 
  { "UARCH", 3006   }, 
  { "UNALIGNED", 3127   }, 
  { "UNLOAD", 3093   }, 
  { "UPDATE", 3271   }, 
  { "USERTIME", 3223   }, 
  { "VALUE", 3082   }, 
  { "VECTOR", 4199   }, 
  { "VGA", 4035   }, 
  { "VHPT_READ", 4154   }, 
  { "VIEW_CPU", 4149   }, 
  { "VPC", 4205   }, 
  { "VPC_DEBUG_MODE", 4187   }, 
  { "VPC_SOFT_LIT", 4204   }, 
  { "WEXIT", 3263   }, 
  { "WINNT", 3015   }, 
  { "WORKINGSETSIZE", 3233   }, 
  { "WRITE", 3237   }, 
  { "WRITES", 3139   }, 
  { "WTOP", 3264   }, 
  { "W_HITS", 3145   }, 
  { "W_MISSES", 3143  },
  { NULL, 0 }
};

static char ** code_map = NULL;
static int min_code, max_code; 

char * unknowncode = "UNKNOWNCODE"; 
void init_code_map()
{
  int i;
  struct code_map_typ * cme;
  int num_els;
  int idx; 

  min_code = 99999999;
  max_code = 0;
  i = 0;
  while(unsorted_code_table[i].code != NULL) {
    cme = &(unsorted_code_table[i]);

    if(cme->tag < min_code) min_code = cme->tag;
    if(cme->tag > max_code) max_code = cme->tag;
    i++; 
  }

  num_els = (max_code - min_code) + 1;

  code_map = (char **) calloc(num_els, sizeof(char *));

  for(i = 0; i < num_els; i++) {
    code_map[i] = unknowncode; 
  }
  
  i = 0;
  while(unsorted_code_table[i].code != NULL) {
    cme = &(unsorted_code_table[i]);

    idx = cme->tag - min_code;
    code_map[idx] = cme->code; 
    i++; 
  }
}

void delete_code_map(void)
{
  if(code_map) {
    free(code_map);
    code_map = NULL;
  }
}
char * code2string(int code)
{
  if(code_map == NULL) init_code_map();

  if(code > max_code) return unknowncode;
  if(code < min_code) return unknowncode;
  return code_map[code - min_code];
}
