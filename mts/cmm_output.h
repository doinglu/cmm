// cmm_output.h
// 2001.12.20   Initial version by doing
// 2015.10.17   Immigrate by doing

#pragma once

#include "std_template/simple_vector.h"
#include "std_template/simple_hash_set.h"
#include "cmm.h"
#include "cmm_value.h"

namespace cmm
{

class Value;
struct StringImpl;

class Output
{
public:
    String format_output(const char_t *format_str, Value *argv, ArgNo argc);
    String type_value(const Value *value);

private:
    enum ErrorCode
    {
        ERR_BUFF_OVERFLOW       = 0x1,     /* buffer overflowed */
        ERR_TO_FEW_ARGS         = 0x2,     /* more arguments spec'ed than passed */
        ERR_INVALID_STAR        = 0x3,     /* invalid arg to * */
        ERR_PRES_EXPECTED       = 0x4,     /* expected presision not found */
        ERR_INVALID_FORMAT_STR  = 0x5,     /* error in format string */
        ERR_INCORRECT_ARG_S     = 0x6,     /* invalid arg to %s */
        ERR_CST_REQUIRES_FS     = 0x7,     /* field size not given for c/t */
        ERR_BAD_INT_TYPE        = 0x8,     /* bad integer type... */
        ERR_UNDEFINED_TYPE      = 0x9,     /* undefined type found */
        ERR_QUOTE_EXPECTED      = 0xA,     /* expected ' not found */
        ERR_UNEXPECTED_EOS      = 0xB,     /* fs terminated unexpectedly */
        ERR_NULL_PS             = 0xC,     /* pad string is null */
        ERR_ARRAY_EXPECTED      = 0xD,     /* Yep!  You guessed it. */
        ERR_RECOVERY_ONLY       = 0xE,     /* err msg already done...just recover */
    };

    /*
     * Format of FormatInfo:
     *   00000000 00xxxxxx : argument type:
     *                            000000 : type not found yet;
     *                            000001 : error type not found;
     *                            000010 : percent sign, null argument;
     *                            000011 : mixed datatype;
     *                            000100 : string;
     *                            001000 : integer;
     *                            001001 : unsigned integer;
     *                            001010 : char;
     *                            001011 : octal;
     *                            001100 : hex;
     *                            001101 : HEX;
     *                            001111 : float;
     *   00000000 xx000000 : justification:
     *                              00 : right;
     *                              01 : centre;
     *                              10 : left;
     *   000000xx 00000000 : positive pad char:
     *                              00 : none;
     *                              01 : ' ';
     *                              10 : '+';
     */
    enum
    {
        INFO_T              = 0x3F,
        INFO_T_ERROR        = 0x1,
        INFO_T_NULL         = 0x2,
        INFO_T_ANY          = 0x3,
        INFO_T_STRING       = 0x4,
        INFO_T_INT          = 0x8,
        INFO_T_UINT         = 0x9,
        INFO_T_CHAR         = 0xA,
        INFO_T_OCT          = 0xB,
        INFO_T_HEX          = 0xC,
        INFO_T_C_HEX        = 0xD,
        INFO_T_FLOAT        = 0xE,
        INFO_T_IP           = 0xF,

        INFO_J              = 0xC0,
        INFO_J_CENTRE       = 0x40,
        INFO_J_LEFT         = 0x80,

        INFO_PP             = 0x300,
        INFO_PP_SPACE       = 0x100,
        INFO_PP_PLUS        = 0x200,
    };

    struct PadInfo
    {
        const char_t *what;
        size_t len;
    };

    struct TabData
    {
        const char_t *start;
        const char_t *cur;
    };

private:
    void    add_char(char_t ch);
    void    add_pad(PadInfo *pad, size_t len);
    void    add_nchars(const char_t *str, size_t len);
    void    add_number(Int64 n, const char *format = "%lld");
    void    add_string(const StringImpl *impl);
    void    add_c_str(const char *c_str);
    void    add_justified(const char_t *str, size_t slen, PadInfo *pad, int fs, int finfo, int trailing);
    char_t *reserve_space(size_t n);
    void    get_next_arg();
    void    error_occurred(ErrorCode code);
    void    type_value_at(const Value *value, size_t ident);

private:
    Value *m_argv;
    ArgNo  m_argc;
    Value *m_current_arg;
    ArgNo  m_current_arg_index;
    simple::unsafe_vector<char_t> m_obuf; // Output buffer
    simple::hash_set<ReferenceImpl *> m_check_loop;
};

}
