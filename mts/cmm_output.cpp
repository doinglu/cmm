// cmm_output.cpp
// 2015.10.17   Immigrate by doing

/* This file is copied from LPMud & modified for VM */
/* Previous information about the file */
/*
 * sprintf.c v1.05 for LPMud 3.0.52
 *
 * An implementation of (s)printf() for LPC, with quite a few
 * extensions (note that as no floating point exists, some parameters
 * have slightly different meaning or restrictions to "standard"
 * (s)printf.)  Implemented by Lynscar (Sean A Reith).
 * 2/28/93: float support for MudOS added by jacques/blackthorn
 *
 * This version supports the following as modifiers:
 *  " "   pad positive integers with a space.
 *  "+"   pad positive integers with a plus sign.
 *  "-"   left adjusted within field size.
 *        NB: std (s)printf() defaults to right justification, which is
 *            unnatural in the context of a mainly string based language
 *            but has been retained for "compatability" ;)
 *  "|"   centered within field size.
 *  "="   column mode if strings are greater than field size.  this is only
 *        meaningful with strings, all other types ignore
 *        this.  columns are auto-magically word wrapped.
 *  "#"   table mode, print a list of '\n' separated 'words' in a
 *        table within the field size.  only meaningful with strings.
 *   n    specifies the field size, a '*' specifies to use the corresponding
 *        arg as the field size.  if n is prepended with a zero, then is padded
 *        zeros, else it is padded with spaces (or specified pad string).
 *  "."n  presision of n, simple strings truncate after this (if presision is
 *        greater than field size, then field size = presision), tables use
 *        presision to specify the number of columns (if presision not specified
 *        then tables calculate a best fit), all other types ignore this.
 *  ":"n  n specifies the fs _and_ the presision, if n is prepended by a zero
 *        then it is padded with zeros instead of spaces.
 *  "@"   the argument is an array.  the corresponding FormatInfo (minus the
 *        "@") is applyed to each element of the array.
 *  "'X'" The char(s) between the single-quotes are used to pad to field
 *        size (defaults to space) (if both a zero (in front of field
 *        size) and a pad string are specified, the one specified second
 *        overrules).  NOTE:  to include "'" in the pad string, you must
 *        use "\\'" (as the backslash has to be escaped past the
 *        interpreter), similarly, to include "\" requires "\\\\".
 * The following are the possible type specifiers.
 *  "%"   in which case no arguments are interpreted, and a "%" is inserted, and
 *        all modifiers are ignored.
 *  "O"   the argument is an LPC datatype.
 *  "s"   the argument is a string.
 *  "d"   the integer arg is printed in decimal.
 *  "i"   as d.
 *  "f"   floating point value.
 *  "c"   the integer arg is to be printed as a character.
 *  "o"   the integer arg is printed in octal.
 *  "x"   the integer arg is printed in hex.
 *  "X"   the integer arg is printed in hex (in capitals).
 */

#include <stdio.h>
#include "std_port/std_port.h"
#include "cmm_common_util.h"
#include "cmm_output.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Append a char to output buffer                                         
void Output::add_char(char_t ch)
{
    m_obuf.push_back(ch);
}

// Append pad by PadInfo (default to pad space)
void Output::add_pad(PadInfo *pad, size_t len)
{
    char_t *p;
    size_t pad_len;

    // Reserved n chars
    p = reserve_space(len);
    
    if (pad && (pad_len = pad->len))
    {
        const char_t *end;
        const char_t *pstr = pad->what;
        size_t i;
        char_t c;
        
        for (i = 0, end = p + len; p < end; i++)
        {
            if (i == pad_len) 
                i = 0;

            if ((c = pstr[i]) == '\\')
            {
                /* guaranteed to have a valid char next */
                *p++ = pstr[++i];
            } else
                *p++ = c;
        }
    } else
    {
        // Pad space
        for (size_t i = 0; i < len; i++)
            p[i] = ' ';
    }
}

// Add n chars
void Output::add_nchars(const char_t *str, size_t len)
{
    char_t *p = reserve_space(len);
    memcpy(p, str, len * sizeof(char_t));
}

// Add an integer
void Output::add_number(Int64 n, const char *format)
{
    char buf[64];
    sprintf(buf, format, n);
    add_nchars(buf, strlen(buf));
}

// Add a string
void Output::add_string(const StringImpl *impl)
{
    add_nchars(impl->ptr(), impl->length());
}

// Add a c string
void Output::add_c_str(const char *c_str)
{
    add_nchars((const char_t *)c_str, strlen(c_str));
}

/* Signal an error.  Note that we call error, so this routine never returns.
 * Anything that has been allocated should be somewhere it can be found and
 * freed later.
 */
/*
 * Adds the string "str" to the buff after justifying it within "fs".
 * "trailing" is a flag which is set if trailing justification is to be done.
 * "str" is unmodified.  trailing is, of course, ignored in the case
 * of right justification.
 */
void Output::add_justified(const char_t *str, size_t slen, PadInfo *pad,
                            int fs, int finfo, int trailing)
{
    /* Fix fs length - since slen is not width of string, for example:
     * Chines "Zhong Wen" is 4bytes in GBK, 6bytes in UTF-8.
     * If we are processs as GBK, no problem, but if we are process as
     * UTF-8, the follow statment:
     * printf("%6s", "Zhong Wen")
     * may case bad alignment since "Zhong wen" is 6 bytes.
     * How ever, If we get the vm_strwidth of "Zhong wen" & change 6 to 8 = 6 - 4(width) + 6(chars),
     * the result is fine */
    fs -= (int)strwidth(str, slen);

    if (fs <= 0)
    {
        add_nchars(str, slen);
    } else
    {
        IntR i;
        switch (finfo & INFO_J)
        {
        case INFO_J_LEFT:
            add_nchars(str, slen);
            if (trailing)
                add_pad(pad, (UintR) fs);
            break;
        case INFO_J_CENTRE:
            i = fs / 2 + fs % 2;
            add_pad(pad, (UintR) i);
            add_nchars(str, slen);
            if (trailing)
            add_pad(pad, (UintR) fs - i);
            break;
        default:
            /* std (s)printf defaults to right
                * justification */
            add_pad(pad, (UintR) fs);
            add_nchars(str, slen);
        }
    }
} /* end of add_justified() */

// Reserve n chars @ end of output buf
char_t *Output::reserve_space(size_t n)
{
    // Reserved n chars
    size_t pos = m_obuf.size();
    m_obuf.push_backs(0, n);
    return m_obuf.get_array_address(pos);
}

// Get next argument, throw error if failed 
void Output::get_next_arg()
{
    if (++m_current_arg_index >= m_argc)
        error_occurred(ERR_TO_FEW_ARGS);

    m_current_arg = (m_argv + m_current_arg_index);
}

/* Signal an error.  Note that we call error, so this routine never returns.
* Anything that has been allocated should be somewhere it can be found and
* freed later.
*/
void Output::error_occurred(ErrorCode code)
{
    char lbuf[256];
    const char *err;

    switch (code)
    {
    case ERR_BUFF_OVERFLOW:
        err = "BUFF_SIZE overflowed...";
        break;
    case ERR_TO_FEW_ARGS:
        err = "More arguments specified than passed.";
        break;
    case ERR_INVALID_STAR:
        err = "Incorrect argument type to *.";
        break;
    case ERR_PRES_EXPECTED:
        err = "Expected presision not found.";
        break;
    case ERR_INVALID_FORMAT_STR:
        err = "Error in format string.";
        break;
    case ERR_INCORRECT_ARG_S:
        err = "Incorrect argument to type %%s.";
        break;
    case ERR_CST_REQUIRES_FS:
        err = "Column/table mode requires a field size.";
        break;
    case ERR_BAD_INT_TYPE:
        err = "!feature - bad integer type!";
        break;
    case ERR_UNDEFINED_TYPE:
        err = "!feature - undefined type!";
        break;
    case ERR_QUOTE_EXPECTED:
        err = "Quote expected in format string.";
        break;
    case ERR_UNEXPECTED_EOS:
        err = "Unexpected end of format string.";
        break;
    case ERR_NULL_PS:
        err = "Null pad string specified.";
        break;
    case ERR_ARRAY_EXPECTED:
        err = "Array expected.";
        break;
    default:
        err = "undefined error in (s)printf!\n";
        break;
    }
    snprintf(lbuf, sizeof(lbuf), "(s)printf(): %s (arg: %d)\n", err, (int)m_current_arg_index);
    throw_error(lbuf);
}

/*
 * THE (s)printf() function.
 * It returns a pointer to it's internal buffer (or a string in the text
 * segment) thus, the string must be copied if it has to survive after
 * this function is called again, or if it's going to be modified (esp.
 * if it risks being free()ed).
 */
String Output::format_output(const char_t *format_str, Value *argv, ArgNo argc)
{
    int      finfo;
    PadInfo  pad;     /* fs pad string */
    size_t   fpos;      /* position in format_str */
    int      fs;          /* field size */
    Integer  pres;        /* presision */
    size_t   i;
    size_t   last;

    // Initialize class members
    m_argv = argv;
    m_argc = argc;
    m_current_arg_index = -1;

    last = 0;
    for (fpos = 0; 1; fpos++)
    {
        char c = format_str[fpos];
        
        if (c == '\n' || ! c)
        {
            if (last != fpos)
            {
                add_nchars(format_str + last, fpos - last);
                last = fpos + 1;
            } else last++;

            if (! c)
                break;
            add_char('\n');
            continue;
        } else
        if (c == '%')
        {
            if (last != fpos)
            {
                add_nchars(format_str + last, fpos - last);
                last = fpos + 1;
            } else last++;
            if (format_str[fpos + 1] == '%')
            {
                add_char('%');
                fpos++;
                last++;
                continue;
            }
            get_next_arg();
            fs = 0;
            pres = 0;
            pad.len = 0;
            finfo = 0;
            for (fpos++; ! (finfo & INFO_T); fpos++)
            {
                if (! format_str[fpos])
                {
                    finfo |= INFO_T_ERROR;
                    break;
                }
                if (((format_str[fpos] >= '0') && (format_str[fpos] <= '9'))
                    || (format_str[fpos] == '*'))
                {
                    if (pres == -1) {   /* then looking for pres */
                        if (format_str[fpos] == '*')
                        {
                            if (m_current_arg->m_type != ValueType::INTEGER)
                                error_occurred(ERR_INVALID_STAR);
                            pres = m_current_arg->m_int;
                            get_next_arg();
                            continue;
                        }
                        pres = format_str[fpos] - '0';
                        for (fpos++;
                                (format_str[fpos] >= '0') && (format_str[fpos] <= '9'); fpos++)
                        {
                            pres = pres * 10 + format_str[fpos] - '0';
                        }
                        if (pres < 0) pres = 0;
                    } else
                    {    /* then is fs (and maybe pres) */
                        if ((format_str[fpos] == '0') && (((format_str[fpos + 1] >= '1')
                                                      && (format_str[fpos + 1] <= '9')) || (format_str[fpos + 1] == '*')))
                        {
                            pad.what = "0";
                            pad.len = 1;
                        } else
                        {
                            if (format_str[fpos] == '*')
                            {
                                if (m_current_arg->m_type != ValueType::INTEGER)
                                    error_occurred(ERR_INVALID_STAR);
                                fs = (int)m_current_arg->m_int;
                                if (fs < 0) fs = 0;
                                if (pres == -2)
                                    pres = fs;  /* colon */
                                get_next_arg();
                                continue;
                            }
                            fs = format_str[fpos] - '0';
                        }
                        for (fpos++;
                                (format_str[fpos] >= '0') && (format_str[fpos] <= '9'); fpos++)
                        {
                            fs = fs * 10 + format_str[fpos] - '0';
                        }
                        if (fs < 0) fs = 0;
                        if (pres == -2)
                        {       /* colon */
                            pres = fs;
                        }
                    }
                    fpos--; /* about to get incremented */
                    continue;
                }
                switch (format_str[fpos])
                {
                case ' ':
                    finfo |= INFO_PP_SPACE;
                    break;
                case '+':
                    finfo |= INFO_PP_PLUS;
                    break;
                case '-':
                    finfo |= INFO_J_LEFT;
                    break;
                case '|':
                    finfo |= INFO_J_CENTRE;
                    break;
                case '.':
                    pres = -1;
                    break;
                case ':':
                    pres = -2;
                    break;
                case 'O':
                    finfo |= INFO_T_ANY;
                    break;
                case 's':
                    finfo |= INFO_T_STRING;
                    break;
                case 'u':
                    finfo |= INFO_T_UINT;
                    break;
                case 'd':
                    finfo |= INFO_T_INT;
                    break;
                case 'i':
                    if (format_str[fpos + 1] == 'p')
                    {
                        fpos++;
                        finfo |= INFO_T_IP;
                    } else
                        finfo |= INFO_T_INT;
                    break;
                case 'g':
                case 'f':
                    finfo |= INFO_T_FLOAT;
                    break;
                case 'c':
                    finfo |= INFO_T_CHAR;
                    break;
                case 'o':
                    finfo |= INFO_T_OCT;
                    break;
                case 'x':
                    finfo |= INFO_T_HEX;
                    break;
                case 'X':
                    finfo |= INFO_T_C_HEX;
                    break;
                case '\'':
                    fpos++;
                    pad.what = format_str + fpos;
                    while (1)
                    {
                        if (! format_str[fpos])
                            error_occurred(ERR_UNEXPECTED_EOS);
                        if (format_str[fpos] == '\\')
                        {
                            if (! format_str[++fpos])
                                error_occurred(ERR_UNEXPECTED_EOS);
                        } else
                        if (format_str[fpos] == '\'')
                        {
                            pad.len = format_str + fpos - pad.what;
                            if (!pad.len)
                                error_occurred(ERR_NULL_PS);
                            break;
                        }
                        fpos++;
                    }
                    break;
                default:
                    finfo |= INFO_T_ERROR;
                }
            }                   /* end of for () */
            if (pres < 0)
                error_occurred(ERR_PRES_EXPECTED);
            /*
             * now handle the different arg types...
             */
            while (1)
            {
                if ((finfo & INFO_T) == INFO_T_ANY)
                {
                    String str = type_value(m_current_arg);
                    m_current_arg = &str;
                    finfo ^= INFO_T_ANY;
                    finfo |= INFO_T_STRING;
                }
                if ((finfo & INFO_T) == INFO_T_ERROR)
                {
                    error_occurred(ERR_INVALID_FORMAT_STR);
                } else
                if ((finfo & INFO_T) == INFO_T_STRING)
                {
                    /*
                     * %s null handling added 930709 by Luke Mewburn
                     * <zak@rmit.oz.au>
                     */
                    const char *arg_str = 0;
                    if (m_current_arg->m_type == ValueType::INTEGER && m_current_arg->m_int == 0)
                    {
                        arg_str = "(Null)";
                    } else
                    if (m_current_arg->m_type != ValueType::STRING)
                        error_occurred(ERR_INCORRECT_ARG_S);
                    else
                    {
                        arg_str = m_current_arg->m_string->c_str();
                    }

                    add_justified(arg_str, strlen(arg_str),
                                  &pad, fs, finfo,
                                  (((format_str[fpos] != '\n') && (format_str[fpos] != '\0'))));
                } else
                if ((finfo & INFO_T) == INFO_T_IP)
                {
                    char temp[100];

                    if (m_current_arg->m_type != ValueType::INTEGER)
                        throw_error("error_occurred: (s)printf(): Incorrect argument(%d) type to %%ip.\n",
                                    (int) m_current_arg_index + 1);
                        
                    sprintf(temp, "%u.%u.%u.%u",
                            (int) ((m_current_arg->m_int >> 24) & 0xFF),
                            (int) ((m_current_arg->m_int >> 16) & 0xFF),
                            (int) ((m_current_arg->m_int >> 8)  & 0xFF),
                            (int) ((m_current_arg->m_int) & 0xFF));

                    add_justified(temp, strlen(temp), &pad, fs, finfo,
                                  (((format_str[fpos] != '\n') && (format_str[fpos] != '\0'))));
                } else
                if (finfo & INFO_T_INT)
                {
                    /* one of the integer * types */
                    char cheat[16];
                    char temp[128];
                    Integer val;
                    Real real;

                    /* Get number */
                    if (m_current_arg->m_type == ValueType::INTEGER)
                        val = m_current_arg->m_int;
                    else
                    if (m_current_arg->m_type == ValueType::REAL)
                        val = (Integer) m_current_arg->m_real;
                    else
                        throw_error("error_occurred: (s)printf(): Incorrect argument(%d) type to %%d.\n",
                                    (int) m_current_arg_index + 1);

                    switch (finfo & INFO_T)
                    {
                    case INFO_T_INT:
                        int64_to_string(temp, sizeof(temp), val, 10, 1, 0);
                        break;

                    case INFO_T_UINT:
                        int64_to_string(temp, sizeof(temp), val, 10, 0, 0);
                        break;

                    case INFO_T_CHAR:
                        temp[0] = (char) val;
                        temp[1] = 0;
                        break;

                    case INFO_T_OCT:
                        int64_to_string(temp, sizeof(temp), val, 8, 0, 0);
                        break;

                    case INFO_T_HEX:
                        int64_to_string(temp, sizeof(temp), val, 16, 0, 0);
                        break;

                    case INFO_T_C_HEX:
                        int64_to_string(temp, sizeof(temp), val, 16, 0, 1);
                        break;

                    case INFO_T_FLOAT:
                        /* A float */
                        *cheat = '%';
                        i = 1;
                        switch (finfo & INFO_PP)
                        {
                        case INFO_PP_SPACE:
                            cheat[i++] = ' ';
                            break;
                        case INFO_PP_PLUS:
                            cheat[i++] = '+';
                            break;
                        }
                        if (pres)
                        {
                            cheat[i++] = '.';
                            sprintf(cheat + i, "%d", (int) pres);
                            i += strlen(cheat + i);
                        }
                        cheat[i++] = 'f';
                        cheat[i] = '\0';
                        if (m_current_arg->m_type == ValueType::REAL)
                            real = (double) m_current_arg->m_real;
                        else
                            /* Convert integer to double for output */
                            real = (double) m_current_arg->m_int;
                
                        sprintf(temp, cheat, real);
                        break;

                    default:
                        error_occurred(ERR_BAD_INT_TYPE);
                    }

                    add_justified(temp, strlen(temp), &pad, fs, finfo,
                                  (((format_str[fpos] != '\n') && (format_str[fpos] != '\0'))));
                } else
                    /* type not found */
                    error_occurred(ERR_UNDEFINED_TYPE);
                break;
            } /* end of while (1) */
            last = fpos;
            fpos--; /* bout to get incremented */
        }
    } /* end of for (fpos=0; 1; fpos++) */

    return String((char_t *)m_obuf.get_array_address(0), m_obuf.size());
} /* end of formatted */

// Output Value
String Output::type_value(const Value *value)
{
    type_value_at(value, 0);
    return String((char_t *)m_obuf.get_array_address(0), m_obuf.size());
}

/* output vm value */
void Output::type_value_at(const Value *value, size_t ident)
{
    char buffer[80];
    size_t i;

    if (ident > 0)
        // Pad space
        add_pad(0, ident);

    if (value->is_reference_value())
    {
        // Check loop
        if (m_check_loop.contains(value->m_reference))
        {
            // This value was already print, just print address
            add_char('<');
            add_c_str(Value::type_to_name(value->m_type));
            add_char(':');
            add_number((Integer)value->m_reference, "0x%llX");
            add_char('>');
            return;
        }
        m_check_loop.put(value->m_reference);
    }

    switch (value->m_type)
    {
    case ValueType::NIL:
        add_c_str("nil");
        break;

    case ValueType::INTEGER:
        add_number(value->m_int);
        break;

    case ValueType::REAL:
        sprintf(buffer, "%g", value->m_real);
        add_c_str(buffer);
        break;

    case ValueType::OBJECT:
        value->m_oid.print(buffer, sizeof(buffer), "Obj");
        add_c_str(buffer);
        break;

    case ValueType::STRING:
        add_char('"');
        add_string(value->m_string);
        add_char('"');
        break;

    case ValueType::BUFFER:
    {
        size_t k, j;
        size_t n;
        unsigned char c, c1, c2, *cp;
        BufferImpl *bp;

        bp = value->m_buffer;
        n = bp->len;
        if (! n)
        {
            add_c_str("<Empty buffer>");
            break;
        }

        if (n > 512)
        {
            sprintf(buffer, "<Buffer:%zu>", n);
            add_c_str(buffer);
            break;
        }

        for (i = 0, k = 0, cp = bp->data(); i < n; i++, cp++)
        {
            /* Append a char with HEX mode */
            c = *cp;
            c1 = c >> 4;
            c2 = c & 0x0F;
            if (c1 < 10) c1 += '0'; else c1 += 'A' - 10;
            if (c2 < 10) c2 += '0'; else c2 += 'A' - 10;
            buffer[k++] = c1;
            buffer[k++] = c2;
            if ((i & 0x0F) == 0x0F)
            {
                /* Reach tail of line */
                buffer[k++] = ' ';
                buffer[k++] = ' ';
                buffer[k++] = ' ';
                buffer[k++] = ' ';
                for (j = 0; j < 16; j++)
                {
                    c = cp[j - 15];
                    if (c < 32 || c == 127 || c == 255)
                        c = '.';
                    buffer[k++] = c;
                }

                /* New line */
                buffer[k++] = ' ';
                buffer[k++] = '\n';
                buffer[k++] = 0;
                add_c_str(buffer);

                if (i < n - 1)
                    /* Should be add ident for next line */
                    add_pad(0, ident);

                /* Reset k */
                k = 0;
            } else
                buffer[k++] = ((i & 7) == 7 && (i != n - 1)) ? '-' : ' ';
        }

        if (k)
        {
            /* Line not finished, add space to finish it */
            while (i & 0x0F)
            {
                buffer[k++] = ' ';
                buffer[k++] = ' ';
                buffer[k++] = ' ';
                i++;
                cp++;
            }
            buffer[k++] = ' ';
            buffer[k++] = ' ';
            buffer[k++] = ' ';
            for (j = 0; j < 16; j++)
            {
                c = (i - 16 + j < n) ? cp[j - 16] : '.';
                if (c < 32 || c == 127 || c == 255)
                    c = '.';
                buffer[k++] = c;
            }
            buffer[k++] = '\n';
            buffer[k++] = 0;
            add_c_str(buffer);
        }
        break;
    }

    case ValueType::ARRAY:
    {
        ArrayImpl& arr = *value->m_array;
        if (! arr.size())
        {
            add_c_str("{ }");
        } else
        {
            add_c_str("{ /* sizeof() == ");
            add_number(arr.size());
            add_c_str(" */\n");
            for (auto it = arr.a.begin(); it != arr.a.end(); ++it)
            {
                type_value_at(&(*it), ident + 4);
                add_c_str(",\n");
            }
            add_pad(0, ident);
            add_c_str("}");
        }
        break;
    }

    case ValueType::MAPPING:
    {
        MapImpl& map = *value->m_map;
        if (! (map.size()))
        {
            add_c_str("[ ]");
        } else
        {
            add_c_str("[ /* sizeof() == ");
            add_number(map.size());
            add_c_str(" */\n");

            for (auto it = map.m.begin(); it != map.m.end(); ++it)
            {
                type_value_at(&it->first, ident + 4);
                add_c_str(" : ");
                type_value_at(&it->second, ident + 4);
                add_c_str(",\n");
            }

            add_pad(0, ident);
            add_c_str("]");
        }
        break;
    }

    case ValueType::FUNCTION:
    {
        add_c_str(" (:");
        add_c_str(" function ");
        add_c_str(" :)");
        break;
    }

    default:
        throw_error("Bad type of value.\n");
        break;
    }
}

} // End of namespace: cmm
