// Initial version 2005.12.4 by doing
// Immigrated 2015.11.3 by doing

#pragma once

#include "std_template/simple_list.h"
#include "std_template/simple_list.h"
#include "cmm.h"

namespace cmm
{

class FilePath
{
public:
    /* File path mount list */
    struct MountNode
    {
        char real_dir[MAX_PATH_LEN];
        char mount_point[MAX_PATH_LEN];
    };
    typedef simple::list<MountNode> MountNodes;

public:
    // Initialize/shutdown this module
    static bool         init();
    static void         shutdown();

public:
    /* Interfaces */
    static ErrorCode    get_root_dir(char *ret_root_dir, size_t max_len);
    static ErrorCode    set_root_dir(const char *str);
    static ErrorCode    get_output_dir(char *ret_output_dir, size_t max_len);
    static ErrorCode    set_output_dir(const char *str);
    static ErrorCode    derive_file_name(const char *name, const char *suffix, char *dest, size_t size);
    static ErrorCode    generate_local_file_name(const char *file_name, char *local_file_name, size_t size);
    static ErrorCode    generate_output_file_name(const char *in_file_name, char *output_file_name, size_t size);
    static ErrorCode    generate_os_file_name(const char *in_file_name, char *full_bame, size_t size);
    static ErrorCode    assure_path(const char *path);

    /* Mount Operations */
    static ErrorCode    mount(const char *real_dir, const char *mount_point);
    static ErrorCode    unmount(const char *mount_point);
    static ErrorCode    get_mount_list(MountNodes* ptr_list);

private:
    static ErrorCode   make_regular_dir(const char *dir, char *regular_dir, size_t size);
    static ErrorCode   lookup_mount_point(const char *mount_point, char *real_dir = NULL, size_t size = 0);
    static ErrorCode   translate_mount_path(const char *in_file_name, char *full_name, size_t size);

private:
    static const char* OUTPUT_SUFFIX;       // Output suffix
    static char m_root_dir[MAX_PATH_LEN];   // Os path
    static char m_output_dir[MAX_PATH_LEN]; // Under root_dir
    static MountNodes* m_mount_nodes;

    static struct std_critical_section* m_cs;
};

}
