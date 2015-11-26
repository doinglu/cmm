/******************************************************************
 *
 * @(#)vm_compiler.c:
 *
 * Purpose: 
 *  To support LPC high-language's compiler.
 *
 * Functions:
 *
 * History:
 *  2011.6.3        Initial version
 *                  ershu
 *
 ***** Copyright 2011, ershu reserved *****************************/

#include "vm_stdafx.h"

#include "vm_binary.h"
#include "vm_file_path.h"
#include "vm_globalbuf.h"
#include "vm_io.h"
#include "vm_jit_compiler.h"
#include "vm_program.h"
#include "vm_scratchpad.h"
#include "vm_shell.h"
#include "vm_type_value.h"
#include "vm_util.h"
#include "assembler/asm_assembler.h"
#include "compiler/vm_compiler.h"
#include "compiler/vm_lex.h"
#include "compiler/vm_macro.h"
#include "compiler/vm_optimize.h"
#include "compiler/vm_syntax.h"

/* Configuration level */
static IntR vm_confOptimizationLevel = 2;

/* For debug only, assume max .asm file size <= 512k */
static char __asmStr[512 * 1024];

/* Enable debug */
static IntR vm_EnableDebug = 0;

/* Internal routines */
static Vm_Status _compile_C(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram);
static Vm_Status _compile_ASM(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram);
static Vm_Status _compileAsGlobal_C(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction);
static Vm_Status _compileAsGlobal_ASM(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction);
static Boolean   _endWithStr(const char *str, const char *p);
static void      _printFormatSource(char *source);

/* Registration */
extern IntR vm_initLpcCompiler()
{
    /* Init identifiers */
    vm_initIdentifiers();
    return 1;
}

/* Registration */
extern void vm_shutdownLpcCompiler()
{
    /* Destruct identifiers */
    vm_destructIdentifiers();
}

extern IntR vm_getOptimizationLevel()
{
    return vm_confOptimizationLevel;
}

extern void vm_setOptimizationLevel(IntR level)
{
    vm_confOptimizationLevel = level;
}

/* Compile specified lpc source file */
extern Vm_Status vm_compile(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram)
{
    /* .asm for debug only */
    if (_endWithStr(pInFileName, ".asm"))
        return _compile_ASM(fileFd, pInFileName, pProgram);
    
    /* Normal lpc source file */
    return _compile_C(fileFd, pInFileName, pProgram);
}

/* Compile specified lpc source file as global object */
extern Vm_Status vm_compileAsGlobal(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction)
{
    /* .asm for debug only */
    if (_endWithStr(pInFileName, ".asm"))
        return _compileAsGlobal_ASM(fileFd, pInFileName, pEntryFunction);

    /* Normal lpc source file */
    return _compileAsGlobal_C(fileFd, pInFileName, pEntryFunction);
}

/* Set compiler debug enable */
extern void vm_compilerEnableDebug(IntR enable)
{
    vm_EnableDebug = enable;
}

/* LPC C version */
static Vm_Status _compile_C(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram)
{
    Vm_Lpc_Context_T *context;
    char *buffer = NULL;
    IntR hasError = 1;
    Vm_Status res = VM_ASSEMBLE_FAILED;
    jmp_buf jmpBuf;
    IntR startRes;

    Vm_Scratchpad_Info_T *scratchPad;
    vm_buildScratch(&scratchPad);

    /* Create compile context */
    context = syntax_create_lang_context();
    context->currentFile = vm_scratchCopyCStr(pInFileName);
    context->jumpBuf = &jmpBuf;
    context->isGlobal = 0;

    /* Start new file in lex */
    startRes = vm_startNewFile(context, pProgram, fileFd);
    
    if (startRes == VM_START_ERROR)
    {
        vm_errprintf("Start new file error.\n");

        res = VM_UNKNOWN_ERROR;

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);
    }
    else if (startRes == VM_START_BINARY)
    {
        vm_endNewFile(context, ! hasError, 1, NULL);

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);

        if (! vm_loadBinary(fileFd, pProgram))
        {
            vm_errprintf("Load binary file \"%s\" failed.\n", pInFileName);
            hasError = 1;
            res = VM_UNKNOWN_ERROR;
        }
        else
        {
            /* Update symbol */
            vm_updateSymbolInfos(pProgram);

            /* Rebuild quick function */
            vm_rebuildProgramQuickFunction(pProgram);

            hasError = 0;
            res = VM_OK;
        }
    }
    else if (startRes == VM_START_SOURCE)
    {
        if (setjmp(jmpBuf) == 0)
        {
            /* Begin to parse */
            vm_parseLpc(context);
            hasError = (context->numErrors != 0);
        }
        else
        {
            hasError = 1;
        }

        /* End file */
        vm_endNewFile(context, ! hasError, 1, NULL);

        /* Copy source */
        if (! hasError)
        {
            if (vm_getOptimizationLevel() >= 2)
                /* optimize asm source */
                buffer = vm_optimizeAsm(context->source);
            else
            {
                buffer = VM_MEM_ALLOC(STRLEN(context->source) + 1);
                STRCPY(buffer, context->source);
            }

            if (vm_EnableDebug)
            {
                vm_printf("=========== %s ===========\n", context->currentInputFile);
                _printFormatSource(buffer);
                vm_printf("==================================\n");
            }
        }

        /* Destroy context */
        syntax_destroy_context(context);

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);

        /* Assemble if have no error */
        if (! hasError && buffer)
        {
            AsmContext_T *asmContext;
            
            /* Create scratchpad for assembling process */
            vm_buildScratch(&scratchPad);

            /* Create assemble context */
            asmContext = asm_createAssembleContext(pProgram, buffer);
            VM_ASSERT(asmContext != NULL);

            /* Assemble result of compiler */
            if (! asm_assembleString(pProgram, asmContext, buffer, NULL, pInFileName))
                res = VM_ASSEMBLE_FAILED;
            else
                res = VM_OK;

            /* Destroy asm context */
            asm_destroyAssembleContext(asmContext);
            vm_destructScratch(scratchPad);
        }

        /* Free source buffer */
        if (buffer)
            VM_MEM_FREE(buffer);
    }

    /* Return result */
    return res;
}

/* LPC ASM version */
static Vm_Status _compile_ASM(IntR fileFd, const char *pInFileName, struct Vm_Program *pProgram)
{
    AsmContext_T *asmContext;
    Vm_Scratchpad_Info_T *scratchPad;
    IntR res;

    /* Read whole asm file content */
    IntR len = vm_read(__asmStr, sizeof(__asmStr), fileFd);
    __asmStr[len] = 0;

    /* Create scratchpad for assembling process */
    vm_buildScratch(&scratchPad);

    /* Create assemble context */
    asmContext = asm_createAssembleContext(pProgram, __asmStr);
    VM_ASSERT(asmContext != NULL);

    /* Assemble result of compiler */
    if (! asm_assembleString(pProgram, asmContext, __asmStr, NULL, NULL))
        res = VM_ASSEMBLE_FAILED;
    else 
        res = VM_OK;

    /* Destroy assemble context */
    asm_destroyAssembleContext(asmContext);

    /* Destroy scratchpad */
    vm_destructScratch(scratchPad);

    return res;
}

/* LPC C version */
static Vm_Status _compileAsGlobal_C(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction)
{
    Vm_Lpc_Context_T *context;
    IntR oldVarNum;
    IntR growNum;
    IntR hasError = 1, i = 0;
    char *buffer = NULL;
    Vm_Status res = VM_ASSEMBLE_FAILED;
    jmp_buf jmpBuf;
    IntR startRes;

    Vm_Scratchpad_Info_T *scratchPad;
    vm_buildScratch(&scratchPad);

    /* Create compile context */
    context = syntax_create_lang_context();
    context->isGlobal = 1;
    context->jumpBuf = &jmpBuf;
    context->currentFile = pInFileName ? vm_scratchCopyCStr(pInFileName) : NULL;

    /* Start new file in lex */
    startRes = vm_startNewFile(context, vm_pGlobalProgram, fileFd);

    if (startRes == VM_START_ERROR)
    {
        vm_error("Start new file error.\n");
        vm_endNewFile(context, ! hasError, 0, NULL);

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);

        res = VM_UNKNOWN_ERROR;
    }
    else if (startRes == VM_START_BINARY)
    {
        Vm_Program_T *tempProgram;

        oldVarNum = vm_pGlobalProgram->memberNum;
        
        /* End new file */
        vm_endNewFile(context, ! hasError, 0, NULL);

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);

        tempProgram = vm_buildProgramHead("@temp_global");
        if (! vm_loadBinary(fileFd, tempProgram))
        {
            vm_error("Load binary file \"%s\" failed.\n", pInFileName);
            hasError = 1;
            res = VM_UNKNOWN_ERROR;
        }
        else
        {
            hasError = 0;

            /* Merge with global program */
            vm_mergeTwoProgram(vm_pGlobalProgram, tempProgram);

            /* Update symbol */
            vm_updateSymbolInfos(vm_pGlobalProgram);
            
            /* Rebuild quick function */
            vm_rebuildProgramQuickFunction(vm_pGlobalProgram);

            /* There are some new global variables, extends global object's members */
            if ((growNum = (vm_pGlobalProgram->memberNum - oldVarNum)) > 0)
            {
                vm_pGlobalVar = VM_MEM_REALLOC(vm_pGlobalVar, sizeof(Vm_Global_Var_T) * vm_pGlobalProgram->memberNum);
                for (i = 0; i < growNum; i++)
                    vm_pGlobalVar[vm_pGlobalProgram->memberNum - 1 - i].pValue = vm_newValue();

                /* If running in JIT mode, we should update base pointers when pObjectValues/pConstants changed */
                vm_updateBasePointerForJit();
            }

            /* Find last entry function */
            vm_findProgramFunctionByCStr(vm_pGlobalProgram, "@entry", 1, pEntryFunction);
            res = VM_OK;
        }

        /* No need to free bytecodes. In scratchpad or finalized and be merged. */
        vm_destructProgramWithByteCodes(tempProgram, False);
    }
    else if (startRes == VM_START_SOURCE)
    {

        if (setjmp(jmpBuf) == 0)
        {
            /* Parse lpc source */
            vm_parseLpc(context);
            hasError = (context->numErrors != 0);
        }
        else
        {
            hasError = 1;
        }

        vm_endNewFile(context, ! hasError, 0, NULL);

        if (! hasError && context->source && STRLEN(context->source) > 0)
        {
            if (vm_getOptimizationLevel() >= 2)
                /* optimize asm source */
                buffer = vm_optimizeAsm(context->source);
            else
            {
                buffer = VM_MEM_ALLOC(STRLEN(context->source) + 1);
                STRCPY(buffer, context->source);
            }

            if (vm_EnableDebug)
            {
                vm_printf("=========== %s ===========\n", context->currentInputFile);
                _printFormatSource(buffer);
                vm_printf("==================================\n");
            }
        }

        /* Destroy context */
        syntax_destroy_context(context);

        /* Destruct scratch pad */
        vm_destructScratch(scratchPad);

        /* Assemble if have no error */
        if (! hasError && buffer)
        {
            AsmContext_T *asmContext;

            /* Create scratchpad for assembling process */
            vm_buildScratch(&scratchPad);

            /* Create assemble context */
            asmContext = asm_createAssembleContext(vm_pGlobalProgram, buffer);
            VM_ASSERT(asmContext != NULL);

            oldVarNum = vm_pGlobalProgram->memberNum;

            /* Assemble result of compiler */
            if (! asm_assembleString(vm_pGlobalProgram, asmContext, buffer, pEntryFunction, pInFileName))
            {
                res = VM_ASSEMBLE_FAILED;
            }
            else
            {            
                /* There are some new global variables, extends global object's members */
                if ((growNum = (vm_pGlobalProgram->memberNum - oldVarNum)) > 0)
                {
                    vm_pGlobalVar = VM_MEM_REALLOC(vm_pGlobalVar, sizeof(Vm_Global_Var_T) * vm_pGlobalProgram->memberNum);
                    for (i = 0; i < growNum; i++)
                        vm_pGlobalVar[vm_pGlobalProgram->memberNum - 1 - i].pValue = vm_newValue();

                    /* If running in JIT mode, we should update base pointers when pObjectValues/pConstants changed */
                    vm_updateBasePointerForJit();
                }

                res = VM_OK;
            }

            /* Destroy asm context */
            asm_destroyAssembleContext(asmContext);

            /* Destruct scratchpad */
            vm_destructScratch(scratchPad);
        }

        /* Free source buffer */
        if (buffer)
            VM_MEM_FREE(buffer);
    }

    /* Return result */
    return res;
}

/* LPC ASM version */
static Vm_Status _compileAsGlobal_ASM(IntR fileFd, const char *pInFileName, Vm_Index_T *pEntryFunction)
{
    IntR len;
    IntR oldVarNum;
    IntR growNum;
    IntR i;
    IntR res;

    AsmContext_T *asmContext;
    Vm_Scratchpad_Info_T *scratchPad;

    /* Read whole asm file content */
    len = vm_read(__asmStr, sizeof(__asmStr), fileFd);
    __asmStr[len] = 0;
    if (len == 2 && __asmStr[0] == '\n' && __asmStr[1] == ';')
        return VM_UNKNOWN_ERROR;

    if (__asmStr[len - 1] == ';')
        __asmStr[len - 1] = 0;

    oldVarNum = vm_pGlobalProgram->memberNum;

    /* Create scratchpad for assembling process */
    vm_buildScratch(&scratchPad);

    /* Create assemble context */
    asmContext = asm_createAssembleContext(vm_pGlobalProgram, __asmStr);
    VM_ASSERT(asmContext != NULL);

    /* Assemble result of compiler */
    if (! asm_assembleString(vm_pGlobalProgram, asmContext, __asmStr, pEntryFunction, NULL))
        res = VM_ASSEMBLE_FAILED;
    else
        res = VM_OK;

    /* Destroy asm context */
    asm_destroyAssembleContext(asmContext);

    /* Destruct scratchpad */
    vm_destructScratch(scratchPad);

    /* There're some new global variables, extends global object's members */
    if ((growNum = (vm_pGlobalProgram->memberNum - oldVarNum)) > 0)
    {
        vm_pGlobalVar = VM_MEM_REALLOC(vm_pGlobalVar, sizeof(Vm_Global_Var_T) * vm_pGlobalProgram->memberNum);
        for (i = 0; i < growNum; i++)
            vm_pGlobalVar[vm_pGlobalProgram->memberNum - 1 - i].pValue = vm_newValue();
    }

    /* If running in JIT mode, we should update base pointers when pObjectValues/pConstants changed */
    vm_updateBasePointerForJit();

    /* Return res */
    return res;
}

/* Test if str is end with p */
static Boolean _endWithStr(const char *str, const char *p)
{
    const char *pos;
    IntR len0, len1, len2;

    if (str == NULL)
        return False;

    pos = vm_stristr(str, p);
    len0 = pos - str;
    len1 = STRLEN(p);
    len2 = STRLEN(str);

    return (len0 + len1 == len2);
}

/* Print formated asm source */
static void _printFormatSource(char *source)
{
    IntR lineStart = 0, lineEnd = 0;
    IntR curPos = 0;
    char line[1024];
    Boolean inFunction = False;
    IntR size = STRLEN(source);

    char *output = vm_getGlobalBuf(VM_GLOBAL_BUF_SIZE, __FUNCTION__);
    output[0] = '\0';

    /* insert line to line */
    while (1)
    {
        char c = 0;
        do
        {
            c = source[curPos];
            curPos ++;
        }
        while (c != '\n' && curPos < size);
        
        lineEnd = curPos + 1;
        STRNCPY(line + 1, source + lineStart, lineEnd - lineStart);
        line[lineEnd - lineStart] = 0;

        if (line[1] == '{')
            inFunction = True;
        else if (line[1] == '}')
            inFunction = False;

        if (inFunction && line[1] != '{' && line[STRLEN(line) - 2] != ':' && line[1] != '@')
            line[0] = '\t';
        else
            line[0] = ' ';

        STRNCPY(output, line, lineEnd - lineStart + 1);
        output[lineEnd - lineStart + 1] = 0;
        lineStart = lineEnd - 1;
        
        /* print a line */
        vm_printf("%s", output);

        if (curPos >= size)
            break;
    }
    
    // printf("%s", output);
    vm_releaseGlobalBuf(output, __FUNCTION__);
}
