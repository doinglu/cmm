// mts.cpp : Defines the entry point for the console application.
//

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#include <stdio.h>
#include <stdlib.h>
#include "std_template/simple_hash_map.h"
#include "std_template/simple_list.h"
#include "std_template/simple_string.h"
#include "std_port/std_port.h"
#include "std_port/std_port_type.h"
#include "std_memmgr/std_memmgr.h"
#include "cmm_buffer_new.h"
#include "cmm_domain.h"
#include "cmm_efun.h"
#include "cmm_memory.h"
#include "cmm_object.h"
#include "cmm_thread.h"
#include "cmm_value.h"

#include "a_name_2.h"
#include "a_desc_2.h"
#include "a_entity_2.h"

long long GetUsCounter()
{
    return std_get_current_us_counter();
}

#define LOCK //lock

namespace std
{

template<>
struct hash<simple::string>
{
    size_t operator()(const simple::string& s) const
    {
        return s.hash_value();
    }
};

}

using namespace cmm;

AutoInitMemoryManager autoInitMemoryManager;

//typedef simple::hash_map<int, int> hmap;
typedef std::unordered_map<int, int> hmap;
long long t;
typedef Integer (*cpp_func_t)(size_t n);
cpp_func_t cpp_func = 0;

#if 0
void IncSum(Test *ob)
{
#if 1
    //hmap table;
    auto thread = Thread::get_current_thread();
    hmap table;
    long long sum = 0;
	cmm::Integer count;
///	Value aaa = new_local_string_value("aaa");
    int to_n = 50;
    int use_cpp = 0;
	for (int i = 0; i < 100; i++)
	{
        if (use_cpp)
        {
            count = cpp_func(to_n);
        }
        else
        {
			count = call(thread, ob, &Test::calc_prime2, to_n).to_int().m_int;
        }
        //        XDELETE(XNEW(int));
        //std::string s("sum");
        //simple::string s("sum");
//        table["sum"]++;
//        table[i] = i;

//        memset(str, 0, sizeof(str));
//        if (memcmp(str, str2, sizeof(str)) != 0)
//            t++;
//        t++;
	}
    t = count;
	//    for (int i = 0; i < 10; i++)
//        table.erase(i);
//    t += sum;
//    if (str[0] != 0)
//        t++;
#endif
}

static void (*fptr)(Test *ob) = IncSum;
IntR threadCount = 0;

void Entry(Test *ob)
{
    long long b, e;
    Thread thread;
    thread.start();
    b = GetUsCounter();
    for (int i = 0; i < 10; i++)
    {
        fptr(ob);
    }
    e = GetUsCounter();
    thread.stop();
    
//    printf("sum = %lld\n", table["sum"]);
    printf("cost = %lld\n", (e - b));
    std_cpu_lock_add(&threadCount, -1);
}
#endif

namespace cmm
{
int conflict = 0;
}

class AAA
{
public:
    AAA(int _n)
    {
        printf("AAA constructor: n = %d\n", _n);
        n = _n;
    }

    AAA() :
        AAA(0)
    {
    }

    ~AAA()
    {
        printf("AAA destructor: n = %d\n", n);
    }

    int n;
};

int main(int argn, char *argv[])
{
    static bool flag = 1;

#if 0
    simple::vector<int> arr;
    arr.push_back(123);
    arr.push_back(456);
    for (auto it = arr.begin(); it != arr.end(); ++it)
       printf("value = %d\n", *it);
    return 0;
#endif

#if 0
    simple::hash_map<simple::string, int> m;
    m["123"] = 0;
    m["123"]++;
    printf("%d\n", m["123"]);
    return 0;
#endif

#if 0
    simple::list<int> list;
    list.append(1);
    list.append(3);
    list.append(5);
    list.remove_at(2);
    list.remove_at(1);
    list.remove_at(0);
    for (auto it = list.begin(); it != list.end(); it++)
        printf("%zu. %d\n", it.get_index(), (int) *it);
    return 0;
#endif

    int maxThreadCount = 2;
    if (argn < 2)
	{
		printf("usage: exe n=thread.\nDefault use %d threads.\n", maxThreadCount);
	} else
    {
        maxThreadCount = atoi(argv[1]);
	    if (maxThreadCount < 1)
	    {
		    printf("Bad thread count %d, expected >= 1.\n", maxThreadCount);
		    return -1;
	    }
    }

    Domain::init();
    Object::init();
    Thread::init();
    Program::init();
    Efun::init();

    auto *thread = Thread::get_current_thread();
    thread->update_start_sp_of_start_context(&argn);

#if 1
    __clone_entity_ob::create_program();
    __feature_desc_ob::create_program();
    __feature_name_ob::create_program();

    Program::update_all_callees();
#endif

    auto *a1 = BUFFER_NEW(AAA, 888);
#if 1
    auto *a2 = BUFFER_NEWN(AAA, 3);
    auto *a11 = BUFFER_ALLOC(a1);
    auto *a21 = BUFFER_ALLOC(a2);
    BUFFER_DELETE(a1);
    BUFFER_DELETEN(a2);
    a11->bind_to_current_domain();
    a21->bind_to_current_domain();
    a11 = 0;
#endif
    printf("Start GC...\n");
    Thread::get_current_thread_domain()->gc();
    printf("End GC...\n");
    //printf("a1 = %p, a2 = %p, a11 = %p, a21 = %p\n", a1, a2, a11, a21);
    printf("&argn = %p\n", (Uint8*) &argn - 0x20);
    printf("thread start = %p, end = %p\n",
           thread->get_this_context()->value.m_start_sp,
           thread->get_this_context()->value.m_end_sp);
    printf("&a1 = %p\n", &a1);

#if 0
    auto *domain = XNEW(Domain);
    auto *program = Program::find_program_by_name(Value("/clone/entity").m_string);
    auto *ob = program->new_instance(domain);
    auto *domain2 = XNEW(Domain);
    auto *ob2 = program->new_instance(domain2);
    call_other(thread, ob->get_oid(), "create");
    call_other(thread, ob2->get_oid(), "create");
    Value ret = call_other(thread, ob->get_oid(), "test_call", ob2->get_oid());
    printf("ret = %d.\n", (int) ret.m_int);
    XDELETE(ob);
#endif

    Efun::shutdown();
    Program::shutdown();
    Thread::shutdown();
    Domain::shutdown();
    Object::shutdown();
    return 0;

#if 0
    simple::vector<Domain *> domains;
    simple::vector<Test *> test_obs;
    domains.push_back(XNEW(Domain));
    test_obs.push_back(XNEW(Test));
    test_obs[0]->set_domain(domains[0]);

    printf("Single thread cost:\n");
//    table["sum"] = 0;
//    table[7] = 1;
#if 1
    threadCount = 1;
    std_create_task(NULL, NULL, (void *)Entry, test_obs[0]);
    while (threadCount > 0)
        std_sleep(1);
#endif

    printf("\nMulti threads cost:\n");
//    table["sum"] = 0;
    for (auto i = 1; i <= maxThreadCount; i++)
    {
        domains.push_back(XNEW(Domain));
        test_obs.push_back(XNEW(Test));
        test_obs[i]->set_domain(domains[0]);
        std_create_task(NULL, NULL, (void *) Entry, test_obs[i]);
    }
    threadCount = maxThreadCount;
    while (threadCount > 0)
        std_sleep(1);
    printf("t = %lld\n", t);
    printf("Conflict = %d\n", conflict);

    // Close all domains
    for (auto i = 0; i < domains.size(); i++)
    {
        domains[i]->gc();
        XDELETE(domains[i]);
        XDELETE(test_obs[i]);
    }
#endif

    Thread::shutdown();
    Program::shutdown();
    Object::shutdown();
    Domain::shutdown();
    return 0;
}
