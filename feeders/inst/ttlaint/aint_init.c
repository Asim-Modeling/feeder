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
 * Routines for reading and parsing the text section and managing the memory
 * for an address space
 */

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
#include <errno.h>
#include <machine/fpu.h>
#include <sys/auxv.h>
#include <fcntl.h>

#define MAIN

#include "icode.h"
#include "opcodes.h"
#include "globals.h"
#include "pmint.h"
#include "gdbserver.h"

/* prototypes for functions */

#include "protos.h"

#ifndef DEBUG
#define USAGE \
"\nUsage: %s [aint options] [-- simulator options --] objfile [objfile options]\n\n\
        or\n\
        %s [aint options] [-EVSIM simulator options --]  (-p \"objfile [objfile args]\")|(-s snapshot)+ \n\
                (-pwf pwffilename)|(-atf atffilename.gz:CPU#) \n\n\
aint options:\n\
        [-r procs]           number of per-process regions, default %d\n\
        [-n threads]         number of threads (contexts), default %d\n\
        [-t i]    trace instructions \n\
        [-t n]    trace nothing \n\
        [-t r]    trace memory references \n\
        [-t s]    trace shared references \n\
        [-t p]    trace private references \n\
        [-t x]    trace synchronization events \n\
        [-t a]    trace references AND synchs (default) \n\
                  (Only the last of multiple -t options holds) \n\
        [-b]      DON'T parse basic-block info \n\
        [-C interval]        checkpoint interval\n\
        [-f statsfile]       file to write Aint statistics to\n\
        [-g host:port]       gdbserver host:port contact information\n\
        [-j jsep]            Alternate value for job separator: jsep defaults to --\n\
        [-q]      quiet operation\n\
        [-x inst_addr]       instr (hex addr) to flag: used for debugging \n\
        [-u]      recognize universal no-op (ldq_u $r31, o(a)) \n\
        [-w]      handle more signals: SEGV and BUSERR \n\
        [-v]                 verbose operation\n\
	[-y]      remove ldgp after sqrtt (only for broken patched executables)\n\
        [-z]      suppress sqrt() -> sqrtt translation\n\
        OBSOLETE OPTIONS: \
        [-k stack_size]      stack size in bytes \n\
"
#else
#define USAGE "\nUsage: %s [-p procs] [-t i|n|r|s|p|x|a] [-j jsep] [-u] [-v] [-w] [-y] [-z] [-- sim_opts] objfile\n\
   or   %s [aint options] [-EVSIM simulator options --]  (-p \"objfile [objfile args]\")+ \n\n"
#endif 

void aint_stats();
void create_addr_space(thread_ptr pthread, process_ptr process);
void init_all_queues();
int parse_thread_arg_list(char **argv, int argc, int arg_start, int pid);
void parse_thread_arg_string(char *arg_string, int pid,
	       char **stdin_name, char **stdout_name);
void copy_argv(thread_ptr pthread, char **envp);
void init_thread(thread_ptr pthread, event_ptr pevent);
static void save_args(int argc, char **argv);
static void parse_args(int argc, char ** argv);
void copy_addr_space(thread_ptr parent, thread_ptr child);
void init_thread_addr_space(thread_ptr parent, thread_ptr child);

static void usage();

FILE *Fobj;
static time_t Start_time;
static time_t Finish_time;
static char Start_date[40];
static char Finish_date[40];
static ulong Mem_size_start;
static ulong Mem_size_finish;
static struct rusage Rusage;
static double Total_elapsed_time;
static double Total_cpu_time;

static int HandleSignals;

static int Argc;
char **Argv;

#define START_DATA_ADDRESS 0x200000000L
#define START_CODE_ADDRESS 0x300000000L

char *aint_brk_address =		(char*) START_DATA_ADDRESS;
char *aint_brk_code_address =	(char*) START_CODE_ADDRESS;

char *debugger = "xterm -e decladebug -pid ";
void invoke_debugger_on_self ()
{
  if (0) {
    char cmd[2048];
    sprintf(cmd, "%s %d %s&\n", debugger, getpid(), Argv[0]);
    system(cmd);
    while(1) {
      /* spin wait for debugger */
    }
  }
}

static void aint_install_sighandlers ()
{
  if (0) {
    struct sigaction action, oaction;
    action.sa_handler = (void (*)(int))(&invoke_debugger_on_self); 
    action.sa_mask = 0;
    action.sa_flags = 0;
    sigaction(SIGILL, &action, &oaction);
    sigaction(SIGFPE, &action, &oaction);
    sigaction(SIGBUS, &action, &oaction);
    sigaction(SIGSEGV, &action, &oaction);
  }
}

void open_as_fileno(process_ptr process, int vfd, char *filename, int oflag, mode_t mode){
  int pfd;
  if (!filename || !(*filename))
    filename = "/dev/null|";

  fprintf(Aint_output,"opening(%s) as fileno %d\n", filename, vfd);

  pfd = open(filename,  oflag, mode);
  process->fd[vfd] = pfd;
    
  if (pfd == -1) {
    fatal("Couldn't open '%s' as fileno=%d with oflag=%d mode=0%o, errno=%d(%s)\n", 
	  filename, vfd, oflag, mode, errno, sys_errlist[errno]);
  }
}

extern void tlb_atexit();

/* Here we assume we are passing as in trace7 */
void
aint_init(int argc, char **argv, char **envp)
{
  
  int next_arg;
  int use_dynamic_loader = 0;
  int main_thread_count=0;
  const char *snapshot_filename = NULL;
  unsigned long matchpc = (unsigned long)-1;
  
  Aint_output = stdout;
  Quiet = 1; /* be quiet by default */

#ifdef NEW_TTL
  /* right now this is hard-coded */
  Aint_marker_filename = "AINT_MARKER_STATS";
  Aint_marker_output = fopen(Aint_marker_filename,"w");
  if (Aint_marker_output==NULL) Aint_marker_output = stdout;
#endif
  
  atexit(tlb_atexit);
  
  Start_time = time(NULL);
  Mem_size_start = (ulong) sbrk(0);
  
  Shmem_start = SHARED_START;
  Shmem_end = Shmem_start;
  
  Recycle_threads = 0;
  
  /* make GNU getopt behave as expected */
  putenv("POSIXLY_CORRECT=1");
  
  aint_install_sighandlers();
  
  Simname = argv[0];
  
  /* Make using basic blocks the default */
  aint_parse_basic_blocks = 1;
  /* make sqrt instruction translation the default */
  replace_sqrt_function_calls = 1;
  
  save_args(argc, argv);
  parse_args(argc, argv);
  
  if (!Quiet)
      fprintf(Aint_output, "sizeof(struct icode) = %d\n", sizeof(struct icode)); 
  
  if (!job_separator)
    job_separator = "--";
  
  next_arg = sim_init(argc, argv);
  
  if (next_arg >= argc) {
    error("Error: missing object file.\n");
    usage();
    pmint_usage(argc, argv);
    exit(1);
  }
  
  /* init_all_queues: 
   * initializes Threads[] and Processes[], among other things 
   */
  init_all_queues();
  if (!Quiet)
    fprintf(Aint_output, "init_queues...\n");
  
  /* Set up the substitutes for syscalls */
  subst_init();
  if (!Quiet)
    fprintf(Aint_output, "set up subs for syscalls. starting main_thread\n"); 
  
  while(next_arg < argc) {
    
    thread_ptr pthread;
    process_ptr process;
    char *Stdin_name=NULL;
    char *Stdout_name=NULL;
    int pid;
    
    main_thread_count++;
    if (main_thread_count>Max_nthreads){
      error("Number of specified threads exceeded user-specified thread limit.\n");
      usage();
      pmint_usage(argc, argv);
      exit(1);
    }
    /* allocate a new thread */
    INLINE_DEQUEUE(&Free_q, pthread);
    INLINE_DEQUEUE(&Free_Process_q, process);
    pid = process->pid;
    pthread->process = process;
    process->threads = pthread;
    
    if (strcmp(argv[next_arg],"-p")==0){
      if (argv[next_arg+1]==NULL){
	fatal("Error no command line followed -p for thread %d.",pthread->tid);;
      }
      /* Parse string giving process objname and args */
      parse_thread_arg_string(argv[next_arg+1], pid,
			      &Stdin_name, &Stdout_name);
      next_arg+=2;
      pmint_set_thread_type (pid, TT_AINT);
      Objname=ProcessArgv[pid][0];
    } 
    else if (strcmp(argv[next_arg],"-s")==0){
      extern int very_verbose;
      if (argv[next_arg+1]==NULL){
	fatal("Error no filename followed -s for thread %d.",pthread->tid);;
      }
      /* Parse string giving process objname and args */
      parse_thread_arg_string(argv[next_arg+1], pid,
			      &Stdin_name, &Stdout_name);
      next_arg+=2;
      snapshot_filename=ProcessArgv[pid][0];
      Objname=read_snapshot_objname(snapshot_filename);
      pmint_set_thread_type (pid, TT_AINT);
      ProcessArgv[pid][0] = Objname;
    }
    else if (strcmp(argv[next_arg],"-pwf")==0){
      if (argv[next_arg+1]==NULL){
	fatal("Error no command line followed -pwf for thread %d.",pthread->tid);;
      }
      /* parse PW filename */
      fprintf(Aint_output, "Setting thread type for pid=%d to TT_PW(%d)\n", pid, TT_PW);
      pmint_set_thread_type (pid, TT_PW);
      ProcessFilename[pid] = argv[next_arg+1];
      ProcessArgv[pid] = NULL;
      ProcessArgc[pid] = 0;
      next_arg+=2;
      Objname = NULL;
      if (!Quiet) fprintf(Aint_output, "PW Trace file tid=%d filename=%s\n", 
			  pthread->tid, ProcessFilename[pid]);
    }
    else if (strcmp(argv[next_arg],"-atf")==0){
      if (argv[next_arg+1]==NULL){
	fatal("Error no command line followed -atf for thread %d.",pthread->tid);;
      }
      /* parse ATF filename */
      pmint_set_thread_type (pid, TT_ATF);
      ProcessFilename[pid] = argv[next_arg+1];
      ProcessArgv[pid] = NULL;
      ProcessArgc[pid] = 0;
      next_arg+=2;
      Objname = NULL;
      warning("ATF inside AINT isn't fully debugged; you're at your own risk.");
      if (!Quiet) fprintf(Aint_output, "ATF Trace file tid=%d filename=%s\n", 
			  pthread->tid, ProcessFilename[pid]);
    }
    else{
      Objname= argv[next_arg];
      /* Parse arg list. */
      pmint_set_thread_type (pid, TT_AINT);
      next_arg = parse_thread_arg_list(argv, argc, next_arg, pid);
    }

    if (Objname==NULL) {
      /* NOP for pwf/atf */
    } else {
      if (!Quiet)
	fprintf(Aint_output, "Object file tid=%d exename=%s\n", pthread->tid, Objname);

#ifdef NEW_TTL
#if 0
      /*
       * Tarantula:
       *
       *  Start by looking for the PC of all ttl_xxx routines so that we can later
       *  patch the binary file
       */
      ttl_find_entry_points(Objname);
#endif      
#endif

#ifdef NEW_TLDS
      /*
       * TLDS:
       *
       *  Start by looking for the PC of all tlds_xxx routines so that we can later
       *  patch the binary file
       */
      tlds_find_entry_points(Objname);
#endif

      /* find interesting procedure addresses */
      {
	const char *names[] = {
	  "sim_user",
	  "sqrt",
	  "sqrtf",
	  "F_sqrt",
	  "F_sqrtf",
	  "__F_sqrt4",
	  "__sqrt4",
	  NULL
	};
	unsigned long addrs[sizeof(names)/sizeof(char*)]; 
	find_proc_addrs(Objname, names, addrs);
	sim_user_addr = addrs[0];
	sqrt_function_addr = addrs[1];
	sqrtf_function_addr = addrs[2];
	f_sqrt_function_addr = addrs[3];
	f_sqrtf_function_addr = addrs[4];
	f_sqrt4_function_addr = addrs[5];
	sqrt4_function_addr = addrs[6];

	if (!Quiet && (replace_sqrt_function_calls))
	{
	    fprintf(Aint_output, 
		    "    sqrt_function_addr=%lx\n"
		    "    sqrtf_function_addr=%lx\n"
		    "    f_sqrt_function_addr=%lx\n"
		    "    f_sqrtf_function_addr=%lx\n"
		    "    f_sqrt4_function_addr=%lx\n"
		    "    sqrt4_function_addr=%lx\n",
		    sqrt_function_addr, sqrtf_function_addr,
		    f_sqrt_function_addr, f_sqrtf_function_addr,
		    f_sqrt4_function_addr, sqrt4_function_addr);
	}	

	initJSRTargets(Objname);
      }
      if ( sim_user_addr != (unsigned long)-1) {
	/*
	 * The entry in the symbol table is from the "globally callable"
	 * symbol (i.e., the true entry to sim_user..).
	 */
	if (!Quiet)
	  fprintf(Aint_output, "found_callbacks from %s ...\n", Objname);
      }

      if (snapshot_filename != NULL) {
	if (!Quiet)
	  fprintf(Aint_output, "restoring program %s from snapshot %s ... ", 
		  Objname, snapshot_filename);
	init_main_thread(pthread);
	restore_process(pthread->process, snapshot_filename);

      } else {
	/* Load the program */
	if (!Quiet)
	  fprintf(Aint_output, "loading program %s ... ", Objname);
	if (Verbose) {
	  read_hdrs(Objname);
	}
	object_file_info(Objname, &process->stack_end, &use_dynamic_loader, &process->brk);
	if (use_dynamic_loader) {
	  const char *sbinloader = "/sbin/loader";
	  informative("AINT_INIT: using dynamic loader '%s'\n", sbinloader);
	  load_object_file(process, sbinloader);
	} else {
	  load_object_file(process, Objname);
	}
        if (!Quiet)
           fprintf(Aint_output, "Setting the break value for %s to %lx\n", Objname, process->brk);
	if (!Quiet)
	  fprintf(Aint_output, "done\n");

      }
      if (snapshot_filename == NULL) {
	/* Allocate memory and initialize address space */
	if (!Quiet)
	  fprintf(Aint_output, "creating_addr_space...");
	create_addr_space(pthread, process);
      }
      if (!Quiet)
	fprintf(Aint_output, "done\n");

    }

    /*
     * Allocate memory for the Objname string and store a pointer to it in the
     * pthread data structure. This is needed to implement the FEED_Symbol
     * interface to the performance model. Because this interface includes the
     * possibility of naming a subroutine, we may need to call 'find_proc_addrs'
     * in the middle of a thread execution. For that, we will need to remember
     * the object file name.
     *
     * roger.
     */
    pthread->Objname = NULL;
    if ( Objname != NULL ) {
     /*
      * malloc enough space to hold the string
      */
     pthread->Objname = (char *)malloc(strlen(Objname) + 1);
     if ( pthread->Objname == NULL ) {
      fatal("Unable to allocate %d bytes of memory for Objname\n",strlen(Objname) + 1);
     }
     strcpy(pthread->Objname,Objname);
    }


    if (snapshot_filename == NULL) {
      /* Copy argc, argv, envp onto the process stack */
      copy_argv(pthread, envp);
      init_main_thread(pthread);
    }

    if(Stdin_name)
      open_as_fileno(process,STDIN_FILENO,Stdin_name,O_RDONLY,0);
    if(Stdout_name)
      open_as_fileno(process,STDOUT_FILENO,Stdout_name,(O_WRONLY|O_CREAT|O_TRUNC),0664);
    if (!Quiet) fprintf(Aint_output, "closed object file and copied args...done\n");

  }

  pmint_init(Max_nthreads, Nthreads);
  if (Debugger_tty_name != NULL)
    gdbserver_init(Debugger_tty_name);

  { 
    /* handle signals during issue */
    struct sigaction action, oaction;

    action.sa_handler = (void(*)(int))handle_signals_for_issue;
    action.sa_mask = 0;
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGFPE, &action, &oaction);
    if (HandleSignals) {
      sigaction(SIGSEGV, &action, &oaction);
      sigaction(SIGBUS, &action, &oaction);
    }
    AINT_in_issue = 0;
    AINT_issue_process = NULL;
  }
}


static void
usage()
{
  fprintf(stdout, USAGE, Simname, Simname,
	  MAX_NPROCS, MAX_NTHREADS);
}

static void
save_args(int argc, char **argv)
{
    int i;
    
    /* copy the args */
    Argc = argc;
    Argv = (char **) malloc((argc + 1) * sizeof(char *));
    if (Argv == NULL)
        fatal("save_args: cannot allocate 0x%x bytes for Argv.\n",
              (argc + 1) * sizeof(char *));
    for (i = 0; i < argc; i++) {
        Argv[i] = (char *) malloc(strlen(argv[i]) + 1);
        if (Argv[i] == NULL)
            fatal("save_args: cannot allocate 0x%x bytes for Argv[%d].\n",
                  strlen(argv[i]) + 1, i);
        strcpy(Argv[i], argv[i]);
    }
    Argv[argc] = NULL;
}

const char *runstate_names[] = {
  "R_RUN",
  "R_SLEEP",
  "R_STALL",
  "R_BLOCKED",
  "R_WAIT",
  "R_ZOMBIE",
  "R_DONE",
  "R_FREE",
  "R_STOPPED", /* by ptrace() equivalent */
  "R_SIGNAL",
  "R_SINGLESTEP"
};

void
aint_done()
{
  
  int i;

  if (0)
  for (i = 0; i < Max_nthreads; i++) {
    fprintf(Aint_output, "AINT_DONE: Threads[%d].runstate = %s\n", 
	    i, runstate_names[Threads[i].runstate]);
  }

  aint_stats();

  sim_done(Total_elapsed_time, Total_cpu_time);
  pmint_done();
  gdbserver_done();
  
  return;
}

void
aint_stats()
{
  if (Aint_output != NULL) {
    int i;
    unsigned int elapsed, minutes, seconds;
    unsigned int user_sec, user_min, user_usec, sys_sec, sys_min, sys_usec;
    unsigned int total_sec, total_min, total_usec;
    double user_fsec, sys_fsec, total_fsec, cpu_seconds;
    int mem_used;
    char *machine_name;
    struct utsname utsname;

    Finish_time = time(NULL);

    getrusage(RUSAGE_SELF, &Rusage);

    if (!Quiet)
    {
	fprintf(Aint_output, "\nCommand line: ");
	for (i=0; i<Argc; i++)
	    fprintf(Aint_output, " %s", Argv[i]);
	fprintf(Aint_output, "\n");
    }

    strcpy(Start_date, ctime(&Start_time));

    /* remove trailing newline */
    Start_date[strlen(Start_date)-1] = 0;
    fprintf(stdout, "\nStarted: %s\n", Start_date);
    
    strcpy(Finish_date, ctime(&Finish_time));

    /* remove trailing newline */
    Finish_date[strlen(Finish_date)-1] = 0;
    fprintf(stdout, "\nFinished: %s\n", Finish_date);

    elapsed = Finish_time - Start_time;
    minutes = elapsed/60;
    seconds = elapsed % 60;
    if (uname(&utsname) < 0)
      machine_name = "(unknown)";
    else
      machine_name = utsname.nodename;

    fprintf(stdout, "Elapsed time for simulation: %u:%02u on %s\n",
	    minutes, seconds, machine_name);

    user_sec = Rusage.ru_utime.tv_sec;
    user_usec = Rusage.ru_utime.tv_usec;

    user_min = user_sec/60;
    user_sec = user_sec % 60;
    user_fsec = user_sec + (double) user_usec/1.0e+6;

    sys_sec = Rusage.ru_stime.tv_sec;
    sys_usec = Rusage.ru_stime.tv_usec;

    sys_min = sys_sec/60;
    sys_sec = sys_sec % 60;
    sys_fsec = sys_sec + (double) sys_usec/1.0e+6;

    total_min = user_min + sys_min;
    total_sec = user_sec + sys_sec;
    total_usec = user_usec + sys_usec;

    if (total_usec >= 1000000) {
      total_sec++;
      total_usec -= 1000000;
    }
    if (total_sec >= 60) {
      total_min++;
      total_sec -= 60;
    }
    total_fsec = total_sec + (double) total_usec/1.0e+6;
    cpu_seconds = total_min*60 + total_fsec;

    fprintf(stdout,
	    "CPU time: user: %u:%05.2f, system: %u:%05.2f total: %u:%05.2f (%.2f sec)\n", user_min, user_fsec, sys_min, sys_fsec, total_min, total_fsec, cpu_seconds);

    Mem_size_finish = (ulong) sbrk(0);
    mem_used = Mem_size_finish - Mem_size_start;
    mem_used = (mem_used + 1023)/1024;
    Total_elapsed_time = Threads[0].time;
    Total_cpu_time = Threads[0].cpu_time + Threads[0].process->child_cpu;
    fprintf(Aint_output, "Space used by malloc: %dK\n", mem_used);

    fprintf(Aint_output, "\nElapsed simulated cycles: %.0f, cpu cycles: %.0f\n", Total_elapsed_time, Total_cpu_time);

    fprintf(Aint_output, "Processors used = %d, average cpu_time/proc = %.1f\n\n", Maxtid+1, Total_cpu_time/(Maxtid + 1));
  }
}

/* Parse command line arguments */
/* Parse flags until we run out of flags or encounter -- or -p.
 * If we encounter -- or -p stop parsing and leave -- or -p in the arg list.
 */
static void
parse_args(int argc, char **argv)
{
  int c, errflag, done;
  extern char *optarg;
  extern int optind;

  /* Set up default values */
  Stack_size = STACK_SIZE;
  Heap_size = HEAP_SIZE;
  Shmem_size = SHMEM_SIZE;
  Ckpoint_freq = DEFAULT_CKPOINT_FREQ;
  Max_nprocs = MAX_NPROCS;
  Max_nthreads = MAX_NTHREADS;
  Trace_option = TRACE_DEFAULT;    /* Defined in globals.h */



  done=0;
  errflag = 0;
  while(!done && ((c = getopt(argc, argv, "bC:Ef:g:h:j:k:n:Opr:qs:x:t:uvVwz")) !=-1) ) {
    switch (c) {
    case 'p':
      /* -p indicates the start of a command. */
      done=1;
      optind--;
      break;
    case 'E':
      /* -E may indicate start of EVSIM commands. */
      done=1;
      break;
    case 'b':
      aint_parse_basic_blocks = 0;
      break;
    case 'C':
      Ckpoint_freq = strtol(optarg, NULL, 0);
      break;
    case 'f':
      Aint_output_filename = optarg;
      if (*Aint_output_filename == '+') {
	Aint_output_filename++;
	Aint_output = fopen(Aint_output_filename, "a");
      } else {
	Aint_output = fopen(Aint_output_filename, "w");
      }
      if (Aint_output == NULL) {
	Aint_output = stdout;
	fatal("Failed to open '%s': %s\n", 
	      Aint_output_filename, sys_errlist[errno]);
      }
      break;
    case 'g':
      Debugger_tty_name = optarg;
      break;
    case 'h':
      Heap_size = strtol(optarg, NULL, 0);
      break;
    case 'j':
      job_separator = optarg;
      break;
    case 'k':
      Stack_size = strtol(optarg, NULL, 0);
      break;
    case 'r':
      Max_nprocs = strtol(optarg, NULL, 0);
      /* Max_nprocs++;     * Workaround for a bug (being investigated) in
			 * create_addr_space() */
      break;
    case 'n':
	Max_nthreads = strtol(optarg, NULL, 0);
	break;
    case 'O':
      aint_use_oracle_thread = 1;
      break;
    case 'q':
      Quiet = 1;
      Verbose = 0;
      if (Aint_output_filename == NULL)
	Aint_output = fopen("/dev/null", "w");
      break;
    case 's':
      Shmem_size = strtol(optarg, NULL, 0);
      break;
    case 'x':
      sscanf(optarg, "%lx", &ObjHook);
      break;

    case 't':
      switch(*optarg) {
      case 'i':
	Trace_option = TRACE_INST;
	break;
      case 'x':
	Trace_option = TRACE_SYNC;
	break;
      case 'n':
	Trace_option = TRACE_NONE;
	break;
      case 'r':
	Trace_option = TRACE_REFS;
	break;
      case 's':
	Trace_option = TRACE_SHARED;
	break;
      case 'p':
	Trace_option = TRACE_PRIVATE;
	break;
      case 'd':
      default:
	Trace_option = TRACE_DEFAULT;
	break;
      }
      break;
    case 'u':
      aint_recognize_unop = 1;
      break;
    case 'v':
      Verbose = 1;
      break;
    case 'V':
      aint_use_verifier_thread = 1;
      break;
    case 'w':
      HandleSignals = 1;
      break;
    case 'y':
      /* */
      zap_bogus_ldgp_after_sqrtt = 1;
      break;
    case 'z':
      /* disable sqrt function call translation */
      replace_sqrt_function_calls = 0;
      break;
    case '-':
      break;
    default:
      errflag = 1;
      break;
  }
  }
  if (optind > 1 && strcmp(argv[optind-1], "--") == 0) {
    optind--;
  }

  if (errflag) {
    usage();
    pmint_usage(argc, argv);
    exit(1);
  }
}


void
create_addr_space(thread_ptr pthread, process_ptr process)
{
  /* the initial stack is below the text segment and grows down */ 
  /* be careful about programs compiled to use 32-bit addresses 
   * -- compute stack_end from process->entry_point
   */
  ulong stack_end = BASE2ROUNDDOWN(process->stack_end, TB_PAGESIZE);
  ulong stack_size = Stack_size;
  int i;
  for (i = 0; i < process->n_segments; i++) {
    segment_t *segment = &process->segments[i];
    if (segment->start < stack_end)
      stack_end = segment->start;
  }
  if (stack_size > stack_end)
    stack_size = stack_end/2;

  pthread->process = process;

  /* magic number from Assembly Language Programmer's Guide */
  Data_end = 0x3ff00000000l;

  /* map the stack segment */
  if (stack_end != 0)
    process_add_segment(process, 
			stack_end - stack_size,
			stack_size,
			PROT_READ|PROT_WRITE,
			MAP_PRIVATE);

#ifdef UNDEFINED
  if (Shmem_size) {
    allocate_fixed(Shmem_start, Shmem_size);
  }
#endif

  MapReg(SP) = stack_end;

  /* Set the gp register */
  MapReg(GP) = process->initial_gp;

} 



void *allocate_pages(long nbytes)
{
  void *p;
  nbytes = BASE2ROUNDUP(nbytes, TB_PAGESIZE);
  p = mmap((caddr_t) aint_brk_address,
         nbytes, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_VARIABLE|MAP_PRIVATE, -1, 0);
  aint_brk_address += nbytes;
  
  /* ensure that we are not overlapping memory areas */
  if (aint_brk_address >= (char*)START_CODE_ADDRESS)
  	fatal (">>>FEEDER PANIC: the reserver logical address space to data has been exhausted, review aint_init.c definitions\n");
	
  return p;
}

void *allocate_code_pages(long nbytes)
{
  void *p;
  nbytes = BASE2ROUNDUP(nbytes, TB_PAGESIZE);
  p = mmap((caddr_t) aint_brk_code_address,
         nbytes, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_VARIABLE|MAP_PRIVATE, -1, 0);
  aint_brk_code_address += nbytes;
  return p;
}

void *
allocate_fixed(long addr, long nbytes)
{
  char *real_addr;

  /* Use mmap to allocate memory at a specific address */
  real_addr = (char *) mmap( (caddr_t) addr, nbytes,
			     PROT_READ|PROT_WRITE,
			     MAP_ANONYMOUS|MAP_FIXED|MAP_PRIVATE, -1, 0);

  if ( (long) real_addr != addr)
    fatal("allocate_fixed: cannot mmap memory segment");
  return (void*)real_addr;
}


/* These next 2 functions parse the arguments for a thread.
 *
 * Both parse the arguments and put the results into ProcessArgc[pid] 
 * and ProcessArgv[pid][].
 *
 * The first parses classic aint style argument lists.
 *   It also returns the index of the next arg,
 * The second parses trace7 style argument strings.
 *   It checks for stdin/stdout redirection, and if it find one 
 *   sets stdin_name/stdout_name and removes them from arg list.
 */
int
parse_thread_arg_list(char **argv, int argc, int arg_start, int pid)
{
  int i;
  int next_arg;
  int num_args;

  /* Look for first job separator (or end of arg list) */
  next_arg = argc;
  for (i=arg_start; i<argc; i++) {
    if (strcmp(argv[i], job_separator) == 0){
      next_arg = i+1;
      argc = i;
      break;
    }
  }

  num_args = argc - arg_start;	/* Number of args, counting program name */

  /* set up ProcessArgv[pid] */
  ProcessArgc[pid] = num_args;
  ProcessArgv[pid] = (char **)malloc((num_args+1)*sizeof(char*));
  if (ProcessArgv[pid] == NULL)
    fatal("Could not allocate space for ProcessArgv[%d]\n", pid);
  for (i=0; i<num_args; i++) 
    ProcessArgv[pid][i] = argv[i+arg_start];
  ProcessArgv[pid][i] = NULL;

  return next_arg;
}


void
parse_thread_arg_string(char *arg_string, int pid,
	  char **stdin_name, char **stdout_name)
{
  int i;
  int argc;
  char *c;

  ProcessArgv[pid] = (char **)malloc((256+1)*sizeof(char*));
  if (ProcessArgv[pid] == NULL)
    fatal("Could not allocate space for ProcessArgv[%d]\n", pid);

    /* Code to parse args, based on code in trace.c */
    c = arg_string;
    argc = 0;
    while (*c) {
      /*
       * Skip whitespace
       */
      while (*c && *c == ' ') c++;
      
      if (*c) {
	ProcessArgv[pid][argc++] = c;
	
	if (argc == 256)
	  error("Too many arguments for child process.");
	
	/*
	 * Find the end of the string.  If the string starts with < or
	 * > then it is a file redirection token.  End the argument there.
	 */
	if (*c == '<') {
	  c++;
	  while (*c && *c == ' ') c++;
	  *stdin_name = c;
	  ProcessArgv[pid][--argc] = 0;
	}
        else if (*c == '>') {
	  c++;
	  while (*c && *c == ' ') c++;
	  *stdout_name = c;
	  ProcessArgv[pid][--argc] = 0;
	}

        while (*c && *c != ' ') c++;
	if (*c) {
	  *c++ = 0;
	}
      }
    }

    ProcessArgv[pid][argc] = 0;
    ProcessArgc[pid] = argc;
}

/* Copy args from ProcessArgv onto stack... */
void
copy_argv(thread_ptr pthread, char **envp)
{
  int i, size, nenv;
  char *sp;
  char **argv_obj, **eptr, **envp_obj;
  char *str_obj;
  long *auxvp_obj;
  process_ptr process = pthread->process;
  int pid = process->pid;
  const char *loader = "/sbin/loader"; 
  long exec_filename_obj;
  long exec_loader_filename_obj;
  long auxv[8];
  int argc;
  char **argv;

  argc=ProcessArgc[pid];
  argv=ProcessArgv[pid];
  nenv = 0;

  /* Print out args for this thread. */
  if (!Quiet){
    fprintf(Aint_output,"Process %d has the following args: ",pid);
    for(i=0;i<argc;i++)
      fprintf(Aint_output," %s ",ProcessArgv[pid][i]);
    fprintf(Aint_output,"\n");
  }


  if (argc > 0) {
    size=0;
    for (i=0; i<argc; i++) {
      size += (strlen(argv[i]) + 1);
    }
    for (eptr = envp; *eptr; nenv++,eptr++) size += strlen(*eptr) + 1;

    /* leave space for the loader_filename */
    size += strlen(loader)+1;


    /* add in space for the argv and envp pointers, including NULLs */
    size += (argc + nenv + 2) * sizeof(char *);
    
    size += sizeof(long);	/* for argc */

    /* leave space for auxv */
    size += sizeof(auxv);

    /* Round up size to quad word boundary */
    size = BASE2ROUNDUP(size, 8);

    /* Allocate space on stack for the args */
    MapReg(SP) -= size;
    /* sp = (long *) MAP(pthread->reg[SP]); */
    sp = (char *) MapReg(SP);

    /* First item is argc */
#ifdef DEBUG_ARGV
    fprintf(Aint_output, "Writing argc=%d at 0x%p\n", argc, sp);
#endif

    /* *sp++ = argc; */
    * (long *) PMAP((long) sp) = argc; sp += sizeof(long);

    /* Next comes the array of pointers argv */
    argv_obj = (char**)sp;
    /* Leave space for the argv array, including the NULL pointer */
    sp += (argc + 1)*sizeof(char*);

    /* Pointer and space for the environment vars */
    envp_obj = (char**)sp;
    sp += (nenv + 1)*sizeof(char*);

    /* auxv */
    auxvp_obj = (long *)sp;
    sp += sizeof(auxv);

    /* where to write the strings */
    str_obj = (char *)sp;

    /* Copy the args to the stack of the main thread */
    informative("process %d: ", process->pid);
    exec_filename_obj = (long)str_obj;
    for (i = 0; i < argc; i++) {

#ifdef DEBUG_ARGV
      fprintf(Aint_output, "Storing arg %s at 0x%p\n", argv[i], str_obj);
#endif
      informative(" %s", argv[i]);

      /* strcpy((char *) PMAP((long)str_obj), ProcessArgv[i]); */
      strcpy_to_object_space(process, (ulong)str_obj, argv[i]);

      * (char **) PMAP((long)(argv_obj + i)) = str_obj;
      str_obj += strlen(argv[i]) + 1;
    }
    * (long **) PMAP((long)(argv_obj + argc)) = NULL;

    /* Copy the environment variables to the stack of the main thread */
    for (i = 0,eptr = envp; i < nenv; i++,eptr++) {

#ifdef DEBUG_ARGV
      fprintf(Aint_output, "Storing envstr %s at 0x%p\n", *eptr, str_obj);
#endif
    
      /* strcpy((char *)PMAP((long)str_obj), *eptr); */
      strcpy_to_object_space(process, (ulong)str_obj, *eptr);

      * (char **) PMAP((long) (envp_obj + i)) = str_obj;
      str_obj += strlen(*eptr) + 1;
    }
    * (char **) PMAP((long) (envp_obj+ nenv)) = NULL;

    /* output the auxv */
    /* auxv filename */
    {
      long auxvi = 0;
      exec_loader_filename_obj = (long)str_obj;
      auxv[auxvi++] = AT_EXEC_FILENAME;
      auxv[auxvi++] = exec_filename_obj;
      auxv[auxvi++] = AT_EXEC_LOADER_FILENAME;
      auxv[auxvi++] = exec_loader_filename_obj;
      auxv[auxvi++] = AT_EXEC_LOADER_FLAGS;
      auxv[auxvi++] = 0xbeef;
      auxv[auxvi++] = 0;
      auxv[auxvi++] = 0;

      strcpy_to_object_space(process, exec_loader_filename_obj, loader);
      memcpy_to_object_space(process, (long)auxvp_obj, (char*)auxv, sizeof(auxv));
    }


    /* Debug: print the values of things on stack. */
#ifdef DEBUG_ARGV
    for (i = 0; i < 30; i++) {
      fprintf(Aint_output, "0x%lx (0x%lx): 0x%lx\n", pthread->Reg[SP].UInt64+8*i, PMAP(pthread->Reg[SP].UInt64+8*i), * (long *) (PMAP(pthread->Reg[SP].UInt64+8*i)));
    }
#endif
  }
}

/* Copy the parent's address space to the child's. This also sets up
 * all the mapping fields in the child's thread structure.
 */
void
copy_addr_space(thread_ptr parent, thread_ptr child)
{
    int i, pid;
    process_ptr parentp, childp;

    struct TB_Entry *tb_entry_freelist;
    void *tb_page_freelist, *next_free_page;
    int copy_count, tbkey;
    struct Shm_ds *parent_shm_ds, *child_shm_ds;

    parentp = parent->process;
    childp = child->process;

    /* The address space has already been allocated */
    pid = childp->pid;
    
    /* for each segment in parent, map corresponding segment to child */
    { 
      int segn;
      for (segn = 0; segn < parentp->n_segments; segn++) {
	segment_t *segment = &parentp->segments[segn];
	process_add_segment(childp, segment->start, segment->size, segment->prot, segment->flags);
      }
    }

    /* for each page-table entry, duplicate the entry, and the page from parent */
    tb_entry_freelist = (struct TB_Entry *) malloc(parentp->num_pages * sizeof(struct TB_Entry));
    if (tb_entry_freelist == NULL) fatal("Cannot allocate page-table entries for child\n");

    tb_page_freelist = allocate_pages(parentp->num_private * TB_PAGESIZE);
    if (tb_page_freelist == (caddr_t) (-1)) fatal("Copy_addr_space: Cannot allocate %d pages for child\n", 
					parentp->num_private);
    copy_count = 0;
    next_free_page = tb_page_freelist;

    for (tbkey=0; tbkey<TB_SIZE; tbkey++) {
      struct TB_Entry *parent_pg, *child_pg;

      parent_pg = parentp->TB[tbkey];
      if (parent_pg) {
	childp->TB[tbkey] = tb_entry_freelist + copy_count;
	childp->TB[tbkey]->lookaside = childp->TB[tbkey];
      }
      child_pg = childp->TB[tbkey];

      while (parent_pg) {
	if (parent_pg->next) 
	  child_pg->next = child_pg+1;
	else
	  child_pg->next = NULL;

	child_pg->tag = parent_pg->tag;

	child_pg->flags = parent_pg->flags;

	/* DONT COPY IF PAGE IS SHARED!!! */
	if (parent_pg->flags & MAP_PRIVATE) {
	  child_pg->page = next_free_page;   
	  next_free_page = (void *) ((long)next_free_page + TB_PAGESIZE);
	  memcpy(child_pg->page, parent_pg->page, TB_PAGESIZE);
	  if (parent_pg->textpage)
	    fatal("copy_addr_space -- cannot copy textpage\n");
	  else
	    child_pg->textpage = NULL;
	} else {
	  /* This is a shared page simply put in a link */
	  child_pg->page = parent_pg->page;
	  if (parent_pg->textpage)
	    child_pg->textpage = parent_pg->textpage;
	  else
	    child_pg->textpage = NULL;
	}

	copy_count++;
	child_pg = child_pg->next;
	parent_pg = parent_pg->next;

      }
    }
    childp->num_pages = parentp->num_pages;
    childp->num_private = parentp->num_private;
    childp->n_instructions = parentp->n_instructions;


    /* Need to copy the Shared-regions list... Currently working under
    *  the assumption that parent won't attach segments before a fork
    */
    parent_shm_ds = parentp->Shmem_regions;
    if (parent_shm_ds) {
      childp->Shmem_regions = (struct Shm_ds *) malloc(sizeof(struct Shm_ds));
      child_shm_ds = childp->Shmem_regions;
      if (child_shm_ds == NULL) fatal("copy_addr_space: cannot allocate shmem descriptor (local)\n");
    }
    for (; parent_shm_ds; parent_shm_ds = parent_shm_ds->next) {
      /* Copy contents */
      child_shm_ds->shmid = parent_shm_ds->shmid;
      child_shm_ds->size = parent_shm_ds->size;
      child_shm_ds->addr = parent_shm_ds->addr;
      if (parent_shm_ds->next) {
	child_shm_ds->next = (struct Shm_ds *) malloc(sizeof(struct Shm_ds));
	if (child_shm_ds->next == NULL) fatal("copy_addr_space: cannot allocate shmem descriptor (local)\n");
      } else child_shm_ds->next = NULL;

      child_shm_ds = child_shm_ds->next;
    }

    /* Copy the value of the next shmat map to use */
    childp->Unsp_Shmat_Current = parentp->Unsp_Shmat_Current;

    /* Copy all the registers 
     * for (i=0; i<33; i++) {
     *	child->reg[i] = parent->reg[i];
     *	child->fp[i] = parent->fp[i];
     * } 
     */

    for (i=0; i< TOTAL_PHYSICALS; i++) {
      child->Reg[i] = parent->Reg[i];
    }
    child->FirstFreePhysicalRegister = parent->FirstFreePhysicalRegister;      
    child->FirstFreeVectorPhysicalRegister = parent->FirstFreeVectorPhysicalRegister;      

    for (i=0; i< TOTAL_LOGICALS; i++) {
      child->RegMap[i] = parent->RegMap[i];
    }
    for (i=0; i<(TOTAL_PHYSICALS/32); i++) {
      child->RegValid[i] = parent->RegValid[i];
    }
    child->fpcr = parent->fpcr;
	
    /* copy the symbol table */
    childp->symtab = parentp->symtab;
    childp->nsymbols = parentp->nsymbols;

}


void
init_thread_addr_space(thread_ptr parent, thread_ptr child)
{
  int i;
  process_ptr process = child->process;
  ulong stack_end = Data_end - (child->tid)*Stack_size;

  /* map the stack */
  process_add_segment(process, 
		      stack_end - Stack_size,
		      (ulong)Stack_size,
		      PROT_READ|PROT_WRITE,
		      MAP_PRIVATE);

  /* Copy all the registers *
  for (i=0; i<33; i++) {
    child->reg[i] = parent->reg[i];
    child->fp[i] = parent->fp[i];
  } */

  for (i=0; i < TOTAL_PHYSICALS; i++) {
    child->Reg[i] = parent->Reg[i];
  }
  child->FirstFreePhysicalRegister = parent->FirstFreePhysicalRegister;
  child->FirstFreeVectorPhysicalRegister = parent->FirstFreeVectorPhysicalRegister;

  for (i=0; i < TOTAL_LOGICALS; i++) {
    child->RegMap[i] = parent->RegMap[i];
  }
  for (i=0; i < (TOTAL_PHYSICALS/32); i++) {
    child->RegValid[i] = parent->RegValid[i];
  }

  child->fpcr = parent->fpcr;
	
  /* Give the child a separate stack
   *  child->Reg[child->RegMap[SP]] = stack_end; 
   */

  MapReg_thr(child, SP) = stack_end;

  /* set the pmint hardware thread type of the child to be the same as that of the parent */
  pmint_set_thread_type(child->tid, pmint_get_thread_type(parent->tid));
}
