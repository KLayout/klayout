
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


#ifndef HDR_tlString
#define HDR_tlString

#include "tlCommon.h"

#include <string>
#include <sstream>
#include <typeinfo>
#include <stdexcept>
#include <stdint.h>
#include <stdarg.h>

#include "tlException.h"
#include "tlVariant.h"
#include "tlTypeTraits.h"

#if defined(HAVE_QT)
class QImage;
#endif

namespace tl {

/**
 *  @brief An exception indicating that string extraction is not available for a certain type
 */
class TL_PUBLIC ExtractorNotImplementedException
  : public tl::Exception
{
public:
  ExtractorNotImplementedException (const std::type_info &ti);
};

/**
 *  @brief An exception indicating that string conversion is not available for a certain type
 */
class TL_PUBLIC StringConversionException
  : public tl::Exception
{
public:
  StringConversionException (const std::type_info &ti);
};

class Extractor;
template <class T> void extractor_impl (tl::Extractor &, T &) { throw ExtractorNotImplementedException (typeid (T)); }
template <class T> bool test_extractor_impl (tl::Extractor &, T &) { throw ExtractorNotImplementedException (typeid (T)); }

/**
 *  @brief Another string class
 * 
 *  The purpose of this class is to provide a different string implementation
 *  that is optimized for special purposes. For example, the assign method
 *  and operator does reuse memory allocated so far. This way this object is
 *  better suited to string objects that frequently change.
 */

class TL_PUBLIC string
{
public:
  typedef std::allocator<char> allocator_t;

  /**
   *  @brief The default constructor
   */
  string ()
    : m_size (0), m_capacity (0), mp_rep (0)
  {
    //  nothing yet ..
  }

  /**
   *  @brief The constructor from a const char *
   */
  string (const char *c);

  /**
   *  @brief The constructor from a const char * taking just a substring
   */
  string (const char *c, size_t from, size_t to);

  /**
   *  @brief The copy constructor
   */
  string (const tl::string &s);

  /**
   *  @brief The copy constructor taking just a substring
   */
  string (const tl::string &s, size_t from, size_t to);

  /**
   *  @brief The constructor creating a string from a STL string
   */
  string (const std::string &s);

  /**
   *  @brief The constructor creating a string from a STL string taking just a substring
   */
  string (const std::string &s, size_t from, size_t to);

  /**
   *  @brief Destructor
   */
  ~string ();

  /**
   *  @brief The assignment from a const char *
   */
  tl::string &operator= (const char *c);

  /**
   *  @brief The assignment from a const char * taking just a substring
   */
  void assign (const char *c, size_t from, size_t to);

  /**
   *  @brief The assignment operator
   */
  tl::string &operator= (const tl::string &s);

  /**
   *  @brief The assignment taking just a substring
   */
  void assign (const tl::string &s, size_t from, size_t to);

  /**
   *  @brief The assignment from a STL string
   */
  tl::string &operator= (const std::string &s);

  /**
   *  @brief The assignment from a STL string taking just a substring
   */
  void assign (const std::string &s, size_t from, size_t to);

  /**
   *  @brief The length of the string
   */
  size_t size () const
  {
    return m_size;
  }

  /**
   *  @brief Reserve a certain number of characters
   */
  void reserve (size_t n);

  /**
   *  @brief The capacity of the string
   */
  size_t capacity () const
  {
    return m_capacity;
  }

  /**
   *  @brief Swap the string with another object
   */
  void swap (tl::string &s)
  {
    std::swap (mp_rep, s.mp_rep);
    std::swap (m_size, s.m_size);
  }

  /**
   *  @brief Equality
   */
  bool operator== (const char *c) const;

  /**
   *  @brief Equality
   */
  bool operator== (const tl::string &s) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const char *c) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const tl::string &s) const;

  /**
   *  @brief Comparison operator
   */
  bool operator< (const char *c) const;
   
  /**
   *  @brief Comparison operator
   */
  bool operator< (const tl::string &s) const;

  /**
   *  @brief Comparison operator
   */
  bool operator<= (const char *c) const;
   
  /**
   *  @brief Comparison operator
   */
  bool operator<= (const tl::string &s) const;

  /**
   *  @brief Comparison operator
   */
  bool operator> (const char *c) const;
   
  /**
   *  @brief Comparison operator
   */
  bool operator> (const tl::string &s) const;

  /**
   *  @brief Comparison operator
   */
  bool operator>= (const char *c) const;
   
  /**
   *  @brief Comparison operator
   */
  bool operator>= (const tl::string &s) const;

  /**
   *  @brief Access to "C" representation
   */
  const char *c_str () const
  {
    return mp_rep == 0 ? "" : mp_rep;
  }
   
  /**
   *  @brief Access to STL representation
   */
  std::string std_str () const
  {
    if (mp_rep == 0) {
      return std::string ();
    } else {
      return std::string (mp_rep, 0, m_size);
    }
  }

  /**
   *  @brief Clear the string
   *
   *  Unlike assignment of an empty string, "clear" releases the memory allocated
   *  by this object.
   */
  void clear ();
   
private:
  size_t m_size;
  size_t m_capacity;
  char *mp_rep;
};

TL_PUBLIC std::string to_upper_case (const std::string &s);
TL_PUBLIC std::string to_lower_case (const std::string &s);

TL_PUBLIC std::string to_string (double d, int prec);
TL_PUBLIC std::string to_string (float d, int prec);
TL_PUBLIC std::string to_string (const unsigned char *cp, int length);
TL_PUBLIC std::string to_string (const char *cp, int length);
TL_PUBLIC std::string to_string_from_local (const char *cp);
TL_PUBLIC std::string to_local (const std::string &s);

template <class T, bool> struct __redirect_to_string;
template <class T> struct __redirect_to_string<T, true> { static std::string to_string (const T &t) { return t.to_string (); } };
template <class T> struct __redirect_to_string<T, false> { static std::string to_string  (const T &) { throw StringConversionException (typeid (T)); } };
template <class T> inline std::string to_string (const T &o) { return __redirect_to_string<T, tl::has_to_string<T>::value>::to_string (o); }

template <> inline std::string to_string (const double &d) { return to_string (d, 12); }
template <> inline std::string to_string (const float &d) { return to_string (d, 6); }
template <> TL_PUBLIC std::string to_string (const int &d);
template <> TL_PUBLIC std::string to_string (const unsigned int &d);
template <> TL_PUBLIC std::string to_string (const long &d);
template <> TL_PUBLIC std::string to_string (const unsigned long &d);
template <> TL_PUBLIC std::string to_string (const long long &d);
template <> TL_PUBLIC std::string to_string (const unsigned long long &d);
#if defined(HAVE_64BIT_COORD)
template <> TL_PUBLIC std::string to_string (const __int128 &d);
template <> TL_PUBLIC std::string to_string (const unsigned __int128 &d);
#endif
template <> TL_PUBLIC std::string to_string (const char * const &cp);
template <> TL_PUBLIC std::string to_string (char * const &cp);
template <> TL_PUBLIC std::string to_string (const unsigned char * const &cp);
template <> TL_PUBLIC std::string to_string (unsigned char * const &cp);
template <> TL_PUBLIC std::string to_string (const bool &b);
template <> inline std::string to_string (const std::string &s) { return s; }

//  variants utilize parsable strings because this way the string representation can be translated back
//  to a variant object
template <> inline std::string to_string (const tl::Variant &v) { return v.to_parsable_string (); }

#if defined(HAVE_QT)
//  some dummy conversions provided for tl::Variant implementation
template <> inline std::string to_string (const QImage &) { return std::string (); }
#endif

/**
 *  @brief Converts UTF-8 to wide character string
 */
std::wstring TL_PUBLIC to_wstring (const std::string &s);

/**
 *  @brief Converts a wide character string to UTF-8
 */
std::string TL_PUBLIC to_string (const std::wstring &ws);

/**
 *  @brief Convert to a quoted string 
 *
 *  The string returned by this method can be read from a corresponding extractor with 
 *  the "read_quoted" method.
 */  
TL_PUBLIC std::string to_quoted_string (const std::string &s);

/**
 *  @brief Escape special characters in a string
 *
 *  Special characters are replaced by escape sequences with a backslash.
 *  The format of the sequence is \xxx where x is the octal number of the 
 *  character and \r, \n and \t representing the CR, LF and TAB character.
 */
TL_PUBLIC std::string escape_string (const std::string &value);

/**
 *  @brief Remove escape sequences from a string
 *
 *  Special characters are replaced by escape sequences with a backslash.
 *  The format of the sequence is \xxx where x is the octal number of the 
 *  character and \r, \n and \t representing the CR, LF and TAB character.
 */
TL_PUBLIC std::string unescape_string (const std::string &value);

/**
 *  @brief Levenshtein distance
 *
 *  This function computes the edit distance ("Levenshtein distance") between two strings.
 */  
TL_PUBLIC int edit_distance (const std::string &a, const std::string &b);

/**
 *  @brief Convert to a word or quoted string 
 *
 *  The string returned by this method can be read from a corresponding extractor with 
 *  the "read_word_or_quoted" method with the given "non_term" characters.
 */  
TL_PUBLIC std::string to_word_or_quoted_string (const std::string &s, const char *non_term = "_.$");

/**
 *  @brief Escapes HTML (or XML) characters from in and adds the result to out
 *  If "replace_newlines" is true, "\n" will be replaced by "<br/>".
 */
TL_PUBLIC void escape_to_html (std::string &out, const std::string &in, bool replace_newlines = true);

/**
 *  @brief Escapes HTML (or XML) characters from in and returns the resulting string
 *  Double quotes are substituted by "&quot;" which makes the resulting string usable for
 *  attributes (in double quotes) too.
 *  If "replace_newlines" is true, "\n" will be replaced by "<br/>".
 */
TL_PUBLIC std::string escaped_to_html (const std::string &in, bool replace_newlines = true);

/**
 *  @brief Replaces the "before" string by "after" in the "subject" string and returns the new string
 *  All occurrences are replaced.
 */
TL_PUBLIC std::string replaced (const std::string &subject, const std::string &before, const std::string &after);

/**
 *  @brief Replicates the given string n times
 */
TL_PUBLIC std::string replicate (const std::string &s, unsigned int n);

/**
 *  @brief Fills the string (to the right) with blanks until the desired length is reached
 */
TL_PUBLIC std::string pad_string_right (unsigned int n, const std::string &s);

/**
 *  @brief Fills the string (to the left) with blanks until the desired length is reached
 */
TL_PUBLIC std::string pad_string_left (unsigned int n, const std::string &s);

/**
 *  @brief Set the number of digits resolution for a micron display
 */
TL_PUBLIC void set_micron_resolution (unsigned int ndigits);

/**
 *  @brief A standard method to convert a micron value into a string for display
 */
TL_PUBLIC std::string micron_to_string (double d);

/**
 *  @brief Set the number of digits resolution for a dbu display
 */
TL_PUBLIC void set_db_resolution (unsigned int ndigits);

/**
 *  @brief A standard method to convert a coordinate in database units (pixel) into a string for display
 */
TL_PUBLIC std::string db_to_string (double d);

/**
 *  @brief A standard method to convert a coordinate in database units (pixel) into a string for display
 */
inline std::string db_to_string (int32_t d) 
{
  return to_string (d);
}

/**
 *  @brief A standard method to convert a coordinate in database units (pixel) into a string for display
 */
inline std::string db_to_string (int64_t d) 
{
  return to_string (d);
}

/**
 *  @brief A generic extractor (parser) class
 *
 *  This class acts like a stream of which elements can be extracted on
 *  a character-by-character or element basis. The "stream" is initialized 
 *  with a character pointer. Then, element by element can be extracted
 *  using the read or try_read methods.
 *  The stream in general consumes white spaces.
 *  It is possible to customize the extractor by overloading the error method for example.
 */
class TL_PUBLIC Extractor
{
public:
  struct end { };
  
  /**
   *  @brief Constructor
   *
   *  This constructor constructs an extractor object from a const char *.
   *  The ownership over this pointer is not passed, i.e. it must be valid
   *  during the parsing.
   */
  Extractor (const char *s = "");

  /**
   *  @brief Constructor
   *
   *  This constructor constructs an extractor object from a string.
   *  It will internally store a copy of the string for parsing, so it can be
   *  passed a temporary object.
   */
  Extractor (const std::string &str);

  /**
   *  @brief Destructor
   */
  virtual ~Extractor () { }

  /**
   *  @brief Read an unsigned integer
   *
   *  A helper method to implement parsers on the character-by-character basis.
   *  This method reads an unsigned integer and returns the value past the last
   *  valid character. It skips blanks at the beginning but not at the end.
   *  On error, an exception is thrown.
   *  
   *  @param cp The current pointer (is being moved)
   *  @param value Where the value is stored
   */
  Extractor &read (unsigned int &value);

  /**
   *  @brief Read an unsigned char int (see read of an unsigned int)
   */
  Extractor &read (unsigned char &value);

  /**
   *  @brief Read an unsigned long (see read of an unsigned int)
   */
  Extractor &read (unsigned long &value);

  /**
   *  @brief Read an unsigned long long (see read of an unsigned int)
   */
  Extractor &read (unsigned long long &value);

  /**
   *  @brief Read a double (see read of an unsigned int)
   */
  Extractor &read (double &value);

  /**
   *  @brief Read a float (see read of an unsigned int)
   */
  Extractor &read (float &value);

  /**
   *  @brief Read a signed int (see read of an unsigned int)
   */
  Extractor &read (int &value);

  /**
   *  @brief Read a signed long (see read of an unsigned int)
   */
  Extractor &read (long &value);

  /**
   *  @brief Read a signed long long (see read of an unsigned int)
   */
  Extractor &read (long long &value);

  /**
   *  @brief Read a boolean value (see read of an unsigned int)
   *
   *  The value can be either 0 or 1 or "true" or "false".
   */
  Extractor &read (bool &value);

  /**
   *  @brief Generic extrator
   * 
   *  This extractor requires a adaptor to actually to the extraction.
   *  The adaptor is a functor taking the extractor and delivering the 
   *  specified type (void tl::extractor_impl (tl::Extractor &, T &)).
   */
  template <class T>
  Extractor &read (T &value)
  {
    tl::extractor_impl (*this, value);
    return *this;
  }

  /**
   *  @brief Read a string (see read of an unsigned int)
   *
   *  The termination character may be specified with the "term"
   *  string: if one of the characters in this string is encountered
   *  or the input ends, the reader stops reading.
   */
  Extractor &read (std::string &value, const char *term = "");

  /**
   *  @brief Read a name string
   *
   *  Name strings are like words, but for the first character digits are not allowed.
   */
  Extractor &read_name (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Read a string consisting of "word" characters
   *
   *  Beside letters and digits the characters given in the "non_term" array are
   *  allowed in the word as well.
   */
  Extractor &read_word (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Read a quoted string 
   *
   *  The string may be either quoted with single or double quotes. Quotes inside the string
   *  may be escaped with a backslash character.
   */
  Extractor &read_quoted (std::string &value);

  /**
   *  @brief Read a string consisting of "word" characters or accept quoted strings
   *  
   *  The string may be either given in the form allowed by "read_word" (with the 
   *  non_term characters) or by "read_quoted".
   */
  Extractor &read_word_or_quoted (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Try to read an unsigned integer
   *
   *  Similar to "read" but does not throw an exception if the input is not 
   *  a valid unsigned integer. In this case, the function returns false.
   *  
   *  @param cp The current pointer (is being moved)
   *  @param value Where the value is stored
   *  @return true, if a value could be read
   */
  bool try_read (unsigned int &value);

  /**
   *  @brief Try to read a signed int (see try to read an unsigned int)
   */
  bool try_read (int &value);

  /**
   *  @brief Try to read an unsigned char int (see try to read an unsigned int)
   */
  bool try_read (unsigned char &value);

  /**
   *  @brief Try to read an unsigned long (see try to read an unsigned int)
   */
  bool try_read (unsigned long &value);

  /**
   *  @brief Try to read an unsigned long long (see try to read an unsigned int)
   */
  bool try_read (unsigned long long &value);

  /**
   *  @brief Try to read a signed long (see try to read an unsigned int)
   */
  bool try_read (long &value);

  /**
   *  @brief Try to read a signed long long (see try to read an unsigned int)
   */
  bool try_read (long long &value);

  /**
   *  @brief Try to read a double (see try to read an unsigned int)
   */
  bool try_read (double &value);

  /**
   *  @brief Try to read a float (see try to read an unsigned int)
   */
  bool try_read (float &value);

  /**
   *  @brief Try to read a boolean value (see try to read an unsigned int)
   *
   *  The value can be either 0 or 1 or "true" or "false".
   */
  bool try_read (bool &value);

  /**
   *  @brief Try to read a string (see try to read an unsigned int)
   *
   *  The termination character may be specified with the "term"
   *  string: if one of the characters in this string is encountered
   *  or the input ends, the reader stops reading.
   */
  bool try_read (std::string &string, const char *term = "");

  /**
   *  @brief Try to read a name string
   *
   *  Name strings are like words, but for the first character digits are not allowed.
   */
  bool try_read_name (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Try to read a string consisting of "word" characters
   *
   *  A word is a non-empty string consisting of letters, digits and the special
   *  characters "_.$" by default. 
   */
  bool try_read_word (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Try to read a quoted string 
   *
   *  See "read_quoted" for details
   */
  bool try_read_quoted (std::string &value);

  /**
   *  @brief Try to read a word or a quoted string 
   *
   *  See "read_word_or_quoted" for details
   */
  bool try_read_word_or_quoted (std::string &value, const char *non_term = "_.$");

  /**
   *  @brief Generic extrator
   * 
   *  This extractor requires a adaptor to actually to the extraction.
   *  The adaptor is a functor taking the extractor and delivering the 
   *  specified type (void tl::test_extractor_impl (tl::Extractor &, T &)).
   */
  template <class T>
  bool try_read (T &value)
  {
    return tl::test_extractor_impl (*this, value);
  }

  /**
   *  @brief Expect a token (a certain string)
   *
   *  If the token is not present, issue an error.
   */
  Extractor &expect (const char *token);

  /**
   *  @brief Expect end of the string
   */
  Extractor &expect_end ();

  /**
   *  @brief Expect more text
   */
  Extractor &expect_more ();

  /**
   *  @brief Test for a token (a certain string)
   *
   *  If the token is not present, return false.
   */
  bool test (const char *token);

  /**
   *  @brief Test for a token (a certain string) in case-insensitive mode
   *
   *  If the token is not present, return false.
   */
  bool test_without_case (const char *token);

  /**
   *  @brief Skip blanks
   *
   *  A helper method to implement parsers on the character-by-character basis.
   *  
   *  @param cp The current pointer (is being moved)
   *  @return The value of cp on return
   */
  const char *skip ();

  /**
   *  @brief Access to the current character
   */
  char operator* () const
  {
    return *m_cp;
  }

  /**
   *  @brief Access to the current position
   */
  const char *get () const
  {
    return m_cp;
  }

  /**
   *  @brief Increment: advance to the next character
   */
  Extractor &operator++ () 
  {
    ++m_cp;
    return *this;
  }

  /**
   *  @brief Test for end of stream
   *
   *  This is not really a const method since it does a "skip" to determine if we
   *  are at the end already.
   */
  bool at_end () 
  {
    return *skip () == 0;
  }

  /**
   *  @brief Throw an error with a context information
   */
  virtual void error (const std::string &msg);

  /**
   *  @brief Some syntactic sugar
   *
   *  Allows extracting something this way: extractor >> x;
   */
  template <class X>
  Extractor &operator>> (X &x) 
  {
    return read (x);
  }

  /**
   *  @brief Extract a token (constant string)
   */
  Extractor &operator>> (const char *token)
  {
    return expect (token);
  }

  /**
   *  @brief Expect the end of the string
   */
  Extractor &operator>> (end)
  {
    return expect_end ();
  }

private:
  template <class T> bool try_read_signed_int (T &value);
  template <class T> bool try_read_unsigned_int (T &value);

  const char *m_cp;
  std::string m_str;
};

TL_PUBLIC void from_string (const std::string &s, const char * &result);
TL_PUBLIC void from_string (const std::string &s, const unsigned char * &result);
TL_PUBLIC void from_string (const std::string &s, double &v);
TL_PUBLIC void from_string (const std::string &s, int &v);
TL_PUBLIC void from_string (const std::string &s, long &v);
TL_PUBLIC void from_string (const std::string &s, long long &v);
TL_PUBLIC void from_string (const std::string &s, unsigned int &v);
TL_PUBLIC void from_string (const std::string &s, unsigned long &v);
TL_PUBLIC void from_string (const std::string &s, unsigned long long &v);
TL_PUBLIC void from_string (const std::string &s, bool &b);

TL_PUBLIC void from_string_ext (const std::string &s, double &v);
TL_PUBLIC void from_string_ext (const std::string &s, int &v);
TL_PUBLIC void from_string_ext (const std::string &s, long &v);
TL_PUBLIC void from_string_ext (const std::string &s, long long &v);
TL_PUBLIC void from_string_ext (const std::string &s, unsigned int &v);
TL_PUBLIC void from_string_ext (const std::string &s, unsigned long &v);
TL_PUBLIC void from_string_ext (const std::string &s, unsigned long long &v);

inline void from_string (const std::string &s, std::string &v) { v = s; }

template <class T> inline void from_string (const std::string &s, T &t)
{
  tl::Extractor ex (s.c_str ());
  ex.read (t);
}

TL_PUBLIC std::string sprintf (const char *fmt, const std::vector<tl::Variant> &a, unsigned int a0 = 0);
TL_PUBLIC std::string sprintf (const std::string &fmt, const std::vector<tl::Variant> &a, unsigned int a0 = 0);

inline std::string sprintf (const std::string &fmt)
{
  std::vector<tl::Variant> a;
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2, const tl::Variant &a3)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  a.push_back (a3);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2, const tl::Variant &a3, const tl::Variant &a4)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  a.push_back (a3);
  a.push_back (a4);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2, const tl::Variant &a3, const tl::Variant &a4, const tl::Variant &a5)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  a.push_back (a3);
  a.push_back (a4);
  a.push_back (a5);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2, const tl::Variant &a3, const tl::Variant &a4, const tl::Variant &a5, const tl::Variant &a6)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  a.push_back (a3);
  a.push_back (a4);
  a.push_back (a5);
  a.push_back (a6);
  return sprintf(fmt, a);
}

inline std::string sprintf (const std::string &fmt, const tl::Variant &a1, const tl::Variant &a2, const tl::Variant &a3, const tl::Variant &a4, const tl::Variant &a5, const tl::Variant &a6, const tl::Variant &a7)
{
  std::vector<tl::Variant> a;
  a.push_back (a1);
  a.push_back (a2);
  a.push_back (a3);
  a.push_back (a4);
  a.push_back (a5);
  a.push_back (a6);
  a.push_back (a7);
  return sprintf(fmt, a);
}

TL_PUBLIC std::string trim (const std::string &s);
TL_PUBLIC std::vector<std::string> split (const std::string &s, const std::string &sep);

/**
 *  @brief Joins a generic iterated list into a single string using the given separator
 */
template <class Iter>
TL_PUBLIC_TEMPLATE std::string join (Iter from, Iter to, const std::string &sep)
{
  std::ostringstream r;

  bool first = true;
  for (Iter i = from; i != to; ++i) {
    if (!first) {
      r << sep;
    }
    first = false;
    r << tl::to_string (*i);
  }

  return r.str ();
}

/**
 *  @brief Joins a vector of strings into a single string using the given separator
 */
inline std::string join (const std::vector<std::string> &strings, const std::string &sep)
{
  return join (strings.begin (), strings.end (), sep);
}

/**
 *  @brief Returns the lower-case character for a wchar_t
 */
TL_PUBLIC wchar_t wdowncase (wchar_t c);

/**
 *  @brief Returns the upper-case character for a wchar_t
 */
TL_PUBLIC wchar_t wupcase (wchar_t c);

/**
 *  @brief Returns the lower-case UTF32 character
 */
TL_PUBLIC uint32_t utf32_downcase (uint32_t c32);

/**
 *  @brief Returns the upper-case UTF32 character
 */
TL_PUBLIC uint32_t utf32_upcase (uint32_t c32);

/**
 *  @brief Parses the next UTF32 character from an UTF-8 string
 *  @param cp The input character's position, will be set to the next character.
 *  @param cpe The end of the string of 0 for "no end"
 */
TL_PUBLIC uint32_t utf32_from_utf8 (const char *&cp, const char *cpe = 0);

/**
 *  @brief Checks if the next characters are CR, LF or CR+LF and skips them
 *
 *  This function returns true, if a line separated was found and skipped
 */
TL_PUBLIC bool skip_newline (const char *&cp);

/**
 *  @brief checks if the given character is a CR character
 */
inline bool is_cr (char c)
{
  return c == '\015';
}

/**
 *  @brief checks if the given character is a LF character
 */
inline bool is_lf (char c)
{
  return c == '\012';
}

/**
 *  @brief checks if the given character is a CR or LF character
 */
inline bool is_newline (char c)
{
  return is_cr (c) || is_lf (c);
}

} // namespace tl

#endif
