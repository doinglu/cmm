// cmm_efun.h

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm_basic_types.h"
#include "cmm_operate.h"

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
    const char *prototype;
} EfunDef;

class Efun
{
public:
    static int init();
    static void shutdown();

public:
    // Add an efun
    static bool add_efun(const String& prefix, EfunEntry entry, const String& prototype_text);

    // Add multiple efuns (array end by 0)
    static void add_efuns(const String& package_name, EfunDef *efun_def_array);

    // Parse prototype & generate function
    static bool parse_efun_prototype(const String& text, Function **ptr_function);

private:
    typedef simple::hash_map<String, Function *> EfunMap;
    static EfunMap *m_efun_map;
};

}
