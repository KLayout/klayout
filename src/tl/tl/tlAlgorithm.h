
#ifndef HDR_tlAlgorithm
#define HDR_tlAlgorithm

#include <iterator>
#include <algorithm>

/*
 *  This header is a copy of the gcc STL "stl_algo" and "stl_algobase"
 *  functionality where performance enhancements were required.
 *  Such enhancements were necessary for example to use the swap
 *  operation where possible.
 */

namespace tl
{
  enum { _M_threshold = 16 };

  template<typename _ForwardIter1, typename _ForwardIter2>
  inline void
  tl_iter_swap(_ForwardIter1 __a, _ForwardIter2 __b)
  {
    std::swap (*__a, *__b);
  }

  template<typename _Tp>
  inline const _Tp&
  __median(const _Tp& __a, const _Tp& __b, const _Tp& __c)
  {
    if (__a < __b) {
      if (__b < __c)
        return __b;
      else if (__a < __c)
        return __c;
      else
        return __a;
    } else if (__a < __c) {
      return __a;
    } else if (__b < __c) {
      return __c;
    } else {
      return __b;
    }
  }

  template<typename _Tp, typename _Compare>
  inline const _Tp&
  __median(const _Tp& __a, const _Tp& __b, const _Tp& __c, _Compare __comp)
  {
    if (__comp(__a, __b)) {
      if (__comp(__b, __c))
        return __b;
      else if (__comp(__a, __c))
        return __c;
      else
        return __a;
    } else if (__comp(__a, __c)) {
      return __a;
    } else if (__comp(__b, __c)) {
      return __c;
    } else {
      return __b;
    }
  }

  template<typename _Size>
  inline _Size
  __lg(_Size __n)
  {
    _Size __k;
    for (__k = 0; __n != 1; __n >>= 1) ++__k;
      return __k;
  }

  template<typename _RandomAccessIter, typename _Tp>
  void
  __unguarded_linear_insert(_RandomAccessIter __last, const _Tp &__val)
  {
    _RandomAccessIter __next = __last;
    --__next;
    while (__val < *__next) {
      *__last = *__next;
      __last = __next;
      --__next;
    }
    *__last = __val;
  }

  template<typename _RandomAccessIter, typename _Tp, typename _Compare>
  void
  __unguarded_linear_insert(_RandomAccessIter __last, const _Tp &__val, 
                            _Compare __comp)
  {
    _RandomAccessIter __next = __last;
    --__next;
    while (__comp(__val, *__next)) {
      *__last = *__next;
      __last = __next;
      --__next;
    }
    *__last = __val;
  }

  template<typename _RandomAccessIter>
  void
  __insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last)
  {
    if (__first == __last) return;

    for (_RandomAccessIter __i = __first + 1; __i != __last; ++__i)
    {
      typename std::iterator_traits<_RandomAccessIter>::value_type __val = *__i;
      if (__val < *__first) {
        std::copy_backward(__first, __i, __i + 1);
        *__first = __val;
      }
      else
        tl::__unguarded_linear_insert(__i, __val);
    }
  }

  template<typename _RandomAccessIter, typename _Compare>
  void
  __insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last,
                   _Compare __comp)
  {
    if (__first == __last) return;

    for (_RandomAccessIter __i = __first + 1; __i != __last; ++__i)
    {
      typename std::iterator_traits<_RandomAccessIter>::value_type __val = *__i;
      if (__comp(__val, *__first)) {
        std::copy_backward(__first, __i, __i + 1);
        *__first = __val;
      }
      else
        tl::__unguarded_linear_insert(__i, __val, __comp);
    }
  }

  template<typename _RandomAccessIter>
  inline void
  __unguarded_insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    for (_RandomAccessIter __i = __first; __i != __last; ++__i)
      tl::__unguarded_linear_insert(__i, _ValueType(*__i));
  }

  template<typename _RandomAccessIter, typename _Compare>
  inline void
  __unguarded_insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last,
                             _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    for (_RandomAccessIter __i = __first; __i != __last; ++__i)
      tl::__unguarded_linear_insert(__i, _ValueType(*__i), __comp);
  }

  template<typename _RandomAccessIter>
  void
  __final_insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last)
  {
    if (__last - __first > _M_threshold) {
      tl::__insertion_sort(__first, __first + _M_threshold);
      tl::__unguarded_insertion_sort(__first + _M_threshold, __last);
    }
    else
      tl::__insertion_sort(__first, __last);
  }

  template<typename _RandomAccessIter, typename _Compare>
  void
  __final_insertion_sort(_RandomAccessIter __first, _RandomAccessIter __last,
                         _Compare __comp)
  {
    if (__last - __first > _M_threshold) {
      tl::__insertion_sort(__first, __first + _M_threshold, __comp);
      tl::__unguarded_insertion_sort(__first + _M_threshold, __last, __comp);
    }
    else
      tl::__insertion_sort(__first, __last, __comp);
  }

  template<typename _RandomAccessIter, typename _Tp>
  _RandomAccessIter
  __unguarded_partition(_RandomAccessIter __first, _RandomAccessIter __last, 
                        const _Tp &__pivot)
  {
    while (true) {
      while (*__first < __pivot)
        ++__first;
      --__last;
      while (__pivot < *__last)
        --__last;
      if (!(__first < __last))
        return __first;
      tl::tl_iter_swap(__first, __last);
      ++__first;
    }
  }

  template<typename _RandomAccessIter, typename _Tp, typename _Compare>
  _RandomAccessIter
  __unguarded_partition(_RandomAccessIter __first, _RandomAccessIter __last,
                        const _Tp &__pivot, _Compare __comp)
  {
    while (true) {
      while (__comp(*__first, __pivot))
        ++__first;
      --__last;
      while (__comp(__pivot, *__last))
        --__last;
      if (!(__first < __last))
        return __first;
      tl::tl_iter_swap(__first, __last);
      ++__first;
    }
  }

  template<typename _RandomAccessIterator, typename _Distance, typename _Tp>
  void 
  __push_heap(_RandomAccessIterator __first, _Distance __holeIndex, 
              _Distance __topIndex, const _Tp &__v)
  {
    _Distance __parent = (__holeIndex - 1) / 2;
    while (__holeIndex > __topIndex && *(__first + __parent) < __v) {
      *(__first + __holeIndex) = *(__first + __parent);
      __holeIndex = __parent;
      __parent = (__holeIndex - 1) / 2;
    }    
    *(__first + __holeIndex) = __v;
  }

  template<typename _RandomAccessIterator>
  inline void 
  push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _DistanceType;

    tl::__push_heap(__first, _DistanceType((__last - __first) - 1), _DistanceType(0), 
        _ValueType(*(__last - 1)));
  }

  template<typename _RandomAccessIterator, typename _Distance, typename _Tp, 
         typename _Compare>
  void
  __push_heap(_RandomAccessIterator __first, _Distance __holeIndex, 
              _Distance __topIndex, const _Tp &__v, _Compare __comp)
  {
    _Distance __parent = (__holeIndex - 1) / 2;
    while (__holeIndex > __topIndex && __comp(*(__first + __parent), __v)) {
      *(__first + __holeIndex) = *(__first + __parent);
      __holeIndex = __parent;
      __parent = (__holeIndex - 1) / 2;
    }
    *(__first + __holeIndex) = __v;
  }

  template<typename _RandomAccessIterator, typename _Compare>
  inline void 
  push_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
            _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _DistanceType;

    tl::__push_heap(__first, _DistanceType((__last - __first) - 1), _DistanceType(0), 
        _ValueType(*(__last - 1)), __comp);
  }

  template<typename _RandomAccessIterator, typename _Distance, typename _Tp>
  void 
  __adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex,
                _Distance __len, const _Tp &__v)
  {
    _Distance __topIndex = __holeIndex;
    _Distance __secondChild = 2 * __holeIndex + 2;
    while (__secondChild < __len) {
      if (*(__first + __secondChild) < *(__first + (__secondChild - 1)))
        __secondChild--;
      *(__first + __holeIndex) = *(__first + __secondChild);
      __holeIndex = __secondChild;
      __secondChild = 2 * (__secondChild + 1);
    }
    if (__secondChild == __len) {
      *(__first + __holeIndex) = *(__first + (__secondChild - 1));
      __holeIndex = __secondChild - 1;
    }
    tl::__push_heap(__first, __holeIndex, __topIndex, __v);
  }

  template<typename _RandomAccessIterator, typename _Tp>
  inline void 
  __pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
             _RandomAccessIterator __result, const _Tp &__v)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _Distance;
    *__result = *__first;
    tl::__adjust_heap(__first, _Distance(0), _Distance(__last - __first), __v);
  }

  template<typename _RandomAccessIterator>
  inline void
  pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    tl::__pop_heap(__first, __last - 1, __last - 1, _ValueType(*(__last - 1)));
  }

  template<typename _RandomAccessIterator, typename _Distance,
     typename _Tp, typename _Compare>
  void
  __adjust_heap(_RandomAccessIterator __first, _Distance __holeIndex, 
                _Distance __len, const _Tp &__v, _Compare __comp)
  {
    _Distance __topIndex = __holeIndex;
    _Distance __secondChild = 2 * __holeIndex + 2;
    while (__secondChild < __len) {
      if (__comp(*(__first + __secondChild), *(__first + (__secondChild - 1))))
        __secondChild--;
      *(__first + __holeIndex) = *(__first + __secondChild);
      __holeIndex = __secondChild;
      __secondChild = 2 * (__secondChild + 1);
    }
    if (__secondChild == __len) {
      *(__first + __holeIndex) = *(__first + (__secondChild - 1));
      __holeIndex = __secondChild - 1;
    }
    tl::__push_heap(__first, __holeIndex, __topIndex, __v, __comp);
  }

  template<typename _RandomAccessIterator, typename _Tp, typename _Compare>
  inline void 
  __pop_heap(_RandomAccessIterator __first, _RandomAccessIterator __last, 
             _RandomAccessIterator __result, const _Tp &__v, _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _Distance;
    *__result = *__first;
    tl::__adjust_heap(__first, _Distance(0), _Distance(__last - __first), 
          __v, __comp);
  }

  template<typename _RandomAccessIterator, typename _Compare>
  inline void 
  pop_heap(_RandomAccessIterator __first,
           _RandomAccessIterator __last, _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    tl::__pop_heap(__first, __last - 1, __last - 1, _ValueType(*(__last - 1)), __comp);
  }

  template<typename _RandomAccessIterator>
  void 
  make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _DistanceType;

    if (__last - __first < 2) return;
    _DistanceType __len = __last - __first;
    _DistanceType __parent = (__len - 2)/2;
  
    while (true) {
      tl::__adjust_heap(__first, __parent, __len, _ValueType(*(__first + __parent)));
      if (__parent == 0) return;
      __parent--;
    }
  }

  template<typename _RandomAccessIterator, typename _Compare>
  inline void 
  make_heap(_RandomAccessIterator __first, _RandomAccessIterator __last,
            _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIterator>::value_type _ValueType;
    typedef typename std::iterator_traits<_RandomAccessIterator>::difference_type _DistanceType;

    if (__last - __first < 2) return;
    _DistanceType __len = __last - __first;
    _DistanceType __parent = (__len - 2)/2;
  
    while (true) {
      tl::__adjust_heap(__first, __parent, __len,
                    _ValueType(*(__first + __parent)), __comp);
      if (__parent == 0) return;
      __parent--;
    }
  }

  template<typename _RandomAccessIterator>
  void
  sort_heap(_RandomAccessIterator __first, _RandomAccessIterator __last)
  {
    while (__last - __first > 1)
      tl::pop_heap(__first, __last--);
  }

  template<typename _RandomAccessIterator, typename _Compare>
  void 
  sort_heap(_RandomAccessIterator __first, _RandomAccessIterator __last, 
            _Compare __comp)
  {
    while (__last - __first > 1)
      tl::pop_heap(__first, __last--, __comp);
  }


  template<typename _RandomAccessIter>
  void
  partial_sort(_RandomAccessIter __first,
               _RandomAccessIter __middle,
               _RandomAccessIter __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    tl::make_heap(__first, __middle);
    for (_RandomAccessIter __i = __middle; __i < __last; ++__i)
      if (*__i < *__first)
        tl::__pop_heap(__first, __middle, __i, _ValueType(*__i));
    tl::sort_heap(__first, __middle);
  }

  template<typename _RandomAccessIter, typename _Compare>
  void
  partial_sort(_RandomAccessIter __first,
               _RandomAccessIter __middle,
               _RandomAccessIter __last,
               _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    tl::make_heap(__first, __middle, __comp);
    for (_RandomAccessIter __i = __middle; __i < __last; ++__i)
      if (__comp(*__i, *__first))
        tl::__pop_heap(__first, __middle, __i, _ValueType(*__i), __comp);
    tl::sort_heap(__first, __middle, __comp);
  }

  template<typename _RandomAccessIter, typename _Size>
  void
  __introsort_loop(_RandomAccessIter __first, _RandomAccessIter __last,
                   _Size __depth_limit)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    while (__last - __first > tl::_M_threshold) {
      if (__depth_limit == 0) {
        tl::partial_sort(__first, __last, __last);
        return;
      }
      --__depth_limit;
      _RandomAccessIter __cut =
        tl::__unguarded_partition(__first, __last,
                  _ValueType(tl::__median(*__first,
                              *(__first + (__last - __first)/2),
                              *(__last - 1))));
      tl::__introsort_loop(__cut, __last, __depth_limit);
      __last = __cut;
    }
  }

  template<typename _RandomAccessIter, typename _Size, typename _Compare>
  void
  __introsort_loop(_RandomAccessIter __first, _RandomAccessIter __last,
                   _Size __depth_limit, _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    while (__last - __first > tl::_M_threshold) {
      if (__depth_limit == 0) {
        tl::partial_sort(__first, __last, __last, __comp);
        return;
      }
      --__depth_limit;
      _RandomAccessIter __cut =
      tl::__unguarded_partition(__first, __last,
                                  _ValueType(tl::__median(*__first,
                                          *(__first + (__last - __first)/2),
                                          *(__last - 1), __comp)),
                                  __comp);
      tl::__introsort_loop(__cut, __last, __depth_limit, __comp);
      __last = __cut;
    }
  }

  /**
   *  @brief Sort the elements of a sequence.
   *  @param  first   An iterator.
   *  @param  last    Another iterator.
   *  @return  Nothing.
   *
   *  Sorts the elements in the range @p [first,last) in ascending order,
   *  such that @p *(i+1)<*i is false for each iterator @p i in the range
   *  @p [first,last-1).
   *
   *  The relative ordering of equivalent elements is not preserved, use
   *  @p stable_sort() if this is needed.
  */
  template<typename _RandomAccessIter>
  inline void
  sort(_RandomAccessIter __first, _RandomAccessIter __last)
  {
    if (__first != __last) {
      bool __unsorted = false;
      for(_RandomAccessIter __i = __first + 1; __i != __last && !__unsorted; ++__i) {
        if (!(__i[-1] < *__i)) __unsorted = true;
      }
      if (__unsorted) {
        tl::__introsort_loop(__first, __last, __lg(__last - __first) * 2);
        tl::__final_insertion_sort(__first, __last);
      }
    }
  }

  /**
   *  @brief Sort the elements of a sequence using a predicate for comparison.
   *  @param  first   An iterator.
   *  @param  last    Another iterator.
   *  @param  comp    A comparison functor.
   *  @return  Nothing.
   *
   *  Sorts the elements in the range @p [first,last) in ascending order,
   *  such that @p comp(*(i+1),*i) is false for every iterator @p i in the
   *  range @p [first,last-1).
   *
   *  The relative ordering of equivalent elements is not preserved, use
   *  @p stable_sort() if this is needed.
  */
  template<typename _RandomAccessIter, typename _Compare>
  inline void
  sort(_RandomAccessIter __first, _RandomAccessIter __last, _Compare __comp)
  {
    if (__first != __last) {
      bool __unsorted = false;
      for(_RandomAccessIter __i = __first + 1; __i != __last && !__unsorted; ++__i) {
        if (!__comp(__i[-1],*__i)) __unsorted = true;
      }
      if (__unsorted) {
        tl::__introsort_loop(__first, __last, __lg(__last - __first) * 2, __comp);
        tl::__final_insertion_sort(__first, __last, __comp);
      }
    }
  }

  template<typename _RandomAccessIter>
  void
  nth_element(_RandomAccessIter __first,
              _RandomAccessIter __nth,
              _RandomAccessIter __last)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    while (__last - __first > 3) {
      _RandomAccessIter __cut =
        tl::__unguarded_partition(__first, __last,
                  _ValueType(tl::__median(*__first,
                              *(__first + (__last - __first)/2),
                              *(__last - 1))));
      if (__cut <= __nth)
        __first = __cut;
      else
        __last = __cut;
    }
    tl::__insertion_sort(__first, __last);
  }

  /**
   *  @brief Sort a sequence just enough to find a particular position
   *         using a predicate for comparison.
   *  @param  first   An iterator.
   *  @param  nth     Another iterator.
   *  @param  last    Another iterator.
   *  @param  comp    A comparison functor.
   *  @return  Nothing.
   *
   *  Rearranges the elements in the range @p [first,last) so that @p *nth
   *  is the same element that would have been in that position had the
   *  whole sequence been sorted. The elements either side of @p *nth are
   *  not completely sorted, but for any iterator @i in the range
   *  @p [first,nth) and any iterator @j in the range @p [nth,last) it
   *  holds that @p comp(*j,*i) is false.
  */
  template<typename _RandomAccessIter, typename _Compare>
  void
  nth_element(_RandomAccessIter __first,
              _RandomAccessIter __nth,
              _RandomAccessIter __last,
              _Compare __comp)
  {
    typedef typename std::iterator_traits<_RandomAccessIter>::value_type _ValueType;

    while (__last - __first > 3) {
      _RandomAccessIter __cut =
        tl::__unguarded_partition(__first, __last,
                  _ValueType(tl::__median(*__first,
                              *(__first + (__last - __first)/2),
                              *(__last - 1),
                              __comp)),
                  __comp);
      if (__cut <= __nth)
        __first = __cut;
      else
        __last = __cut;
    }
    tl::__insertion_sort(__first, __last, __comp);
  }

}

#endif

