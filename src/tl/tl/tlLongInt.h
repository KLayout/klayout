
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

*/


#ifndef HDR_tlLongInt
#define HDR_tlLongInt

#include "tlCommon.h"

#include <algorithm>

namespace tl
{

/**
 *  @brief A universal long unsigned int
 *
 *  The long unsigned int is composed of N chunks of B type.
 *  B can be any unsigned int type. BI is the working type
 *  which must be twice the size of B.
 *
 *  Specifically, this universal int is intended to emulate
 *  __uint128 by using:
 *
 *    typedef long_uint<4, uint32_t, uint64_t> __uint128;
 */
template<unsigned int N, class B, class BI>
class long_uint
{
public:
  enum { bits = sizeof (B) * 8 };

  typedef B basic_type;

  /**
   *  @brief Default constructor
   *  This will initialize the value to 0.
   */
  long_uint ()
  {
    for (unsigned int i = 0; i < N; ++i) {
      b[i] = 0;
    }
  }

  /**
   *  @brief Initialize the universal unsigned int with a POD type
   *  If T is a signed type, the upper bit won't be sign-extended.
   */
  template <class T>
  long_uint (T t)
  {
    unsigned int i = 0;
    unsigned int tbits = sizeof (T) * 8;

    while (tbits > 0) {
      b[i] = B (t);
      if (tbits > bits) {
        t >>= bits;
      }
      tbits -= bits;
      ++i;
    }
    for ( ; i < N; ++i) {
      b[i] = 0;
    }
  }

  /**
   *  @brief Convert the universal long int to the given type
   *  Casting will potentially reduce the number of significant
   *  bits of the value.
   */
  template <class T>
  operator T () const
  {
    unsigned int tbits = sizeof (T) * 8;
    T t = 0;

    if (tbits <= bits) {
      t = T (b[0]);
    } else {
      unsigned int i = sizeof (T) / sizeof (B);
      for ( ; i > 0 && tbits > 0; ) {
        --i;
        t <<= bits;
        t |= b[i];
        tbits -= bits;
      }
    }

    return t;
  }

  /**
   *  @brief Initialize the universal unsigned int from another one with a different width
   */
  template<unsigned int N2>
  long_uint (const long_uint<N2, B, BI> &o)
  {
    for (unsigned int i = 0; i < N; ++i) {
      b[i] = i < N2 ? o.b[i] : 0;
    }
  }

  /**
   *  @brief zero predicate
   *  This method returns true, if the value is 0.
   */
  bool is_zero () const
  {
    for (unsigned int i = 0; i < N; ++i) {
      if (b[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const long_uint<N, B, BI> &o) const
  {
    for (unsigned int i = 0; i < N; ++i) {
      if (b[i] != o.b[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const long_uint<N, B, BI> &b) const
  {
    return !operator== (b);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const long_uint<N, B, BI> &o) const
  {
    for (unsigned int i = N; i > 0; ) {
      --i;
      if (b[i] != o.b[i]) {
        return b[i] < o.b[i];
      }
    }
    return false;
  }

  /**
   *  @brief Greater or equal
   */
  bool operator>= (const long_uint<N, B, BI> &b) const
  {
    return !operator< (b);
  }

  /**
   *  @brief Less or equal
   */
  bool operator<= (const long_uint<N, B, BI> &b) const
  {
    return b >= *this;
  }

  /**
   *  @brief Greater
   */
  bool operator> (const long_uint<N, B, BI> &b) const
  {
    return b < *this;
  }

  /**
   *  @brief Multiplication
   */
  template <unsigned int N2>
  long_uint<N, B, BI> operator* (const long_uint<N2, B, BI> &o) const
  {
    long_uint<N + N2, B, BI> res;

    for (unsigned int i = 0; i < N; ++i) {
      for (unsigned int j = 0; j < N2; ++j) {
        BI p = BI (b[i]) * BI (o.b[j]);
        unsigned int n = i + j;
        while (p > 0) {
          B &r = res.b[n];
          B rold = r;
          r += B (p);
          p >>= bits;
          if (r < rold) {
            p += 1;
          }
          ++n;
        }
      }
    }

    return long_uint<N, B, BI> (res);
  }

  /**
   *  @brief In-place multiplication
   */
  template <unsigned int N2>
  long_uint<N, B, BI> &operator*= (const long_uint<N2, B, BI> &o)
  {
    *this = *this * o;
    return *this;
  }

  /**
   *  @brief Bitwise inversion
   */
  long_uint<N, B, BI> operator~ () const
  {
    long_uint<N, B, BI> res = *this;
    for (unsigned int i = 0; i < N; ++i) {
      res.b[i] = ~res.b[i];
    }
    return res;
  }

  /**
   *  @brief Boolean and operator
   */
  long_uint<N, B, BI> operator& (const long_uint<N, B, BI> &o) const
  {
    long_uint<N, B, BI> res = *this;
    res &= o;
    return res;
  }

  /**
   *  @brief In-place boolean and operator
   */
  long_uint<N, B, BI> &operator&= (const long_uint<N, B, BI> &o)
  {
    for (unsigned int i = 0; i < N; ++i) {
      b[i] &= o.b[i];
    }
    return *this;
  }

  /**
   *  @brief Boolean xor operator
   */
  long_uint<N, B, BI> operator^ (const long_uint<N, B, BI> &o) const
  {
    long_uint<N, B, BI> res = *this;
    res ^= o;
    return res;
  }

  /**
   *  @brief In-place boolean xor operator
   */
  long_uint<N, B, BI> &operator^= (const long_uint<N, B, BI> &o)
  {
    for (unsigned int i = 0; i < N; ++i) {
      b[i] ^= o.b[i];
    }
    return *this;
  }

  /**
   *  @brief Boolean or operator
   */
  long_uint<N, B, BI> operator| (const long_uint<N, B, BI> &o) const
  {
    long_uint<N, B, BI> res = *this;
    res |= o;
    return res;
  }

  /**
   *  @brief In-place boolean or operator
   */
  long_uint<N, B, BI> &operator|= (const long_uint<N, B, BI> &o)
  {
    for (unsigned int i = 0; i < N; ++i) {
      b[i] |= o.b[i];
    }
    return *this;
  }

  /**
   *  @brief Left-shift operator
   */
  long_uint<N, B, BI> operator<< (unsigned int n) const
  {
    long_uint<N, B, BI> res = *this;
    res.lshift (n);
    return res;
  }

  /**
   *  @brief In-place left-shift operator
   */
  long_uint<N, B, BI> &operator<<= (unsigned int n)
  {
    lshift (n);
    return *this;
  }

  /**
   *  @brief Bitwise left-shift
   *  This method will left-shift the value by the given number of bits
   */
  void lshift (unsigned int n)
  {
    unsigned int w = n / bits;
    if (w > 0) {
      n -= w * bits;
      for (unsigned int i = N - w; i > 0; ) {
        --i;
        b[i + w] = b[i];
      }
      for (unsigned int i = 0; i < w; ++i) {
        b[i] = 0;
      }
    }

    B carry = 0;
    for (unsigned int i = 0; i < N; ++i) {
      BI p = b[i];
      p <<= n;
      p |= carry;
      carry = B (p >> bits);
      b[i] = B (p);
    }
  }

  /**
   *  @brief right-shift operator
   */
  long_uint<N, B, BI> operator>> (unsigned int n) const
  {
    long_uint<N, B, BI> res = *this;
    res.rshift (n);
    return res;
  }

  /**
   *  @brief In-place right-shift operator
   */
  long_uint<N, B, BI> &operator>>= (unsigned int n)
  {
    rshift (n);
    return *this;
  }

  /**
   *  @brief Bitwise right-shift
   *  This method will right-shift the value by the given number of bits.
   *  The sign flag will be used to fill the upper bits.
   */
  void rshift (unsigned int n, bool sign = false)
  {
    unsigned int w = n / bits;
    if (w > 0) {
      n -= w * bits;
      for (unsigned int i = 0; i < N - w; ++i) {
        b[i] = b[i + w];
      }
      for (unsigned int i = N - w; i < N; ++i) {
        b[i] = sign ? ~B (0) : B (0);
      }
    }

    if (n > 0) {
      B carry = sign ? (~B (0) << (bits - n)) : 0;
      for (unsigned int i = N; i > 0; ) {
        --i;
        BI p = BI (b[i]) << bits;
        p >>= n;
        p |= (BI (carry) << bits);
        carry = B (p);
        b[i] = B (p >> bits);
      }
    }
  }

  /**
   *  @brief Sets the given bit in the universal long int
   *  Bit 0 is the rightmost (LSB) one.
   */
  void set_bit (unsigned int n)
  {
    unsigned int i = n / bits;
    n -= i * bits;
    if (i < N) {
      b[i] |= (1 << n);
    }
  }

  /**
   *  @brief Gets the number of zero bits counted from the MSB to the right
   */
  unsigned int zero_bits_from_msb () const
  {
    unsigned int zb = 0;
    for (unsigned int i = N; i > 0; ) {
      --i;
      if (!b[i]) {
        zb += bits;
      } else {
        B m = 1 << (bits - 1);
        B n = b[i];
        while (! (n & m)) {
          ++zb;
          n <<= 1;
        }
        break;
      }
    }
    return zb;
  }

  /**
   *  @brief Division and modulo operation
   *  This method returns the division of *this and d (the divider) in the first
   *  element of the returned pair and the modulo value (remainder) in the second.
   */
  std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > divmod (const long_uint<N, B, BI> &d) const
  {
    long_uint<N, B, BI> rem = *this;
    long_uint<N, B, BI> div;

    if (d.is_zero ()) {
      //  should assert?
      return std::make_pair (div, rem);
    }

    unsigned int bd = d.zero_bits_from_msb ();

    while (rem >= d) {

      unsigned int brem = rem.zero_bits_from_msb ();
      unsigned int shift = bd - brem;

      if (shift == 0) {
        rem -= d;
        div.set_bit (0);
      } else {
        long_uint<N, B, BI> sub = d;
        sub.lshift (shift);
        if (sub > rem) {
          shift -= 1;
          sub.rshift (1);
        }
        div.set_bit (shift);
        rem -= sub;
      }

    }

    return std::make_pair (div, rem);
  }

  /**
   *  @brief Division operator
   */
  long_uint<N, B, BI> operator/ (const long_uint<N, B, BI> &b) const
  {
    return divmod (b).first;
  }

  /**
   *  @brief In-place division operator
   */
  long_uint<N, B, BI> &operator/= (const long_uint<N, B, BI> &b)
  {
    *this = divmod (b).first;
    return *this;
  }

  /**
   *  @brief Modulo operator
   */
  long_uint<N, B, BI> operator% (const long_uint<N, B, BI> &b) const
  {
    return divmod (b).second;
  }

  /**
   *  @brief In-place modulo operator
   */
  long_uint<N, B, BI> &operator%= (const long_uint<N, B, BI> &b)
  {
    *this = divmod (b).second;
    return *this;
  }

  /**
   *  @brief Addition with the basic type
   */
  long_uint<N, B, BI> operator+ (B o) const
  {
    long_uint<N, B, BI> res;

    B carry = o;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = res.b[i];
      r = b[i];
      B rold = r;
      r += carry;
      carry = 0;
      if (r < rold) {
        carry = 1;
      }
    }

    return res;
  }

  /**
   *  @brief In-place addition with the basic type
   */
  long_uint<N, B, BI> &operator+= (B o)
  {
    B carry = o;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = b[i];
      B rold = r;
      r += carry;
      carry = 0;
      if (r < rold) {
        carry = 1;
      }
    }

    return *this;
  }

  /**
   *  @brief Addition
   */
  long_uint<N, B, BI> operator+ (const long_uint<N, B, BI> &o) const
  {
    long_uint<N, B, BI> res;

    B carry = 0;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = res.b[i];
      r = b[i];
      B rold = r;
      r += carry;
      carry = 0;
      if (r < rold) {
        carry = 1;
      }
      rold = r;
      r += o.b[i];
      if (r < rold) {
        carry = 1;
      }
    }

    return res;
  }

  /**
   *  @brief In-place addition
   */
  long_uint<N, B, BI> &operator+= (const long_uint<N, B, BI> &o)
  {
    B carry = 0;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = b[i];
      B rold = r;
      r += carry;
      carry = 0;
      if (r < rold) {
        carry = 1;
      }
      rold = r;
      r += o.b[i];
      if (r < rold) {
        carry = 1;
      }
    }

    return *this;
  }

  /**
   *  @brief Subtraction
   */
  long_uint<N, B, BI> operator- (const long_uint<N, B, BI> &o) const
  {
    long_uint<N, B, BI> res;

    B carry = 0;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = res.b[i];
      r = b[i];
      B rold = r;
      r -= carry;
      carry = 0;
      if (r > rold) {
        carry = 1;
      }
      rold = r;
      r -= o.b[i];
      if (r > rold) {
        carry = 1;
      }
    }

    return res;
  }

  /**
   *  @brief In-place subtraction
   */
  long_uint<N, B, BI> &operator-= (const long_uint<N, B, BI> &o)
  {
    B carry = 0;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = b[i];
      B rold = r;
      r -= carry;
      carry = 0;
      if (r > rold) {
        carry = 1;
      }
      rold = r;
      r -= o.b[i];
      if (r > rold) {
        carry = 1;
      }
    }

    return *this;
  }

  /**
   *  @brief Subtraction with the basic type
   */
  long_uint<N, B, BI> operator- (B o) const
  {
    long_uint<N, B, BI> res;

    B carry = o;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = res.b[i];
      r = b[i];
      B rold = r;
      r -= carry;
      carry = 0;
      if (r > rold) {
        carry = 1;
      }
    }

    return res;
  }

  /**
   *  @brief In-place subtraction with the basic type
   */
  long_uint<N, B, BI> &operator-= (B o)
  {
    B carry = o;
    for (unsigned int i = 0; i < N; ++i) {
      B &r = b[i];
      B rold = r;
      r -= carry;
      carry = 0;
      if (r > rold) {
        carry = 1;
      }
    }

    return *this;
  }

  B b[N];
};

/**
 *  @brief A universal long signed int
 *
 *  The long unsigned int is composed of N chunks of B type.
 *  B can be any unsigned int type. BI is the working type
 *  which must be twice the size of B.
 *
 *  B and BI must be unsigned types.
 *
 *  Specifically, this universal int is intended to emulate
 *  __int128 by using:
 *
 *    typedef long_int<4, uint32_t, uint64_t> __int128;
 */
template<unsigned int N, class B, class BI>
class long_int
  : public long_uint<N, B, BI>
{
public:
  /**
   *  @brief Default constructor
   *  This will initialize the value to 0.
   */
  long_int ()
    : long_uint<N, B, BI> ()
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Initialize the universal int with a POD type
   *  If T is a signed type, the upper bit will be sign-extended.
   */
  template <class T>
  long_int (T t)
  {
    unsigned int tbits = sizeof (T) * 8;

    for (unsigned int i = 0; i < N; ++i) {
      long_uint<N, B, BI>::b[i] = B (t);
      if (tbits <= long_uint<N, B, BI>::bits) {
        t = (t < 0 ? ~B (0) : 0);
      } else {
        t >>= long_uint<N, B, BI>::bits;
      }
    }
  }

  /**
   *  @brief Convert the universal long int to the given type
   *  Casting will potentially reduce the number of significant
   *  bits of the value.
   */
  template <class T>
  operator T () const
  {
    unsigned int tbits = sizeof (T) * 8;
    T t = 0;

    if (tbits <= long_uint<N, B, BI>::bits) {
      t = T (long_uint<N, B, BI>::b[0]);
    } else {
      unsigned int i = sizeof (T) / sizeof (B);
      for ( ; i > 0 && tbits > 0; ) {
        --i;
        t <<= long_uint<N, B, BI>::bits;
        t |= long_uint<N, B, BI>::b[i];
        tbits -= long_uint<N, B, BI>::bits;
      }
    }

    return t;
  }

  /**
   *  @brief Initialize the universal unsigned int from an unsigned one with a different width
   *  Sign inversion will happen, if the unsigned int is bigger than the maximum value
   *  representable by our type.
   */
  template<unsigned int N2>
  long_int (const long_uint<N2, B, BI> &o)
    : long_uint<N, B, BI> (o)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Initialize the universal int from another one with a different width
   */
  template<unsigned int N2>
  long_int (const long_int<N2, B, BI> &o)
    : long_uint<N, B, BI> (o)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief negative predicate
   *  This method returns true, if the value is negative.
   */
  bool is_neg () const
  {
    return (long_uint<N, B, BI>::b[N - 1] & (1 << (long_uint<N, B, BI>::bits - 1))) != 0;
  }

  /**
   *  @brief zero predicate
   *  This method returns true, if the value is 0.
   */
  bool is_zero () const
  {
    for (unsigned int i = 0; i < N; ++i) {
      if (long_uint<N, B, BI>::b[i]) {
        return false;
      }
    }
    return true;
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const long_uint<N, B, BI> &o) const
  {
    //  we cast both arguments to unsigned as C++ does
    return long_uint<N, B, BI>::operator< (o);
  }

  /**
   *  @brief Less operator
   */
  bool operator< (const long_int<N, B, BI> &o) const
  {
    if (is_neg () != o.is_neg ()) {
      return is_neg () > o.is_neg ();
    } else {
      return long_uint<N, B, BI>::operator< (o);
    }
  }

  /**
   *  @brief Greater or equal
   */
  bool operator>= (const long_uint<N, B, BI> &b) const
  {
    return !operator< (b);
  }

  /**
   *  @brief Greater or equal
   */
  bool operator>= (const long_int<N, B, BI> &b) const
  {
    return !operator< (b);
  }

  /**
   *  @brief Less or equal
   */
  bool operator<= (const long_uint<N, B, BI> &b) const
  {
    return b >= *this;
  }

  /**
   *  @brief Less or equal
   */
  bool operator<= (const long_int<N, B, BI> &b) const
  {
    return b >= *this;
  }

  /**
   *  @brief Greater
   */
  bool operator> (const long_uint<N, B, BI> &b) const
  {
    return b < *this;
  }

  /**
   *  @brief Greater
   */
  bool operator> (const long_int<N, B, BI> &b) const
  {
    return b < *this;
  }

  /**
   *  @brief Sign inversion (two's complement)
   */
  long_int<N, B, BI> operator- () const
  {
    long_uint<N, B, BI> res = ~*this;
    res += B (1);
    return long_int<N, B, BI> (res);
  }

  /**
   *  @brief Multiplication with unsigned
   */
  template <unsigned int N2>
  long_uint<N, B, BI> operator* (const long_uint<N2, B, BI> &o) const
  {
    return long_uint<N, B, BI>::operator* (o);
  }

  /**
   *  @brief Multiplication with signed
   */
  template <unsigned int N2>
  long_int<N, B, BI> operator* (const long_int<N2, B, BI> &o) const
  {
    if (is_neg () && ! o.is_neg ()) {
      return -long_int<N, B, BI> ((-*this).long_uint<N, B, BI>::operator* (o));
    } else if (! is_neg () && o.is_neg ()) {
      return -long_int<N, B, BI> (long_uint<N, B, BI>::operator* (-o));
    } else if (is_neg () && o.is_neg ()) {
      return long_int<N, B, BI> ((-*this).long_uint<N, B, BI>::operator* (-o));
    } else {
      return long_int<N, B, BI> (long_uint<N, B, BI>::operator* (o));
    }
  }

  /**
   *  @brief In-place multiplication with unsigned
   */
  template <unsigned int N2>
  long_int<N, B, BI> &operator*= (const long_uint<N2, B, BI> &o)
  {
    long_uint<N, B, BI>::operator= (*this * o);
    return *this;
  }

  /**
   *  @brief In-place multiplication with signed
   */
  template <unsigned int N2>
  long_int<N, B, BI> &operator*= (const long_int<N2, B, BI> &o)
  {
    *this = *this * o;
    return *this;
  }

  /**
   *  @brief Left-shift operator
   */
  long_int<N, B, BI> operator<< (unsigned int n) const
  {
    long_int<N, B, BI> res = *this;
    res.lshift (n);
    return res;
  }

  /**
   *  @brief In-place left-shift operator
   */
  long_int<N, B, BI> &operator<<= (unsigned int n)
  {
    long_uint<N, B, BI>::lshift (n);
    return *this;
  }

  /**
   *  @brief right-shift operator
   */
  long_int<N, B, BI> operator>> (unsigned int n) const
  {
    long_uint<N, B, BI> res = *this;
    res.rshift (n, is_neg ());
    return res;
  }

  /**
   *  @brief In-place right-shift operator
   */
  long_int<N, B, BI> &operator>>= (unsigned int n)
  {
    long_uint<N, B, BI>::rshift (n, is_neg ());
    return *this;
  }

  /**
   *  @brief Division and modulo operation with unsigned type
   *  This method returns the division of *this and d (the divider) in the first
   *  element of the returned pair and the modulo value (remainder) in the second.
   */
  std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > divmod (const long_uint<N, B, BI> &d) const
  {
    return long_uint<N, B, BI>::divmod (d);
  }

  /**
   *  @brief Division operator with unsigned
   */
  long_uint<N, B, BI> operator/ (const long_uint<N, B, BI> &b) const
  {
    return divmod (b).first;
  }

  /**
   *  @brief In-place division operator with unsigned
   */
  long_int<N, B, BI> &operator/= (const long_uint<N, B, BI> &b)
  {
    long_uint<N, B, BI>::operator= (divmod (b).first);
    return *this;
  }

  /**
   *  @brief Modulo operator with unsigned
   */
  long_uint<N, B, BI> operator% (const long_uint<N, B, BI> &b) const
  {
    return divmod (b).second;
  }

  /**
   *  @brief In-place modulo operator with unsigned
   */
  long_int<N, B, BI> &operator%= (const long_uint<N, B, BI> &b)
  {
    long_uint<N, B, BI>::operator= (divmod (b).second);
    return *this;
  }

  /**
   *  @brief Division and modulo operation
   *  This method returns the division of *this and d (the divider) in the first
   *  element of the returned pair and the modulo value (remainder) in the second.
   */
  std::pair<long_int<N, B, BI>, long_int<N, B, BI> > divmod (const long_int<N, B, BI> &d) const
  {
    if (is_neg () && !d.is_neg ()) {
      std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > res = (-*this).long_uint<N, B, BI>::divmod (d);
      return std::make_pair (-long_int<N, B, BI> (res.first), -long_int<N, B, BI> (res.second));
    } else if (! is_neg () && d.is_neg ()) {
      std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > res = long_uint<N, B, BI>::divmod (-d);
      //  The definition of the modulo sign is consistent with int arithmetics
      return std::make_pair (-long_int<N, B, BI> (res.first), long_int<N, B, BI> (res.second));
    } else if (is_neg () && d.is_neg ()) {
      std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > res = (-*this).long_uint<N, B, BI>::divmod (-d);
      //  The definition of the modulo sign is consistent with int arithmetics
      return std::make_pair (long_int<N, B, BI> (res.first), -long_int<N, B, BI> (res.second));
    } else {
      std::pair<long_uint<N, B, BI>, long_uint<N, B, BI> > res = long_uint<N, B, BI>::divmod (d);
      return std::make_pair (long_int<N, B, BI> (res.first), long_int<N, B, BI> (res.second));
    }
  }

  /**
   *  @brief Division operator
   */
  long_int<N, B, BI> operator/ (const long_int<N, B, BI> &b) const
  {
    return divmod (b).first;
  }

  /**
   *  @brief In-place division operator
   */
  long_int<N, B, BI> &operator/= (const long_int<N, B, BI> &b)
  {
    *this = divmod (b).first;
    return *this;
  }

  /**
   *  @brief Modulo operator
   */
  long_int<N, B, BI> operator% (const long_int<N, B, BI> &b) const
  {
    return divmod (b).second;
  }

  /**
   *  @brief In-place modulo operator
   */
  long_int<N, B, BI> &operator%= (const long_int<N, B, BI> &b)
  {
    *this = divmod (b).second;
    return *this;
  }

  /**
   *  @brief Addition with basic type
   */
  long_int<N, B, BI> operator+ (B o) const
  {
    return long_int<N, B, BI> (long_uint<N, B, BI>::operator+ (o));
  }

  /**
   *  @brief Addition with unsigned
   */
  long_uint<N, B, BI> operator+ (const long_uint<N, B, BI> &o) const
  {
    return long_uint<N, B, BI>::operator+ (o);
  }

  /**
   *  @brief Addition
   */
  long_int<N, B, BI> operator+ (const long_int<N, B, BI> &o) const
  {
    return long_int<N, B, BI> (long_uint<N, B, BI>::operator+ (o));
  }

  /**
   *  @brief In-place addition with basic_type
   */
  long_int<N, B, BI> &operator+= (B o)
  {
    long_uint<N, B, BI>::operator+= (o);
    return *this;
  }

  /**
   *  @brief In-place addition
   */
  long_int<N, B, BI> &operator+= (const long_int<N, B, BI> &o)
  {
    long_uint<N, B, BI>::operator+= (o);
    return *this;
  }

  /**
   *  @brief In-place addition with unsigned
   */
  long_int<N, B, BI> &operator+= (const long_uint<N, B, BI> &o)
  {
    long_uint<N, B, BI>::operator+= (o);
    return *this;
  }

  /**
   *  @brief Subtraction with basic_type
   */
  long_int<N, B, BI> operator- (B o) const
  {
    return long_int<N, B, BI> (long_uint<N, B, BI>::operator- (o));
  }

  /**
   *  @brief Subtraction with unsigned
   */
  long_uint<N, B, BI> operator- (const long_uint<N, B, BI> &o) const
  {
    return long_uint<N, B, BI>::operator- (o);
  }

  /**
   *  @brief Subtraction
   */
  long_int<N, B, BI> operator- (const long_int<N, B, BI> &o) const
  {
    return long_int<N, B, BI> (long_uint<N, B, BI>::operator- (o));
  }

  /**
   *  @brief In-place subtraction with basic_type
   */
  long_int<N, B, BI> &operator-= (B o)
  {
    long_uint<N, B, BI>::operator-= (o);
    return *this;
  }

  /**
   *  @brief In-place subtraction with unsigned
   */
  long_int<N, B, BI> &operator-= (const long_uint<N, B, BI> &o)
  {
    long_uint<N, B, BI>::operator-= (o);
    return *this;
  }

  /**
   *  @brief In-place subtraction
   */
  long_int<N, B, BI> &operator-= (const long_int<N, B, BI> &o)
  {
    long_uint<N, B, BI>::operator-= (o);
    return *this;
  }
};

/**
 *  @brief Less operator with unsigned and signed
 */
template<unsigned int N, class B, class BI>
bool operator< (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  //  we cast both arguments to unsigned (as C++ does)
  return a.operator< (b);
}

/**
 *  @brief Less or equal operator with unsigned and signed
 */
template<unsigned int N, class B, class BI>
bool operator<= (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return !(b < a);
}

/**
 *  @brief Greater operator with unsigned and signed
 */
template<unsigned int N, class B, class BI>
bool operator> (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return b < a;
}

/**
 *  @brief Greater or equal operator with unsigned and signed
 */
template<unsigned int N, class B, class BI>
bool operator>= (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return !(a < b);
}

/**
 *  @brief Multiplication with unsigned and signed
 */
template<unsigned int N, unsigned int N2, class B, class BI>
long_uint<N, B, BI> operator* (const long_uint<N, B, BI> &a, const long_int<N2, B, BI> &b)
{
  return a.operator* (b);
}

/**
 *  @brief Division with unsigned and signed
 */
template<unsigned int N, class B, class BI>
long_uint<N, B, BI> operator/ (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return a.operator/ (b);
}

/**
 *  @brief Modulo with unsigned and signed
 */
template<unsigned int N, class B, class BI>
long_uint<N, B, BI> operator% (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return a.operator% (b);
}

/**
 *  @brief Addition with unsigned and signed
 */
template<unsigned int N, class B, class BI>
long_uint<N, B, BI> operator+ (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return a.operator+ (b);
}

/**
 *  @brief Subtraction with unsigned and signed
 */
template<unsigned int N, class B, class BI>
long_uint<N, B, BI> operator- (const long_uint<N, B, BI> &a, const long_int<N, B, BI> &b)
{
  return a.operator- (b);
}

}

#endif
