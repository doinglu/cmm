// cmm_vm.c

#include "std_port/std_port.h"
#include "cmm.h"
#include "cmm_vm.h"


namespace cmm
{

// Define a instruction
#define _INST(code, opern, memo) { Instruction::code, #code, &Simulator::x##code, opern, memo }

Simulator::InstructionInfo Simulator::m_instructionInfo[] =
{
    _INST(NOP,      0, "$$"),
    _INST(ADDI,     3, "$$ $1, $2, $3"),
    _INST(ADDR,     3, "$$ $1, $2, $3"),
    _INST(ADDX,     3, "$$ $1, $2, $3"),
    _INST(SUBI,     3, "$$ $1, $2, $3"),
    _INST(SUBR,     3, "$$ $1, $2, $3"),
    _INST(SUBX,     3, "$$ $1, $2, $3"),
    _INST(MULI,     3, "$$ $1, $2, $3"),
    _INST(MULR,     3, "$$ $1, $2, $3"),
    _INST(MULX,     3, "$$ $1, $2, $3"),
    _INST(DIVI,     3, "$$ $1, $2, $3"),
    _INST(DIVR,     3, "$$ $1, $2, $3"),
    _INST(DIVX,     3, "$$ $1, $2, $3"),
    _INST(EQI,      3, "$$ $1, $2, $3"),
    _INST(EQR,      3, "$$ $1, $2, $3"),
    _INST(EQX,      3, "$$ $1, $2, $3"),
    _INST(GTI,      3, "$$ $1, $2, $3"),
    _INST(GTR,      3, "$$ $1, $2, $3"),
    _INST(GTX,      3, "$$ $1, $2, $3"),
    _INST(LTI,      3, "$$ $1, $2, $3"),
    _INST(LTR,      3, "$$ $1, $2, $3"),
    _INST(LTX,      3, "$$ $1, $2, $3"),
    _INST(NEI,      3, "$$ $1, $2, $3"),
    _INST(NER,      3, "$$ $1, $2, $3"),
    _INST(NEX,      3, "$$ $1, $2, $3"),
    _INST(GEI,      3, "$$ $1, $2, $3"),
    _INST(GER,      3, "$$ $1, $2, $3"),
    _INST(GEX,      3, "$$ $1, $2, $3"),
    _INST(LEI,      3, "$$ $1, $2, $3"),
    _INST(LER,      3, "$$ $1, $2, $3"),
    _INST(LEX,      3, "$$ $1, $2, $3"),
    _INST(ANDI,     3, "$$ $1, $2, $3"),
    _INST(ANDX,     3, "$$ $1, $2, $3"),
    _INST(ORI,      3, "$$ $1, $2, $3"),
    _INST(ORX,      3, "$$ $1, $2, $3"),
    _INST(XORI,     3, "$$ $1, $2, $3"),
    _INST(XORX,     3, "$$ $1, $2, $3"),
    _INST(NOTI,     3, "$$ $1, $2, $3"),
    _INST(NOTX,     3, "$$ $1, $2, $3"),
    _INST(LSHI,     3, "$$ $1, $2, $3"),
    _INST(LSHX,     3, "$$ $1, $2, $3"),
    _INST(RSHI,     3, "$$ $1, $2, $3"),
    _INST(RSHX,     3, "$$ $1, $2, $3"),
    _INST(CAST,     3, "$$ $1, ($2.type)$3"),
    _INST(ISTYPE,   3, "$$ $1, $3 is $2.type"),
    _INST(LDN,      2, "$$ $1, nil"),
    _INST(LDI,      2, "$$ $1, $2"),
    _INST(LDR,      2, "$$ $1, $2"),
    _INST(LDX,      2, "$$ $1, $2"),
    _INST(RIDXMS,   3, "$$ $1, $2[$3]"),
    _INST(RIDXXX,   3, "$$ $1, $2[$3]"),
    _INST(LIDXMS,   3, "$$ $1[$3], $2"),
    _INST(LIDXXX,   3, "$$ $1[$3], $2"),
    _INST(CALLNEAR, 2, "$$ $1($2...)"),
    _INST(CALLFAR,  3, "$$ $3.$1($2...)"),
    _INST(CALLNAME, 2, "$$ $1($2...)"),
    _INST(CALLOTHER,3, "$$ $1($2...)"),
    _INST(CHKPARAM, 0, "$$"),
    _INST(RET,      1, "$$ $1"),
    _INST(LOOPIN,   3, "$$ ($1=$2 to $3)"),
    _INST(LOOPRANGE,3, "$$ ($1 in $2)"),
    _INST(LOOPEND,  0, "LOOP END"),
    { (Instruction::Code)0, 0, 0,  } // 0 Mark end
};

// Map Code to index
Uint8 Simulator::m_code_map[256];

int Simulator::init()
{
    // Build code map: code->index
    for (Uint8 i = 0; m_instructionInfo[i].name != 0; i++)
        m_code_map[m_instructionInfo[i].code] = i;
    return 0;
}

void Simulator::shutdown()
{
}

void Simulator::xNOP() {}
void Simulator::xADDI() {}
void Simulator::xADDR() {}
void Simulator::xADDX() {}
void Simulator::xSUBI() {}
void Simulator::xSUBR() {}
void Simulator::xSUBX() {}
void Simulator::xMULI() {}
void Simulator::xMULR() {}
void Simulator::xMULX() {}
void Simulator::xDIVI() {}
void Simulator::xDIVR() {}
void Simulator::xDIVX() {}
void Simulator::xEQI() {}
void Simulator::xEQR() {}
void Simulator::xEQX() {}
void Simulator::xGTI() {}
void Simulator::xGTR() {}
void Simulator::xGTX() {}
void Simulator::xLTI() {}
void Simulator::xLTR() {}
void Simulator::xLTX() {}
void Simulator::xNEI() {}
void Simulator::xNER() {}
void Simulator::xNEX() {}
void Simulator::xGEI() {}
void Simulator::xGER() {}
void Simulator::xGEX() {}
void Simulator::xLEI() {}
void Simulator::xLER() {}
void Simulator::xLEX() {}
void Simulator::xANDI() {}
void Simulator::xANDX() {}
void Simulator::xORI() {}
void Simulator::xORX() {}
void Simulator::xXORI() {}
void Simulator::xXORX() {}
void Simulator::xNOTI() {}
void Simulator::xNOTX() {}
void Simulator::xLSHI() {}
void Simulator::xLSHX() {}
void Simulator::xRSHI() {}
void Simulator::xRSHX() {}
void Simulator::xCAST() {}
void Simulator::xISTYPE() {}
void Simulator::xLDN() {}
void Simulator::xLDI() {}
void Simulator::xLDR() {}
void Simulator::xLDX() {}
void Simulator::xRIDXMS() {}
void Simulator::xRIDXXX() {}
void Simulator::xLIDXMS() {}
void Simulator::xLIDXXX() {}
void Simulator::xCALLNEAR() {}
void Simulator::xCALLFAR() {}
void Simulator::xCALLNAME() {}
void Simulator::xCALLOTHER() {}
void Simulator::xCHKPARAM() {}
void Simulator::xRET() {}
void Simulator::xLOOPIN() {}
void Simulator::xLOOPRANGE() {}
void Simulator::xLOOPEND() {}

}
