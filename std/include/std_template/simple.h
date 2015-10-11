// simple.h

#ifndef _SIMPLE_H_
#define _SIMPLE_H_

namespace simple
{

template<class T> struct remove_reference      {typedef T type;};
template<class T> struct remove_reference<T&>  {typedef T type;};
template<class T> struct remove_reference<T&&> {typedef T type;};

template<class T>
typename remove_reference<T>::type&& move(T&& a)
{
    typedef typename remove_reference<T>::type&& RvalRef;
    return static_cast<RvalRef>(a);
}

template<class T>
T&& forward(typename remove_reference<T>::type& a)
{
  return static_cast<T&&>(a);
}

} // End of namespace: simple

#endif
