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
 * @author Artur Klauser, Jim Vash
 *
 * Trace reader for IA64 GTrace (Gambit) traces.
 * This is a cleaned up version (ie. conforming to ASIM coding standards) of
 * Matt Reilly's Ciardi GTrace reader. It is intended to be used as
 * part of the code to implement an ASIM Trace Reader plugin module
 * for the ASIM Trace Feeder set of modules.
 */

// generic
#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <iostream>

// ASIM core
#include "asim/syntax.h"
#include "asim/trace.h"
#include "asim/mesg.h"
#include "asim/ioformat.h"

// ASIM local module
//#include "ia64ciardibundle.h"
//#include "ia64ciardi.h"
#include "asim/provides/tracereader.h"
#include "common_ge_token.h"
#include "trans_GEcodes.h"

namespace iof = IoFormat;
using namespace iof;

//#define CIARDI_WARN

#ifdef CIARDI_WARN
#define TRACE_WARNING(msg) cerr << "IA64Ciardi line #" << lineNum <<": " << msg;
#else
#define TRACE_WARNING(msg)
#endif

IA64_CIARDI_CLASS::action_rec IA64_CIARDI_CLASS::action_vec[] = {
    { "bb" , &IA64_CIARDI_CLASS::bb_action },
    { "br" , &IA64_CIARDI_CLASS::br_action },
    { "da" , &IA64_CIARDI_CLASS::da_action },
    { "c2" , &IA64_CIARDI_CLASS::c2_action },
    { "sp" , &IA64_CIARDI_CLASS::sp_action },
    { "bh" , &IA64_CIARDI_CLASS::bh_action },
    { "mv" , &IA64_CIARDI_CLASS::mv_action },
    { "c1" , &IA64_CIARDI_CLASS::c1_action },
    { "rf" , &IA64_CIARDI_CLASS::rf_action },
    { "in" , &IA64_CIARDI_CLASS::in_action },
    { "ir" , &IA64_CIARDI_CLASS::ir_action },
    { "or" , &IA64_CIARDI_CLASS::or_action },
    { "bs" , &IA64_CIARDI_CLASS::bs_action },
    { NULL , &IA64_CIARDI_CLASS::xx_action }
};

/**
 * Default constructor: initialize this object
 */
IA64_CIARDI_CLASS::IA64_CIARDI_CLASS (
    UINT32 id)    ///< ID of this trace stream
    : traceID(id)
{
    traceIP = 0;
    bundleLineInBuffer = false;
    wPtr = -1;
    infile = NULL;
    lineNum = 0;
    used = false;
    compressed = false;

    SetTraceableName("IA64_CIARDI_CLASS");

    //initialize the registers in the bundles to invalid
    for(int i=0; i<WSize; i++)
    {
        bundle[i].ClearRegisters();
    }


    // initialize HexDigit translation table
    memset (char2int, ~0, sizeof(char2int)); // default is error: ~0
    for (char c = '0'; c <= '9'; c++) {
        // 0..9 translation 0..9
        char2int[c] = c - '0';
    }
    for (char c = 'A'; c <= 'F'; c++) {
        // A..F translation 10..15
        char2int[c] = c - 'A' + 10;
    }
    for (char c = 'a'; c <= 'f'; c++) {
        // a..f translation 10..15
        char2int[c] = c - 'a' + 10;
    }
}

/**
 * Default destructor: print some stats
 */
IA64_CIARDI_CLASS::~IA64_CIARDI_CLASS()
{
    if (used)
    {
        Format fmtLabel = Format("    ", "-25", " = ");
        T1(fmtLabel("num trace lines read") << lineNum);
    }

    delete_code_map();
}

/**
 * Open a file associated with this reader. A filename of "-" means stdin.
 * @returns true on success, false on failure
 */
bool
IA64_CIARDI_CLASS::FileOpen (
    const char * fileName)  ///< file to open
{
    used = true;
    if(fileName[0] == '-' && fileName[1] == 0) {
        infile = stdin;
        return true;
    }

    // check to see if we have a compressed file
    char * p = strrchr(fileName,'.');
    compressed = ((p != NULL)
                   && (   (!strcmp (p, ".bz2"))
                       || (!strcmp (p, ".gz"))
                       || (!strcmp (p, ".z"))
                       || (!strcmp (p, ".Z"))));

    // now open; compressed files decompress on-the-fly
    if (compressed) {
        std::ostringstream pipecmd;
        if (!strcmp (p, ".bz2"))
        {
            pipecmd << "bunzip2 -c " << fileName;
        } else {
            pipecmd << "gunzip -c " << fileName;
        }
        T1("Opening input via pipe '" << pipecmd.str()
              << "'");
        infile = popen (pipecmd.str().c_str(), "r");
    } else {
        T1("Opening input '" << fileName << "'");
        infile = fopen(fileName, "r");
    }
    if(infile == NULL) {
        return false;
    }

    lineNum = 0; // reset line number

    // synchronize traceIP: read trace file until we find a taken branch
    // and thus know the IP of the current point in the trace
    T1("Synchronizing trace IP ... ");
    bool successInitTraceIP = false;
    while(ReadNextLine()) {
#if 0 /* CIARDI_DUMP */
        printf("%s", buffer);
#endif
        if(buffer[0] == 'b' && buffer[1] == 'r')
        {
            // special rule: at the beginning of the trace we
            // search for a "br"anch action to set the initial traceIP
            successInitTraceIP = br_action_init_traceIP();
            if (successInitTraceIP)
            {
                break;
            }
        }
    }
    if (! successInitTraceIP)
    {
        // attempt to find a trace IP failed - report error
        T1(endl << "ERROR: no sync <br> found");

        return false;
    }

    return true;
}

/**
 * Close the file associated with this reader.
 */
void
IA64_CIARDI_CLASS::FileClose (void)
{
    if (compressed) {
        pclose(infile);
    } else {
        fclose(infile);
    }
    infile = NULL;
}

/**
 * Get the next bundle from the trace file.
 */
IA64_CIARDI_BUNDLE
IA64_CIARDI_CLASS::GetNextBundle (void)
{
    // fill object-wide bundle window to be accessed by all parsing methods

    // slide the bundle window
    if (wPtr != -1) {
#if 0 /* CIARDI_DUMP */
        for (UINT32 line = 0; line < bundle[0].lines; line++) {
            printf("%s", bundle[0].buffers[line]);
        }
#endif
        for (INT32 idx = 0; idx < wPtr; idx++) {
            bundle[idx] = bundle[idx + 1];
        }
        wPtr--;
    }

    // process previous BB record if it exists
    if (bundleLineInBuffer) {
        bb_action();
#if 0 /* CIARDI_DUMP */
        ASSERTX(bundle[wPtr].lines == 0);
        sprintf(bundle[wPtr].buffers[bundle[wPtr].lines++], buffer);
#endif
    }

    // continue processing records
    while (ReadNextLine()) {
        // ignore comment lines
        if (buffer[0] == '#') {
#if 0 /* CIARDI_DUMP */
            if (wPtr == -1) {
                printf("%s", buffer);
            }
            else {
                sprintf(bundle[wPtr].buffers[bundle[wPtr].lines++], buffer);
                ASSERTX(bundle[wPtr].lines < 10);
            }
#endif
            continue;
        }
        // process the record
        int i;
        for (i = 0; action_vec[i].cmdstr != NULL; i++) {
            if (strncmp(action_vec[i].cmdstr, buffer, 2) == 0) {
                (this->*action_vec[i].fn)();
                break;
            }
        }
        // check for invalid records
        if (action_vec[i].cmdstr == NULL) {
            BadCode();
        }
        // abort if the window is full of bundles
        if (bundleLineInBuffer == true) {
            return &bundle[0];
        }
#if 0 /* CIARDI_DUMP */
        else {
            if (wPtr == -1) {
                printf("%s", buffer);
            }
            else {
                sprintf(bundle[wPtr].buffers[bundle[wPtr].lines++], buffer);
                ASSERTX(bundle[wPtr].lines < 10);
            }
        }
#endif
    }

    if (wPtr != -1) {
        return &bundle[0];
    } else {
        return NULL;
    }
}

#if 0 /* CIARDI_DUMP */
void
IA64_CIARDI_CLASS::HackSPRecord(UINT64 vpregs)
{
    // find the SP record
    for (UINT32 line = 0; line < bundle[0].lines; line++) {
        if (bundle[0].buffers[line][0] == 's' && bundle[0].buffers[line][1] == 'p') {
            UINT64 ip;
            UINT64 pregs;
            int inst_id;
            char dummy[3];
            int stat;

            stat = sscanf(bundle[0].buffers[line], "%2s "FMT64X" "FMT64X" %x",
                          dummy, &ip, &pregs, &inst_id);

            ASSERTX(stat == 4);

            sprintf(bundle[0].buffers[line], "%2s "FMT64X" "FMT64X" %x\n",
                    dummy, ip, vpregs, inst_id);

            return;
        }
    }
    // must find one...
    ASSERTX(false);
}

void
IA64_CIARDI_CLASS::HackC1Record()
{
    // find the C1 record
    for (UINT32 line = 0; line < bundle[0].lines; line++) {
        if (bundle[0].buffers[line][0] == 'c' && bundle[0].buffers[line][1] == '1') {
            UINT64 ip;
            int pr1;
            int v1;
            int inst_id;
            char dummy[3];
            int stat;

            stat = sscanf(bundle[0].buffers[line], "%2s "FMT64X" %x %x %x",
                          dummy, &ip, &pr1, &v1, &inst_id);

            ASSERTX(stat == 5);

            sprintf(bundle[0].buffers[line], "%2s  "FMT64X"  %x  %x  %x\n",
                    dummy, ip, 63, v1, inst_id);

            return;
        }
    }
    // must find one...
    ASSERTX(false);
}
#endif

//-----------------------------------------------------------------------------
// decoder action functions
//-----------------------------------------------------------------------------

/**
 * Decoder for bundle info \n
 * format: \n
 * bb <128-bit hex value: encoded bundle>
 */
void
IA64_CIARDI_CLASS::bb_action (void)
{
    // abort if window is full of bundles
    if ((wPtr + 1) == WSize) {
        bundleLineInBuffer = true;
        return;
    } else {
        bundleLineInBuffer = false;
        wPtr++;
    }

    // initialize the bundle
    bundle[wPtr].Clear();

    // fill bundle info
    Read2Quad(&(buffer[3]), &bundle[wPtr].bits[1], &bundle[wPtr].bits[0]);
    bundle[wPtr].vpc = traceIP.GetAddr();

    static const Format bfmt = "016x";
    T1("TRACE line #" << lineNum << ": "
          << "Bundle: " << fmt("8x", bundle[wPtr].vpc) << "  "
          // note: little endian
          << bfmt(bundle[wPtr].bits[1])
          << bfmt(bundle[wPtr].bits[0]));

    UINT32 sNum = traceIP.GetSyllableIndex();
    input_register_syllable=sNum; 
    output_register_syllable=sNum; 
    // if we have a traceIP that does not point to the first syllable of a
    // bundle, we have to mark the preceeding syllables as wrongpath
    for (UINT32 i = 0; i < sNum; i++) {
        bundle[wPtr].syllable[i].wrongpath = true;
    }

    traceIP.IncBundle();
}

/**
 * Decoder for branch info \n
 * format: \n
 * br <code1> <code2> <ip> <dest> <hint> <cfm> <instid> <?> \n
 * code1 ... conditional, uncond., etc. \n
 * code2 ... taken, not-taken, etc.
 */
void
IA64_CIARDI_CLASS::br_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    int code1;
    int code2;
    UINT64 ip;
    UINT64 dest; 
    int hint;
    UINT64 cfm;
    int inst_id;
    int reg;
    char dummy[3]; 
    int stat;

    stat = sscanf(buffer, "%2s %x %x "FMT64X" "FMT64X" %x "FMT64X" %x %x",
                  dummy, &code1, &code2,
                  &ip, &dest,
                  &hint, &cfm,
                  &inst_id, &reg);

    if (stat != 9) {
        ParseError(stat);
        return;
    }

    T1("TRACE line #" << lineNum << ": "
          << ">>\t" << fmt("08x", ip) << " BR"
          << " " << code2string(code1) << " " << code2string(code2)
          << " -> " << fmt("08x", dest)
          << "  [" << fmt_x(inst_id) << "]");

    UINT32 sNum = SyllableId(ip);

    bundle[wPtr].syllable[sNum].taken = (code2 == GE_TAKEN);
    if (bundle[wPtr].syllable[sNum].taken) {
        traceIP.Set (BundleAddr(dest), SyllableId(dest)); 
    }

    if (bundle[wPtr].syllable[sNum].taken) {
        bundle[wPtr].syllable[sNum].target = traceIP; // taken syllable
    } else {
        IADDR_CLASS my_ip = IADDR_CLASS(BundleAddr(bundle[wPtr].vpc), sNum);
        bundle[wPtr].syllable[sNum].target = my_ip.Next(); // fallthrough syllable
    }
    bundle[wPtr].syllable[sNum].cfm = cfm;

    // if we have a taken branch, we mark the remaining syllables in
    // the bundle as wrongpath
    if (bundle[wPtr].syllable[sNum].taken) {
        for (UINT32 i = sNum + 1; i < NumSyllables; i++) {
            bundle[wPtr].syllable[i].wrongpath = true;
        }
    }

    CheckConsistentIP(bundle[wPtr].vpc, ip, "branch");
}

/**
 * SPECIAL Decoder for first taken branch info to determine initial traceIP \n
 * format: \n
 * br <code1> <code2> <ip> <dest> <hint> <cfm> <instid> <?> \n
 * code1 ... conditional, uncond., etc. \n
 * code2 ... taken, not-taken, etc.
 *
 * @return true if finding a traceIP was successfull
 */
bool
IA64_CIARDI_CLASS::br_action_init_traceIP (void)
{
    int code1;
    int code2;
    UINT64 ip;
    UINT64 dest; 
    int hint;
    UINT64 cfm;
    int inst_id;
    int reg;
    char dummy[3]; 
    int stat;
    bool isTaken;

    stat = sscanf(buffer, "%2s %x %x "FMT64X" "FMT64X" %x "FMT64X" %x %x",
                  dummy, &code1, &code2,
                  &ip, &dest,
                  &hint, &cfm,
                  &inst_id, &reg);

    if (stat != 9) {
        ParseError(stat);
        return false;
    }

    // start trace 'br' better be a taken branch
    isTaken = (code2 == GE_TAKEN);

    if (isTaken) {
        // set the trace start IP
        traceIP.Set (BundleAddr(dest), SyllableId(dest)); 

        T1("Setting trace start IP to " << traceIP);

        return true;
    } else {
        return false;
    }
}

/**
 * Decoder for memory (data) access info. \n
 * format: \n
 * da <code> <ip> <virtual_ea> <hint> <size> <non_access?> <instid> \n
 * code ... read, write, etc.
 */
void
IA64_CIARDI_CLASS::da_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    int code; 
    UINT64 ip;
    UINT64 addr;
    int hint;
    int size;
    int non_access;
    int inst_id;
    char dummy[3]; 
    int stat;

    stat = sscanf(buffer, "%2s %x "FMT64X" "FMT64X" %x %x %x %x",
                  dummy, &code,
                  &ip, &addr,
                  &hint, &size, 
                  &non_access, &inst_id);

    if(stat != 8) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
          << ">>\t" << fmt("08x", ip) << " MEMACC "
          << code2string(code)
          << " [" << fmt("08x", addr) << "]"
          << " size = " << size
          << " NA:" << non_access
          << " [" << fmt_x(inst_id) << "]");
    
    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].addr = addr;

    CheckConsistentIP(bundle[wPtr].vpc, ip, "memory access");
}

/**
 * Decoder for predicate outcome info (1 destination). \n
 * format: \n
 * c1 <ip> <pred_destreg> <pred_value> <instid>
 */
void
IA64_CIARDI_CLASS::c1_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 ip;
    int pr1;
    int v1;
    int inst_id;
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s "FMT64X" %x %x %x",
                  dummy, &ip, &pr1, &v1, &inst_id);

    if(stat != 5) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
          << ">>\t" << fmt("08x", ip) << " PRED1"
          << " pr[" << pr1 << "]" << "<-" << v1
          << "  [" << fmt_x(inst_id) << "]");

    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].value[0] = v1;
    bundle[wPtr].syllable[sNum].vpr[0]   = pr1;

    CheckConsistentIP(bundle[wPtr].vpc, ip, "predicate 1");
}

/**
 * Decoder for predicate outcome info (2 destinations). \n
 * format: \n
 * c2 <ip> <pred1_destreg> <pred1_value> <pred2_destreg> <pred2_value> <inst_id>
 */
void
IA64_CIARDI_CLASS::c2_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 ip;
    int pr1;
    int v1;
    int pr2;
    int v2;
    int inst_id;
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s "FMT64X" %x %x %x %x %x",
                  dummy, &ip, &pr1, &v1, &pr2, &v2, &inst_id);

    if(stat != 7) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
          << ">>\t" << fmt("08x", ip) << " PRED2"
          << " pr[" << pr1 << "]" << "<-" << v1
          << " pr[" << pr2 << "]" << "<-" << v2
          << "  [" << fmt_x(inst_id) << "]");

    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].value[0] = v1;
    bundle[wPtr].syllable[sNum].value[1] = v2;
    bundle[wPtr].syllable[sNum].vpr[0]   = pr1;
    bundle[wPtr].syllable[sNum].vpr[1]   = pr2;

    CheckConsistentIP(bundle[wPtr].vpc, ip, "predicate 2");
}

/**
 * Decoder for branch hint info. \n
 * format: \n
 * bh <ip> <dest> <hint> <tag> <instid>
 */
void
IA64_CIARDI_CLASS::bh_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 ip;
    UINT64 dest;
    int hint;
    UINT64 tag;
    int inst_id;
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s "FMT64X" "FMT64X" %x "FMT64X" %x",
                  dummy, &ip, &dest,
                  &hint, &tag, &inst_id);

    if(stat != 6) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
             << ">>\t" << fmt("08x", ip) << " BHINT"
             << " " << fmt("08x", dest)
             << " tag =" << fmt("08x", tag)
             << " hint =" << hint
             << "  [" << fmt_x(inst_id) << "]");

    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].hint = hint;
    bundle[wPtr].syllable[sNum].tag = tag;

    CheckConsistentIP(bundle[wPtr].vpc, ip, "branch hint");
}

/**
 * Decoder for set prediction info. \n
 * format: \n
 * sp <ip> <prediction_reg> <instid>
 */
void
IA64_CIARDI_CLASS::sp_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 ip;
    UINT64 pregs;
    int inst_id;
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s "FMT64X" "FMT64X" %x",
                  dummy, &ip, &pregs, &inst_id);

    if(stat != 4) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
             << ">>\t" << fmt("08x", ip) << " SET_PRED"
             << " pr[" << fmt("08x", pregs) << "]"
             << "  [" << fmt_x(inst_id) << "]");
    
    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].movpr = true;
    bundle[wPtr].syllable[sNum].pregs = pregs;

    CheckConsistentIP(bundle[wPtr].vpc, ip, "set prediction");
}


/**
 * Decoder for ?mv? info. \n
 * format: \n
 * mv <code?> <size?> ...?
 */
void
IA64_CIARDI_CLASS::mv_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    int code;
    UINT64 val1;
    UINT64 val2; 
    int inst_id;
    int size;
    char dummy[3];
    char remainder[BufferSize]; 
    int stat;

    stat = sscanf(buffer, "%2s %x %x %s", 
                  dummy, &code, &size, remainder);

    if(stat != 4) {
        ParseError(stat);
        return;
    }

    T1("TRACE line #" << lineNum << ": "
             << ">>\t???? MV"
             << " " << code2string(code)
             << " size = " << size
             << " val = " << remainder);
}


/**
 * Decoder for rfi (return from interrupt) info. \n
 * format: \n
 * rf <destip> <ip> <icount?> <cfm> \n
 */
void
IA64_CIARDI_CLASS::rf_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 destip;
    UINT64 ip;
    UINT64 cfm;
    int icount; 
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s "FMT64X" "FMT64X" %x "FMT64X,
                  dummy, &destip, &ip, &icount, &cfm);

    if(stat != 5) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
             << ">>\t RFI" 
             << " " << fmt("08x", destip)
             << " from " << fmt("08x", ip)
             << "  icount = " << icount << "(d)"
             << "  cfm = " << fmt("08x", cfm));

    IADDR_CLASS target = IADDR_CLASS(BundleAddr(destip), SyllableId(destip));

    traceIP = target;

    UINT32 sNum = SyllableId(ip);
    bundle[wPtr].syllable[sNum].target = target;
    bundle[wPtr].syllable[sNum].cfm = cfm;

    // for a return from interrupt we mark the remaining syllables in
    // the bundle as wrongpath;
    // note: if we return into the middle of a bundle, we will also mark
    // the initial syllables in the target as wrongpath once we read
    // the target "bb" bundle bits
    for (UINT32 i = sNum + 1; i < NumSyllables; i++) {
        bundle[wPtr].syllable[i].wrongpath = true;
    }

    CheckConsistentIP(bundle[wPtr].vpc, ip, "Return From Interrupt");
}


/**
 * Decoder for int (Interrupt) info. \n
 * format: \n
 * in <code?> <destip> <ip> <icount?> <psr>
 */
void
IA64_CIARDI_CLASS::in_action (void)
{
    // ignore all records until we process a BB
    if (wPtr == -1) return;

    UINT64 destip;
    UINT64 ip;
    UINT64 psr; 
    int code;
    int icount; 
    char dummy[3];
    int stat;

    stat = sscanf(buffer, "%2s %x "FMT64X" "FMT64X" %x "FMT64X,
                  dummy, &code, &destip, &ip, &icount, &psr);

    if(stat != 6) {
        ParseError(stat);
        return;
    }
    T1("TRACE line #" << lineNum << ": "
             << ">>\t INT" 
             << " " << code2string(code)
             << " " << fmt("08x", destip)
             << " from " << fmt("08x", ip)
             << "  icount = " << icount << "(d)"
             << "  psr = " << fmt("08x", psr));

    UINT32 sNum = SyllableId(ip);

    // find where to start searching backward for syllable to tag with interrupt
    if (bundle[wPtr].vpc != BundleAddr(ip)) {
        // BB record corresponding to IN record *was not* printed
        // start searching at end of current bundle
        CheckConsistentIP(traceIP.GetAddr(), ip, "Interrupt");
        sNum = 3;
    } else {
        // BB record corresponding to IN record *was* printed
        // mark rest of bundle wrongpath and start searching from that point
        for (UINT32 i = sNum; i < NumSyllables; i++) {
            bundle[wPtr].syllable[i].wrongpath = true;
        }
    }

    // search backward for syllable to tag with interrupt
    for (;;) {
        // search the current bundle
        while (sNum > 0) {
            sNum--;
            if (bundle[wPtr].syllable[sNum].wrongpath == false) {

                IADDR_CLASS target = IADDR_CLASS(BundleAddr(destip), SyllableId(destip));
                traceIP = target;
                bundle[wPtr].syllable[sNum].trap = true;
                bundle[wPtr].syllable[sNum].target = target;

                return;
            }
        }
        // roll back the bundle window and continue searching
        ASSERTX(wPtr > 0);
        wPtr--;
        sNum = 3;
    }
}


//Read on register from the record.. it should be of the 
//form
// <regtype><regnum> <regvalue>
ARCH_REGISTER_CLASS
IA64_CIARDI_CLASS::Read_Single_Register(INT32 &buf_pos)
{

  UINT64 value[2]= {0,0};
  ARCH_REGISTER_TYPE type = REG_TYPE_INVALID;;
  INT32 reg_number= 0;
  UINT32 reg_bits=0; //how many bits of register value were read

  //First get the type. The buf_position we were given should be the start of
  //the register information
  switch (buffer[buf_pos]) {
    //The register is a floating point type
    case 'f':
      type=REG_TYPE_FP82;
      break;
            
    //The register is a predicated register
    case 'p':
      type=REG_TYPE_PREDICATE;      
      break;
      
    case 'b':
      type=REG_TYPE_BR;      
      break;
      
    case 'r':
      type=REG_TYPE_INT;      
      break;
      
    default:
      cerr<<"Unknow register type in ir record the line is"<<buffer<<endl;
      cerr<<"Parse failed on "<<buffer[buf_pos]<<endl;
      break;
  }

  //Next get the register number this should be in decimal
  buf_pos++;

  while(buffer[buf_pos] != ' ')
  {
      ASSERTX((buffer[buf_pos] >= '0' && buffer[buf_pos]<= '9' ));
      reg_number= reg_number*10;
      reg_number= reg_number | INT32(buffer[buf_pos] - '0');
      buf_pos++;
  }

  //Could do a better check here by individual register type but for not
  //Just make sure the register number is less then 128
  ASSERTX(reg_number<128);

  //Finally get the register value
  buf_pos++;
  
  //make sure there is at least one digit
  ASSERTX((buffer[buf_pos] >= 'A' && buffer[buf_pos]<= 'F' )||
           (buffer[buf_pos] >= 'a' && buffer[buf_pos]<= 'f' )||
           (buffer[buf_pos] >= '0' && buffer[buf_pos]<= '9' ))
      
  while(buffer[buf_pos] != ' ' && //while we have not reached the end of the value
        buffer[buf_pos]!='\n' && //and not hit the end of the line
        buffer[buf_pos]!='\0' && //and it is not the end of the string
        buf_pos< BufferSize) //and not the end of the buffer
  {
      ASSERTX((buffer[buf_pos] >= 'A' && buffer[buf_pos]<= 'F' )||
              (buffer[buf_pos] >= 'a' && buffer[buf_pos]<= 'f' )||
              (buffer[buf_pos] >= '0' && buffer[buf_pos]<= '9' ))

      if(reg_bits>63)
      {
          value[1]=(value[1]<<4);
          value[1]=value[1] | value[0]>>60;
          value[0]=(value[0]<<4);
          value[0]=value[0] | HexDigit(buffer[buf_pos]);                
      }
      else
      {
          value[0]=(value[0]<<4);
          value[0]=value[0] | HexDigit(buffer[buf_pos]);
      }
      reg_bits+=4;
      buf_pos++;
  }

  return ARCH_REGISTER_CLASS(type,reg_number,value[0],value[1]);

}
/*****************************************************
 *
 *Get the input registers
 *
 *****************************************************/
void 
IA64_CIARDI_CLASS::ir_action(void)
{
    // ignore all records until we process a BB
    // or if we are not recording register values
    if (wPtr == -1 || IGNORE_REG_VALUES) 
    {
        return;
    }

    INT32 buf_pos=2; //it is ok to skip the first two characters
                      //since it would not have made it here if they
                      //were not valid
    ARCH_REGISTER_CLASS regs[NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS];

    UINT32 num_of_registers_read=0;
    //clear the array
    for(UINT32 i=0; i<(NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS); i++)
    {
        regs[i]=ARCH_REGISTER_CLASS();
    }
 
    while(buffer[buf_pos]!='\n' && //while we have not hit the end of the line
          buffer[buf_pos]!='\0' && //or the end of the string
          buf_pos< BufferSize && //or the end of the buffer
          num_of_registers_read<(NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS)) //and we have not found all the numbers we are looking for
    {
        buf_pos++;
        regs[num_of_registers_read]=Read_Single_Register(buf_pos);
        num_of_registers_read++;

    }

    UINT32 sNum =input_register_syllable;
    for(UINT32 i=0; i < (NUM_SRC_PRED_REGS+NUM_SRC_GP_REGS); i++)
    {
        bundle[wPtr].syllable[sNum].output_registers[i]=regs[i];
    }
    input_register_syllable++;
}
void 
IA64_CIARDI_CLASS::or_action(void)
{

    // ignore all records until we process a BB
    // or if we are not recording register values
    if (wPtr == -1 || IGNORE_REG_VALUES) 
    {
        return;
    }

    INT32 buf_pos=2; //it is ok to skip the first two characters
                       //since it would not have made it here if they
                       //were not valid
    ARCH_REGISTER_CLASS regs[NUM_DST_REGS];

    UINT32 num_of_registers_read=0;
    //clear the array
    for(UINT32 i=0; i<(NUM_DST_REGS); i++)
    {
        regs[i]=ARCH_REGISTER_CLASS();
    }
 
    while(buffer[buf_pos]!='\n' && //while we have not hit the end of the line
          buffer[buf_pos]!='\0' && //or the end of the string
          buf_pos< BufferSize && //or the end of the buffer
          num_of_registers_read<(NUM_DST_REGS)) //and we have not found all the numbers we are looking for
    {
        buf_pos++;
        regs[num_of_registers_read]=Read_Single_Register(buf_pos);
        num_of_registers_read++;

    }

     UINT32 sNum =output_register_syllable;
      for(UINT32 i=0; i <  NUM_DST_REGS; i++)
     {
         bundle[wPtr].syllable[sNum].output_registers[i]=regs[i];
     }
 
     output_register_syllable++;

}
void 
IA64_CIARDI_CLASS::bs_action(void)
{
    // ignore all records until we process a BB
    // or if we are not recording register values
    if (wPtr == -1 || IGNORE_REG_VALUES) 
    {
        return;
    }

     UINT64 bsp;
     char dummy[3];
 
     sscanf(buffer, "%2s "FMT64X"",dummy, &bsp);
     UINT32 sNum =output_register_syllable;
     bundle[wPtr].syllable[sNum].bsp=bsp;
}

/*
 * misc info (any unknown command is mapped to this)
 * just throws the line away;
 */
void
IA64_CIARDI_CLASS::xx_action (void)
{
}

//-----------------------------------------------------------------------------
// Support routines
//-----------------------------------------------------------------------------

/**
 * Check consistency of trace bundle IP and extra info bundle IP.
 */
void
IA64_CIARDI_CLASS::CheckConsistentIP (
    const UINT64 trIP,    ///< assumed IP of trace bundle
    const UINT64 otherIP, ///< IP encoded in additional bundle info
    const char* msg)      ///< additional bundle info description
{
    const UINT64 bundleTrIP = BundleAddr(trIP);
    const UINT64 bundleOtherIP = BundleAddr(otherIP);
    if (bundleTrIP != bundleOtherIP) {
        TRACE_WARNING("Inconsistent trace info: trace bundle IP = "
                      << fmt_x(bundleTrIP) << endl
                      << "\tbut additional bundle info ("
                      << msg << ") IP = " << fmt_x(bundleOtherIP) << endl);
    }
}

/**
 * Print error message.
 */
void
IA64_CIARDI_CLASS::BadCode (void)
{
    TRACE_WARNING("Can't decode opcode " << buffer[0] << buffer[1] << endl);
}

/**
 * Error parsing line.
 */
void
IA64_CIARDI_CLASS::ParseError (
    int stat)    ///< number of params that were parsed correctly
{
    TRACE_WARNING("ooops... couldn't parse [" << buffer << "]"
                  << " stat = " << stat << endl);
}

/**
 * Convert a hex-digit character to an integer.
 */
inline UINT32
IA64_CIARDI_CLASS::HexDigit (
    const char c) const ///< character to decode
{
    return char2int[c]; // return 0..15 or MAX_UINT32 for error
}

/**
 * Decode 16 hex digits (MSB first) into a UINT64.
 *
 * Note: this has been performance hacked quite a bit to be fast on
 *       a PentiumIII type machine compiled with g++ v3.0.1 -O4
 *
 * @return result of conversion (explicit),
 *         and error syndrome (implicit); the error syndrome is an
 *         integer indicating that ANY of the input characters was not
 *         of the appropriate type; not trying to pin down the exact
 *         one makes error checking a lot faster;
 */
UINT64
IA64_CIARDI_CLASS::ReadQuad (
    const char * const buf,   ///< input buffer with (at least) 16 hex digits
    UINT32 * const syn) const ///< error syndrome
{
    UINT64 accumulator;
    const char * p = buf;
    UINT32 tmp1;
    UINT32 tmp2;
    UINT32 tmp3;
    UINT32 tmp4;
    UINT32 syndrome = 0; ///< will collect ~0 on error

    accumulator = 0;
    for(int i = 0; i < 4; i++) {
        accumulator =
            (accumulator << 16)
          | (((syndrome |= (tmp1 = HexDigit(*p))),     tmp1) << 12)
          | (((syndrome |= (tmp2 = HexDigit(*(p+1)))), tmp2) << 8)
          | (((syndrome |= (tmp3 = HexDigit(*(p+2)))), tmp3) << 4)
          | (((syndrome |= (tmp4 = HexDigit(*(p+3)))), tmp4)); 
        p += 4;
    }
    //cout << buf << fmt_x(accumulator) << fmt_x(syndrome) << endl;

    *syn |= syndrome; // propagage error (~0)

    return accumulator;
}

/**
 * Decode 32 hex digits (MSB first) into two UINT64s.
 */
void
IA64_CIARDI_CLASS::Read2Quad (
    const char * const buf,  ///< input buffer with (at least) 32 hex digits
    UINT64 * const hi,       ///< output most significant UINT64
    UINT64 * const lo) const ///< output least significant UINT64
{
    UINT32 syndrome = 0; ///< will collect ~0 on error

    *hi = ReadQuad (buf, &syndrome);
    *lo = ReadQuad (buf + 16, &syndrome);

    ASSERT(((INT32) syndrome >= 0), "IA64Ciardi line #" << lineNum << ": could not read 32-digit hex value " << buf << endl);
}

/**
 * Reads the next line of the trace file
 *
 * @return false if no more lines available
 */
inline bool
IA64_CIARDI_CLASS::ReadNextLine (void)
{
    if (fgets(buffer, BufferSize, infile) != NULL) {
        lineNum++;
        return true; // read success
    } else {
        buffer[0] = '\0';
        return false; // read error
    }
}
