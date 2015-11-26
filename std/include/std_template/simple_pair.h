// simple_pair.h

#pragma once

namespace simple
{

// pair
template<typename V1, typename V2>
struct pair
{	// store a pair of values
    typedef pair<V1, V2> type;
    typedef V1 first_type;
    typedef V2 second_type;

    pair()
        : first(), second()
    {	// default construct
    }

    pair(const V1& v1, const V2& v2)
        : first(v1), second(v2)
    {	// construct from specified values
    }

    pair(const pair&) = default;
    pair(pair&&) = default;

    type& operator = (const type& p2) = default;
    type& operator = (type&& p2) = default;

    template<typename T1, typename T2>
    type& operator = (const pair<T1, T2>& p2)
    {	// assign from compatible pair
        first = p2.first;
        second = p2.second;
        return (*this);
    }

    template<class T1, class T2>
    type& operator = (pair<T1, T2>&& p2)
    {	// assign from moved compatible pair
        first = simple::forward(p2.first);
        second = simple::forward(p2.second);
        return (*this);
    }

    V1 first;	// the first stored value
    V2 second;	// the second stored value
};

} // End of namespace: simple
