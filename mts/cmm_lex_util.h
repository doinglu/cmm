// cmm_lex_util.h
// Initial version 2006.7.19 by doing
// Immigrated 2015.11.5 by doing

#pragma once

#include "cmm.h"

namespace cmm
{

/* Lex utilities functions */
bool cmm_merge_quoted_strings(char *str);
bool cmm_append_input_at_buffer_head(char *text, size_t size, char **ppat, const char *content);
bool cmm_get_token(char *token, size_t size, char **ppat);
void cmm_trim_to(char* str, size_t size, const char* from);

}
