// cmm_efun.h

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm_basic_types.h"
#include "cmm_operate.h"

namespace cmm
{

#define DECLARE_EFUN(name) \
    extern Value efun_##name(Thread *_thread, Value *__args, ArgNo __n);

#define DEFINE_EFUN(type, name, arguments) \
    const char *efun_##name##_prototype = #type " " #name #arguments; \
    Value efun_##name(Thread *_thread, Value *__args, ArgNo __n)

#define EFUN_ITEM(name) \
    efun_##name, efun_##name##_prototype

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

namespace PrototypeGrammar
{
struct Prototype;
}

class Efun
{
public:
    static int init();
    static void shutdown();

public:
    // Add an efun
    static bool add_efun(Program *program, const String& prefix, EfunEntry entry, const String& prototype_text);

    // Add multiple efuns (array end by 0)
    static void add_efuns(const String& package_name, EfunDef *efun_def_array);

    // Parse prototype & generate function
    static bool parse_efun_prototype(const String& text, PrototypeGrammar::Prototype *ptr_prototype);

public:
    // Invoke
    static Value invoke(Thread *thread, Value& function_name, Value *args, ArgNo n);

private:
    typedef simple::hash_map<StringImpl *, Program *> EfunPackage;
    static EfunPackage *m_efun_packages;

    typedef simple::hash_map<StringImpl *, Function *> EfunMap;
    static EfunMap *m_efun_map;
};

}
