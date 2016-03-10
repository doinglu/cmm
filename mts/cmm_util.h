// cmm_util.h
// Initial version by doing Mar/6/2016
// Common utilities

#include "std_template/simple_vector.h"

#pragma once

namespace cmm
{

template<typename T>
typename simple::vector<T>::iterator
   insert_sorted(simple::vector<T>& vec, T const& item)
{
    return vec.insert
        (
            simple::upper_bound(vec.begin(), vec.end(), item),
            item 
        );
}

template<typename T, typename PredFunc>
typename simple::vector<T>::iterator
    insert_sorted(simple::vector<T>& vec, T const& item, PredFunc pred)
{
    return vec.insert
        (
            simple::upper_bound(vec.begin(), vec.end(), item, pred),
            item 
        );
}

}
