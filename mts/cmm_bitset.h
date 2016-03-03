// cmm_bitset.h
// Initial version by doing Feb/29/2016
// Support bitset operating

#pragma once

#include "std_template/simple_vector.h"
#include "cmm.h"

namespace cmm
{

class Bitset
{
    const size_t LenMask = sizeof(UintR) - 1;

public:
    Bitset() :
        m_bitmap(0),
        m_size(0)
    {
    }

    ~Bitset()
    {
        if (m_bitmap)
            XDELETEN(m_bitmap);
    }

public:
    void set_size(size_t size);

public:
    bool is_marked(size_t pos);
    bool set(size_t pos);
    bool unset(size_t pos);

public:
    Bitset& operator |=(const Bitset& other);
    Bitset& operator &=(const Bitset& other);
    template <typename T>
    void from_array(const simple::vector<T>& arr);
    template <typename T>
    void to_array(simple::vector<T>& arr);

private:
    UintR* m_bitmap;
    size_t m_size;
};

}
