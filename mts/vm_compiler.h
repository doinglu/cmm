// vm_compiler.h
// To support C--'s compiler
// Initial version 2011.6.3 by ershu


#ifndef __VM_COMPILER_H__
#define __VM_COMPILER_H__

/* Compile context */
typedef struct CompileContext {
    char       *source;
    IntR        len;
    IntR        pos;
    IntR        curline;
    IntR        totalErrors;
    IntR        totalWarnings;
    char        fileName[256];
    struct Vm_Fun_T   *pInFunction;
    struct Vm_Program *pProgramHead;
    struct Vm_Program *pProgramNow;
} CompileContext_T;

/* Registration */
extern IntR      vm_initLpcCompiler();
extern void      vm_shutdownLpcCompiler();

/* Configurations */
extern IntR      vm_getOptimizationLevel();
extern void      vm_setOptimizationLevel(IntR level);

/* Interfaces */
extern Vm_Status            vm_compile(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram);
extern Vm_Status            vm_compileAsGlobal(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction);
extern void                 vm_compilerEnableDebug(IntR enable);

#endif /* end of __VM_COMPILER_H__ */
