// simple_util.h

#pragma once

namespace simple
{

template<class T> struct remove_reference { typedef T type; };
template<class T> struct remove_reference<T&> { typedef T type; };
template<class T> struct remove_reference<T&&> { typedef T type; };

template<class T>
typename remove_reference<T>::type&& move(T&& a)
{
    typedef typename remove_reference<T>::type&& RvalRef;
    return static_cast<RvalRef>(a);
}

template<class T> struct raw_type_struct { typedef T type; };
template<class T> struct raw_type_struct<const T> { typedef T type; };
template<class T> struct raw_type_struct<volatile T> { typedef T type; };
template<class T> struct raw_type_struct<const volatile T> { typedef T type; };

template<class T>
typename raw_type_struct<T>::type raw_type(T a)
{
    typedef typename raw_type_struct<T>::type RawType;
    return static_cast<RawType>(a);
}

template<class T>
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

} // End of namespace: simple
