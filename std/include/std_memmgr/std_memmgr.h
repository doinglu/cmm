// std_memmgr.c:
// Initial version 2002.2.24 by doing

#ifndef _STD_MEM_MGR_H_
#define _STD_MEM_MGR_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "std_port/std_port.h"

// Error code
#define STD_NOT_INITIALIZED             -10001
#define STD_ALREADY_INITIALIZED         -10002
#define STD_MEMORY_BLOCK_ACTIVE         -10003
#define STD_BAD_MEMORY_GROUP            -10004
#define STD_NO_SUCH_MEMORY_GROUP        -10005
#define STD_MEMORY_NOT_ENOUGH           -10006
#define STD_FAILED_INIT_BA_POOL         -10007

#define STD_MEM_ALLOC(size)             std_allocate_memory(size, "STD", __FILE__, __LINE__)
#define STD_MEM_ALLOCX(size, f, l)      std_allocate_memory(size, "STD", f, l)
#define STD_MEM_REALLOC(p, size)        std_reallocate_memory(p, size, "STD", __FILE__, __LINE__)
#define STD_MEM_FREE(ptr)               std_free_memory(ptr, "STD", __FILE__, __LINE__)

// Options for this manager
#define STD_USE_BA_ALLOC                0x0001
typedef struct std_init_mgr_para
{
    int     options;
    size_t  ba_reserve_size;        // Valid when STD_USE_BA_ALLOC only
} std_init_mgr_para_t;

#ifdef _DEBUG
/* Do stat for debug mode with block detail mode */
#define STD_BLOCK_DETAIL        1
#define STD_STAT_ALLOC          (STD_BLOCK_DETAIL)
#else
/* Not stat */
#define STD_BLOCK_DETAIL        0
#define STD_STAT_ALLOC          0
#endif

/* Don't use tiny allocation */
#ifdef WIN32
#define STD_USE_TINY_ALLOC      1
#else
#define STD_USE_TINY_ALLOC      0
#endif

#if STD_STAT_ALLOC
/* Code node to allocate memory */
typedef struct Mem_Code_Node
{
    const  char *file;
    Uint32 line;
} mem_code_node_t;

/* Stat of operation */
typedef struct mem_code_node_stat
{
    mem_code_node_t n;
    size_t counter;
    size_t size;
} mem_code_node_stat_t;
#endif

/* Memory configuration information */
typedef struct std_memory_config
{
    Uint32 block_size;
    Uint32 block_count;
} std_memory_config_t;

/* Special value in the head of memory header */
#define STD_MEMORY_BLOCK_SIGN   0x99998086
#define STD_MEMORY_BLOCK_TAIL   0xAF

/* Flags of memory header */
#define STD_MEMORY_GROUP_BLOCK  0x0001  /* Block is a allocated in group */
#define STD_MEMORY_L2           0x0002  /* _l2 memory                     */
#define STD_MEMORY_ALLOCATED    0x0010  /* Block is not free now         */

/* Alignment size */
#ifdef PLATFORM64
#define STD_MEMORY_ALIGNMENT_SIZE           8
#else
#define STD_MEMORY_ALIGNMENT_SIZE           4
#endif

/* Reserved bytes for each block
* Must be n * STD_MEMORY_ALIGNMENT_SIZE */
#ifdef _DEBUG
#define STD_BLOCK_RESERVED      16
#else
#define STD_BLOCK_RESERVED      0
#endif

/* For tiny block alignment */
#define STD_TINY_ALIGNMENT_SIZE             sizeof(void *)

/* Page pool */
/* The page pool is a memory pool to hold n same size pages */
#define STD_MEM_PAGE_POOL_SIZE              (1024 * 1024)

/* _size of per page in page pool */
#define STD_PER_MEM_PAGE_SIZE               4096

/* Page pool structure */
struct std_mem_page;
typedef struct std_mem_page_pool
{
    struct std_mem_page *free_page;
    Uint16 page_count;
} std_mem_page_pool_t;

/* Page strucuture */
/* The first field is reserved for page management,
 * don't use for allocator */
typedef struct std_mem_page
{
    union
    {
        struct std_mem_page      *next_free_page;
        struct std_mem_page_Pool *page_pool;
    } v;
} std_mem_page_t;

/* _size of values page (bytes) */
#define STD_TINY_BLOCK_PAGE_SIZE            4096

/* Count of page list (all memory pools) */
#define STD_TINY_BLOCK_PAGE_LIST_COUNT      256

/* Tiny page */
typedef struct std_tiny_page_header
{
    Uint16 used;
    Uint16 count;
    Uint16 block_size;
    Uint16 unused;
    struct std_tiny_page_header   *next_page;
    struct std_tiny_memory_header *free_list;
} std_tiny_page_header_t;

/* Tiny memory block in pool */
typedef struct std_tiny_memory_header
{
    /* This field must be last field in the structure
     * To make sure the sizeof(Vm_Value_t) is alignment to machine register */
    union
    {
        struct std_tiny_page_header   *page; /* The owner page (when allocated)     */
        struct std_tiny_memory_header *next; /* The next free value (not allocated) */
    } p;
} std_tiny_memory_header_t;

/* Memory block header */
/* For pre-allocated blocks, the header is act as single-direction
 * list. For those l2 blocks, the header would be double-direction
 * list. */
typedef struct std_memory_header
{
    /* Header of memory */
#if STD_BLOCK_DETAIL
    Uint32      sign;
    Uint32      line;
    const char *file;
    const char *module;
    time_t      allocate_time;
    struct      std_memory_header *next_l2;
#ifdef PLATFORM64
    void       *unused;    /* To alignment 16 */
#endif
#endif /* End of detail information */
    Uint32      size;
    Uint8       serial;
    Uint8       tag;
    Uint16      flags;
    union
    {
        struct  std_memory_header *next;
        struct  std_lms *owner_lms;
    };
} std_memory_header_t;

/* OS memory function */
#ifdef GSLIB_OVERRIDE_OS_MALLOC
void* _override_gslib_os_malloc(size_t);
void* _override_gslib_os_realloc(void *, size_t);
void  _override_gslib_os_free(void *);
#define OS_MALLOC(n)            _override_gslib_os_malloc((size_t) n)
#define OS_REALLOC(p, n)        _override_gslib_os_realloc(p, (size_t) n)
#define OS_FREE(p)              _override_gslib_os_free(p)
#else
#define OS_MALLOC(n)            malloc((size_t) (n))
#define OS_REALLOC(p, n)        realloc(p, (size_t) (n));
#define OS_FREE(p)              free(p)
#endif

/* To get block header */
#define STD_GET_HDR(p)          (((std_memory_header_t *) p) - 1)

/* Parameter for pdb_memoryStat */
typedef struct std_mem_stat
{
    size_t alloc_times;
    size_t alloc_size;
    size_t free_times;
    size_t free_size;
    size_t total_reserved_size;
    size_t total_used_size;
    size_t total_l2_size;
    size_t peak_reserved_size;
    size_t extend_size;
} std_mem_stat_t;

/* Parameter for pdb_stable_heapStat */
typedef struct std_stable_heap_stat
{
    size_t stable_heap_allocated_size;
    size_t stable_heap_size;
    size_t stable_heap_free_list_times;
    size_t stable_heap_allocate_times;
    size_t stable_heap_allocateFailed;
} std_stable_heap_stat_t;

struct std_lms;

/* Function provide to main module */
int        std_init_mem_mgr(std_init_mgr_para_t* paras);
int        std_shutdown_mem_mgr();

/* Allocate page */
void      *std_allocate_mem_page();
void       std_free_mem_page(void *page);

/* Get stat information */
int        std_is_mem_mgr_installed();
int        std_get_block_reserved(size_t *ptr_reserved_bytes);
int        std_get_tiny_block_page_listsCount(size_t *ptr_count);
int        std_get_tiny_block_page_list(std_tiny_page_header_t **pp_lists, size_t index);

void      *std_allocate_memory(size_t size, const char *moduleName, const char *file, int line);
void      *std_reallocate_memory(void *ptr, size_t size, const char *moduleName, const char *file, int line);
void       std_free_memory(void *ptr, const char *moduleName, const char *file, int line);

#if STD_BLOCK_DETAIL
struct std_memory_header *std_get_l2mem_blocks_list();
#endif

#if STD_STAT_ALLOC
/* To get stat of alloc */
int        std_get_code_node_stat(mem_code_node_stat_t **pp_code_nodes, size_t *ptr_code_nodes_count);
int        std_reset_code_node_stat(size_t min_size, size_t max_size);
#endif

int        std_memory_stat(int flag, std_mem_stat_t *ptr_mem_stat, char *msg, size_t size);

/* Os memory allocation routines */
void      *std_os_malloc(size_t size);
void      *std_os_realloc(void *p, size_t size);
void       std_os_free(void *p);

#ifdef __cplusplus
}
#endif

#endif
