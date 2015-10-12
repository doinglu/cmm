// simple_string

#ifndef _SIMPLE_STRING_H_
#define _SIMPLE_STRING_H_

#include "std_port/std_port_util.h"
#include "simple_shared_ptr.h"
#include <string.h>

namespace simple
{

typedef char char_t;

class string
{
    enum
    {
        ShortStringLen = 8,
    };

public:
    typedef unsigned int string_hash_t;
    typedef unsigned int string_size_t;

    string(size_t size)
    {
        m_len = (string_size_t) size;
        // Allocate memory for long string
        if (is_dynamic_allocated())
            m_alloc = XNEWN(char_t, m_len + 1);
        *data_ptr() = 0;
    }

    string(const char *c_str = "")
    {
        m_len = (string_size_t) strlen(c_str);

        // Allocate memory for long string
        if (is_dynamic_allocated())
            m_alloc = XNEWN(char_t, m_len + 1);

        memcpy(data_ptr(), c_str, (m_len + 1) * sizeof(char_t));
    }

	string(const string& s)
	{
		m_len = s.m_len;

		// Allocate memory for long string
		if (is_dynamic_allocated())
		{
			m_alloc = XNEWN(char_t, m_len + 1);
			memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));
		} else
		{
			memcpy(m_buf, s.m_buf, sizeof(m_buf));
		}
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

        m_len = s.m_len;

        // Allocate memory for long string
        if (is_dynamic_allocated())
            m_alloc = XNEWN(char_t, m_len + 1);

        memcpy(data_ptr(), s.data_ptr(), (m_len + 1) * sizeof(char_t));
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

        return *this;
    }

    char_t operator [](size_t index) const
    {
        if (index > m_len)
            throw "Index is out of range in string.";
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
            m_hash_value = hash_string(c_str()) + 1;

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
    static unsigned int hash_string(const char_t *c_str);

private:
    bool is_dynamic_allocated() const
    {
        return m_len >= ShortStringLen;
    }

    char_t *data_ptr() const
    {
        return is_dynamic_allocated() ? m_alloc : (char_t *) m_buf;
    }

private:
public:////----
    mutable string_hash_t m_hash_value = 0;
    string_size_t m_len; // Limited to 4G
    union
    {
        char_t *m_alloc;
        char_t  m_buf[ShortStringLen];
    };
};

class shared_string : public shared_ptr<string>
{
};

inline bool operator == (const string& s1, const string& s2)
{
    return s1.compare(s2) == 0;
}

inline bool operator < (const string& s1, const string& s2)
{
    return s1.compare(s2) < 0;
}

} // End of namespace: simple

#endif
