
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


#ifndef HDR_dbOASIS
#define HDR_dbOASIS

#include "dbPluginCommon.h"
#include "dbPoint.h"
#include "dbVector.h"

#include "tlException.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlAssert.h"

#include <string>
#include <vector>

namespace db
{

/**
 *  @brief The diagnostics interface for reporting problems in the reader or writer
 */
class OASISDiagnostics
{
public:
  virtual ~OASISDiagnostics ();

  /**
   *  @brief Issue an error with positional information
   */
  virtual void error (const std::string &txt) = 0;

  /**
   *  @brief Issue a warning with positional information
   */
  virtual void warn (const std::string &txt, int warn_level = 1) = 0;
};

class RepetitionBase;
class RepetitionIteratorBase;
class OASISReader;

/**
 *  @brief A repetition iterator
 */
class DB_PLUGIN_PUBLIC RepetitionIterator
{
public:
  /**
   *  @brief Create a repetition with the given implementation
   */
  RepetitionIterator (RepetitionIteratorBase *base);

  /**
   *  @brief Destructor
   */
  ~RepetitionIterator ();

  /**
   *  @brief Copy constructor
   */
  RepetitionIterator (const RepetitionIterator &d);

  /**
   *  @brief Assignment 
   */
  RepetitionIterator &operator= (const RepetitionIterator &d);

  /**
   *  @brief Comparison
   */
  bool operator== (const RepetitionIterator &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const RepetitionIterator &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Tell, if the iterator is at the end
   */
  bool at_end () const;

  /**
   *  @brief Increment
   */
  RepetitionIterator &operator++ ();

  /**
   *  @brief Access
   */
  db::Vector operator* () const;

private:
  RepetitionIteratorBase *mp_base; 
};

/**
 *  @brief A class representing a repetition
 */
class DB_PLUGIN_PUBLIC Repetition
{
public:
  /**
   *  @brief Create a repetition with the given implementation
   */
  Repetition (RepetitionBase *base = 0);

  /**
   *  @brief Destructor
   */
  ~Repetition ();

  /**
   *  @brief Copy constructor
   */
  Repetition (const Repetition &d);

  /**
   *  @brief Replace the base pointer
   */
  Repetition &operator= (RepetitionBase *base);

  /**
   *  @brief Assignment 
   */
  Repetition &operator= (const Repetition &d);

  /**
   *  @brief "Less" predicate
   */
  bool operator< (const Repetition &d) const;

  /**
   *  @brief Comparison
   */
  bool operator== (const Repetition &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const Repetition &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Gets the number of elements in this repetition
   */
  size_t size () const;

  /**
   *  @brief Check, if the repetition is an repetition at all 
   *
   *  This method returns true, if the repetition is not singular.
   *  Singular repetitions are created by the default contructor.
   */
  bool is_singular () const
  {
    return mp_base == 0;
  }

  /**
   *  @brief Checks, if the repetition is a regular one
   *
   *  This method returns true, if the repetition is regular. It 
   *  returns true, if the repetition can be represented as a 
   *  set of points 0..i*a+j*b (i=0..n-1,j=0..m-1).
   *  This method does not only return a flag but the parameters as
   *  well.
   */
  bool is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const;

  /**
   *  @brief Check, if the repetition is a iterated one
   *
   *  @return 0 if not, otherwise a pointer to a vector of points
   */
  const std::vector<db::Vector> *is_iterated () const;

  /**
   *  @brief Get the iterator
   */
  RepetitionIterator begin () const;

  /**
   *  @brief Access to the base object
   */
  void set_base (RepetitionBase *b);

  /**
   *  @brief Access to the base object
   */
  RepetitionBase *base ()
  {
    return mp_base;
  }

  /**
   *  @brief Access to the base object
   */
  const RepetitionBase *base () const
  {
    return mp_base;
  }

private:
  RepetitionBase *mp_base; 
};

//  Base classes

class DB_PLUGIN_PUBLIC RepetitionBase
{
public:
  RepetitionBase ()
  {
    // .. nothing yet ..
  }

  virtual ~RepetitionBase ()
  {
    // .. nothing yet ..
  }

  virtual RepetitionBase *clone () const = 0;
  virtual RepetitionIteratorBase *begin () const = 0;
  virtual unsigned int type () const = 0;
  virtual bool equals (const RepetitionBase *) const = 0;
  virtual bool less (const RepetitionBase *) const = 0;
  virtual bool is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const = 0;
  virtual size_t size () const = 0;
  virtual const std::vector<db::Vector> *is_iterated () const = 0;
  virtual unsigned int type () = 0;
};


class RepetitionIteratorBase 
{
public:
  RepetitionIteratorBase ()
  {
    // .. nothing yet ..
  }

  virtual ~RepetitionIteratorBase ()
  {
    // .. nothing yet ..
  }

  virtual RepetitionIteratorBase *clone () const = 0;
  virtual void inc () = 0;
  virtual db::Vector get () const = 0;
  virtual unsigned int type () const = 0;
  virtual bool equals (const RepetitionIteratorBase *) const = 0;
  virtual bool at_end () const = 0;
};


//  Regular repetitions

class RegularRepetition 
  : public RepetitionBase
{
public:
  RegularRepetition (const db::Vector &a, const db::Vector &b, size_t n, size_t m);

  virtual RepetitionBase *clone () const;
  virtual RepetitionIteratorBase *begin () const;
  virtual unsigned int type () const;
  virtual bool equals (const RepetitionBase *b) const;
  virtual bool less (const RepetitionBase *b) const;
  virtual bool is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const;
  virtual const std::vector<db::Vector> *is_iterated () const;
  virtual unsigned int type () { return 1; }
  virtual size_t size () const { return m_n * m_m; }

private:
  friend class RegularRepetitionIterator;

  db::Vector m_a, m_b;
  size_t m_n, m_m;
};

class RegularRepetitionIterator 
  : public RepetitionIteratorBase
{
public:
  RegularRepetitionIterator (const RegularRepetition *rep, size_t i, size_t j);

  virtual RepetitionIteratorBase *clone () const;
  virtual void inc ();
  virtual db::Vector get () const;
  virtual unsigned int type () const;
  virtual bool equals (const RepetitionIteratorBase *) const;
  virtual bool at_end () const;

private:
  const RegularRepetition *mp_rep;
  size_t m_i, m_j;
};

//  Irregular repetitions

class IrregularRepetition 
  : public RepetitionBase
{
public:
  IrregularRepetition ();

  virtual RepetitionBase *clone () const;
  virtual RepetitionIteratorBase *begin () const;
  virtual unsigned int type () const;
  virtual bool equals (const RepetitionBase *b) const;
  virtual bool less (const RepetitionBase *b) const;
  virtual bool is_regular (db::Vector &a, db::Vector &b, size_t &n, size_t &m) const;
  virtual const std::vector<db::Vector> *is_iterated () const;
  virtual unsigned int type () { return 2; }
  virtual size_t size () const { return m_points.size () + 1; }

  void reserve (size_t n) 
  {
    m_points.reserve (n);
  }

  void push_back (const db::Vector &v)
  {
    m_points.push_back (v);
  }

  std::vector<db::Vector> &points ()
  {
    return m_points;
  }

private:
  friend class IrregularRepetitionIterator;

  std::vector <db::Vector> m_points;
};

class IrregularRepetitionIterator 
  : public RepetitionIteratorBase
{
public:
  IrregularRepetitionIterator (const IrregularRepetition *rep, size_t i);

  virtual RepetitionIteratorBase *clone () const;
  virtual void inc ();
  virtual db::Vector get () const;
  virtual unsigned int type () const;
  virtual bool equals (const RepetitionIteratorBase *) const;
  virtual bool at_end () const;

private:
  const IrregularRepetition *mp_rep;
  size_t m_i;
};

/**
 *  @brief A template class representing a modal variable
 */
template <class T>
class modal_variable
{
public:
  /**
   *  @brief Set up the modal variable
   *
   *  The reader is required since errors are reported to the reader.
   *  The name is reported in the error message. 
   *  This constructor creates an uninitialized variable.
   */
  modal_variable (OASISDiagnostics *reader = 0, const char *name = "")
    : mp_diag (reader), m_name (name), m_t (), m_initialized (false)
  {
    //  .. nothing yet ..
  }

  /**
   *  @brief Get the value of the modal variable
   * 
   *  An error will be reported if the value is not initialized.
   */
  const T &get () const;

  /**
   *  @brief Get the value of the modal variable
   * 
   *  This method does not report an error if the value is not initialized.
   *  After modifying the object, set_initialized is supposed to be called.
   */
  T &get_non_const ()
  {
    return m_t;
  }

  /**
   *  @brief Equality
   */
  bool operator== (const T &t) const
  {
    return (m_initialized && m_t == t);
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const T &t) const
  {
    return !operator== (t);
  }

  /**
   *  @brief Assignment operator
   */
  modal_variable &operator= (const modal_variable<T> &d)
  {
    m_t = d.m_t;
    m_initialized = d.m_initialized;
    return *this;
  }

  /**
   *  @brief Assign a value to the modal variable
   *
   *  This sets the variable into the initialized state.
   */
  template <class X>
  modal_variable &operator= (const X &x)
  {
    m_t = x;
    m_initialized = true;
    return *this;
  }

  /**
   *  @brief Swap with a target value (mainly intended for T==vector<something>)
   *
   *  This sets the variable into the initialized state.
   */
  template <class X>
  modal_variable &swap (X &x)
  {
    m_t.swap (x);
    m_initialized = true;
    return *this;
  }

  /**
   *  @brief Reset the initialized state of the variable
   */
  void reset () 
  {
    m_initialized = false;
  }

  /**
   *  @brief Set the initialized state of the variable
   *
   *  This method is supposed to be used after a modification has been performed
   *  with the non-const accessor.
   */
  void set_initialized () 
  {
    m_initialized = true;
  }

  /**
   *  @brief Test, if the variable is set
   */
  bool is_set () const
  {
    return m_initialized;
  }

private:
  OASISDiagnostics *mp_diag;
  std::string m_name;
  T m_t;
  bool m_initialized;
};

template <class T>
inline const T &
modal_variable<T>::get () const
{
  if (! m_initialized) {
    if (mp_diag) {
      mp_diag->warn (tl::to_string (tr ("Modal variable accessed before being defined: ")) + m_name);
    } else {
      tl_assert (false);
    }
  }
  return m_t;
}

}

#endif

