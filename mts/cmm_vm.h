// cmm_vm.h

#pragma once

#include "cmm_basic_types.h"

namespace cmm
{

// Single instruction structure
struct Instruction
{
    enum ParaType
    {
        LOCAL = 0,
        ARGUMENT = 1,
        OBJECT_MEMBER = 2,
        GLOBAL_MEMBER = 3,
    };

    enum Code
    {
        NOP  = 0,       // N/A
        ADDI = 1,       // p1<-p2, p3
        ADDR = 2,       // p1<-p2, p3
        ADDX = 3,       // p1<-p2, p3
        SUBI = 4,       // p1<-p2, p3
        SUBR = 5,       // p1<-p2, p3
        SUBX = 6,       // p1<-p2, p3
        MULI = 7,       // p1<-p2, p3
        MULR = 8,       // p1<-p2, p3
        MULX = 9,       // p1<-p2, p3
        DIVI = 10,      // p1<-p2, p3
        DIVR = 11,      // p1<-p2, p3
        DIVX = 12,      // p1<-p2, p3
        EQI  = 13,      // p1<-p2, p3
        EQR  = 14,      // p1<-p2, p3
        EQX  = 15,      // p1<-p2, p3
        GTI  = 16,      // p1<-p2, p3
        GTR  = 17,      // p1<-p2, p3
        GTX  = 18,      // p1<-p2, p3
        LTI  = 19,      // p1<-p2, p3
        LTR  = 20,      // p1<-p2, p3
        LTX  = 21,      // p1<-p2, p3
        NEI  = 22,      // p1<-p2, p3
        NER  = 23,      // p1<-p2, p3
        NEX  = 24,      // p1<-p2, p3
        GEI  = 25,      // p1<-p2, p3
        GER  = 26,      // p1<-p2, p3
        GEX  = 27,      // p1<-p2, p3
        LEI  = 28,      // p1<-p2, p3
        LER  = 29,      // p1<-p2, p3
        LEX  = 30,      // p1<-p2, p3
        ANDI = 31,      // p1<-p2, p3
        ANDX = 33,      // p1<-p2, p3
        ORI  = 34,      // p1<-p2, p3
        ORX  = 36,      // p1<-p2, p3
        XORI = 37,      // p1<-p2, p3
        XORX = 39,      // p1<-p2, p3
        NOTI = 40,      // p1<-p2, p3
        NOTX = 42,      // p1<-p2, p3
        LSHI = 43,      // p1<-p2, p3
        LSHX = 45,      // p1<-p2, p3
        RSHI = 46,      // p1<-p2, p3
        RSHX = 48,      // p1<-p2, p3
        CAST = 51,      // p1<-(p2)p3
        ISTYPE = 54,    // p1<-(p2)p3?
        LDN    = 57,    // p1<-p2
        LDI    = 58,    // p1<-p2
        LDR    = 59,    // p1<-p2
        LDX    = 60,    // p1<-p2
        RIDXMS = 61,    // p1<-p2[p3]
        RIDXXX = 63,    // p1<-p2[p3]
        LIDXMS = 64,    // p1[p3]<-p2
        LIDXXX = 66,    // p1[p3]<-p2
        CALLNEAR  = 69, // call(fun=p1, args=p2)
        CALLFAR   = 72, // call(fun=p1, args=p2, component=p3)
        CALLNAME  = 75, // call(fun_name=p1, args=p2)
        CALLOTHER = 78, // call(fun_name=p1, args=p2, oid=p3)
        CHKPARAM  = 81, // chkparam
        RET       = 84, // ret p1
        LOOPIN    = 84, // loop(p1=p2 to p3)
        LOOPRANGE = 87, // loop(p1 in p2)
        LOOPEND   = 90, // loop end
    };

    Code  code;
    ParaType t1 : 2;    
    ParaType t2 : 2;
    ParaType t3 : 2;
    Uint16 p1;
    Uint16 p2;
    Uint16 p3;
};

// The simulator
class Simulator
{
public:
    typedef void (Simulator::*Entry)();
    struct InstructionInfo
    {
        Instruction::Code code; // Code of instruction
        const char *name;       // name
        Entry entry;            // C++ function entry
        size_t opern;           // count of operand
        const char *memo;       // Memo for output
    };

public:
    static int init();
    static void shutdown();

public:
    void xNOP();
    void xADDI();
    void xADDR();
    void xADDX();
    void xSUBI();
    void xSUBR();
    void xSUBX();
    void xMULI();
    void xMULR();
    void xMULX();
    void xDIVI();
    void xDIVR();
    void xDIVX();
    void xEQI();
    void xEQR();
    void xEQX();
    void xGTI();
    void xGTR();
    void xGTX();
    void xLTI();
    void xLTR();
    void xLTX();
    void xNEI();
    void xNER();
    void xNEX();
    void xGEI();
    void xGER();
    void xGEX();
    void xLEI();
    void xLER();
    void xLEX();
    void xANDI();
    void xANDX();
    void xORI();
    void xORX();
    void xXORI();
    void xXORX();
    void xNOTI();
    void xNOTX();
    void xLSHI();
    void xLSHX();
    void xRSHI();
    void xRSHX();
    void xCAST();
    void xISTYPE();
    void xLDN();
    void xLDI();
    void xLDR();
    void xLDX();
    void xRIDXMS();
    void xRIDXXX();
    void xLIDXMS();
    void xLIDXXX();
    void xCALLNEAR();
    void xCALLFAR();
    void xCALLNAME();
    void xCALLOTHER();
    void xCHKPARAM();
    void xRET();
    void xLOOPIN();
    void xLOOPRANGE();
    void xLOOPEND();

public:
    // All instructions
    static InstructionInfo m_instructionInfo[];

    // Map code -> instruction array offset
    static Uint8 m_code_map[256];
};

}
