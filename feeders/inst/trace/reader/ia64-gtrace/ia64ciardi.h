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
 * @author Artur Klauser
 * @brief Ciardi IA64 Trace Reader for GTrace format (Gambit)
 */

#ifndef _IA64_CIARDI_H
#define _IA64_CIARDI_H

// generic
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

// ASIM core
#include "asim/syntax.h"

// ASIM public modules
#include "asim/provides/isa.h"

// ASIM local module
#include "ia64ciardibundle.h"

typedef class IA64_CIARDI_CLASS *IA64_CIARDI;
class IA64_CIARDI_CLASS : public TRACEABLE_CLASS
{
  private:
    // constants
    static const int BundleSize = 0x10;
    static const UINT64 BundleAddrMask = ~(BundleSize - 1);
    static const UINT32 NumSyllables = 3;
    static const UINT64 SyllableMask = 0x3;

    static const int BufferSize = 1024;
    static const INT32 WSize = 2;

    // nested types
    typedef void (IA64_CIARDI_CLASS::*aproc)(void);

    /// decoder action records
    static struct action_rec {
        char * cmdstr;
        aproc fn;
    } action_vec[];

    // variables
    const UINT32 traceID;            ///< ID of this trace stream
    FILE *infile;                    ///< input file handle
    UINT64 lineNum;                  ///< input line number
    bool used;                       ///< reader has been used
    bool compressed;                 ///< input file is compressed
    char buffer[BufferSize];
    bool bundleLineInBuffer;
    INT32 wPtr;
    IADDR_CLASS traceIP;             ///< IP of current position in the trace
    IA64_CIARDI_BUNDLE_CLASS bundle[WSize]; ///< current bundle information
    INT8 char2int[256];              ///< HexDigit translations
    UINT32 input_register_syllable; ///< counter for ir records
    UINT32 output_register_syllable; ///< counter for the or records

    // support methods
    UINT64 BundleAddr (UINT64 addr) { return addr & BundleAddrMask; }
    UINT8 SyllableId (UINT64 addr) { return addr & SyllableMask; }
    UINT64 MakeAddr (UINT64 bundleAddr, UINT8 syllableId)
    {
        return bundleAddr | syllableId;
    }
    void CheckConsistentIP (const UINT64 trIP, const UINT64 otherIP,
        const char* msg);
    void ParseError (int stat);
    void BadCode(void);
    UINT32 HexDigit (const char c) const;
    UINT64 ReadQuad(const char * const buf, UINT32 * const syn) const;
    void Read2Quad(const char * const buf, UINT64 * const hi,
        UINT64 * const lo) const;
    bool ReadNextLine (void);

    ARCH_REGISTER_CLASS Read_Single_Register(INT32 &buf_pos);

    // line parsing methods
    void bb_action(void);
    void br_action(void);
    bool br_action_init_traceIP(void);
    void da_action(void);
    void c1_action(void);
    void c2_action(void);
    void bh_action(void);
    void sp_action(void);
    void mv_action(void);
    void rf_action(void);
    void in_action(void);
    void ir_action(void);
    void or_action(void);
    void bs_action(void);
    void xx_action(void);

  public:
    // constructors / destructors / initializers
    IA64_CIARDI_CLASS (UINT32 id = 0);
    ~IA64_CIARDI_CLASS();

    // interface
    bool FileOpen (const char * fileName);
    void FileClose (void);
    IA64_CIARDI_BUNDLE GetNextBundle (void);
#if 0 /* CIARDI_DUMP */
    void HackSPRecord (UINT64 vpregs);
    void HackC1Record ();
#endif
};

#endif // _IA64_CIARDI_H
