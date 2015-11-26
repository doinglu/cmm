// cmm_util.cpp
// 2004.5.16    Initial version by doing
// 2015.10.17   Immigrate to cmm by doing

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include "std_port/std_port.h"
#include "std_socket/std_socket_port.h"
#include "cmm_common_util.h"

namespace cmm
{

/* Internal routines &*/
static size_t _raw_strwidth_by_char(const char *str, size_t size);

/* Reigsteration functions */
static StrwidthFunc _registered_strwidth_func = _raw_strwidth_by_char;

/* Registration routines */

/* new function to calculate string's length */
StrwidthFunc register_strwidth_func(StrwidthFunc func)
{
    StrwidthFunc prev_func;

    /* Overwrite the registered function & return old one */
    prev_func = _registered_strwidth_func;
    _registered_strwidth_func = func;

    return prev_func;
}

/* Utilities functions */

/* Convert table:
 * \r - "\r"
 * \t - "\t"
 * \n - "\n"
 * "  - \"
 * < 0x20 - "\xXX"
 */
extern void convert_string_to_readable(char *temp, size_t size, const char *str)
{
    char *out;
    const char *p;

    STD_ASSERT(size > 4);

    out = temp;
    p = str;
    while (*p && size > 4 /* Reserved for \xXX */)
    {
        switch (*p)
        {
        case '\r':
            size -= 2;
            *(out++) = '\\';
            *(out++) = 'r';
            break;

        case '\t':
            size -= 2;
            *(out++) = '\\';
            *(out++) = 't';
            break;

        case '\n':
            size -= 2;
            *(out++) = '\\';
            *(out++) = 'n';
            break;

        case '\"':
            size -= 2;
            *(out++) = '\\';
            *(out++) = '\"';
            break;

        default:
            if (*p < 0x20)
            {
                unsigned char ch1, ch2;
                size -= 4;

                ch1 = ((*p) >> 4) & 0x0F;
                ch2 = ((*p) & 0x0F);
                if (ch1 >= 10) ch1 += 'A' - 10;
                if (ch2 >= 10) ch2 += 'A' - 10;
                *(out++) = '\\';
                *(out++) = 'x';
                *(out++) = ch1;
                *(out++) = ch2;
            } else
            {
                size--;
                *(out++) = *p;
            }

            break;
        }

        /* Process next char */
        p++;
    }

    /* Terminate */
    *out = 0;
}

// Escape char table (even offset is written char, odd offset is real char)
static char _table[] =
{
    'n', '\n',
    't', '\t',
    'v', '\v',
    'r', '\r',
    0, 0,
};

/* Convert c escape chars in string */
extern size_t convert_c_string(char *output, const char *input)
{
    size_t len;
    int    found;
    size_t i, k, j;

    len = strlen(input);
    for (i = 0, j = 0; i <= len; i++, j++)
    {
        // 如果不是转义开始，那么直接拷贝
        if (input[i] != '\\')
        {
            output[j] = input[i];
            continue;
        }

        // 查表进行转义
        found = 0;
        for (k = 0; _table[k] != 0; k += 2)
        {
            if (input[i + 1] == _table[k])
            {
                output[j] = _table[k + 1];

                // 跳过一个字符
                i++;
                found = 1;
                break;
            }

            // 继续查表中的下一项
        }

        // 没有找到，拷贝\后的字符
        if (! found)
        {
            // 直接取\后的字符，跳过
            output[j] = input[i + 1];
            i++;
        }

        // 转换input的下一个字符
    }

    return j;
}

/* Make an integer to string */
/* temp must has enough space to carry result */
extern void int_to_string(char *temp, size_t size, int val, int base, bool has_sign_flag, bool is_upper_case)
{
    char  ch;
    size_t uval;
    char *p = temp;

    STD_ASSERT(size >= 8);
    if (size < 1)
        /* No enough space */
        return;

    if (size < 2)
    {
        /* No enough space, return 0 only */
        temp[0] = 0;
        return;
    }

    if (has_sign_flag && val < 0)
    {
        /* Check for sign */
        *(p++) = '-';
        val = -val;
    }

    /* Get length */
    uval = (size_t) val;
    do
    {
        uval /= base;
        p++;
    } while (uval > 0);

    if (p - temp >= (int) size)
    {
        /* No enough memory */
        temp[0] = 0;
        return;
    }

    *(p--) = 0;

    /* Put values */
    uval = (size_t) val;
    do
    {
        /* Get last digit */
        ch = (char) (uval % base);
        uval /= base;

        /* Convert to 0-9 [A-F/a-f] */
        if (ch < 10)
            ch += '0';
        else
        if (is_upper_case)
            ch += 'A' - 10;
        else
            ch += 'a' - 10;

        /* Insert to string */
        *(p--) = ch;
    } while (uval > 0);
}

/* Make an integer (64bits) to string */
/* temp must has enough space to carry result */
extern void int64_to_string(char *temp, size_t size, Int64 val, int base, bool has_sign_flag, bool is_upper_case)
{
    char  ch;
    Int64 uval;
    char *p = temp;

    STD_ASSERT(size >= 8);
    if (size < 1)
        /* No enough space */
        return;

    if (size < 2)
    {
        /* No enough space, return 0 only */
        temp[0] = 0;
        return;
    }

    if (has_sign_flag && val < 0)
    {
        /* Check for sign */
        *(p++) = '-';
        val = -val;
    };

    /* Get length */
    uval = (Uint64) val;
    do
    {
        uval /= base;
        p++;
    } while (uval > 0);

    if (p - temp >= (int) size)
    {
        /* No enough memory */
        temp[0] = 0;
        return;
    }

    *(p--) = 0;

    /* Put values */
    uval = (Uint64) val;
    do
    {
        /* Get last digit */
        ch = (char) (uval % base);
        uval /= base;

        /* Convert to 0-9 [A-F/a-f] */
        if (ch < 10)
            ch += '0';
        else
        if (is_upper_case)
            ch += 'A' - 10;
        else
            ch += 'a' - 10;

        /* Insert to string */
        *(p--) = ch;
    } while (uval > 0);
}

/* Remove escape char of string */
/* When encounter \, remove it */
extern void remove_escape_of_string(char *str)
{
    int i, k;

    for (i = 0, k = 0; str[i]; i++)
    {
        if (str[i] == '\\')
            /* Got escape char, ignore */
            continue;

        str[k++] = str[i];
    }
    str[k] = 0;
}

/* String to int */
/* When skip_space set to 1, means skip space first */
extern int strtol(const char *ptr, char **endptr, int base, int skip_space)
{
    auto *s = (const unsigned char *)ptr;
    int val;
    int neg, any, c;

    neg = 0;
    c = *s++;

    if (skip_space)
    {
        /* Skip space at string head */
        while (isspace(c))
            c = *s++;
    }

    if (c == '-')
    {
        neg = 1;
        c = *s++;
    } else
    if (c == '+')
        c = *s++;

    any = 0;
    val = 0;
    for (;;)
    {
        if (c >= 'a' && c <= 'f')
            /* a - f */
            c = c - 'a' + 10;
        else
        if (c >= 'A' && c <= 'F')
            /* A - F */
            c = c - 'A' + 10;
        else
        if (c >= '0' && c <= '9')
            /* 0 - 9 */
            c -= '0';
        else
            break;

        if (c >= base)
            /* Out of base */
            break;

        /* Accumulate */
        val *= base;
        val += c;

        /* Get next char */
        c = *s++;
        any = 1;
    }

    /* Return endptr */
    if (endptr != 0)
        *endptr = any ? (char *)s - 1 : (char *)ptr;

    if (neg)
        val = -val;

    return val;
}

/* String to Int64 */
/* When skip_space set to 1, means skip space first */
extern Int64 strtol64(const char *ptr, char **endptr, int base, int skip_space)
{
    auto *s = (const unsigned char *)ptr;
    Int64 val;
    int   neg, any, c;

    neg = 0;
    c = *s++;

    if (skip_space)
    {
        /* Skip space at string head */
        while (isspace(c))
            c = *s++;
    }

    if (c == '-')
    {
        neg = 1;
        c = *s++;
    } else
    if (c == '+')
        c = *s++;

    any = 0;
    val = 0;
    for (;;)
    {
        if (c >= 'a' && c <= 'f')
            /* a - f */
            c = c - 'a' + 10;
        else
        if (c >= 'A' && c <= 'F')
            /* A - F */
            c = c - 'A' + 10;
        else
        if (c >= '0' && c <= '9')
            /* 0 - 9 */
            c -= '0';
        else
            break;

        if (c >= base)
            /* Out of base */
            break;

        /* Accumulate */
        val *= base;
        val += c;

        /* Get next char */
        c = *s++;
        any = 1;
    }

    /* Return endptr */
    if (endptr != 0)
        *endptr = any ? (char *)s - 1 : (char *)ptr;

    if (neg)
        val = -val;

    return val;
}

/* String to double */
/* When skip_space set to 1, means skip space first */
extern double strtof(const char *nptr, char **endptr, int skip_space)
{
    auto *s = (const unsigned char *)nptr;
    double acc;
    int   neg, c, any, div;

    div = 1;
    neg = 0;

    c = *s++;

    if (skip_space)
    {
        /* Skip space at string head */
        while (isspace(c))
            c = *s++;
    }

    if (c == '-')
    {
        neg = 1;
        c = *s++;
    } else
    if (c == '+')
        c = *s++;

    for (acc = 0, any = 0;; c = *s++)
    {
        if (isdigit((unsigned char) c))
            c -= '0';
        else
        if ((div == 1) && (c == '.'))
        {
            div = 10;
            continue;
        } else
            break;
        if (div == 1)
        {
            acc *= (double) 10;
            acc += (double) c;
        } else
        {
            acc += (double) c / (double) div;
            div *= 10;
        }
        any = 1;
    }

    if (neg)
        acc = -acc;

    if (endptr != 0)
        *endptr = any ? (char *)s - 1 : (char *) nptr;

    return acc;
}

/* stricmp */
extern int stricmp(const char *s1, const char *s2)
{
    unsigned char ch1, ch2;

    for (;;)
    {
        ch1 = (unsigned char) *(s1++);
        ch2 = (unsigned char) *(s2++);
        ch1 = toupper((unsigned char) ch1);
        ch2 = toupper((unsigned char) ch2);
        if (ch1 < ch2)
            return -1;
        if (ch1 > ch2)
            return 1;

        if (! ch1)
            return 0;
    }
}

/* strstr without case sensitive */
extern const char *stristr(const char *str, const char *sub)
{
    int i, k;
    size_t len, slen;

    len = strlen(str);
    slen = strlen(sub);

    for (i = 0; i <= len; i++)
    {
        /* Compare */
        for (k = 0; k < slen; k++)
            if (toupper((unsigned char) str[i + k]) != toupper((unsigned char) sub[k]))
                break;

        if (k >= slen)
            /* Matched */
            return str + i;
    }

    return 0;
}

/* do strstr with reverse mode */
extern const char *strrstr(const char *str, const char *sub)
{
    int i;
    size_t slen;

    slen = strlen(sub);
    for (i = (int)(strlen(str) - slen); i >= 0; i--)
        if (strncmp(str + i, sub, slen) == 0)
            /* Matched */
            return str + i;

    /* Not matched */
    return 0;
}

/* check width (1 char - 1 length) */
extern size_t strwidth(const char *str, size_t size)
{
    STD_ASSERT(_registered_strwidth_func);
    /* This function is hooked */
    return _registered_strwidth_func(str, size);
}

/* is string empty? */
extern bool is_empty_string(const char *str)
{
    auto *s = (const unsigned char *)str;
    char ch;

    while ((ch = *(s++)))
        if (! isspace(ch))
            /* Not empty string */
            return false;

    /* Empty string */
    return true;
}

/* inettoa */
extern void inettoa(char *str, unsigned int ip)
{
    STD_ASSERT(str);

    sprintf(str, "%u.%u.%u.%u",
            ip >> 24,
            (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF,
            ip & 0xFF);
}

/* atoinet */
extern Uint32 atoinet(const char *str)
{
    unsigned int c[4];

    STD_ASSERT(str);

    if (sscanf(str, "%u.%u.%u.%u", &c[0], &c[1], &c[2], &c[3]) != 4)
        return 0;

    return (Uint32) ((c[0] << 24) | (c[1] << 16) | (c[2] << 8) | c[3]);
}

/* Read a line from content, return */
/* When last line is got, return 0 for next line */
extern const char *get_line(char *buffer, size_t size, const char *content)
{
    const char *p;
    size_t len;

    STD_ASSERT(buffer);
    STD_ASSERT(size > 0);

    if (!content)
        /* No content */
        return 0;

    p = strchr(content, '\n');
    if (p)
    {
        /* Get pointer for next line */
        len = p - content;
        p++;
    } else
    {
        /* Set len to whole string */
        len = strlen(content);
    }

    if (len > 0 && content[len - 1] == '\r')
        /* \r\n, drop it */
        len--;

    if (len >= size)
        len = size - 1;

    memcpy(buffer, content, len);
    buffer[len] = 0;

    /* Return next line */
    return p;
}

/* Trim space of string */
extern void trim_string(char *str)
{
    const char *p, *p2;

    p = str;
    while (*p && isspace((unsigned char) *p)) p++;

    p2 = str + strlen(str) - 1;
    while (p2 >= p && isspace((unsigned char) *p2)) p2--;

    if (p2 < p)
    {
        /* Empty string */
        str[0] = 0;
        return;
    }

    /* Move content of string */
    if (p != str)
        memmove(str, p, (p2 - p) + 1);
    str[p2 - p + 1] = 0;
}

/* Read field information from ini file */
size_t read_ini(const char *section, const char *key,
                const char *def, char *buffer, int size,
                FILE *fp)
{
    bool ok;
    bool section_matched;
    size_t len;
    char *p, *p2, *p3;

    STD_ASSERT(fp);
    STD_ASSERT(section);
    STD_ASSERT(key);
    STD_ASSERT(def);
    STD_ASSERT(buffer);
    STD_ASSERT(size > 0);

    section_matched = false;
    ok = 0;
    do
    {
        /* Read file line by line */
        while (! ok && ! feof(fp) && fgets(buffer, (int) size, fp))
        {
            p = buffer;
            while (*p && isspace(*p)) p++;
            if (*p == '[')
            {
                /* Skip white space */
                do { p++; } while (*p && isspace(*p));

                p2 = p;
                while (*p2 && *p2 != ']') p2++;
                if (*p2)
                {
                    *p2 = 0;
                    section_matched = (stricmp(p, section) == 0);
                }

                /* Finish this line */
                continue;
            }

            if (!section_matched)
                /* Check for next section */
                continue;

            /* Check for key */
            p2 = p;
            while (*p2 && *p2 != '=') p2++;
            if (! *p2)
                /* Not found '=', skip this line */
                continue;

            p3 = p2 - 1;
            while (p3 > p && isspace(*p3))
                p3--;

            p3++;
            *p3 = 0;
            if (stricmp(p, key) != 0)
                /* Key is mismatched */
                continue;

            /* Key is matched, copy the value to buffer */
            do { p2++; } while (*p2 && isspace(*p2));
            p = p2;
            while (*p2 && *p2 != '\r' && *p2 != '\n')
                p2++;
            *p2 = 0;
            len = p2 - p;
            if (len >= size)
                len = size - 1;

            memmove(buffer, p, len);
            buffer[len] = 0;

            /* Value is copied, set OK flag */
            ok = 1;
        }

        /* End of operation */
    } while (0);

    if (! ok)
    {
        /* Not found the key, return default value */
        strncpy(buffer, def, size - 1);
        len = strlen(def);
        if (len >= size)
            /* Buffer is too small, truncated */
            buffer[len = size - 1] = 0;
    }

    return len;
}

/* Test if the specified number n is not 1.#inf or NaN. */
static int __isnan(double x) { return x != x; }
static int __isinf(double x) { return ! __isnan(x) && __isnan(x - x); }
extern bool is_finite(double n)
{
    if (__isnan(n) || __isinf(n))
        return false;
    return true;
}

/* FIX32_SCALE * MAX_FLOAT_TO_FIX32 < MAX_INT */
const double FIX32_SCALE = 1000;
const double MAX_FLOAT_TO_FIX32 = 2000000;

/* Util function: convert real to fix32 */
extern Int32 real_to_fix32(double x)
{
    Int32 fix;

    if (! is_finite(x))
        throw "Try to convert an invalid real number to fix32.\n";

    if (x < -MAX_FLOAT_TO_FIX32 || x > MAX_FLOAT_TO_FIX32)
        throw "Real number is too large for fix32.\n";

    fix = (Int32) (x * FIX32_SCALE);

    return fix;
}

/* Util function: convert fix32 to real */
extern double fix32_to_real(Int32 x)
{
    double flt;

    flt = ((double) x / FIX32_SCALE);

    return flt;
}

/* get width by size */
static size_t _raw_strwidth_by_char(const char *str, size_t size)
{
    size_t len;

    len = strlen(str);
    return len > size ? len : size;
}

}
