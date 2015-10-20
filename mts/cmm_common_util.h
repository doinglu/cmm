// cmm_common_util.h
// 2004.5.16    Initial version by doing
// 2015.10.17   Immigrate to cmm by doing

#pragma once

namespace cmm
{

/* For argument skip_space@strtol/strtof */
enum
{
    SKIP_HEAD_SPACE = 1,
    CALC_AT_HEAD    = 0,
};

/* For argument acceptUndefined@query* */
enum
{
    ACCEPT_NIL     = 1,
    NOT_ACCEPT_NIL = 0,
};

// Routine type for native strwidth
typedef size_t (*StrwidthFunc)(const char *str, size_t size);

/* Hook registered routines */
extern StrwidthFunc register_strwidth_func(StrwidthFunc func);

/* Utilities functions */
extern void        convert_string_to_readable(char *temp, size_t size, const char *str);
extern size_t      convert_c_string(char *output, const char *input);
extern void        int_to_string(char *temp, size_t size, int val, int base, int sign_flag, int upper_case);
extern void        int64_to_string(char *temp, size_t size, Int64 val, int base, int sign_flag, int upper_case);
extern void        remove_escape_of_string(char *str);
extern int         strtol(const char *ptr, char **endptr, int base, int skip_space);
extern Int64       strtol64(const char *ptr, char **endptr, int base, int skip_space);
extern double      strtof(const char *nptr, char **endptr, int skip_space);
extern int         stricmp(const char *s1, const char *s2);
extern const char *stristr(const char *str, const char *sub);
extern const char *strrstr(const char *str, const char *sub);
extern size_t      strwidth(const char *str, size_t size);
extern bool        is_empty_string(const char *str);
extern void        inettoa(char *str, Uint32 ip);
extern Uint32      atoinet(const char *str);
extern Uint32      htonl(Uint32 n);
extern Uint32      ntohl(Uint32 n);
extern Uint16      htons(Uint16 n);
extern Uint16      ntohs(Uint16 n);
extern const char *get_line(char *buffer, size_t size, const char *content);
extern void        trim_string(char *str);
extern size_t      read_ini(const char *section, const char *key, const char *def, char *buffer, int size, FILE *fp);
extern bool        is_finite(double n);
extern Int32       real_to_fix32(double x);
extern double      fix32_to_real(Int32 x);

}
