// cmm_efun.h

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm_basic_types.h"
#include "cmm_operate.h"
#include "cmm_string_ptr.h"

namespace cmm
{

class Function;
class Thread;
class Value;

// External function entry
typedef Value (*EfunEntry)(Thread *_thread, Value *_args, ArgNo n);

// Efun definition
typedef struct
{
    EfunEntry entry;
    const char *name;
    const char *prototype;
} EfunDef;

class Efun
{
public:
    static int init();
    static void shutdown();

public:
    // Add multiple efuns (array end by 0)
    static void add_efuns(const Value& package_name_value, EfunDef *efun_def_array);

    // Parse prototype & generate function
    static void parse_efun_prototype(Function *function, const Value& prototype);

    // Parse string text to words
    static Array parse_words(String& text);

private:
    typedef simple::hash_map<String, Function *> EfunMap;
    static EfunMap *m_efun_map;
};

}
