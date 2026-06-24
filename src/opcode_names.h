/*
 * Gearcoleco - ColecoVision Emulator
 * Copyright (C) 2021  Ignacio Sanchez

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/ 
 * 
 */

#ifndef OPCODE_NAMES_H
#define	OPCODE_NAMES_H

enum GC_OPCode_Type
{
    GC_OPCode_Type_Implied = 0,
    GC_OPCode_Type_Index,
    GC_OPCode_Type_1b,
    GC_OPCode_Type_2b,
    GC_OPCode_Type_Indexed,
    GC_OPCode_Type_Relative,
    GC_OPCode_Type_Indexed_1b,
    GC_OPCode_Type_Data
};

struct stOPCodeInfo
{
    const char* name[GC_Disassembler_Syntax_Count];
    int size;
    int type;
};

#define GC_OPCODE(name, size, type) { { name, name, name, name }, size, type }
#define GC_OPCODE_SYNTAX(gearcoleco, wladx, tniasm, z88dk, size, type) { { gearcoleco, wladx, tniasm, z88dk }, size, type }

#include "opcodexx_names.h"
#include "opcodecb_names.h"
#include "opcodeed_names.h"
#include "opcodedd_names.h"
#include "opcodefd_names.h"
#include "opcodeddcb_names.h"
#include "opcodefdcb_names.h"

#endif	/* OPCODE_NAMES_H */

