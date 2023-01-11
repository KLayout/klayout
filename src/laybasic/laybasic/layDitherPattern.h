
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



#ifndef HDR_layDitherPattern
#define HDR_layDitherPattern

#include "laybasicCommon.h"

#if defined(HAVE_QT)
#  include <QObject>
#  include <QBitmap>
#endif

#include "dbObject.h"

#include <stdint.h>

#include <vector>
#include <string>
#include <map>
#include <memory>

namespace lay
{

/**
 *  @brief A class representing a single dither pattern
 */
class LAYBASIC_PUBLIC DitherPatternInfo
{
public:
  /** 
   *  @brief The default constructor
   */
  DitherPatternInfo ();
  
  /** 
   *  @brief The copy constructor
   */
  DitherPatternInfo (const DitherPatternInfo &d);
  
  /**
   *  @brief Assignment operator
   */
  DitherPatternInfo &operator= (const DitherPatternInfo &d);

  /**
   *  @brief Comparison of pattern bitmap
   */
  bool same_bitmap (const DitherPatternInfo &d) const;

  /**
   *  @brief Comparison of pattern bitmap (operator<)
   */
  bool less_bitmap (const DitherPatternInfo &d) const;

  /**
   *  @brief Equality operator
   *
   *  This operator compares bitmaps, names and order index
   */
  bool operator== (const DitherPatternInfo &d) const;

  /**
   *  @brief Equality operator
   *
   *  This operator compares bitmaps, names and order index
   */
  bool operator< (const DitherPatternInfo &d) const;

  /**
   *  @brief Inequality operator
   */
  bool operator!= (const DitherPatternInfo &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief Read access to the name
   */
  const std::string &name () const
  {
    return m_name;
  }

  /**
   *  @brief Write access to the name
   */
  void set_name (const std::string &name) 
  {
    m_name = name;
  }

  /**
   *  @brief Read access to the order index
   */
  unsigned int order_index () const
  {
    return m_order_index;
  }

  /**
   *  @brief Write access to the name
   */
  void set_order_index (unsigned int oi) 
  {
    m_order_index = oi;
  }

#if defined(HAVE_QT)
  /**
   *  @brief Get a monochrome bitmap object for this pattern
   *
   *  @param width The desired width (-1 for default)
   *  @param height The desired height (-1 for default)
   *  @param frame_width The width of the frame around the bitmap
   */
  QBitmap get_bitmap (int width = -1, int height = -1, int frame_width = -1) const;
#endif

  /**
   *  @brief Gets the the dither pattern
   *
   *  The pattern returned is guaranteed to be at least of
   *  size (32*pattern_stride)x64 bits. If the actual width or height is smaller,
   *  the pattern is repeated to fill this area.
   *  The pattern stride may be bigger than 1 to accommodate pattern with
   *  a width that is not a fraction of 32. Such pattern are repeated until
   *  they fill a multiple of 32 bits.
   */
  const uint32_t * const *pattern () const
  {
    return & (m_pattern[0]);
  }

  /**
   *  @brief Replaces the dither pattern
   *
   *  'w' and 'h' denote the width and height of the pattern passed.
   *  If 'w' is less than 32, the lowest 'w' bits must contain the pattern.
   *  'w' and 'h' are supposed to be a integer value between 1 and 32.
   *  The pattern is required to be an array of at least h words. Only the
   *  first w bits of these words are taken.
   */
  void set_pattern (const uint32_t *pattern, unsigned int w, unsigned int h);

  /**
   *  @brief Replaces the dither pattern (64 bit version)
   */
  void set_pattern (const uint64_t *pattern, unsigned int w, unsigned int h);

  /**
   *  @brief Gets the pattern stride
   *
   *  The pattern stride is the number of words each pattern is made of
   *  The first width bits are repeated until they fill a multiple of
   *  32 bits. The number of words required for this is the pattern stride.
   */
  unsigned int pattern_stride () const
  {
    return m_pattern_stride;
  }

  /**
   *  @brief Gets the width
   */
  unsigned int width () const
  {
    return m_width;
  }

  /**
   *  @brief Gets the height
   */
  unsigned int height () const
  {
    return m_height;
  }

  /**
   *  @brief Scales the existing pattern
   *
   *  Each bit is stretch into n bits vertically and horizontally.
   *  Smart interpolation is attempted.
   */
  void scale_pattern (unsigned int n);

  /**
   *  @brief Gets a scaled version of the pattern
   */
  const DitherPatternInfo &scaled (unsigned int n) const;

  /**
   *  @brief Load from a string
   */
  void from_string (const std::string &s);

  /**
   *  @brief Convert to string
   */
  std::string to_string () const;

  /**
   *  @brief Load from a set of strings (one per line)
   */
  void from_strings (const std::vector<std::string> &s);

  /**
   *  @brief Convert to strings (one per line)
   */
  std::vector<std::string> to_strings () const;

private:
  uint32_t *m_pattern[64];
  uint32_t m_buffer [64 * 64];
  unsigned int m_width, m_height;
  unsigned int m_pattern_stride;
  unsigned int m_order_index;
  std::string m_name;
  mutable std::unique_ptr<std::map<unsigned int, DitherPatternInfo> > m_scaled_pattern;

  void set_pattern_impl (const uint32_t *pattern, unsigned int w, unsigned int h);
  void set_pattern_impl (const uint64_t *pattern, unsigned int w, unsigned int h);
  void assign_no_lock (const DitherPatternInfo &other);
};

/**
 *  @brief This class represents the set of dither pattern available
 *
 *  The main method for accessing the pattern is through the "pattern"
 *  method which delivers a DitherPatternInfo object. The pattern can be 
 *  replaced with a new pattern, except for the first pattern which
 *  cannot be changed. 
 */
class LAYBASIC_PUBLIC DitherPattern :
    public db::Object
{
public:
  typedef std::vector<DitherPatternInfo> pattern_vector;
  typedef pattern_vector::const_iterator iterator;

  /**
   *  @brief The default constructor
   *
   *  This method initializes the first 16 pattern.
   */
  DitherPattern ();

  /**
   *  @brief The copy constructor
   */
  DitherPattern (const DitherPattern &d);

  /**
   *  @brief The destructor
   */
  ~DitherPattern ();

  /**
   *  @brief Assignment operator
   */
  DitherPattern &operator= (const DitherPattern &p);

  /**
   *  @brief Equality
   */
  bool operator== (const DitherPattern &p) const
  {
    return m_pattern == p.m_pattern;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const DitherPattern &p) const
  {
    return m_pattern != p.m_pattern;
  }

  /**
   *  @brief Deliver the pattern with the given index
   *
   *  If the index is not valid, an empty pattern is returned.
   */
  const DitherPatternInfo &pattern (unsigned int i) const;

  /**
   *  @brief Replace the pattern with the given index
   *
   *  The first pattern cannot be replaced. In this case, the change
   *  request is simply ignored. 
   *  By replacing the pattern with one with an order_index of 0, 
   *  the pattern is virtually deleted (such pattern are not shown in the editor)
   */
  void replace_pattern (unsigned int i, const DitherPatternInfo &p);

  /**
   *  @brief Add a new pattern, searching for a empty slot and returning that index
   *
   *  This method will look for the first pattern with a order index of 0 
   *  or create a new entry if no such pattern exists. This entry will be used
   *  to place the pattern to. The order_index will be set to the highest value
   *  plus one thus placing the new pattern at the end of the list in the editor.
   */
  unsigned int add_pattern (const DitherPatternInfo &p);

  /**
   *  @brief Scales the pattern by the given factor
   */
  void scale_pattern (unsigned int n);

  /**
   *  @brief Renumber the order indices to numbers increasing by 1 only
   *
   *  This method should be called when a pattern is deleted by setting it's
   *  order_index to 0.
   */
  void renumber ();

  /**
   *  @brief Merge two dither pattern lists
   *
   *  *this is filled with all the pattern of "other" which are not
   *  member of this list yet. A mapping table is filled, mapping 
   *  an index of "other" to an index inside *this;
   */
  void merge (const DitherPattern &other, std::map<unsigned int, unsigned int> &index_map);

  /**
   *  @brief Return the number stipples 
   */
  unsigned int count () const
  {
    return (unsigned int) m_pattern.size ();
  }

  /**
   *  @brief The begin iterator delivering the custom pattern objects
   * 
   *  The corresponding end iterator is delivered with end()
   */
  iterator begin_custom () const;

  /**
   *  @brief The begin iterator delivering all pattern objects
   */
  iterator begin () const 
  {
    return m_pattern.begin ();
  }

  /**
   *  @brief The begin iterator delivering the past-the-end pattern object
   */
  iterator end () const 
  {
    return m_pattern.end ();
  }

  /**
   *  @brief Implementation of the db::Object interface
   */
  void undo (db::Op *op);

  /**
   *  @brief Implementation of the db::Object interface
   */
  void redo (db::Op *op);

  /**
   *  @brief Accessor to the default dither pattern set
   */
  static const DitherPattern &default_pattern ();

private:
  std::vector<DitherPatternInfo> m_pattern;
};

}

#endif

