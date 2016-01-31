// cmm_file_path.cpp
// Initial version 2005.12.4 by doing
// Immigrated 2015.11.3 by doing

#include <sys/stat.h>
#include <string.h>
#include "std_port/std_port_platform.h"
#include "std_port/std_port_cs.h"
#include "cmm_file_path.h"

#ifdef _WINDOWS
#include <windows.h>
#endif

namespace cmm
{

// Root directory & output files directory
const char* FilePath::OUTPUT_SUFFIX = ".cpp";
char FilePath::m_root_dir[MAX_PATH_LEN] = "./";
char FilePath::m_output_dir[MAX_PATH_LEN] = "";

FilePath::MountNodes* FilePath::m_mount_nodes = NULL;

std_critical_section_t* FilePath::m_cs = NULL;

bool FilePath::init()
{
    std_new_critical_section(&m_cs);
    m_mount_nodes = new MountNodes();
    return true;
}

void FilePath::shutdown()
{
    std_delete_critical_section(m_cs);
    delete m_mount_nodes;
}

// Get the root directory
ErrorCode FilePath::get_root_dir(char *ret_dir, size_t max_len)
{
    STD_ASSERT(ret_dir != NULL);
    STD_ASSERT(max_len > 0);

    std_enter_critical_section(m_cs);
    strncpy(ret_dir, m_root_dir, max_len - 1);
    ret_dir[max_len - 1] = 0;
    std_leave_critical_section(m_cs);

    return OK;
}

// Set root directory
ErrorCode FilePath::set_root_dir(const char *str)
{
    ErrorCode ret = OK;
    size_t len;

    STD_ASSERT(str != NULL);

    std_enter_critical_section(m_cs);
    if (! str[0])
        // Miss setting, use current director
        str = "./";

    if ((len = strlen(str)) >= sizeof(m_root_dir) - 1)
    {
        ret = PATH_NAME_TOO_LONG;
    } else
    {
        strcpy(m_root_dir, str);
        if (len)
        {
            // If the root path is not empty, then I will assure the last
            // char of the path name is a slash.
            // Remove the last slash if it existed in the root path
            while (len > 0 && m_root_dir[len - 1] == PATH_SEPARATOR)
                len--;
            m_root_dir[len++] = PATH_SEPARATOR;
            m_root_dir[len] = 0;
        }
    }
    std_leave_critical_section(m_cs);

    return OK;
}

// Get the output directory
ErrorCode FilePath::get_output_dir(char *ret_dir, size_t max_len)
{
    STD_ASSERT(ret_dir != NULL);
    STD_ASSERT(max_len > 0);

    std_enter_critical_section(m_cs);
    strncpy(ret_dir, m_output_dir, max_len - 1);
    ret_dir[max_len - 1] = 0;
    std_leave_critical_section(m_cs);

    return OK;
}

// Set ouotput file directory
// The path is under root-dir
ErrorCode FilePath::set_output_dir(const char *str)
{
    ErrorCode ret;
    size_t len;

    STD_ASSERT(str != NULL);

    std_enter_critical_section(m_cs);

    if (! str[0])
        // Miss setting, use output under current director
        str = "./output";

    if ((len = strlen(str)) >= sizeof(m_output_dir) - 1)
    {
        ret = PATH_NAME_TOO_LONG;
    } else
    {
        strcpy(m_output_dir, str);
        if (len)
        {
            // If the root path is not empty, then I will assure the last
            // char of the path name is a slash.
            // Remove the last slash if it existed in the root path
            while (len > 0 && m_output_dir[len - 1] == PATH_SEPARATOR)
                len--;
            m_output_dir[len++] = PATH_SEPARATOR;
            m_output_dir[len] = 0;
        }
    }

    std_leave_critical_section(m_cs);

    return OK;
}

// Derive file name, remove ./ && ../ from file.
// For those file name doesn't indicate suffix, append the
// suffix when it isn't NULL
// Return 0 means failed
ErrorCode FilePath::derive_file_name(const char *name, const char *suffix,
                                     char *dest, size_t size)
{
    const char *from;
    char *put;
    size_t len;

    STD_ASSERT(name != NULL);
    STD_ASSERT(dest != NULL);
    STD_ASSERT(size > 1);

    size--;

    len  = 0;
    from = name;
    put  = dest;

    // Normal file name
    while (*from)
    {
        while (*from == PATH_SEPARATOR) from++;

        if (from[0] == '.' && from[1] == '.' &&
            from[2] == PATH_SEPARATOR)
        {
            // back 1 level
            if (! len)
                return PATH_OUT_OF_ROOT;

            while (--len > 0 && put[len] != PATH_SEPARATOR);
            from += 3;          // skip "../"
        } else
        if (from[0] == '.' && from[1] == PATH_SEPARATOR)
        {
            // ingore
            from += 2;
        } else
        {
            // a directory
            const char *tmp;
            tmp = strchr(from, PATH_SEPARATOR);
            if (tmp != NULL)
            {
                if (len + 1 + (tmp - from) > size)
                    // File name too long
                    return PATH_NAME_TOO_LONG;

                put[len++] = PATH_SEPARATOR;
                strncpy(put + len, from, (size_t) (tmp - from));
                len += tmp - from;
                from = tmp + 1;
            } else
            {

                if (len + 1 + strlen(from) > size)
                    // File name too long
                    return PATH_NAME_TOO_LONG;

                // this was the last component
                put[len++] = PATH_SEPARATOR;
                strcpy(put + len, from);
                len += strlen(put + len);
                break;
            }
        }
        // check next directory
    }

    // Terminate string
    put[len] = 0;

    if (suffix != NULL && strchr(put, '.') == NULL)
    {
        // Append suffix
        if (len + strlen(suffix) <= size)
            strcat(put, suffix);
    }

    return OK;
}

// Generate local file name
// Return 0 means failed
ErrorCode FilePath::generate_local_file_name(const char *file_name, char *local_file_name, size_t size)
{
    char regular_name[MAX_PATH_LEN];

    STD_ASSERT(file_name != NULL);
    STD_ASSERT(local_file_name != NULL);

    // Derive the file name
    auto ret = derive_file_name(file_name, NULL, regular_name, sizeof(regular_name));
    if (ret != OK)
        return ret;

    return generate_os_file_name(regular_name, local_file_name, size);
}

// Generate a file name in root dir
// Return 0 means failed
ErrorCode FilePath::generate_os_file_name(const char *in_file_name, char *full_name, size_t size)
{
    size_t len;

    STD_ASSERT(in_file_name != NULL);
    STD_ASSERT(full_name != NULL);

    if (translate_mount_path(in_file_name, full_name, size))
        // This file is in a mount direcory
        return OK;

#if 0
    if (is_in_virtual_volume(in_file_name))
    {
        // For those files in virtual volume, don't combine with whole path
        strncpy(full_name, in_file_name, size - 1);
        full_name[size - 1] = 0;
        return OK;
    }
#endif

    // Generate file name
    get_root_dir(full_name, size);
    len = strlen(full_name);
    if (len + strlen(in_file_name) >= size)
        // File name too long
        return PATH_NAME_TOO_LONG;

    if (! full_name[0])
    {
        // No set root dir?
        // Use current directory
        strcpy(full_name, in_file_name);
    } else
    {
        // If the first char in buf is a slash, remove it.
        while (in_file_name[0] == PATH_SEPARATOR)
            in_file_name++;

        // Append @ the tail of root dir
        strcpy(full_name + len, in_file_name);
    }

    // Ok. File name is generated
    return OK;
}

// Generate output file name
// x/y/z.c -> output/x/y/z.cpp
ErrorCode FilePath::generate_output_file_name(const char *in_file_name, char *output_file_name, size_t size)
{
    char temp_name[MAX_PATH_LEN];

    STD_ASSERT(in_file_name != NULL);
    STD_ASSERT(output_file_name != NULL);

    // Remove slash @ tail when necessary
    get_output_dir(temp_name, sizeof(temp_name));
    auto temp_len = strlen(temp_name);
    if (in_file_name[0] == PATH_SEPARATOR &&
        temp_name[temp_len - 1] == PATH_SEPARATOR)
    {
        --temp_len;
        temp_name[temp_len] = 0;
    }

    if (strlen(in_file_name) + temp_len > size - 5)
        // File name is too long
        return PATH_NAME_TOO_LONG;

    // Generate output file name
    strcat(temp_name, in_file_name);
    if (strchr(temp_name, '.') == NULL)
        strcat(temp_name, OUTPUT_SUFFIX);
    else
    {
        IntR i;

        // Search for last dot
        for (i = strlen(temp_name) - 1; i >= 0; i--)
        {
            if (temp_name[i] == '.')
            {
                if (strcmp(temp_name + i, OUTPUT_SUFFIX) == 0)
                    // It's already an output file
                    return FILE_NAME_WAS_OUTPUT_FILE;
                break;
            }
        }

        // Append suffix
        strcpy(temp_name + i, OUTPUT_SUFFIX);
    }

    if (strlen(temp_name) >= size)
        return PATH_NAME_TOO_LONG;

    strcpy(output_file_name, temp_name);
    return OK;
}

// Create directories for path when necessary
ErrorCode FilePath::assure_path(const char *path)
{
    char   temp_name[MAX_PATH_LEN];
    char  *p;
    char   ch;
    struct stat st;

#if false
    if (is_in_dbfs_volume(path))
        // For DBFS directory, always return OK
        return 1;

    if (is_in_packed_volume(path))
        // For packed fs, always return FAILED
        return 0;
#endif

    // Copy the path to temporary buffer
    strncpy(temp_name, path, sizeof(temp_name) - 1);
    temp_name[sizeof(temp_name) - 1] = 0;

    // Skip leading path separator
    p = temp_name;
    while (*p == PATH_SEPARATOR)
        p++;

    // Lookup sub-directory recursively
    while ((p = strchr(p, PATH_SEPARATOR)) != NULL)
    {
        ch = *p;
        *p = 0;

        if (stat(temp_name, &st) == -1)
        {
            // Need to create
#ifdef _WINDOWS
            if (! CreateDirectory(temp_name, NULL))
                // Failed to create directory
                return CAN_NOT_CREATE_DIRECTORY;
#else
            if (mkdir(temp_name, 0777) == -1)
                // Failed to create directory
                return CAN_NOT_CREATE_DIRECTORY;
#endif
        } else
        if (st.st_mode & S_IFDIR)
            // Already be directory, do nothing
            ;
        else
            // A file is already existed, return failed
            return CAN_NOT_CREATE_DIRECTORY;

        *p = ch;
        p++;
    }

    return OK;
}

// Mount directory to mount point
ErrorCode FilePath::mount(const char *real_dir, const char *mount_point)
{
    ErrorCode ret = OK;
    MountNode node;

    STD_ASSERT(real_dir != NULL);
    STD_ASSERT(mount_point != NULL);

    if (! make_regular_dir(real_dir, node.real_dir, sizeof(node.real_dir)) ||
        ! make_regular_dir(mount_point, node.mount_point, sizeof(node.real_dir)))
        // Failed to make path to regular name, must be too long
        return PATH_NAME_TOO_LONG;

    // Check first char of mount point, expect path separator
    if (node.mount_point[0] != PATH_SEPARATOR)
    {
        STD_TRACE("Mount point must be leaded by path separator '%c'\n",
                  PATH_SEPARATOR);
        return BAD_MOUNT_POINT;
    }

    if (lookup_mount_point(node.mount_point) == OK)
        // The mount point is already existed
        return MOUNT_POINT_EXISTED;

    // Put to list
    std_enter_critical_section(m_cs);
    try
    {
        m_mount_nodes->append(node);
    }
    catch (...)
    {
        ret = NOT_ENOUGH_MEMORY;
    }
    std_leave_critical_section(m_cs);

    return ret;
}

// Unmount mount point
ErrorCode FilePath::unmount(const char *mount_point)
{
    ErrorCode ret = NOT_FOUND_MOUNT_POINT;
    char regular_mount_point[MAX_PATH_LEN];

    STD_ASSERT(mount_point != NULL);

    if (! make_regular_dir(mount_point, regular_mount_point, sizeof(regular_mount_point)))
        // Failed to make path to regular name, must be too long
        return NOT_FOUND_MOUNT_POINT;

    std_enter_critical_section(m_cs);
    for (auto it = m_mount_nodes->begin(); it != m_mount_nodes->end(); ++it)
    {
        // Compare with this node
        if (strcmp(regular_mount_point, it->mount_point) == 0)
        {
            // Matched, remove this one
            m_mount_nodes->remove(it);
            ret = OK;
            break;
        }
    }
    std_leave_critical_section(m_cs);

    // Not found in mount list
    return ret;
}

// Return the mount list
ErrorCode FilePath::get_mount_list(MountNodes* ptr_list)
{
    ErrorCode ret = OK;

    STD_ASSERT(ptr_list != NULL);

    std_enter_critical_section(m_cs);
    try
    {
        // Copy it
        *ptr_list = *m_mount_nodes;
    }
    catch (...)
    {
        ret = NOT_ENOUGH_MEMORY;
    }
    std_leave_critical_section(m_cs);
    return ret;
}

// Make path name to regular path name
// Convert all '\' to separator
// Append VM_PATH separator @ tail if not existed
ErrorCode FilePath::make_regular_dir(const char *dir, char *regular_dir, size_t size)
{
    // 1 (terminator) + 1 (separator), so there is size - 2
    if (strlen(dir) >= size - 2)
        return PATH_NAME_TOO_LONG;

    if (! *dir)
    {
        // Empty name
        regular_dir[0] = PATH_SEPARATOR;
        regular_dir[1] = 0;
        return OK;
    }

    // Copy & convert
    while (*dir)
    {
        if (*dir == '\\')
            *regular_dir = PATH_SEPARATOR;
        else
            *regular_dir = *dir;

        dir++;
        regular_dir++;
    }

    // Append separator @ tail, *(regular_dir - 1) must be
    // valid since dir won't be empty name
    if (*(regular_dir - 1) != PATH_SEPARATOR)
        *(regular_dir++) = PATH_SEPARATOR;

    // Terminate string
    *regular_dir = 0;

    return OK;
}

// Return real director by mount point
// Return NULL means not found in mount list
ErrorCode FilePath::lookup_mount_point(const char *mount_point, char *real_dir, size_t size)
{
    ErrorCode ret = NOT_FOUND_MOUNT_POINT;

    std_enter_critical_section(m_cs);
    for (auto& it : *m_mount_nodes)
    {
        // Compare with this node
        if (strcmp(mount_point, it.mount_point) == 0)
        {
            // Matched
            if (real_dir != NULL)
            {
                // Copy to real dir
                if (size <= strlen(it.real_dir))
                {
                    ret = PATH_NAME_TOO_LONG;
                    break;
                }
                strcpy(real_dir, it.real_dir);
            }
            ret = OK;
            break;
        }
    }
    std_leave_critical_section(m_cs);

    return ret;
}

// Try to match the directory in mount list & conver to final path
ErrorCode FilePath::translate_mount_path(const char *in_file_name, char *full_name, size_t size)
{
    char  try_mount_point[MAX_PATH_LEN];
    char  real_dir[MAX_PATH_LEN];
    char *to;
    IntR  offset;

    STD_ASSERT(in_file_name != NULL);
    STD_ASSERT(full_name != NULL);

    // 1 is terminator, 1 is tail path separator, so there is 2
    if (strlen(in_file_name) >= sizeof(try_mount_point) - 2)
    {
        STD_TRACE("in_file_name (%s) is too long to found in mount point.\n",
                  in_file_name);
        return PATH_NAME_TOO_LONG;
    }
    strcpy(try_mount_point, in_file_name);

    while ((to = strrchr(try_mount_point, PATH_SEPARATOR)) != NULL)
    {
        // Terminate path after separator
        to[1] = 0;

        // Get offset of file name in in_file_name
        offset = to - try_mount_point + 1;
        STD_ASSERT(offset >= 0 && offset <= (IntR) strlen(in_file_name));
        ErrorCode ret = lookup_mount_point(try_mount_point, real_dir, sizeof(real_dir));
        if (ret == OK)
        {
            // Found this mount point
            if (strlen(real_dir) + strlen(in_file_name + offset) >= size)
            {
                STD_TRACE("full_name is no enough space to carry real file name\n");
                return PATH_NAME_TOO_LONG;
            }

            // Generate full path name
            strcpy(full_name, real_dir);
            strcat(full_name, in_file_name + offset);
            return OK;
        }

        // Remove last separator
        *to = 0;
    }

    // Not found in mount list
    return NOT_FOUND_MOUNT_POINT;
}

}
