// std_memmgr.c:
// Initial version 2002.2.24 by doing

 // Module header files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "std_port/std_port.h"
#include "std_memmgr/std_memmgr.h"
#include "std_memmgr/std_bin_alloc.h"

typedef enum std_block_size
{
    STD_BSIZE_0         = 32,
    STD_BSIZE_1         = 64,
    STD_BSIZE_2         = 128,
    STD_BSIZE_3         = 256,
    STD_BSIZE_4         = 512,
    STD_BSIZE_5         = 1024,
    STD_BSIZE_6         = 2048,
    STD_BSIZE_7         = 4096,
    STD_BSIZE_8         = 8192,
    STD_BSIZE_9         = 16384,
} std_block_size_t;

// Configuration of memory block serials
std_memory_config_t mem_config[] =
{
    // _block size       _block count
    { STD_BSIZE_0,      0,      },
    { STD_BSIZE_1,      0,      },
    { STD_BSIZE_2,      0,      },
    { STD_BSIZE_3,      0,      },
    { STD_BSIZE_4,      0,      },
    { STD_BSIZE_5,      0,      },
    { STD_BSIZE_6,      0,      },
    { STD_BSIZE_7,      0,      },
    { STD_BSIZE_8,      0,      },
    { STD_BSIZE_9,      0,      },
};

// _memory to waste
// Waste memory to shift the offset of flat memory
#define STD_WASTE_SIZE          0

// The minimum size of blocks-cluster when allocted blocks in _l2
#define STD_BLOCKS_CLUSTER_SIZE 65536

#if STD_STAT_ALLOC
// Do stat for debug mode
static mem_code_node_stat_t *_alloc_nodes = NULL;
static size_t _alloc_nodes_count = 0;
static size_t _alloc_nodes_size = 0;
static size_t _allocMin_size = 0, _alloc_max_size = (size_t) -1;
#endif

// _memory group structure
typedef struct std_memory_grp
{
    Uint32    block_size;
    Uint32    block_count;
    UintR     peak_used;
    UintR     used;
    UintR     dropped;
    UintR     peak_freedByOther;
    UintR     freedByOther;
    std_memory_header_t *free_list;
    std_memory_header_t *free_listByOtherThreads;
} std_memory_grp_t;

// Extend block group structure
typedef struct std_Extend_grp
{
    int block_size;
    int block_count;
    struct std_Extend_grp *next;
} std_Extend_grp_t;

// Local memory storage
typedef struct std_lms
{
    std_task_id_t     task_id;    
    std_memory_grp_t  mem_grp[STD_SIZE_N(mem_config)];
    std_mem_stat_t    stat;
    struct std_lms   *next;
} std_lms_t;

#if STD_BLOCK_DETAIL
static std_memory_header_t *_l2_mem_blocks = NULL;
#endif

// Flags for initialization
#define MEMMGR_NOT_INIT      0
#define MEMMGR_INIT_OK       1
#define MEMMGR_SHUTDOWNING  -1

// Initialization flag
static int    _init_mem_mgr_flag = MEMMGR_NOT_INIT;
static int    _mem_mgr_options = 0;
static Uint8 *_waste_memory = NULL;
static size_t _block_groups = STD_SIZE_N(mem_config);
static size_t _max_block_size = STD_BSIZE_9;

static std_bin_alloc_t _ba_pool;

// Global information for memory manager
static std_spin_lock_t _code_node_stat_spin_lock;
static std_spin_lock_t _tiny_block_spin_lock;
static std_spin_lock_t _l2_mem_block_list;
static std_spin_lock_t _lms_list_spin_lock;

// _tls for lms
static std_tls_t  _lms_tls_id;
static std_lms_t *_lms_list = NULL;

// _statistic for tiny
static std_mem_stat_t _tiny_mem_stat;

// _statistic for extend information
static size_t _extend_times  = 0;
static size_t _extend_size   = 0;

// Extend group list
static std_Extend_grp_t *_extend_grp_list = NULL;

// _tiny block page list
static std_tiny_page_header_t *_tiny_block_page_lists[STD_TINY_BLOCK_PAGE_LIST_COUNT / STD_TINY_ALIGNMENT_SIZE + 1];

// The heap to put stablize memory block
// _allocation can't be free during whole process life cycle
static Uint8 *_stable_heapStart = NULL;
static size_t _stable_heap_size = 0;

// _statistics for stable heap
static size_t _stable_heap_allocated_size = 0;
static size_t _stable_heap_free_list_times = 0;
static size_t _stable_heap_allocate_times = 0;
static size_t _stable_heap_allocateFailed = 0;

// Internal functions
static void* _std_internal_alloc(size_t size);
static void  _std_internal_free(void* p, size_t size);
static std_memory_header_t *_std_l2_allocate_memory(size_t size);
static void  _std_l2_free_memory(std_memory_header_t *block);
static std_memory_header_t *_std_extend_blocks_of_group(std_lms_t *lms, Uint n_grpNo);
static void  _std_get_mem_stat(std_mem_stat_t *mem_stat);
static int   _std_is_blockIn_list(std_memory_header_t *block, const char *module_name, const char *file, int line);
static char *_std_last_name(const char *file);
static int   _std_stricmp(const char *s1, const char *s2);
static void  _std_show_mem_block(Uint8 *buf, size_t len);
static void  _std_dump_mem_block(FILE *fp, Uint8 *buf, size_t len);
static char *_std_ctime(time_t *pTime);

// _spin lock routines for memory group
static void _std_get_all_mem_spin_lock();
static void _std_release_all_mem_spin_lock();

// To manager local memory storage
static std_lms_t *_std_create_lms();
static std_lms_t *_std_get_or_create_lms();
static int        _std_destroy_lms(std_lms_t *lms);

// _tiny block routines
static std_tiny_page_header_t *std_create_tiny_block_page(size_t tiny_block_size);
static void std_delete_tiny_block_page(std_tiny_page_header_t *page);

#if STD_STAT_ALLOC
// Internal statistics routines
static mem_code_node_stat_t *std_find_or_insert_code_node(const char *file, int line);
static mem_code_node_stat_t *std_insert_code_node(mem_code_node_t *comp, size_t index);
static void                  std_add_code_node_stat_counter(const char *file, int line, int delta, size_t size);

// Find the node, if not found, insert one, return NULL
// if failed to allocate memory
static mem_code_node_stat_t *std_find_or_insert_code_node(const char *file, int line)
{
    mem_code_node_t comp;
    int m;
    int ret;

    // Generate node to compare
    comp.file = file;
    comp.line = (Uint32) line;

    if (_alloc_nodes_count > 0)
    {
        // Search the node
        int b, e;
        b = 0;
        e = (int)_alloc_nodes_count - 1;
        for (;;)
        {
            m = (b + e) / 2;

            // Compare the code node
            ret = memcmp(&comp, &_alloc_nodes[m].n, sizeof(comp));
            if (ret == 0)
            {
                // Found
                return &_alloc_nodes[m];
            } else
            if (ret < 0)
            {
                // Try more left
                e = m - 1;
                if (b > e)
                    // Can't move left anymore, insert @ b
                    return std_insert_code_node(&comp, b);

                continue;
            } else
            {
                // Try more right
                b = m + 1;
                if (b > e)
                    // Can't move right anymore, insert @ >e
                    return std_insert_code_node(&comp, e + 1);

                continue;
            }
        }
        // Never be here
    }

    // _list is empty
    return std_insert_code_node(&comp, 0);
}

// _insert code node into _alloc_nodes
// Index must in [0.._alloc_nodes_count]
static mem_code_node_stat_t *std_insert_code_node(mem_code_node_t *comp, size_t index)
{
    mem_code_node_stat_t *pNew;

    STD_ASSERT(index >= 0 && index <= _alloc_nodes_count);
    if (_alloc_nodes_count >= _alloc_nodes_size)
    {
        // Reallocate memory
        size_t new_size;

        // Get new size of the code node list
        if (_alloc_nodes_size == 0)
            new_size = 16;
        else
            new_size = _alloc_nodes_size * 2;

        pNew = (mem_code_node_stat_t *) OS_REALLOC(_alloc_nodes, new_size * sizeof(mem_code_node_stat_t));
        if (pNew == NULL)
            // Failed to allocate
            return NULL;

        // _allocate is done
        _alloc_nodes = pNew;
        _alloc_nodes_size = new_size;
    }

    // Move the memory
    memmove(&_alloc_nodes[index + 1],
            &_alloc_nodes[index],
            (_alloc_nodes_count - index) * sizeof(mem_code_node_stat_t));
    _alloc_nodes_count++;

    // _insert the node & return
    memset(&_alloc_nodes[index], 0, sizeof(mem_code_node_stat_t));
    memcpy(&_alloc_nodes[index].n, comp, sizeof(*comp));
    return &_alloc_nodes[index];
}

// Update counter of allocation times at specified code node
static void std_add_code_node_stat_counter(const char *file, int line, int delta, size_t size)
{
    mem_code_node_stat_t *p_stat;

    STD_ASSERT(delta == 1 || delta == -1);

    std_get_spin_lock(&_code_node_stat_spin_lock);

    do
    {
        if (size < _allocMin_size || size > _alloc_max_size)
            // Not in specified range, ignore
            break;

        // Find the code node
        p_stat = std_find_or_insert_code_node(file, line);
        if (p_stat == NULL)
            // Failed to find the code node
            break;

        // Update counter
        if (delta > 0)
        {
            // _allocation
            p_stat->counter++;
            p_stat->size += size;
        } else
        if (delta < 0)
        {
            // _free
            p_stat->counter--;
            p_stat->size -= size;
        }
        // Done
    } while (0);

    std_release_spin_lock(&_code_node_stat_spin_lock);
}

#endif

// Initialize memory manger
int std_init_mem_mgr(std_init_mgr_para_t* paras)
{
    int i;

    if (_init_mem_mgr_flag != MEMMGR_NOT_INIT)
        // ALread initialized, return OK
        return STD_ALREADY_INITIALIZED;

    // Set the options
    if (paras == NULL)
    {
        // Use default options
        _mem_mgr_options = 0;
    } else
    {
        // Use specified options
        _mem_mgr_options = paras->options;
        if (_mem_mgr_options & STD_USE_BA_ALLOC)
        {
            if (!std_ba_create(&_ba_pool, paras->ba_reserve_size))
                STD_FATAL("Failed to create bin-alloc pool.\n");
            printf("Create bin-alloc pool, size = 0x%zx.\n", paras->ba_reserve_size);
        }
    }

    // Assure the configuration is correct
    // In debug mode, STD_BLOCK_RESERVED must > 0 & =x4
    STD_ASSERT(STD_BLOCK_RESERVED % STD_MEMORY_ALIGNMENT_SIZE == 0);

    STD_ASSERT(sizeof(std_memory_header_t) % STD_MEMORY_ALIGNMENT_SIZE == 0);

    if (! std_allocate_tls(&_lms_tls_id))
        STD_FATAL("Failed to allocate tls for lms.\n");

    // _allocate the FLAT memory
    _waste_memory = (Uint8 *) OS_MALLOC(STD_WASTE_SIZE);

    if (_waste_memory == NULL)
        STD_FATAL("PDB can not allocate the waste memory\n");

    memset(&_tiny_mem_stat, 0, sizeof(std_mem_stat_t));

    // Initialize page list
    // Only the valid page can be used (aligment by machine register length) such as
    // page[0] page[4] page[9] for 32 bits machine
    for (i = 0; i < STD_SIZE_N(_tiny_block_page_lists); i++)
        _tiny_block_page_lists[i] = NULL;

#if STD_BLOCK_DETAIL
    // Initialize l2 first block pointer
    _l2_mem_blocks = NULL;
#endif

    // Initialize the critical section
    std_init_spin_lock(&_code_node_stat_spin_lock);
    std_init_spin_lock(&_tiny_block_spin_lock);
    std_init_spin_lock(&_l2_mem_block_list);
    std_init_spin_lock(&_lms_list_spin_lock);

    // Initialize ok
    _init_mem_mgr_flag = MEMMGR_INIT_OK;
    return 0;
}

// Shutdown the manager
int std_shutdown_mem_mgr()
{
    std_Extend_grp_t    *extend_grp;
    std_memory_header_t *header;
    std_lms_t           *lms;
    Uint8  *p;
    int     k, n;
    int     ok;

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
        // Not initialized now? return OK
        return 0;

    // Enter protect
    _std_get_all_mem_spin_lock();

    // Set status to shutdowning
    _init_mem_mgr_flag = MEMMGR_SHUTDOWNING;

    ok = 1;

    // Check extend group
    n = 0;
    extend_grp = _extend_grp_list;
    while (extend_grp != NULL)
    {
        // Lookup all blocks in this group
        p = (Uint8 *) (extend_grp + 1);
        for (k = 0; k < extend_grp->block_count;
             k++, p += sizeof(*header) + extend_grp->block_size + STD_BLOCK_RESERVED)
        {
            // Acquire the address of block header
            header = (std_memory_header_t *) p;

            if (! (header->flags & STD_MEMORY_ALLOCATED))
                // _freed block, ignored
                continue;

            printf("%3d. 0x%p[%7d]  %3s/%s  %5d@%s [Extended]\n",
                   (int) (++n), header + 1, (int) header->size,
#if STD_BLOCK_DETAIL
                   header->module,
                   _std_ctime(&header->allocate_time),
                   (int) header->line, _std_last_name(header->file)
#else
                   "N/A",
                   "N/A",
                   0, "N/A"
#endif
            );
            _std_show_mem_block((Uint8 *) (header + 1),
                             header->size > 256 ? 256 : header->size);

            // The shutdown of PDB won't be OK
            ok = 0;
        }

        extend_grp = extend_grp->next;
    }

#if STD_BLOCK_DETAIL
    // Check _l2 memory
    if (_l2_mem_blocks != NULL)
    {
        std_memory_header_t *header;

        printf("There are still some _l2 memory not freed.\n");

        header = _l2_mem_blocks;
        while (header != NULL)
        {
            printf("%3d. 0x%p[%7d]  %3s/%s  %5d@%s [Extended]\n",
                   (int) (++n), header + 1, (int) header->size,
                   header->module,
                   _std_ctime(&header->allocate_time),
                   (int) header->line, _std_last_name(header->file)
            );
            _std_show_mem_block((Uint8 *) (header + 1),
                             header->size > 256 ? 256 : header->size);
            header = header->next_l2;
        }        

        // The shutdown of PDB won't be OK
        ok = 0;
    }
#endif

    if (! ok)
    {
        // Can not shutdown PDB gracefully
        std_mem_stat_t mem_stat;

#if STD_STAT_ALLOC
        printf("Detail allocation information from code node stat.\n");
        do
        {
            mem_code_node_stat_t *p_code_nodes;
            size_t counts, i;

            std_release_spin_lock(&_code_node_stat_spin_lock);
            std_get_code_node_stat(&p_code_nodes, &counts);
            std_get_spin_lock(&_code_node_stat_spin_lock);
            for (i = 0; i < counts; i++)
            {
                if (p_code_nodes[i].size <= 0)
                    // Skip location without memory allocated
                    continue;

                printf("%s:%d: size = %d, count = %d.\n",
                       p_code_nodes[i].n.file, p_code_nodes[i].n.line,
                       (int) p_code_nodes[i].size,
                       (int) p_code_nodes[i].counter);
            }
        } while (0);
#endif

        // std_memory_stat will lock, so we release lock now
        _std_release_all_mem_spin_lock();
        std_memory_stat(2, &mem_stat, NULL, 0);
        _std_get_all_mem_spin_lock();
        printf("Memory alloc times   = %u\n"
               "Memory free times    = %u\n"
               "Memory reserved size = %uK\n"
               "Memory peak size     = %uK\n",
               (int) mem_stat.alloc_times,
               (int) mem_stat.free_times,
               (int) ((mem_stat.total_reserved_size + 1023) / 1024),
               (int) ((mem_stat.peak_reserved_size + 1023) / 1024));

        return STD_MEMORY_BLOCK_ACTIVE;
    }

    // _free extend groups
    while (_extend_grp_list != NULL)
    {
        std_Extend_grp_t *extend_grp;

        // Take off 1 node
        extend_grp = _extend_grp_list;
        _extend_grp_list = _extend_grp_list->next;

        // _free it
        _std_internal_free(extend_grp, STD_BLOCKS_CLUSTER_SIZE);
    }

    // Don't free l2-extend blocks for debug

    // Free lms
    while ((lms = _lms_list) != NULL)
    {
        // Remove lms from list
        _lms_list = lms->next;
        lms->next = NULL;

        // Destroy the lms (also report memory leak)
        _std_destroy_lms(lms);
    }

    // _free waste memory
    OS_FREE(_waste_memory);
    _waste_memory = NULL;

    // _free stable heap
    if (_stable_heapStart != NULL)
    {
        OS_FREE(_stable_heapStart);
        _stable_heapStart = NULL;
    }

    // Delete the mutex
    std_destroy_spin_lock(&_lms_list_spin_lock);
    std_destroy_spin_lock(&_code_node_stat_spin_lock);
    std_destroy_spin_lock(&_tiny_block_spin_lock);
    std_destroy_spin_lock(&_l2_mem_block_list);

    std_free_tls(_lms_tls_id);
    _lms_tls_id = (std_tls_t) 0;

    if (_mem_mgr_options & STD_USE_BA_ALLOC)
    {
        printf("Destruct bin-alloc pool, current_used = 0x%zx, current_reserved = 0x%zx.\n",
               _ba_pool.current_used, _ba_pool.current_reserved);
        std_ba_destruct(&_ba_pool);
    }
    
    // Shutdown Ok
    _init_mem_mgr_flag = MEMMGR_NOT_INIT;
    return 0;
}

// Is the memory manager installed?
int std_is_mem_mgr_installed()
{
    return _init_mem_mgr_flag == MEMMGR_INIT_OK;
}

// Return the bytes reserved for each block
int std_get_block_reserved(size_t *ptr_reserved_bytes)
{
    *ptr_reserved_bytes = STD_BLOCK_RESERVED;
    return 0;
}

// Return the count of tiny block page lists
int std_get_tiny_block_page_lists_count(size_t *ptr_count)
{
    STD_ASSERT(ptr_count != NULL);

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
        // Not initialized yet
        return STD_NOT_INITIALIZED;

    *ptr_count = STD_SIZE_N(_tiny_block_page_lists);
    return 0;
}

// Return tiny block page list information
int std_get_tiny_block_page_list(std_tiny_page_header_t **pp_lists, Uint index)
{
    STD_ASSERT(pp_lists != NULL);

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
        // Not initialized yet
        return STD_NOT_INITIALIZED;

    STD_ASSERT(index >= 0);
    if (index >= STD_SIZE_N(_tiny_block_page_lists))
        // Bad page list
        return STD_UNKNOWN_ERROR;

    *pp_lists = _tiny_block_page_lists[index];
    return 0;
}

// How many groups are supported by me
int std_config_mem_block_groups(int new_group)
{
    STD_ASSERT(new_group >= 0);
    if (new_group >= STD_SIZE_N(mem_config))
        return STD_BAD_MEMORY_GROUP;

    _block_groups = new_group;
    if (_block_groups == 0)
        _max_block_size = 0;
    else
        _max_block_size = mem_config[_block_groups - 1].block_size;
    return 0;
}

// Config memory block information
int std_config_mem_block(size_t block_size, size_t block_count)
{
    size_t i;

    for (i = 0; i < _block_groups; i++)
    {
        if (mem_config[i].block_size == block_size)
        {
            // Found
            mem_config[i].block_count = (Uint32) block_count;
            return 0;
        }
    }

    // Failed to config
    return STD_NO_SUCH_MEMORY_GROUP;
}

// _allocate memory.
// Return NULL when PDB memory manager not initialized
void *std_allocate_memory(size_t size, const char *module_name,
                          const char *file, int line)
{
    // PDB memory manager
    std_lms_t           *lms;
    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    Uint i;

    STD_ASSERT(file != NULL);
    STD_ASSERT(module_name != NULL);

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
        return NULL;

    lms = _std_get_or_create_lms();

    STD_ASSERT(_max_block_size == 0 ||
              _max_block_size == mem_config[_block_groups - 1].block_size);
    if (size > _max_block_size)
    {
        // Oh. The memory block size is too large to be allocated
        // Use the _l2 os malloc
        block = _std_l2_allocate_memory(size);
        if (block == NULL)
            // Failed to allocate
            return NULL;
    } else
    {
        // Find out the memory group
        if (size > STD_BSIZE_3)
        {
            // 4, 5, 6, 7, 8, 9
            if (size > STD_BSIZE_7)
            {
                // 8, 9
                if (size > STD_BSIZE_8)
                    i = 9;
                else
                    i = 8;
            } else
            {
                // 4, 5, 6, 7
                if (size > STD_BSIZE_5)
                {
                    // 6, 7
                    if (size > STD_BSIZE_6)
                        i = 7;
                    else
                        i = 6;
                } else
                {
                    // 4, 5
                    if (size > STD_BSIZE_4)
                        i = 5;
                    else
                        i = 4;
                }
            }
        } else
        {
            // 0, 1, 2, 3
            if (size > STD_BSIZE_1)
            {
                // 2, 3
                if (size > STD_BSIZE_2)
                    i = 3;
                else
                    i = 2;
            } else
            {
                // 0, 1
                if (size > STD_BSIZE_0)
                    i = 1;
                else
                    i = 0;
            }
        }

        grp = &lms->mem_grp[i];
        STD_ASSERT(grp->block_size >= size);

        // Try to get free block from this group
        // ATTENTION: We use spin-lock (not only CAS) here to prevent ABA problem
        if ((block = grp->free_list) != NULL)
        {
            // Got a block, remove it from list
            grp->free_list = block->next;
        } else
        if (grp->free_listByOtherThreads != NULL)
        {
            // Other threads free blocks, take back them
            block = (std_memory_header_t *) std_cpu_lock_xchg(&grp->free_listByOtherThreads, NULL);
            grp->free_list = block->next;
            grp->freedByOther = 0;
        } else
        {
            // No free blocks? Extend them & get a free block
            block = _std_extend_blocks_of_group(lms, i);
            if (block == NULL)
            {
                // Failed to allocate
                grp->dropped++;
                return NULL;
            }
        }

        STD_ASSERT(block != NULL);

        // Erase the next field, replace it with lms
        block->owner_lms = lms;

#if STD_BLOCK_DETAIL
        // Init tag
        block->tag = 0;
#endif

        // _stat
        grp->used++;
        if (grp->used > grp->peak_used)
            grp->peak_used = grp->used;
    }

    // Set the flag of the header
    STD_ASSERT(! (block->flags & STD_MEMORY_ALLOCATED));

    // Set the flags of the block to indicate the memory is allocated
    block->flags |= STD_MEMORY_ALLOCATED;

    // The data area is following the header immediatly
    block->size = (Uint32) size;
#if STD_BLOCK_DETAIL
    block->file = file;
    block->line = (Uint32) line;
    block->module = module_name;
    // Record the time when allocating
    time(&block->allocate_time);
#endif
#if (STD_BLOCK_RESERVED > 0)
    *((Uint8 *) block + sizeof(*block) + size) = STD_MEMORY_BLOCK_TAIL;
#endif

#ifdef _DEBUG
    // To set break pointer here
    // if (block + 1 == (void *) 0x0)
    //     printf("!!!(1)\n");

    // Clear allocated memory block
    memset(block + 1, 0x55, block->size);
#endif

#if STD_STAT_ALLOC
    // Update counter
    std_add_code_node_stat_counter(file, line, 1, size);
#endif

    // Record the successful allocation times & count used size
    lms->stat.alloc_times++;
    lms->stat.alloc_size += size;
    lms->stat.total_used_size += size;

    return block + 1;
}

// Realloc memory.
void *std_reallocate_memory(void *ptr, size_t size, const char *module_name,
                            const char *file, int line)
{
    void *new_ptr;
    std_memory_header_t *block;
    size_t prev_size;

    new_ptr = std_allocate_memory(size, module_name, file, line);
    if (new_ptr == NULL)
        return NULL;

    // Get the header of the memory block
    block = ((std_memory_header_t *) ptr) - 1;
    prev_size = block->size;

    // Move data to new block
    memcpy(new_ptr, ptr, prev_size < size ? prev_size : size);
    std_free_memory(ptr
#if STD_BLOCK_DETAIL
                    , block->module, block->file, block->line
#else
                    , module_name, file, line
#endif
                   );

    return new_ptr;
}


// _free memory.
void std_free_memory(void *ptr, const char *module_name,
                     const char *file, int line)
{
    // PDB memory manager
    std_lms_t           *lms;
    std_memory_grp_t    *grp;
    std_memory_header_t *block;

    STD_ASSERT(file != NULL);
    STD_ASSERT(module_name != NULL);

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
    {
        STD_FATAL("PDB is not initialized.\n");
        return;
    }

    // Get lms
    lms = _std_get_or_create_lms();

    STD_ASSERT(ptr != NULL);

    // Get the header of the memory block
    block = ((std_memory_header_t *) ptr) - 1;

#if STD_BLOCK_DETAIL
    STD_ASSERT(block->sign == STD_MEMORY_BLOCK_SIGN);
#endif

#if (STD_BLOCK_RESERVED > 0)
    if (*((Uint8 *) block + sizeof(*block) + block->size) != STD_MEMORY_BLOCK_TAIL)
    {
        STD_FATAL("Bad block (Tail flag is error 2).\n");
        return;
    }
#endif
    STD_ASSERT(block->flags & STD_MEMORY_ALLOCATED);
    block->flags &= ~STD_MEMORY_ALLOCATED;

#if STD_STAT_ALLOC
    // Update counter
    std_add_code_node_stat_counter(block->file, block->line, -1, block->size);
#endif

    // Attention: Do THREAD-UNSAFE stat for reallocate
    // Clear the freed block & Update used size
    lms->stat.free_times++;
    lms->stat.free_size += block->size;
    lms->stat.total_used_size -= block->size;

#ifdef _DEBUG
    // Set breakpoints here
    if (ptr == (void *) 0)
        printf("!!!(2)\n");
#endif

    // Clear freed memory block
    memset(ptr, 0xAA, block->size);

    // Is this block pre-allocated
    if (block->flags & STD_MEMORY_GROUP_BLOCK)
    {
        // The memory is a pre-allocated block, return to the list
        STD_ASSERT(block->owner_lms != NULL);
        STD_ASSERT(block->serial < _block_groups);
        
        grp = &block->owner_lms->mem_grp[block->serial];
        if (block->owner_lms == lms)
        {
            // The block belong to current thread, return to list
            block->next = grp->free_list;
            grp->free_list = block;
        } else
        {
            // The block belong to other thread, return safety
            for (;;)
            {
                block->next = grp->free_listByOtherThreads;
                if (std_cpu_lock_cas(&grp->free_listByOtherThreads, block->next, block))
                    break;
            }

            // ATTENTION: Thread unsafe stat
            grp->freedByOther++;
            if (grp->freedByOther > grp->peak_freedByOther)
                grp->peak_freedByOther = grp->freedByOther;
        }

        // ATTENTION: Do THREAD-UNSAFE stat
        grp->used--;
    } else
    {
        // Use _l2 free
        _std_l2_free_memory(block);
    }

    STD_REFER(module_name);
    STD_REFER(file);
    STD_REFER(line);
}

// Lookup lms list
int std_lms_and_tiny_stat(char *msg, size_t size)
{
    // PDB memory manager
    std_lms_t           *lms;
    std_memory_grp_t    *grp;
    char   temp[2048], *p;
    size_t i;

    STD_ASSERT(msg != NULL);
    STD_ASSERT(size > 0);

    if (! std_is_mem_mgr_installed())
    {
        // Not installed
        strncpy(msg, "Pdb memmgr is not installed.\n", size);
        msg[size - 1] = 0;
        return 0;
    }

    _std_get_all_mem_spin_lock();

    lms = _lms_list;
    while (lms != NULL)
    {
        char task_name[STD_MAX_tASK_NAME_LEN];

        p = temp;

        // Show runtime memory block allocated
        std_get_task_name(lms->task_id, task_name, sizeof(task_name));
        sprintf(p, "Thread name: %s\n", task_name);
        p += strlen(p);
        sprintf(p, "Group Size  Reserved Used     PeakUsed Dropped FreedByOther PeakFreedByOther\n");
        p += strlen(p);
        sprintf(p, "------------------------------------------------------------------------------\n");
        p += strlen(p);
        for (i = 0; i < _block_groups; i++)
        {
            UintR block_count = 0, used = 0, peak_used = 0;
            UintR dropped = 0, freedByOther = 0, peak_freedByOther = 0;

            grp = &lms->mem_grp[i];
            block_count       = grp->block_count;
            used             = grp->used;
            peak_used         = grp->peak_used;
            dropped          = grp->dropped;
            freedByOther     = grp->freedByOther;
            peak_freedByOther = grp->peak_freedByOther;

            sprintf(p,
                "%-5d %-5d %-8d %-8d %-8d %-7d %-12d %d\n",
                (int) i, (int) mem_config[i].block_size,
                (int) block_count, (int) used,
                (int) peak_used, (int) dropped,
                (int) freedByOther, (int) peak_freedByOther);
            p += strlen(p);
        }
        sprintf(p, "------------------------------------------------------------------------------\n");
        p += strlen(p);

        // Copy to result
        strncpy(msg, temp, (size_t) size - 1);
        msg[size - 1] = 0;
        size -= strlen(msg);
        msg += strlen(msg);

        lms = lms->next;
    }

    // _tiny stat
    p = temp;
    sprintf(p,
            "Tiny stat:\n"
            "Allocated times = %u\nAllocSize = %u\nFreed times = %u\nFreed size = %u\n"
            "Total reserved size = %u\nPeak reserved size = %u\n",
            (Uint) _tiny_mem_stat.alloc_times,
            (Uint) _tiny_mem_stat.alloc_size,
            (Uint) _tiny_mem_stat.free_times,
            (Uint) _tiny_mem_stat.free_size,
            (Uint) _tiny_mem_stat.total_reserved_size,
            (Uint) _tiny_mem_stat.peak_reserved_size);
    // Copy to result
    strncpy(msg, temp, (size_t) size - 1);
    msg[size - 1] = 0;

    _std_release_all_mem_spin_lock();
    return 0;
}

// Return the statistics information
// Be sure msg must have size at least 512 bytes
int std_memory_stat(int flag, std_mem_stat_t *ptr_mem_stat,
                    char *msg, size_t size)
{
    // PDB memory manager
    std_memory_grp_t *grp;
    std_mem_stat_t    mem_stat;
    char   temp[2048], *p;
    size_t i;

    if (_init_mem_mgr_flag == MEMMGR_NOT_INIT)
    {
        // Not installed
        if (msg != NULL)
        {
            strncpy(msg, "Pdb memmgr is not installed.\n", size);
            msg[size - 1] = 0;
        }

        if (ptr_mem_stat != NULL)
            memset(ptr_mem_stat, 0, sizeof(*ptr_mem_stat));
        return 0;
    }

    // Get memory stat from lms
    _std_get_mem_stat(&mem_stat);

    // _lock to generate stat information
    _std_get_all_mem_spin_lock();

    if (msg != NULL)
        STD_ASSERT(size > 0);

    p = temp;

    if (flag <= 0)
        temp[0] = 0;

    if (flag > 0)
    {
        // Show runtime memory block allocated
        sprintf(p, "\nGroup Size  Reserved Used     PeakUsed Dropped FreedByOther PeakFreedByOther\n");
        p += strlen(p);
        sprintf(p, "------------------------------------------------------------------------------\n");
        p += strlen(p);
        for (i = 0; i < _block_groups; i++)
        {
            UintR block_count = 0, used = 0, peak_used = 0;
            UintR dropped = 0, freedByOther = 0, peak_freedByOther = 0;
            std_lms_t *lms;

            // _count of all lms
            lms = _lms_list;
            while (lms != NULL)
            {
                grp = &lms->mem_grp[i];
                block_count       += grp->block_count;
                used              += grp->used;
                peak_used         += grp->peak_used;
                dropped           += grp->dropped;
                freedByOther      += grp->freedByOther;
                peak_freedByOther += grp->peak_freedByOther;
                lms = lms->next;
            }

            sprintf(p,
                "%-5d %-5d %-8d %-8d %-8d %-7d %-12d %d\n",
                (int) i, (int) mem_config[i].block_size,
                (int) block_count, (int) used,
                (int) peak_used, (int) dropped,
                (int) freedByOther, (int) peak_freedByOther);
            p += strlen(p);
        }
        sprintf(p, "------------------------------------------------------------------------------\n");
        p += strlen(p);
    }

    if (flag > 1)
    {
#if STD_BLOCK_DETAIL
        std_memory_header_t *header;
        sprintf(p, "L2 memory block information:\n");
        p += strlen(p);
        // Show runtime _l2 memory allocated
        for (i = 1, header = _l2_mem_blocks;
            header != NULL && (p - temp) < sizeof(temp) - 256;
            i++, header = header->next)
        {
            sprintf(p,
                    "%3d. 0x%p[%7d]  %3s/%s  %5d@%s\n",
                    (int) i, header + 1, (int) header->size,
                    header->module,
                    _std_ctime(&header->allocate_time),
                    (int) header->line, _std_last_name(header->file));
            p += strlen(p);
        }
#else
        sprintf(p, "L2 memory block information is N/A.\n");
        p += strlen(p);
#endif
    }

    _std_release_all_mem_spin_lock();

    if (flag > 0)
    {
        // Show runtime debug information
        sprintf(p,
                "Allocated times = %u\nFree times = %u\nPeak memory reserved = %uK\n",
                (int) mem_stat.alloc_times, (int) mem_stat.free_times,
                (int) ((mem_stat.peak_reserved_size + 1023) / 1024));
        p += strlen(p);
    }

    // Copy to result
    if (msg != NULL)
    {
        strncpy(msg, temp, (size_t) size - 1);
        msg[size - 1] = 0;
    } else
        // Print out statistics
        printf("%s", temp);

    // Return statistics information
    if (ptr_mem_stat != NULL)
        memcpy(ptr_mem_stat, &mem_stat, sizeof(std_mem_stat_t));

    return 0;
}

// Lookup used memory
// The input parameters is used as a filter. Only the
// memory match the parameters would be show.
// module_name == NULL means don't care.
// file_name   == NULL means don't care (line# is also don't care).
// line       == 0    means don't care.
// max means how many blocks would be show.
int std_memory_list(int max,
                   size_t *pTotal_block_size,
                   size_t *pTotal_l2_size,
                   size_t *pTotal_block_count,
                   size_t *pTotal_l2_count,
                   const char *module_name,
                   const char *file_name, int line)
{
#if STD_BLOCK_DETAIL1
    // PDB memory manager

    // Lookup pre-allocated blocks
    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    size_t total_block_size, total_l2_size;
    size_t total_block_count, total_l2_count;
    int    no;
    int    i, k;

    if (std_is_mem_mgr_installed())
    {
        // I MUST STOP THE MEM MGR RUNNING FIRST
        // TODO: ....

        // Just compare the last name, don't include the path information
        if (file_name != NULL)
            file_name = _std_last_name(file_name);

        // Initialize the counter
        total_block_size  = 0;
        total_l2_size     = 0;
        total_block_count = 0;
        total_l2_count    = 0;
        no = 1;

        _std_get_all_mem_spin_lock();

        // Lookup pre-allocated memory block groups
        for (i = 0; i < _block_groups; i++)
        {
            // Lookup in this group
            grp = &_memory_ptr_grp[i];
            k = grp->block_count;
            block = grp->blockAt;
            while (k--)
            {
                // Check block k in serial i
                if ((block->flags & STD_MEMORY_ALLOCATED) &&
                    _std_is_blockIn_list(block,
                    module_name, file_name, line))
                {
                    // OK! Show the eligble memory block
                    STD_ASSERT(block->flags & STD_MEMORY_GROUP_BLOCK);
                    total_block_size += block->size;
                    total_block_count++;
                    if (max > 0)
                    {
                        // Show the information
                        max--;
                        printf("%5d. 0x%p(%7d)  BLK  %3s/%s  %5d@%s\n",
                            (int) (no++),
                            block + 1, (int) block->size,
                            block->module,
                            _std_ctime(&block->allocate_time),
                            (int) block->line, _std_last_name(block->file));
                    }
                }

                // Lookup next block
                block = (std_memory_header_t *)
                    (((Uint8 *) block) + sizeof(*block) +
                    grp->block_size + STD_BLOCK_RESERVED);
            }

            // Lookup next group
        }

        // Lookup l2 memory block
        block = _l2_mem_blocks;
        while (block != NULL)
        {
            STD_ASSERT(block->flags & STD_MEMORY_ALLOCATED);
            if (_std_is_blockIn_list(block,
                module_name, file_name, line))
            {
                // OK! Show the eligble memory block
                STD_ASSERT(! (block->flags & STD_MEMORY_GROUP_BLOCK));
                total_l2_size += block->size;
                total_l2_count++;
                if (max > 0)
                {
                    // Show the information
                    max--;
                    printf("%5d. 0x%p(%7d)  _l2   %3s/%s  %5d@%s\n",
                        (int) (no++),
                        block + 1, (int) block->size,
                        block->module,
                        _std_ctime(&block->allocate_time),
                        (int) block->line, _std_last_name(block->file));
                }
            }

            // Check next block
            block = block->next;
        }

        _std_release_all_mem_spin_lock();

        // Return result
        if (pTotal_block_size != NULL)
            *pTotal_block_size = total_block_size;

        if (pTotal_l2_size != NULL)
            *pTotal_l2_size = total_l2_size;

        if (pTotal_block_count != NULL)
            *pTotal_block_count = total_block_count;

        if (pTotal_l2_count != NULL)
            *pTotal_l2_count = total_l2_count;

        return 0;
    } else
    {
        // OS memory manager
        if (pTotal_block_size != NULL)
            *pTotal_block_size = 0;

        if (pTotal_l2_size != NULL)
            *pTotal_l2_size = 0;

        if (pTotal_block_count != NULL)
            *pTotal_block_count = 0;

        if (pTotal_l2_count != NULL)
            *pTotal_l2_count = 0;

        return 0;
    }
#else
    // No MEM stat
    return 0;
#endif // End of STD_BLOCK_DETAIL
}

// Mark unmarked memory block to new tag
// Tag must be in [1..255]
// block_size indicate the block group, if 0, means all groups
int std_memoryMark(Uint8 tag, size_t block_size, size_t *pMatched)
{
#if STD_BLOCK_DETAIL1
    // PDB memory manager

    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    int     i, k;
    size_t  n;

    if (std_is_mem_mgr_installed())
    {
        if (tag < 1)
            return STD_UNKNOWN_ERROR;

        // I MUST STOP THE MEM MGR RUNNING FIRST
        _std_get_all_mem_spin_lock();

        n = 0;
        for (i = 0; i < _block_groups; i++)
        {
            grp = &_memory_ptr_grp[i];
            if (grp->block_size == block_size || ! block_size)
            {
                // Ok! Lookup blocks in this group
                block = grp->blockAt;
                for (k = 0; k < grp->block_count; k++)
                {
                    if (block->flags & STD_MEMORY_ALLOCATED)
                    {
                        // _allocated block
                        if (! block->tag)
                        {
                            // No tag, assign new tag to it
                            block->tag = tag;
                            n++;
                        }
                    }

                    // Lookup next block
                    block = (std_memory_header_t *)
                        (((Uint8 *) block) + sizeof(*block) +
                        grp->block_size + STD_BLOCK_RESERVED);
                }
            }
        }

        _std_release_all_mem_spin_lock();

        if (pMatched != NULL)
            *pMatched = n;

        return 0;
    } else
    {
        // OS memory manager
        if (pMatched != NULL)
            *pMatched = 0;

        return 0;
    }
#else
    // No MEM stat
    return 0;
#endif // End of STD_BLOCK_DETAIL
}

// Unmark marked memory block
// tag == 0, means unmark all
// block_size indicate the block group, if 0, means all groups
int std_memoryUnmark(Uint8 tag, size_t block_size, size_t *pMatched)
{
#if STD_BLOCK_DETAIL1
    // PDB memory manager

    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    int    i, k;
    size_t n;

    if (std_is_mem_mgr_installed())
    {
        // I MUST STOP THE MEM MGR RUNNING FIRST
        _std_get_all_mem_spin_lock();

        n = 0;
        for (i = 0; i < _block_groups; i++)
        {
            grp = &_memory_ptr_grp[i];
            if (grp->block_size == block_size || ! block_size)
            {
                // Ok! Lookup blocks in this group
                block = grp->blockAt;
                for (k = 0; k < grp->block_count; k++)
                {
                    if (block->flags & STD_MEMORY_ALLOCATED)
                    {
                        // _allocated block
                        if (block->tag == tag || ! tag)
                        {
                            // Remove tag from this block
                            block->tag = 0;
                            n++;
                        }
                    }

                    // Lookup next block
                    block = (std_memory_header_t *)
                        (((Uint8 *) block) + sizeof(*block) +
                        grp->block_size + STD_BLOCK_RESERVED);
                }
            }
        }

        _std_release_all_mem_spin_lock();

        if (pMatched != NULL)
            *pMatched = n;

        return 0;
    } else
    {
        // OS memory manager
        if (pMatched != NULL)
            *pMatched = 0;

        return 0;
    }
#else
    // No MEM stat
    return 0;
#endif // End of STD_BLOCK_DETAIL
}

// Lookup for marked blocks
// Tag must be in [1..255]
// block_size indicate the block group, if 0, means all groups
// show_flag == 1 means show detail information of block
// show_flag == 2 means show detail information of block include data
int std_memoryShowMarked(Uint8 tag, size_t block_size,
                         int   show_flag, size_t *pMatched)
{
#if STD_BLOCK_DETAIL1
    // PDB memory manager
    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    int    i, k;
    size_t n;

    if (std_is_mem_mgr_installed())
    {
        if (tag < 1)
            return STD_UNKNOWN_ERROR;

        // I MUST STOP THE MEM MGR RUNNING FIRST
        _std_get_all_mem_spin_lock();

        n = 0;
        for (i = 0; i < _block_groups; i++)
        {
            grp = &_memory_ptr_grp[i];
            if (grp->block_size == block_size || ! block_size)
            {
                // Ok! Lookup blocks in this group
                block = grp->blockAt;
                for (k = 0; k < grp->block_count; k++)
                {
                    if (block->flags & STD_MEMORY_ALLOCATED)
                    {
                        // _allocated block
                        if (block->tag == tag)
                        {
                            // Show information of this block
                            if (show_flag)
                                printf("%3d. 0x%p[%7d]  %3s/%s  %5d@%s\n",
                                (int) (++n),
                                block + 1, (int) block->size,
                                block->module,
                                _std_ctime(&block->allocate_time),
                                (int) block->line, _std_last_name(block->file));

                            if (show_flag >= 2)
                                _std_show_mem_block((Uint8 *) (block + 1),
                                block->size > 256 ? 256 : block->size);
                        }
                    }

                    // Lookup next block
                    block = (std_memory_header_t *)
                        (((Uint8 *) block) + sizeof(*block) +
                        grp->block_size + STD_BLOCK_RESERVED);
                }
            }
        }

        _std_release_all_mem_spin_lock();

        if (pMatched != NULL)
            *pMatched = n;

        return 0;
    } else
    {
        if (pMatched != NULL)
            *pMatched = 0;

        return 0;
    }
#else
    // No MEM stat
    return 0;
#endif // End of STD_BLOCK_DETAIL
}

// Dump memory block information to a file
int std_dummemory_ptr(char *file_name)
{
#if STD_BLOCK_DETAIL1
    // PDB memory manager
    std_memory_grp_t    *grp;
    std_memory_header_t *block;
    FILE   *fp;
    int     i, k;
    size_t  n;
    time_t  ti;

    if (std_is_mem_mgr_installed())
    {
        // I MUST STOP THE MEM MGR RUNNING FIRST
        _std_get_all_mem_spin_lock();

        fp = fopen(file_name, "w");
        if (fp == NULL)
            return STD_UNKNOWN_ERROR;

        time(&ti);
        fprintf(fp, "Memory dump on %s\n", _std_ctime(&ti));

        // Show runtime memory block allocated
        fprintf(fp, "Group   Size   Reserved  Used      PeakUsed    Dropped\n");
        fprintf(fp, "----------------------------------------------------\n");
        for (i = 0; i < _block_groups; i++)
        {
            grp = &_memory_ptr_grp[i];
            fprintf(fp,
                "%-5d   %-5d  %-5d     %-8d  %-8d   %-6d\n",
                (int) i,
                (int) grp->block_size, (int) grp->block_count, (int) grp->used,
                (int) grp->peak_used, (int) grp->dropped);
        }
        fprintf(fp, "----------------------------------------------------\n");

        fprintf(fp, "Memory allocation list:\n");
        n = 0;
        for (i = 0; i < _block_groups; i++)
        {
            grp = &_memory_ptr_grp[i];

            fprintf(fp,
                "\nGroup: %u  Size: %u  Count: %u  Used: %u  Peak: %u  Dropped: %u\n\n",
                (int) i, (int) grp->block_size,
                (int) grp->block_count, (int) grp->used,
                (int) grp->peak_used, (int) grp->dropped);

            // Ok! Lookup blocks in this group
            block = grp->blockAt;
            for (k = 0; k < grp->block_count; k++)
            {
                if (block->flags & STD_MEMORY_ALLOCATED)
                {
                    // _allocated block

                    // Show information of this block
                    fprintf(fp,
                        "%3d. 0x%p[%7d]  %3s/%s  %5d@%s\n",
                        (int) ++n,
                        block + 1, (int) block->size,
                        block->module,
                        _std_ctime(&block->allocate_time),
                        (int) block->line, _std_last_name(block->file));

                    // Dump block
                    _std_dump_mem_block(fp, (Uint8 *) (block + 1),
                        block->size > 256 ? 256 : block->size);
                }

                // Lookup next block
                block = (std_memory_header_t *)
                    (((Uint8 *) block) + sizeof(*block) +
                    grp->block_size + STD_BLOCK_RESERVED);
            }
        }

        fprintf(fp, "L2 memory block information:\n");
        // Show runtime _l2 memory allocated
        for (i = 1, block = _l2_mem_blocks;
            i < 128 && block != NULL;
            i++, block = block->next)
        {
            fprintf(fp,
                "%3d. 0x%p[%7d]  %3s/%s  %5d@%s\n",
                (int) i,
                block + 1, (int) block->size,
                block->module,
                _std_ctime(&block->allocate_time),
                (int) block->line, _std_last_name(block->file));

            // Dump block
            _std_dump_mem_block(fp, (Uint8 *) (block + 1),
                block->size > 256 ? 256 : block->size);
        }

        _std_release_all_mem_spin_lock();

        fclose(fp);

        return 0;
    } else
    {
        // OS memory manager
        return 0;
    }
#else
    // No MEM stat
    return 0;
#endif // End of STD_BLOCK_DETAIL
}

#if STD_BLOCK_DETAIL
std_memory_header_t *std_get_l2mem_blocks_list()
{
    return _l2_mem_blocks;
}
#endif

#if STD_STAT_ALLOC
// Get code node stat list
int std_get_code_node_stat(mem_code_node_stat_t **pp_code_nodes, size_t *ptr_code_nodes_count)
{
    STD_ASSERT(pp_code_nodes != NULL);
    STD_ASSERT(ptr_code_nodes_count != NULL);

    if (_alloc_nodes == NULL || _alloc_nodes_count < 1)
        // Not code node stat
        return STD_UNKNOWN_ERROR;

    std_get_spin_lock(&_code_node_stat_spin_lock);
    *pp_code_nodes = _alloc_nodes;
    *ptr_code_nodes_count = _alloc_nodes_count;
    std_release_spin_lock(&_code_node_stat_spin_lock);
    return 0;
}

// Reset stat of alloc
// After reset, only allocation in [min_size..max_size] should be recorded
int std_reset_code_node_stat(size_t min_size, size_t max_size)
{
    std_get_spin_lock(&_code_node_stat_spin_lock);

    // Clear stat information
    if (_alloc_nodes != NULL)
    {
        OS_FREE(_alloc_nodes);
        _alloc_nodes = NULL;
    }
    _alloc_nodes_count = 0;
    _alloc_nodes_size = 0;

    // Reset range
    _allocMin_size = min_size;
    _alloc_max_size = max_size;

    std_release_spin_lock(&_code_node_stat_spin_lock);

    return 0;
}
#endif

// _allocate a tiny block from a page
void *std_allocate_tiny_block(size_t size)
{
#if STD_USE_TINY_ALLOC
    int index;
    std_tiny_page_header_t   *page;
    std_tiny_memory_header_t *tiny_block;

    STD_ASSERT(size >= 0 && size <= (STD_SIZE_N(_tiny_block_page_lists) - 1) * STD_TINY_ALIGNMENT_SIZE);

    // Get the entry index of tiny block (make assure the entry is aligmented)
    index = (size + STD_TINY_ALIGNMENT_SIZE - 1) / STD_TINY_ALIGNMENT_SIZE;
    size = index * STD_TINY_ALIGNMENT_SIZE;
    STD_ASSERT(size >= 0 && size <= (STD_SIZE_N(_tiny_block_page_lists) - 1) * STD_TINY_ALIGNMENT_SIZE);
    STD_ASSERT(index >= 0 && index < STD_SIZE_N(_tiny_block_page_lists));

    // Enter protect
    std_get_spin_lock(&_tiny_block_spin_lock);

    // Get the page
    page = _tiny_block_page_lists[index];
    if (page == NULL)
    {
        // Leave protect when create new tiny_block_page
        std_release_spin_lock(&_tiny_block_spin_lock);

        // _create a new page & add the page into list
        page = std_create_tiny_block_page(size);
        if (page == NULL)
        {
            // Failed to allocate page

            // _free the mutex, ready for return
            return NULL;
        }

        // Re-enter protect
        std_get_spin_lock(&_tiny_block_spin_lock);
        page->next_page = _tiny_block_page_lists[index];
        _tiny_block_page_lists[index] = page;
    }

    // Get block from the page
    STD_ASSERT(page->free_list != NULL);
    STD_ASSERT(page->count > page->used);

    // Take off block from the list
    tiny_block = page->free_list;
    page->free_list = tiny_block->p.next;

    // Updated used field of page
    page->used++;

    if (page->used >= page->count)
    {
        // _all blocks are used out, remove the page from the list
        _tiny_block_page_lists[index] = page->next_page;
    }

    // Erase the field p in tiny block & replaced with page pointer
    tiny_block->p.page = page;

    _tiny_mem_stat.alloc_times++;
    _tiny_mem_stat.alloc_size += size;
    _tiny_mem_stat.total_used_size += size;

    // _free the mutex, ready for return
    std_release_spin_lock(&_tiny_block_spin_lock);

    // Return the memory pointer
    return tiny_block + 1;
#else
    return std_allocate_memory(size, "tiny", __FILE__, __LINE__);
#endif
}

// _free a tiny block from a page
void std_free_tiny_block(void *p)
{
#if STD_USE_TINY_ALLOC
    std_tiny_page_header_t   *page;
    std_tiny_memory_header_t *tiny_block;
    size_t                    size;

    tiny_block = ((std_tiny_memory_header_t *) p) - 1;

    // Enter protect
    std_get_spin_lock(&_tiny_block_spin_lock);

    page = tiny_block->p.page;
    STD_ASSERT(page->used > 0 && page->used <= page->count);
    STD_ASSERT(page->block_size % STD_TINY_ALIGNMENT_SIZE == 0);
    STD_ASSERT(page->block_size <= (STD_SIZE_N(_tiny_block_page_lists) - 1) * STD_TINY_ALIGNMENT_SIZE);
    size = page->block_size;

    // Return the tiny block
    tiny_block->p.next = page->free_list;
    page->free_list = tiny_block;

    if (page->used >= page->count)
    {
        // The page has free space, move the page back to the list
        page->next_page = _tiny_block_page_lists[page->block_size / STD_TINY_ALIGNMENT_SIZE];
        _tiny_block_page_lists[page->block_size / STD_TINY_ALIGNMENT_SIZE] = page;
    }

    // Updated used field of page
    if (--page->used <= 0)
    {
        std_tiny_page_header_t **ppage;

        // Remove the page from list
        ppage = &_tiny_block_page_lists[page->block_size / STD_TINY_ALIGNMENT_SIZE];
        while (*ppage != page)
            ppage = &(*ppage)->next_page;

        STD_ASSERT(*ppage == page);
        *ppage = page->next_page;

        // Leave protect when free page
        std_release_spin_lock(&_tiny_block_spin_lock);

        // _free the page
        std_delete_tiny_block_page(page);
        return;
    }

    _tiny_mem_stat.free_times++;
    _tiny_mem_stat.free_size += size;
    _tiny_mem_stat.total_used_size -= size;

    // _free the mutex, ready for return
    std_release_spin_lock(&_tiny_block_spin_lock);

#else
    std_free_memory(p, "tiny", __FILE__, __LINE__);
#endif
}

#if 1 // Disable this function temporary
// Stable heap memory allocation
void *std_allocateFromStableHeap(size_t size)
{
    // _statistics
    _stable_heap_allocate_times++;

    // Make alignment
    size = (size + STD_MEMORY_ALIGNMENT_SIZE - 1) & ~(STD_MEMORY_ALIGNMENT_SIZE - 1);

    if (_stable_heapStart == NULL)
    {
        // No stable heap yet
        _stable_heap_allocateFailed++;
        return NULL;
    }

    if (size + _stable_heap_allocated_size <= _stable_heap_size)
    {
        Uint8 *p;

        // _memory is enough
        p = _stable_heapStart + _stable_heap_allocated_size;
        _stable_heap_allocated_size += size;
        return p;
    }

    // Run out of memory of stable heap
    _stable_heap_allocateFailed++;
    return NULL;
}

// Get stat
int std_getStableHeap_stat(std_stable_heap_stat_t *mem_stat)
{
    // Return the statistics information
    mem_stat->stable_heap_allocated_size  = _stable_heap_allocated_size;
    mem_stat->stable_heap_size           = _stable_heap_size;
    mem_stat->stable_heap_free_list_times      = _stable_heap_free_list_times;
    mem_stat->stable_heap_allocate_times  = _stable_heap_allocate_times;
    mem_stat->stable_heap_allocateFailed = _stable_heap_allocateFailed;
    return 0;
}

// Return 1 if the p is in stable heap
int std_isComeFromStableHeap(const void *p)
{
    if (_stable_heapStart == NULL)
        // No stable heap yet
        return 0;

    if ((Uint8 *) p <= (_stable_heapStart + _stable_heap_allocated_size) &&
        (Uint8 *) p >= _stable_heapStart)
        // The memory block is in stable heap
        return 1;

    // Not in stable heap
    return 0;
}

// _free? Do nothing but stat the operation
void std_returnToStableHeap(void *p)
{
    STD_ASSERT(std_isComeFromStableHeap(p));

    // _statistics
    _stable_heap_free_list_times++;
}

// Initialize, to create a stable heap
// This routine can be called only once during who process live cycle
int std_setupStableHeap(size_t max_size)
{
    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
    {
        STD_FATAL("PDB is not initialized.\n");
        return STD_UNKNOWN_ERROR;
    }

    if (_stable_heapStart != NULL)
        STD_FATAL("The stable heap is already created.\n");

    // WARNING: Don't remove the reserved bytes. If someone try to allocate 0 bytes
    // @ tail of stable heap, the pointer may be (char *) (start + max_size), it may be
    // ambigous to next block of memory. (Actually, it's impossible to happen since every
    // memory block has a header, but I don't like this assumption.)
    // _allocate memory with n bytes reserved
    _stable_heapStart = (Uint8 *) OS_MALLOC(max_size + STD_MEMORY_ALIGNMENT_SIZE); // Reserve bytes
    if (_stable_heapStart != NULL)
    {
        // Ok
        _stable_heap_size = max_size;
        return 0;
    }

    // Failed to allocate
    printf("Failed to allocate stable heap with size = %d.\n",
           (int) max_size);
    return STD_MEMORY_NOT_ENOUGH;
}
#endif

// Internal functions                                             *

// Base allocation
static void* _std_internal_alloc(size_t size)
{
    if (_mem_mgr_options & STD_USE_BA_ALLOC)
    {
        void* p = std_ba_alloc(&_ba_pool, size);
        return p;
    } else
        return OS_MALLOC(size);
}

// Base free
static void _std_internal_free(void* p, size_t size)
{
    if (_mem_mgr_options & STD_USE_BA_ALLOC)
        std_ba_free(&_ba_pool, p, size);
    else
        OS_FREE(p);
}

// _l2 memory allocation. Base on OS malloc
// Return NULL when PDB memory manager not initialized
static std_memory_header_t *_std_l2_allocate_memory(size_t size)
{
    std_memory_header_t *block;
    std_lms_t *lms;
    size_t     real_size;

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
        return NULL;

    lms = _std_get_or_create_lms();

    // Calculate the real size I should allocate
    real_size = sizeof(std_memory_header_t) + STD_BLOCK_RESERVED + size;

    // _allocate it from base allocator
    block = (std_memory_header_t *)_std_internal_alloc(real_size);

    if (block == NULL)
        return NULL;

    // Init the flag
    memset(block, 0, sizeof(std_memory_header_t));
    block->flags = STD_MEMORY_L2;
#if STD_BLOCK_DETAIL
    block->sign = STD_MEMORY_BLOCK_SIGN;
#endif

    // Set to _l2 group (not belong to [0.._block_groups - 1]
    block->serial = (Uint8) _block_groups;

    // ATTENTION: Do THREAD-UNSAFE stat
    // Record the peak used of the memory
    lms->stat.total_l2_size += size;

    // For _l2-memory allocateion stat
    lms->stat.total_reserved_size += real_size;
    if (lms->stat.total_reserved_size > lms->stat.peak_reserved_size)
        lms->stat.peak_reserved_size = lms->stat.total_reserved_size;

#if STD_BLOCK_DETAIL
    // Add into list
    std_get_spin_lock(&_l2_mem_block_list);
    block->next_l2 = _l2_mem_blocks;
    _l2_mem_blocks = block;
    std_release_spin_lock(&_l2_mem_block_list);
#endif

    return block;
}

// _l2 memory free
static void _std_l2_free_memory(std_memory_header_t *block)
{
    std_lms_t *lms;

    if (_init_mem_mgr_flag != MEMMGR_INIT_OK)
    {
        STD_FATAL("PDB is not initialized.\n");
        return;
    }

    lms = _std_get_or_create_lms();

    STD_ASSERT(block != NULL);

#if STD_BLOCK_DETAIL
    std_get_spin_lock(&_l2_mem_block_list);
    do
    {
        // Remove from list
        std_memory_header_t **pp;
        pp = &_l2_mem_blocks;
        while (*pp != NULL && *pp != block)
            pp = &(*pp)->next_l2;
        if (*pp == block)
            *pp = (*pp)->next_l2;
    } while (0);
    std_release_spin_lock(&_l2_mem_block_list);
#endif

    // ATTENTION: Do THREAD-UNSAFE stat
    lms->stat.total_l2_size -= block->size + sizeof(block);
    lms->stat.total_reserved_size -= block->size + sizeof(block);

    // _free memory
    _std_internal_free(block, sizeof(std_memory_header_t) + STD_BLOCK_RESERVED + block->size);
    return;
}

// _allocate n new blocks (size) of specified serial.
// Initialize them and put them into free list of the group
// Reserve 1 block & return it
// Make sure the allocation size being aligned to STD_BLOCKS_CLUSTER_SIZE
static std_memory_header_t *_std_extend_blocks_of_group(std_lms_t *lms, Uint n_grpNo)
{
    std_memory_grp_t    *grp;
    std_Extend_grp_t    *extend_grp;
    std_memory_header_t *block, *block_list, *pLast_blockIn_list;
    Uint8 *memory_ptr;
    size_t single_block_mem_size;
    int    block_count;
    int    i;

    STD_ASSERT(n_grpNo < _block_groups);
    grp = &lms->mem_grp[n_grpNo];

    // At least allocate memory size: STD_BLOCKS_CLUSTER_SIZE
    // Calculate the blocks count in cluster
    single_block_mem_size = sizeof(std_memory_header_t) + STD_BLOCK_RESERVED + grp->block_size;
    block_count = (int) ((STD_BLOCKS_CLUSTER_SIZE - sizeof(std_Extend_grp_t)) / single_block_mem_size);
    if (block_count < 1)
        STD_FATAL("STD_BLOCKS_CLUSTER_SIZE is too small to carry group blocks.\n");

    // Get the single block memory size & allocate cluster
    extend_grp = (std_Extend_grp_t *)_std_internal_alloc(STD_BLOCKS_CLUSTER_SIZE);
    if (extend_grp == NULL)
        // Failed to allocate memory, can't extend
        return NULL;
    memory_ptr = (Uint8 *) (extend_grp + 1);

    // Init structure
    extend_grp->block_count = block_count;
    extend_grp->block_size = grp->block_size;

    // _create a free block list
    block_list = NULL;
    pLast_blockIn_list = (std_memory_header_t *) memory_ptr;
    for (i = 0; i < block_count; i++)
    {
        block = (std_memory_header_t *) memory_ptr;
        memory_ptr += single_block_mem_size;

        // Initialize this block
#if STD_BLOCK_DETAIL
        block->file   = NULL;
        block->module = NULL;
        block->sign   = STD_MEMORY_BLOCK_SIGN;
#endif
        block->flags  = STD_MEMORY_GROUP_BLOCK;

        block->serial = (Uint8) n_grpNo;
        block->tag    = 0;

        // Add into list
        block->next = block_list;
        block_list = block;
    }

    // Reserve one from the list (return it)
    block = block_list;
    block_list = block_list->next;

    // Link the list into free list of this group
    if (block_list != NULL)
    {
        STD_ASSERT(grp->free_list == NULL);
        pLast_blockIn_list->next = grp->free_list;
        grp->free_list = block_list;
    }

    // Link to head of extend group list safety
    for (;;)
    {
        extend_grp->next = _extend_grp_list;
        if (std_cpu_lock_cas(&_extend_grp_list, extend_grp->next, extend_grp))
            break;
    }

    // _stat reserved block count
    grp->block_count += block_count;

    // ATTENTION: Do THREAD-UNSAFE stat
    _extend_times++;
    _extend_size += sizeof(std_Extend_grp_t) + single_block_mem_size * block_count;

    // Record the peak used of the memory
    lms->stat.total_reserved_size += single_block_mem_size * block_count;
    if (lms->stat.total_reserved_size > lms->stat.peak_reserved_size)
        lms->stat.peak_reserved_size = lms->stat.total_reserved_size;

    return block;
}

// _count all mem stat for lms & tiny
static void _std_get_mem_stat(std_mem_stat_t *mem_stat)
{
    std_lms_t *lms;

    _std_get_all_mem_spin_lock();

    memset(mem_stat, 0, sizeof(std_mem_stat_t));

    // _count waste size
    mem_stat->total_reserved_size = STD_WASTE_SIZE;
    mem_stat->peak_reserved_size = STD_WASTE_SIZE;

    // _count lms
    lms = _lms_list;
    while (lms != NULL)
    {
        mem_stat->alloc_times         += lms->stat.alloc_times;
        mem_stat->alloc_size          += lms->stat.alloc_size;
        mem_stat->free_times          += lms->stat.free_times;
        mem_stat->free_size           += lms->stat.free_size;
        mem_stat->extend_size         += lms->stat.extend_size;
        mem_stat->total_reserved_size += lms->stat.total_reserved_size;
        mem_stat->peak_reserved_size  += lms->stat.peak_reserved_size;
        mem_stat->total_used_size     += lms->stat.total_used_size;
        mem_stat->total_l2_size       += lms->stat.total_l2_size;
        lms = lms->next;
    }
    mem_stat->alloc_times += _tiny_mem_stat.alloc_times;
    mem_stat->alloc_size  += _tiny_mem_stat.alloc_size;
    mem_stat->free_times  += _tiny_mem_stat.free_times;
    mem_stat->free_size   += _tiny_mem_stat.free_size;
    // Don't count reserved/peak size from tiny_mem_stat. It's already counted
    // in lms

    _std_release_all_mem_spin_lock();
}

// Is the block in list?
// First I will check whether it should be filter out
// by module_name/file_name/line#
static int _std_is_blockIn_list(std_memory_header_t *block,
                                const char *module_name,
                                const char *file_name,
                                int line)
{
#if STD_BLOCK_DETAIL
    STD_ASSERT(block != NULL);

    if (module_name != NULL &&
        _std_stricmp(block->module, module_name))
        // Module name is wrong
        return 0;

    // Get the last name of the file name of the memory block
    if (file_name != NULL)
    {
        if (_std_stricmp(file_name, _std_last_name(block->file)))
            // File name is wrong
            return 0;

        // When file name is valid, try to compare the line#
        if (line > 0 && line != block->line)
            // Not in the same line
            return 0;
    }
#endif

    return 1;
}

// Return the pure file name
static char *_std_last_name(const char *file)
{
    register const char *p;

    p = file + strlen(file);
    while (p >= file && *p != '/' && *p != '\\') p--;
    return (char *) (p + 1);
}

// STRICMP
static int _std_stricmp(const char *s1, const char *s2)
{
    Uint8 ch1, ch2;

    for (;;)
    {
        ch1 = (Uint8) *(s1++);
        ch2 = (Uint8) *(s2++);
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

// Show data
static void _std_show_mem_block(Uint8 *buf, size_t len)
{
    size_t i;
    int    x = 0;
    Uint8  temp[18] = "................\n";

    printf("--------------------------------------------------->>>\n");
    for (i = 0; i < len; i++)
    {
        x = i & 0x0F;

        if (x == 0)
            printf("%04X: ", (int) i);

        temp[x] = buf[i];
        if (temp[x] < 32 || temp[x] == 0xFF)
            temp[x] = '.';

        printf("%02X", buf[i]);

        if (x < 15)
            printf(x == 7 ? "-" : " ");
        else
            printf("        %s", temp);
    }

    if (x < 15)
    {
        // Line not finished, append space
        while (x < 15)
        {
            printf("   ");
            temp[++x] = ' ';
        }
        printf("       %s", temp);
    }
    printf("--------------------------------------------------->>>\n");
}


// Show data
static void _std_dump_mem_block(FILE *fp, Uint8 *buf, size_t len)
{
    size_t i;
    int    x;
    Uint8  temp[18] = "................\n";

    fprintf(fp, "--------------------------------------------------->>>\n");
    for (i = 0; i < len; i++)
    {
        x = i & 0x0F;

        if (x == 0)
            fprintf(fp, "%04X: ", (int) i);

        temp[x] = buf[i];
        if (temp[x] < 32 || temp[x] == 0xFF)
            temp[x] = '.';

        fprintf(fp, "%02X", buf[i]);

        if (x < 15)
            fprintf(fp, x == 7 ? "-" : " ");
        else
            fprintf(fp, "        %s", temp);
    }

    if (x < 15)
    {
        // Line not finished, append space
        while (x < 15)
        {
            fprintf(fp, "   ");
            temp[++x] = ' ';
        }
        fprintf(fp, "       %s", temp);
    }
    fprintf(fp, "--------------------------------------------------->>>\n");
}

// Convert time to string
static char *_std_ctime(time_t *pTime)
{
    static char temp[16];
    char *str;

    str = ctime(pTime);
    memcpy(temp, str + 11, 8);
    temp[8] = 0;
    return temp;
}

// _lock all group
static void _std_get_all_mem_spin_lock()
{
    std_get_spin_lock(&_code_node_stat_spin_lock);
    std_get_spin_lock(&_tiny_block_spin_lock);
    std_get_spin_lock(&_l2_mem_block_list);
    std_get_spin_lock(&_lms_list_spin_lock);
}

// Release all group
static void _std_release_all_mem_spin_lock()
{
    std_release_spin_lock(&_l2_mem_block_list);
    std_release_spin_lock(&_tiny_block_spin_lock);
    std_release_spin_lock(&_code_node_stat_spin_lock);
    std_release_spin_lock(&_lms_list_spin_lock);
}

// _create lms for current thread
static std_lms_t *_std_create_lms()
{
    std_lms_t *lms;
    int        i;

    lms = OS_MALLOC(sizeof(std_lms_t));
    if (lms == NULL)
        STD_FATAL("Failed to create LMS for current thread.\n");
    memset(lms, 0, sizeof(std_lms_t));

    // Get task name
    lms->task_id = std_get_current_task_id();

    for (i = 0; i < STD_SIZE_N(mem_config); i++)
        lms->mem_grp[i].block_size = mem_config[i].block_size;

    std_set_tls_data(_lms_tls_id, lms);

    // Put me into list
    std_get_spin_lock(&_lms_list_spin_lock);
    lms->next = _lms_list;
    _lms_list = lms;
    std_release_spin_lock(&_lms_list_spin_lock);

    return lms;
}

// Get lms of current thread, create new if not found
static std_lms_t *_std_get_or_create_lms()
{
    std_lms_t *lms;

    lms = std_get_tls_data(_lms_tls_id);
    if (lms == NULL)
        lms = _std_create_lms();

    return lms;
}

// Destory lms (only for shutdown or thread end)
// Caller must assure lms is removed from list
static int _std_destroy_lms(std_lms_t *lms)
{
    STD_ASSERT(lms->next == NULL);

    // TODO: _free resource
    return 1;
}

// _allocate a tiny block page
static std_tiny_page_header_t *std_create_tiny_block_page(size_t tiny_block_size)
{
    Uint   count;
    size_t block_and_header_size;
    size_t page_size;
    Uint   i;
    std_tiny_page_header_t   *page;
    Uint8                    *tiny_block_ptr;

    STD_ASSERT(tiny_block_size % STD_TINY_ALIGNMENT_SIZE == 0);
    STD_ASSERT(tiny_block_size >= 0 && tiny_block_size <= (STD_SIZE_N(_tiny_block_page_lists) - 1) * STD_TINY_ALIGNMENT_SIZE);

    // _allocate a memory size <= STD_TINY_BLOCK_PAGE_SIZE & fill with blocks
    block_and_header_size = sizeof(std_tiny_memory_header_t) + tiny_block_size;
    STD_ASSERT(STD_TINY_BLOCK_PAGE_SIZE >= sizeof(std_tiny_page_header_t) + block_and_header_size);
    count = (Uint) ((STD_TINY_BLOCK_PAGE_SIZE - sizeof(std_tiny_page_header_t)) / block_and_header_size);
    STD_ASSERT(count <= 0xFFFF);
    page_size = sizeof(std_tiny_page_header_t) + count * block_and_header_size;
    page = (std_tiny_page_header_t *) std_allocate_memory(page_size, "TINY", __FILE__, __LINE__);
    if (page == NULL)
        return NULL;

    // Init page, build free list
    memset(page, 0, sizeof(std_tiny_page_header_t));
    page->count     = (Uint16) count;
    page->used      = 0;
    page->block_size = (Uint16) tiny_block_size;
    page->next_page = NULL;
    page->free_list     = NULL;
    for (i = 0, tiny_block_ptr = (Uint8 *) (page + 1);
         i < count;
         i++, tiny_block_ptr += block_and_header_size)
    {
        // Add this block into free list
        ((std_tiny_memory_header_t *) tiny_block_ptr)->p.next = page->free_list;
        page->free_list = (std_tiny_memory_header_t *) tiny_block_ptr;
    }

    _tiny_mem_stat.total_reserved_size += page_size;
    if (_tiny_mem_stat.peak_reserved_size < _tiny_mem_stat.total_reserved_size)
        _tiny_mem_stat.peak_reserved_size = _tiny_mem_stat.total_reserved_size;

    // Return the page
    return page;
}

// _free a tiny block page
static void std_delete_tiny_block_page(std_tiny_page_header_t *page)
{
    size_t page_size;
    size_t block_and_header_size;

    STD_ASSERT(page->used == 0);

    block_and_header_size = sizeof(std_tiny_memory_header_t) + page->block_size;
    page_size = sizeof(std_tiny_page_header_t) + page->count * block_and_header_size;
    _tiny_mem_stat.total_reserved_size -= page_size;

    // _free the page memory
    std_free_memory(page, "TINY", __FILE__, __LINE__);
}

// Os memory allocation routines
#define STD_OS_RESERVE_SIZE     32
#define STD_OS_SIZE_SIZE        sizeof(size_t)
#define STD_OS_PAD_DATA         0xF4

// Internal routines of warp routines
static void std_os_mark_valid(void *pb, size_t size);
static void std_os_check_valid(void *pb);

size_t os_alloc_size = 0;

// Warp of malloc
void *std_os_malloc(size_t size)
{
    void *pb;

    pb = malloc(size + STD_OS_RESERVE_SIZE * 2 + STD_OS_SIZE_SIZE);
    if (pb == NULL)
        return NULL;

    // Do stat
    os_alloc_size += size;

    std_os_mark_valid(pb, size);
    return ((char *) pb) + STD_OS_RESERVE_SIZE + STD_OS_SIZE_SIZE;
}

// Warp of realloc
void *std_os_realloc(void *p, size_t size)
{
    void  *pb;
    size_t old_size;

    if (p == NULL)
    {
        pb = NULL;
        old_size = 0;
    } else
    {
        pb = ((char *) p) - STD_OS_RESERVE_SIZE - STD_OS_SIZE_SIZE;
        std_os_check_valid(pb);
        old_size = *(size_t *) (((char *) p) - STD_OS_SIZE_SIZE);
    }

    pb = realloc(pb, size + STD_OS_RESERVE_SIZE * 2 + STD_OS_SIZE_SIZE);
    if (pb == NULL)
        // Failed to reallocate
        return NULL;

    // Do stat
    os_alloc_size -= old_size;
    os_alloc_size += size;

    std_os_mark_valid(pb, size);
    return ((char *) pb) + STD_OS_RESERVE_SIZE + STD_OS_SIZE_SIZE;
}

// Warp of free
void std_os_free(void *p)
{
    void *pb;

    if (p == NULL)
        return;

    pb = ((char *) p) - STD_OS_RESERVE_SIZE - STD_OS_SIZE_SIZE;
    std_os_check_valid(pb);

    // Do stat
    os_alloc_size -= *(size_t *) (((char *) p) - STD_OS_SIZE_SIZE);

    free(pb);
}

// Mark a memory block (pad head & tail)
static void std_os_mark_valid(void *pb, size_t size)
{
    void *p, *pe;

    // ++++++++_size-------------++++++++
    // pb          p            pe 
    p = ((char *) pb) + STD_OS_RESERVE_SIZE + STD_OS_SIZE_SIZE;
    pe = ((char *) p) + size;
    memset(pb, STD_OS_PAD_DATA, STD_OS_RESERVE_SIZE);
    memset(pe, STD_OS_PAD_DATA, STD_OS_RESERVE_SIZE);

    // Record size
    *(size_t *) (((char *) pb) + STD_OS_RESERVE_SIZE) = size;
}

// Check valid of a memory block
static void std_os_check_valid(void *pb)
{
    size_t old_size;
    void  *pe;
    int    i;

    // ++++++++_size-------------++++++++
    // pb          p            pe 

    // Get size, pb, pe
    old_size = *(size_t *) (((char *) pb) + STD_OS_RESERVE_SIZE);
    pe = ((char *) pb) + STD_OS_RESERVE_SIZE + STD_OS_SIZE_SIZE + old_size;

    for (i = 0; i < STD_OS_RESERVE_SIZE; i++)
    {
        if (((char *) pb)[i] != (char) STD_OS_PAD_DATA)
            STD_FATAL("Head corruption of memory.\n");

        if (((char *) pe)[i] != (char) STD_OS_PAD_DATA)
            STD_FATAL("Tail corruption of memory.\n");
    }
}

