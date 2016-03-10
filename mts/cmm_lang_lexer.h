// cmm_lang_lexer.h
// Initial version 2001.12.12 by doing
// Immigrated 2015.10.28 by doing

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm.h"
#include "cmm_efun.h"
#include "cmm_gc_alloc.h"
#include "cmm_lang_component.h"
#include "cmm_mmm_value.h"

namespace cmm
{

#define SKIPWHITE   while (vm_isspace(*p) && (*p != '\n')) p++

// for find_or_add_ident
#define FOA_GLOBAL_SCOPE        0x1

// Max name length
enum
{
    MAX_FILE_NAME_LEN = 128,
    MAX_LINE_SIZE = 4096
};

struct DefinedName
{
    short localNum, globalNum;
};

typedef char* (*Vm_Def_Func_Ptr_T)();

// Defined name's type
enum DefType
{
    DEF_IS_UNDEFINED  = 0x0001,
    DEF_IS_PREDEF     = 0x0002,
    DEF_IS_ONCE       = 0x0004,
    DEF_IS_FUNC       = 0x0008,
    DEF_IS_IN_OBJECT  = 0x4000,
    DEF_IS_UNFINALIZE = 0x8000,
};

// Attrib of refered
enum RefStatus
{
    VAR_BEEN_ASSIGNED = 0x0001,  // Varaible is assigned
    VAR_BEEN_READ     = 0x0002,  // SyntaxVariable is used    
    FUN_BEEN_CALLED   = 0x0004,  // Function been called
};

struct Keyword
{
    const char* c_str_word; // Use for define in source file
    Uint32      token;          // flags here
    Uint32      sem_value;      // semantic value for predefined tokens
};

/*
 * Information about all instructions. This is not really needed as the
 * automatically generated efun_arg_types[] should be used.
 */

class Lang;
class Program;

// Line no
typedef int LineNo;

struct IfStatement;

class LangLexer : LangComponent
{
    friend class Lang;

public:
    enum
    {
        DEFMAX = 12800, // At lease MAXLINE * 3
        MAXLINE = 4096,
        MLEN = 4096,
        NSIZE = 256,
        EXPANDMAX = 25000,
        NARGS = 25,
        MARKS = '@',
    };

    // Lex-in crypt mode
    enum CryptType
    {
        LEX_NO_CRYPT = 0,
        LEX_CRYPT_TYPE_XOR = 17,
        LEX_CRYPT_TYPE_SHIFT = 23,
    };

    // Linked buffer
    struct LinkedBuf
    {
        LinkedBuf* prev;
        char  buf[DEFMAX];
        char* buf_end;
        char* last_new_line;
        char* out;
    };

public:
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

public:
    LangLexer(Lang* lang_context);
    ~LangLexer();

public:
    StringImpl* add_file_name(const simple::string& file_name);
    StringImpl* find_file_name(const simple::string& file_name);
    StringImpl* find_file_name_not_exact_match(const simple::string& file_name);
    void        destruct_file_names();
    StringImpl* get_current_file_name();
    LineNo      get_current_line();
    Uint32      get_default_attrib();
    bool        set_default_attrib(Uint32 attrib);
    bool        start_new_file(Program *program, IntR fd, const simple::string& file_name);
    bool        end_new_file(bool succ);
    int         lex_in();

private:
    void        lex_errorp(const char* msg);
    void        add_input(char* p);
    void        skip_white();
    IntR        cond_get_exp(IntR priority);
    bool        skip_to(const char* token, const char* atoken);
    void        handle_cond(IntR c);
    void        handle_pragma(char*);
    void        set_current_line_file(char* linefile);
    void        skip_line();
    void        skip_comment();
    void        del_trail(char* sp);
    void        generate_file_dir_name();
    void        get_next_char(IntR *ptr_ch);
    void        get_alpha_string_to(char* p, char* to, char* to_end);
    IntR        cmy_get_char();
    void        refill();
    void        refill_buffer();
    int         get_char_in_cond_exp();
    void        handle_elif(char* sp);
    void        handle_else();
    void        handle_endif();
    char*       copy_string(const char* c_str, size_t len = SIZE_MAX);

private:
    static Keyword *get_keyword(const simple::string& name);

private:
    typedef simple::string(*ExpandFunc)(Lang* lang_context);
    typedef simple::hash_map<simple::string, ExpandFunc> ExpandFuncMap;
    static ExpandFuncMap *expand_builtin_macro_funcs;

    static int    init_predefines();
    static void   shutdown_predefines();
    static void   add_predefine(const simple::string& macro, ExpandFunc func);

    // Predefine expansion functions
    static simple::string expand_file_name(Lang* lang_context);
    static simple::string expand_pure_file_name(Lang* lang_context);
    static simple::string expand_dir_name(Lang* lang_context);
    static simple::string expand_line_no(Lang* lang_context);
    static simple::string expand_function_name(Lang* lang_context);
    static simple::string expand_counter(Lang* lang_context);

private:
    // current output buffer point
    char*        m_out;

    // text buffer of a line
    char         m_text[MAXLINE];

    // Position of the last new line char
    char*        m_last_new_line;

    // Default attribute of compiler
    Uint32       m_default_attrib;

    // For #if/#else/#endif processing
    IfStatement* m_if_top;

    // Current file path
    StringImpl*  m_current_file_name;

    // Current file handle
    IntR         m_in_file_fd;

    // Current line/Total lines
    LineNo       m_current_line;
    LineNo       m_total_lines;
    Uint32       m_line_words;
    LineNo       m_current_line_base;////----
    LineNo       m_current_line_saved;////----

    // For __FILE__, __LINE__
    simple::string m_current_dir_string;
    simple::string m_current_file_string;
    simple::string m_current_pure_file_string;

    // main source buffer
    LinkedBuf    m_main_buf;

    // Current linked buffer
    LinkedBuf*   m_current_buf;

    // At head of input file?
    bool         m_is_start;

    // For crypt
    CryptType    m_in_crypt_type;
    Uint8        m_in_crypt_code;

    // Is the line no should be fixed?
    LineNo       m_fixed_line;

    // unique count, used to generate __COUNTER__
    Uint32       m_unique_counter;

private:
    // Keyword array in C++ source file
    static Keyword m_define_keywords[];

    // Keyword mapping for runtime
    typedef simple::hash_map<simple::string, Keyword*> KeywordMap;
    static KeywordMap* m_keywords;

    // File name list
    typedef simple::vector<StringImpl*> FileNameList;
    static FileNameList* m_file_name_list;

    // Critical secion for access
    static struct std_critical_section *m_cs;

    // Opcode mapping
    static char m_optab[];
    static char m_optab2[];
};

}
