// simple_algorithm.h
// Initial version by doing Feb/29/2016
// Copied from MS STL

#pragma once

#include "simple.h"
#include "simple_util.h"

namespace simple
{

// lower_bound With PredFunc
template<typename RandomIt, typename T, typename PredFunc>
RandomIt lower_bound(RandomIt first, RandomIt last, const T& val, PredFunc pred)
{	// find first element not before val, using pred
    auto count = last - first;

    while (0 < count)
    {	// divide and conquer, find half that contains answer
        auto count2 = count / 2;
        RandomIt mid = first + count2;

        if (pred(*mid, val))
        {	// try top half
            first = ++mid;
            count -= count2 + 1;
        }
        else
            count = count2;
    }
    return first;
}

// lower_bound
template<typename RandomIt, typename T>
inline RandomIt lower_bound(RandomIt first, RandomIt last, const T& val)
{
    return lower_bound(first, last, val, less<>());
}

// upper_bound with PredFunc
template<typename RandomIt, typename T, typename PredFunc>
RandomIt upper_bound(RandomIt first, RandomIt last, const T& val, PredFunc pred)
{	// find first element not before val, using pred
    auto count = last - first;

    while (0 < count)
    {	// divide and conquer, find half that contains answer
        auto count2 = count / 2;
        RandomIt mid = first + count2;

        if (!pred(val, *mid))
        {	// try top half
            first = ++mid;
            count -= count2 + 1;
        }
        else
            count = count2;
    }
    return first;
}

// upper_bound
template<typename RandomIt, typename T>
inline RandomIt upper_bound(RandomIt first, RandomIt last, const T& val)
{
    return upper_bound(first, last, val, less<>());
}

// binary_search with PredFunc
template<typename RandomIt, typename T, typename PredFunc>
inline bool binary_search(RandomIt first, RandomIt last, const T& val, PredFunc pred)
{	// test if val equivalent to some element, using pred
    first = lower_bound(first, last, val, pred);
    return (first != last && !pred(val, *first));
}

// binary_search
template<typename RandomIt, typename T>
inline bool binary_search(RandomIt first, RandomIt last, const T& val)
{	// test if val equivalent to some element, using operator<
    return binary_search(first, last, val, less<>());
}

// Insert sort with PredFunc
// Better for almostly sorted array
template<typename RandomIt, typename PredFunc>
inline void insert_sort(RandomIt first, RandomIt last, PredFunc pred)
{
    for (auto it = first + 1; it < last; ++it)
    {
        if (!pred(*it, *(it - 1)))
            // Not small then previous one
            continue;

        // Shift array & put it @ found_it
        auto found_it = upper_bound(first, it, *it, pred);
        auto temp = move(*it);
        for (auto move_it = it; move_it > found_it; --move_it)
            *move_it = move(*(move_it - 1));
        *found_it = temp;
    }
}

// Insert sort
template<typename RandomIt>
inline void insert_sort(RandomIt first, RandomIt last)
{
    insert_sort(first, last, less<>());
}

// merge with PredFunc
template<typename InIt1, typename InIt2, typename OutIt, typename PredFunc>
OutIt merge(InIt1 first1, InIt1 last1,
                   InIt2 first2, InIt2 last2,
                   OutIt dest, PredFunc pred)
{	// copy merging ranges, both using Pred
    if (first1 != last1 && first2 != last2)
        for (;;)
        {	// merge either first or second
            if (pred(*first2, *first1))
            {	// merge first
                *dest++ = *first2++;
                if (first2 == last2)
                    break;
            } else
            {	// merge second
                *dest++ = *first1++;
                if (first1 == last1)
                    break;
            }
        }

    dest = copy(first1, last1, dest);	// copy any tail
    return (copy(first2, last2, dest));
}

// merge
template<typename InIt1, typename InIt2, typename OutIt>
inline OutIt merge(InIt1 first1, InIt1 last1,
                   InIt2 first2, InIt2 last2,
                   OutIt dest)
{
    return merge(first1, last1, first2, last2, dest, less<>());
}

// set_intersection with Predfunc
template<typename InIt1, typename InIt2, typename OutIt, typename PredFunc>
OutIt set_intersection(InIt1 first1, InIt1 last1,
                              InIt2 first2, InIt2 last2,
                              OutIt dest, PredFunc pred)
{	// AND sets [first1, last1) and [first2, last2), using pred
    for (;first1 != last1 && first2 != last2;)
    {
        if (pred(*first1, *first2))
            ++first1;
        else
        if (pred(*first2, *first1))
            ++first2;
        else
        {
            *dest++ = *first1++;
            ++first2;
        }
    }

    return (dest);
}

// set_intersection
template<typename InIt1, typename InIt2, typename OutIt>
inline OutIt set_intersection(InIt1 first1, InIt1 last1,
                       InIt2 first2, InIt2 last2,
                       OutIt dest)
{
    return set_intersection(first1, last1, first2, last2, dest, less<>());
}

// set_union with Predfunc
template<typename InIt1, typename InIt2, typename OutIt, typename PredFunc>
OutIt set_union(InIt1 first1, InIt1 last1,
                       InIt2 first2, InIt2 last2,
                       OutIt dest, PredFunc pred)
{	// OR sets [first1, last1) and [first2, last2), using pred
    for (;first1 != last1 && first2 != last2;)
    {
        if (pred(*first1, *first2))
        {	// copy first
            *dest++ = *first1;
            ++first1;
        } else
        if (pred(*first2, *first1))
        {	// copy second
            *dest++ = *first2;
            ++first2;
        } else
        {	// advance both
            *dest++ = *first1;
            ++first1;
            ++first2;
        }
    }
    dest = copy(first1, last1, dest);
    return copy(first2, last2, dest);
}

// set_union
template<typename InIt1, typename InIt2, typename OutIt>
inline OutIt set_union(InIt1 first1, InIt1 last1,
                       InIt2 first2, InIt2 last2,
                       OutIt dest)
{
    return set_union(first1, last1, first2, last2, dest, less<>());
}

// unique with PredFunc
template<typename RandomIt, typename PredFunc>
RandomIt unique(RandomIt first, RandomIt last, PredFunc pred)
{	// remove each satisfying pred with previous
    if (first != last)
        for (RandomIt firstb; (void)(firstb = first), ++first != last;)
            if (pred(*firstb, *first))
            {	// copy down
                for (; ++first != last;)
                    if (!pred(*firstb, *first))
                        *++firstb = move(*first);
                return (++firstb);
            }
    return (last);
}

// unique
template<typename RandomIt>
inline RandomIt unique(RandomIt first, RandomIt last)
{
    return unique(first, last, equal_to<>());
}

// equal with PredFunc
template<typename InIt1, typename InIt2, typename PredFunc>
inline bool equal(InIt1 first1, InIt1 last1, InIt2 first2, InIt2 last2, PredFunc pred)
{	// compare [first1, last1) to [first2, ...) using pred
    auto size1 = last1 - first1;
    auto size2 = last2 - first2;
    if (size1 != size2)
        return false;
    while (size1--)
        if (!pred(*first1++, *first2++))
            return false;
    return true;
}

// equal
template<typename InIt1, typename InIt2>
inline bool equal(InIt1 first1, InIt1 last1, InIt2 first2, InIt2 last2)
{
    return equal(first1, last1, first2, last2, equal_to<>());
}

} // End of namespace: simple
