// simple_util.h

#pragma once

#include <string.h>

namespace simple
{

template<typename T> struct remove_reference { typedef T type; };
template<typename T> struct remove_reference<T&> { typedef T type; };
template<typename T> struct remove_reference<T&&> { typedef T type; };

template<typename T>
typename remove_reference<T>::type&& move(T&& a)
{
    typedef typename remove_reference<T>::type&& RvalRef;
    return static_cast<RvalRef>(a);
}

template<typename T> struct raw_type_struct { typedef T type; };
template<typename T> struct raw_type_struct<const T> { typedef T type; };
template<typename T> struct raw_type_struct<volatile T> { typedef T type; };
template<typename T> struct raw_type_struct<const volatile T> { typedef T type; };

template<typename T>
typename raw_type_struct<T>::type raw_type(T a)
{
    typedef typename raw_type_struct<T>::type RawType;
    return static_cast<RawType>(a);
}

template<typename T>
T&& forward(typename remove_reference<T>::type& a)
{
    return static_cast<T&&>(a);
}

// TEMPLATE FUNCTIONS

template<typename T> inline
    void swap(T& a, T& b)
{
    T tmp(a);
    a = simple::move(b);
    b = simple::move(tmp);
}

// TEMPLATE OPERATORS
template<typename T> inline
    bool operator != (const T& p1, const T& p2)
{	// test for inequality, in terms of equality
    return (!(p1 == p2));
}

template<typename T> inline
bool operator > (const T& p1, const T& p2)
{	// test if p1 > p2, in terms of operator<
    return (p2 < p1);
}

template<typename T> inline
bool operator <= (const T& p1, const T& p2)
{	// test if p1 <= p2, in terms of operator<
    return (!(p2 < p1));
}

template<typename T> inline
bool operator >= (const T& p1, const T& p2)
{	// test if p1 >= p2, in terms of operator<
    return (!(p1 < p2));
}

// less
template<typename T = void>
struct less
{	// functor for operator<
    typedef typename remove_reference<T>::type argument_type;
    typedef argument_type first_argument_type;
    typedef argument_type second_argument_type;
    typedef bool result_type;

    constexpr bool operator()(const argument_type& left, const argument_type& right) const
    {	// apply operator< to operands
        return (left < right);
    }
};

// less
template<>
struct less<void>
{	// functor for operator<

    template<typename T1, typename T2>
    constexpr bool operator()(const T1& left, const T2& right) const
    {	// apply operator< to operands
        return (left < right);
    }
};

// equal_to
template<typename T = void>
struct equal_to
{	// functor for operator==
    typedef typename remove_reference<T>::type argument_type;
    typedef argument_type first_argument_type;
    typedef argument_type second_argument_type;
    typedef bool result_type;

    constexpr bool operator()(const argument_type& left, const argument_type& right) const
    {	// apply operator== to operands
        return (left == right);
    }
};

// equal_to
template<>
struct equal_to<void>
{	// functor for operator==

    template<typename T1, typename T2>
    constexpr bool operator()(const T1& left, const T2& right) const
    {	// apply operator== to operands
        return (left == right);
    }
};

// copy
template<typename InIt, typename OutIt>
inline OutIt copy(InIt first, InIt last, OutIt dest)
{	// copy [first, last) to [dest, ...), arbitrary iterators
    for (; first != last; ++dest, (void)++first)
        *dest = *first;
    return dest;
}

// init to zero
template <typename T, typename ITYPE>
inline void init_class(T** ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(T*) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(bool* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(bool) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(char* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(char) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(short* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(short) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(int* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(int) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(long long* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(long long) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(unsigned char* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(char) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(unsigned short* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(short) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(unsigned int* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(int) * n); ok_count += n; }
template <typename ITYPE>
inline void init_class(unsigned long long* ptr, size_t n, ITYPE& ok_count) { memset(ptr, 0, sizeof(long long) * n); ok_count += n; }

// construct class
template <typename T, typename ITYPE>
inline void init_class (T* ptr, size_t n, ITYPE& ok_count)
{
    while (n)
    {
        new (ptr)T();
        n--;
        ok_count++;
        ptr++;
    }
}

} // End of namespace: simple
