// cmm_vm.h

#pragma once

#include "cmm.h"
#include "cmm_value.h"

namespace cmm
{

class Function;
class Program;
class Thread;

// Single instruction structure
struct Instruction
{
    enum ParaType
    {
        CONSTANT = 0,   // In this program
        ARGUMENT = 1,   // Arguments in this context
        LOCAL = 2,      // Local variables in this context
        MEMBER = 3,     // Members in this object
    };

    typedef Uint16 ParaValue;

    enum Code
    {
        NOP       = 0,   // N/A
        ADDI      = 1,   // p1<-p2, p3
        ADDR      = 2,   // p1<-p2, p3
        ADDX      = 3,   // p1<-p2, p3
        SUBI      = 4,   // p1<-p2, p3
        SUBR      = 5,   // p1<-p2, p3
        SUBX      = 6,   // p1<-p2, p3
        MULI      = 7,   // p1<-p2, p3
        MULR      = 8,   // p1<-p2, p3
        MULX      = 9,   // p1<-p2, p3
        DIVI      = 10,  // p1<-p2, p3
        DIVR      = 11,  // p1<-p2, p3
        DIVX      = 12,  // p1<-p2, p3
        MODI      = 13,  // p1<-p2, p3
        MODR      = 14,  // p1<-p2, p3
        MODX      = 15,  // p1<-p2, p3
        EQI       = 16,  // p1<-p2, p3
        EQR       = 17,  // p1<-p2, p3
        EQX       = 18,  // p1<-p2, p3
        NEI       = 19,  // p1<-p2, p3
        NER       = 20,  // p1<-p2, p3
        NEX       = 21,  // p1<-p2, p3
        GTI       = 22,  // p1<-p2, p3
        GTR       = 23,  // p1<-p2, p3
        GTX       = 24,  // p1<-p2, p3
        LTI       = 25,  // p1<-p2, p3
        LTR       = 26,  // p1<-p2, p3
        LTX       = 27,  // p1<-p2, p3
        GEI       = 28,  // p1<-p2, p3
        GER       = 29,  // p1<-p2, p3
        GEX       = 30,  // p1<-p2, p3
        LEI       = 31,  // p1<-p2, p3
        LER       = 32,  // p1<-p2, p3
        LEX       = 33,  // p1<-p2, p3
        ANDI      = 34,  // p1<-p2, p3
        ANDX      = 36,  // p1<-p2, p3
        ORI       = 37,  // p1<-p2, p3
        ORX       = 39,  // p1<-p2, p3
        XORI      = 41,  // p1<-p2, p3
        XORX      = 42,  // p1<-p2, p3
        REVI      = 43,  // p1<-p2, p3
        REVX      = 45,  // p1<-p2, p3
        NEGI      = 49,  // p1<-p2, p3
        NEGX      = 51,  // p1<-p2, p3
        LSHI      = 55,  // p1<-p2, p3
        LSHX      = 57,  // p1<-p2, p3
        RSHI      = 58,  // p1<-p2, p3
        RSHX      = 60,  // p1<-p2, p3
        CAST      = 63,  // p1<-(p2)p3
        ISTYPE    = 66,  // p1<-p3 is (p2)?
        LDMULX    = 69,  // p1...<-p2... x p3
        LDI       = 70,  // p1<-p2p3 (combine to 32bits)
        LDR       = 71,  // p1<-p2.p3 (p3 /= 10000)
        LDX       = 72,  // p1<-p2
        LDARGN    = 73,  // p1<-argn
        RIDXXX    = 75,  // p1<-p2[p3]
        LIDXXX    = 78,  // p1[p3]<-p2
        MKEARR    = 81,  // p1<-[], capacity=p2
        MKIARR    = 84,  // p1<-p2... x p3
        MKEMAP    = 87,  // p1<-{}, capacity=p2
        MKIMAP    = 90,  // p1<-p2... x p3
        JMP       = 102, // Jmp p1
        JCOND     = 105, // Jmp p1 when p2 != 0
        CALLNEAR  = 108, // p1<-call(p2 = function_no, p3 = argn, args)
        CALLFAR   = 111, // p1<-call(p2 = constant:component_no,function_no, p3 = argn, args)
        CALLNAME  = 114, // p1<-call(p2 = constant:name, p3 = argn, args)
        CALLOTHER = 117, // p1<-call(p2 = oid,name p3 = argn, args)
        CALLEFUN  = 120, // p1<-call(p2 = name, p3 = argn, args)
        CHKPARAM  = 123, // chkparam
        RET       = 126, // ret p1
        LOOPIN    = 129, // loop(p1=p2 to p3)
        LOOPRANGE = 132, // loop(p1 in p2)
        LOOPEND   = 135, // loop end
    };

    // Condition for jump
    enum Cond
    {
        EQ = 0,
        NE = 1,
        GT = 2,
        LT = 3,
        GE = 4,
        LE = 5,
    };

    Code      code;
    ParaType  t1;
    ParaType  t2;
    ParaType  t3;
    ParaValue p1;
    ParaValue p2;
    ParaValue p3;

    // Valid range for p1-p3
    enum
    {
        PARA_UMAX = 65535,
        PARA_SMIN = -32768,
        PARA_SMAX = 32767,
    };
};

// Component class for interpreter
class InterpreterComponent : public AbstractComponent
{
public:
    Value interpreter(Thread *_thread, Value *__args, ArgNo __n);
};

// The simulator
class Simulator
{
public:
    typedef void (Simulator::*Entry)();
    struct InstructionInfo
    {
        Instruction::Code code; // Code of instruction
        const char *name;  // name
        Entry entry;    // C++ function entry
        size_t opern;   // count of operand
        const char *memo;  // Memo for output
    };

public:
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

private:
    inline Value *get_parameter_value(int index);
    inline int get_parameter_imm(int index);

private:
    Value *m_args;          // All arguments
    Value *m_locals;        // All local variables & registers
    Value *m_constants;     // All constants in program
    Value *m_object_vars;   // All object vars in this object
    ArgNo  m_argn;
    LocalNo m_localn;
    ConstantIndex m_constantn;
    VariableNo    m_object_varn;

private:
    Thread   *m_thread;
    Domain   *m_domain;
    AbstractComponent *m_component;
    const Program *m_program;
    const Function *m_function;
    const Instruction *m_byte_codes;
    const Instruction *m_ip;
    const Instruction *m_this_code;

public:
    static Value interpreter(AbstractComponent *_component, Thread *_thread, Value *__args, ArgNo __n);
    static Integer make_function_constant(ComponentNo component_no, FunctionNo function_no);

public:
    Value run();

private:
    // All instructions
    static InstructionInfo m_instruction_info[];

    // Map code -> instruction array offset
    static Uint8 m_code_map[256];

private:
    // Instruction simulation entries
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
    void xMODI();
    void xMODR();
    void xMODX();
    void xEQI();
    void xEQR();
    void xEQX();
    void xNEI();
    void xNER();
    void xNEX();
    void xGTI();
    void xGTR();
    void xGTX();
    void xLTI();
    void xLTR();
    void xLTX();
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
    void xREVI();
    void xREVX();
    void xNEGI();
    void xNEGX();
    void xLSHI();
    void xLSHX();
    void xRSHI();
    void xRSHX();
    void xCAST();
    void xISTYPE();
    void xLDMULX();
    void xLDI();
    void xLDR();
    void xLDX();
    void xLDARGN();
    void xRIDXXX();
    void xLIDXXX();
    void xMKEARR();
    void xMKIARR();
    void xMKEMAP();
    void xMKIMAP();
    void xJMP();
    void xJCOND();
    void xCALLNEAR();
    void xCALLFAR();
    void xCALLNAME();
    void xCALLOTHER();
    void xCALLEFUN();
    void xCHKPARAM();
    void xRET();
    void xLOOPIN();
    void xLOOPRANGE();
    void xLOOPEND();
};

}
