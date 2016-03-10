// simple_string

#pragma once

#include "std_port/std_port_util.h"
#include "simple_shared_ptr.h"
#include <string.h>

namespace simple
{

typedef char char_t;
typedef unsigned char uchar_t;

enum reserve_space
{
    EMPTY
};

class string
{
    enum
    {
        SHORT_STRING_LEN = 8,
    };

public:
    typedef unsigned int string_hash_t;
    typedef unsigned int string_size_t;

    // ATTENTION:
    // Argument unused is to prevent this class being constructed from size_t 
    string(reserve_space unused, size_t size)
    {
        set_length(size);
        data_ptr()[0] = 0;
    }

    string(const char *c_str = "", size_t len = SIZE_MAX)
    {
        size_t str_length = strlen(c_str);
        if (len > str_length)
            len = str_length;
        set_length(len);

        memcpy(data_ptr(), c_str, m_len * sizeof(char_t));
        data_ptr()[m_len] = 0; // Append terminator
    }

	string(const string& s)
	{
        set_length(s.m_len);
		memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));
    }

    string(string&& s)
    {
        m_len = s.m_len;

        // Allocate memory for long string
        if (is_dynamic_allocated())
        {
            // Steal memory allocated by s & set s to zero length
            m_alloc = s.m_alloc;
            s.m_len = 0;
        } else
            memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));
    }

    ~string()
    {
        if (is_dynamic_allocated())
        {
            XDELETEN(m_alloc);
            STD_DEBUG_SET_NULL(m_alloc);
        }
        STD_DEBUG_SET_NULL(m_len);
    }

    string& operator = (const string& s)
    {
        if (is_dynamic_allocated())
            XDELETEN(m_alloc);

        set_length(s.m_len);
        memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));
        m_hash_value = s.m_hash_value;
        return *this;
    }

    string& operator = (string&& s)
    {
        if (is_dynamic_allocated())
            XDELETEN(m_alloc);

        m_len = s.m_len;

        // Allocate memory for long string
        if (is_dynamic_allocated())
        {
            // Steal memory allocated by s & set s to zero length
            m_alloc = s.m_alloc;
            s.m_len = 0;
        }
        else
            memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));

        m_hash_value = s.m_hash_value;
        return *this;
    }

    // Concat
    string operator + (const string& s) const
    {
        size_t new_len = this->m_len + s.m_len;
        string new_str(reserve_space::EMPTY, new_len);

        char_t *new_p = new_str.data_ptr();
        memcpy(new_p, this->data_ptr(), this->m_len * sizeof(char_t));
        memcpy(new_p + this->m_len, s.data_ptr(), (s.m_len + 1) * sizeof(char_t));
        return simple::move(new_str);
    }

    // Concat
    string& operator += (const string& s)
    {
        size_t new_len = this->m_len + s.m_len;
        string new_str(reserve_space::EMPTY, new_len);

        char_t *new_p = new_str.data_ptr();
        memcpy(new_p, this->data_ptr(), this->m_len * sizeof(char_t));
        memcpy(new_p + this->m_len, s.data_ptr(), (s.m_len + 1) * sizeof(char_t));
        return *this = simple::move(new_str);
    }

    char_t operator [](size_t index) const
    {
        if (index > m_len)
            throw "Index is out of range in string.\n";
        return data_ptr()[index];
    }

    const char_t *c_str() const
    {
        return data_ptr();
    }

    int compare(const string& sd) const
    {
        size_t len = m_len > sd.m_len ? m_len : sd.m_len;
        // tip: Since the last char of string is 0, if the two
        // strings' length are not equal, the memcpy also return
        // the right result.
        return memcmp(data_ptr(), sd.data_ptr(), len);
    }

    // Hash me
    size_t hash_value() const
    {
        if (! m_hash_value)
            m_hash_value = hash_string(data_ptr()) + 1;

        return (size_t) m_hash_value;
    }

    // Get length
    size_t length() const
    {
        return m_len;
    }

    // Generate by format string
    string& snprintf(const char *fmt, size_t n, ...);

public:
    // Hash a c_str
    static unsigned int hash_string(const char_t *c_str, size_t maxn = 64);

private:
    bool is_dynamic_allocated() const
    {
        return m_len >= SHORT_STRING_LEN;
    }

    char_t *data_ptr() const
    {
        return is_dynamic_allocated() ? m_alloc : (char_t *) m_buf;
    }

    // Reserve space & set length to specified size
    void set_length(size_t size)
    {
        m_len = (string_size_t)size;
        // Allocate memory for long string
        if (is_dynamic_allocated())
            m_alloc = XNEWN(char_t, m_len + 1);
    }

private:
    mutable string_hash_t m_hash_value = 0;
    string_size_t m_len; // Limited to 4G
    union
    {
        char_t *m_alloc;
        char_t  m_buf[SHORT_STRING_LEN];
    };
};

class shared_string : public shared_ptr<string>
{
};

inline bool operator == (const string& s1, const string& s2)
{
    return s1.compare(s2) == 0;
}

inline bool operator != (const string& s1, const string& s2)
{
    return s1.compare(s2) != 0;
}

inline bool operator < (const string& s1, const string& s2)
{
    return s1.compare(s2) < 0;
}

inline bool operator <= (const string& s1, const string& s2)
{
    return s1.compare(s2) <= 0;
}

inline bool operator > (const string& s1, const string& s2)
{
    return s1.compare(s2) > 0;
}

inline bool operator >= (const string& s1, const string& s2)
{
    return s1.compare(s2) >= 0;
}

} // End of namespace: simple
