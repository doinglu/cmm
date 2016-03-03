// cmm_bitset.cpp
// Initial version by doing Feb/29/2016
// Support bitset operating

#include "std_port/std_port.h"
#include "cmm.h"
#include "cmm_bitset.h"

namespace cmm
{

void Bitset::set_size(size_t size)
{
    // Align the size to sizeof(UintR)
    size = (size + LenMask) & ~(LenMask);
    size_t word_count = size / sizeof(UintR);
    auto* new_bitmap = XNEWN(UintR, word_count);
    if (m_bitmap)
        XDELETEN(m_bitmap);
    m_size = size;
    m_bitmap = new_bitmap;
}

// Is this bit set?
bool Bitset::is_marked(size_t pos)
{
    STD_ASSERT(pos < m_size);
    size_t offset = pos / sizeof(UintR);
    UintR bit = (UintR)1 << (pos & LenMask);
    return (m_bitmap[offset] & bit) ? true : false;
}

// Set bit
// Return previous bit state
bool Bitset::set(size_t pos)
{
    STD_ASSERT(pos < m_size);
    size_t offset = pos / sizeof(UintR);
    UintR bit = (UintR)1 << (pos & LenMask);
    bool ret = (m_bitmap[offset] & bit) ? true : false;
    m_bitmap[offset] |= bit;
    return ret;
}

// Unset bit
// Return previous bit state
bool Bitset::unset(size_t pos)
{
    STD_ASSERT(pos < m_size);
    size_t offset = pos / sizeof(UintR);
    UintR bit = (UintR)1 << (pos & LenMask);
    bool ret = (m_bitmap[offset] & bit) ? true : false;
    m_bitmap[offset] &= ~bit;
    return ret;
}

// OR the set
Bitset& Bitset::operator |=(const Bitset& other)
{
    STD_ASSERT(other.m_size == m_size);
    for (size_t i = 0; i < m_size / sizeof(UintR); i++)
        m_bitmap[i] |= other.m_bitmap[i];
    return *this;
}

// AND the set
Bitset& Bitset::operator &=(const Bitset& other)
{
    STD_ASSERT(other.m_size == m_size);
    for (size_t i = 0; i < m_size / sizeof(UintR); i++)
        m_bitmap[i] &= other.m_bitmap[i];
    return *this;
}

template <typename T>
void Bitset::from_array(const simple::vector<T>& arr)
{
    for (auto it : arr)
        set(it);
}

template <typename T>
void Bitset::to_array(simple::vector<T>& arr)
{
    for (size_t i = 0; i < m_size / sizeof(UintR); i++)
    {
        auto word = m_bitmap[i];
        if (word)
        {
            for (size_t k = 0; k < sizeof(UintR) * 8; k++)
            {
                if (word & 1)
                    arr.push_back[i * (sizeof(UintR) * 8) + k];
                word >>= 1;
            }
        }
    }
}

}
