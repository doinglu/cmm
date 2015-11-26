// cmm_lex_util.cpp
// Initial version 2006.7.19 by doing
// Immigrated 2015.11.5 by doing

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "cmm_lex_util.h"

namespace cmm
{

/* Type of char */
enum CharType
{
    CHAR_WORD      = 1,
    CHAR_QUOTATION = 2,
    CHAR_OPERATOR  = 3,
    CHAR_SPACE     = 4,
};

/* Internal routines */
static CharType cmm_get_char_type(char ch);

/* Lex utilities functions */

/* Merge n sub strings to a string */
/* The sub strings must be enclosed with quotation.
 * For example:
 *   str = "aaa" "bbb" "ccc"
 * After merge:
 *   str = "aaabbbccc"
 */
bool cmm_merge_quoted_strings(char *str)
{
    char ch, *src, *dst;
    bool quote = false, added = false;

    src = str;
    dst = str;
    while ((ch = *(src++)))
    {
        if (ch == '\"')
        {
            /* Revert flag */
            quote = ! quote;
            if (! added)
            {
                /* Encounter first quotation */
                added = true;
                *(dst++) = ch;
            }
            continue;
        }

        if (! quote)
        {
            if (! isspace((unsigned char) ch))
                /* Not white space out of quoted part, can't merge strings */
                return false;

            /* Skip space */
        } else
            /* Copy character within string */
            *(dst++) = ch;
    }

    if (! added)
        /* Not found qutation */
        return false;

    if (quote)
        /* Quotation not matched */
        return false;

    /* Enclose string */
    *(dst++) = '\"';
    *(dst++) = 0;
    return true;
}

/* Get a token */
/* All chars are seprated two following classes:
 * 1. WORD      (digit, letter or underline, extend chars)
 * 2. QUOTATION (")
 * 3. OPERATOR  (other chars)
 * For each class exception QUOTATION, the routine would scan until
 * encounter other class chars.
 * For QUOTATION, the routine would scan until encounter next QUOTATION */
bool cmm_get_token(char *token, size_t size, char **ppat)
{
    char *p, *pb;
    size_t len;

    STD_ASSERT(token != NULL);
    STD_ASSERT(size > 1);

    p = *ppat;
    while (*p && isspace((unsigned char) *p))
        /* Skip space */
        p++;

    if (p != *ppat)
    {
        /* Return a vacuolar string */
        *ppat = p;
        token[0] = ' ';
        token[1] = 0;
        return true;
    }

    if (! *p)
    {
        /* Empty string */
        *ppat = p;
        token[0] = 0;
        return true;
    }

    /* Save begin pointer of token */
    pb = p;
    switch (cmm_get_char_type(*(p++)))
    {
    case CHAR_WORD:
        /* Get until encounter other class char */
        while (*p && (*p == '.' || cmm_get_char_type(*p) == CHAR_WORD))
            p++;
        break;

    case CHAR_QUOTATION:
        while (*p)
        {
            if (*p == '\\' && *(p + 1))
            {
                /* Skip \" */
                p += 2;
                continue;
            }

            if (cmm_get_char_type(*p) == CHAR_QUOTATION)
            {
                /* String closed */
                p++;
                break;
            }

            /* Skip current char */
            p++;
        }
        break;

    case CHAR_OPERATOR:
        while (*p && cmm_get_char_type(*p) == CHAR_OPERATOR)
            p++;
        break;

    default:
        STD_FATAL("Bad result of character.\n");
        return false;
    }

    /* Calculate length of token */
    len = p - pb;
    if (len >= size)
    {
        /* String is too long to fill into token buffer */
        STD_TRACE("Token word is too long (len = %zu, size = %zu) to fetch.\n", len, size);
        return false;
    }

    /* Copy token into buffer */
    memcpy(token, pb, len);
    token[len] = 0;

    /* Set result pointer */
    *ppat = p;
    return true;
}

/* Add input @ *ppat in buffer text[0..size - 1], the *ppat may be moved
 * after operation */
bool cmm_append_input_at_buffer_head(char *text, size_t size, char **ppat, const char *content)
{
    char *p = *ppat;
    size_t  used_len;
    size_t  content_len;

    /* Get used data length in text @ *ppat */
    used_len = strlen(p);

    /* Assure ppat is in buffer text */
    STD_ASSERT(*ppat >= text && *ppat < text + size);

    /* Assure buffer is not overflow */
    STD_ASSERT(p + used_len < text + size);

    /* Skip space of content */
    while (isspace((unsigned char) *content))
        content++;
    content_len = strlen(content);

    if (used_len + content_len >= size)
        /* The content can't be inserted into text buffer */
        return false;

    if (p < text + content_len)
    {
        /* The content can't be insert now, I must move ppat pointer first */
        memmove(text + content_len, p, used_len + 1 /* Include terminator */);
        memcpy(text, content, content_len);
        *ppat = text;
    } else
    {
        /* Just append @ head */
        p -= content_len;
        *ppat = p;
        memcpy(p, content, content_len);
    }

    STD_ASSERT(*ppat >= text && *ppat < text + size);
    return true;
}

/* Return class of char */
static CharType cmm_get_char_type(char ch)
{
    /* Never check for terminor '\0' */
    STD_ASSERT(ch != 0);

    if (isspace((unsigned char) ch))
        return CHAR_SPACE;

    if (isdigit((unsigned char) ch) ||
        isalpha((unsigned char) ch) ||
        ch == '_' ||
        (ch & 0x80))
        /* Digit, letter or underline, extend chars */
        return CHAR_WORD;

    if (ch == '\"')
        /* Quotation */
        return CHAR_QUOTATION;

    /* All others chars */
    return CHAR_OPERATOR;
}

}
