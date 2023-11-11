
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



#ifndef HDR_layLineStyle
#define HDR_layLineStyle

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
 *  @brief A class representing a line style
 */
class LAYBASIC_PUBLIC LineStyleInfo
{
public:
  /** 
   *  @brief The default constructor
   */
  LineStyleInfo ();
  
  /** 
   *  @brief The copy constructor
   */
  LineStyleInfo (const LineStyleInfo &d);
  
  /**
   *  @brief Assignment operator
   */
  LineStyleInfo &operator= (const LineStyleInfo &d);

  /**
   *  @brief Comparison of pattern string
   */
  bool same_bits (const LineStyleInfo &d) const;

  /**
   *  @brief Comparison of pattern string (operator<)
   */
  bool less_bits (const LineStyleInfo &d) const;

  /**
   *  @brief Equality operator
   *
   *  This operator compares pattern string, names and order index
   */
  bool operator== (const LineStyleInfo &d) const;

  /**
   *  @brief Equality operator
   *
   *  This operator compares pattern string, names and order index
   */
  bool operator< (const LineStyleInfo &d) const;

  /**
   *  @brief Inequality operator
   */
  bool operator!= (const LineStyleInfo &d) const
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

  /**
   *  @brief Gets the the pattern string
   *
   *  The pattern returned is guaranteed to be at least of
   *  size (32*pattern_stride)x64 bits. If the actual width or height is smaller,
   *  the pattern is repeated to fill this area.
   *  The pattern stride may be bigger than 1 to accommodate pattern with
   *  a width that is not a fraction of 32. Such pattern are repeated until
   *  they fill a multiple of 32 bits.
   */
  const uint32_t *pattern () const
  {
    return m_pattern;
  }

#if defined(HAVE_QT)
  /**
   *  @brief Get a monochrome bitmap object for this pattern
   *
   *  @param width The desired width (-1 for default)
   *  @param height The desired height (-1 for default)
   *  @param The intended frame width in pixels
   */
  QBitmap get_bitmap (int width = -1, int height = -1, int frame_width = 1) const;
#endif

  /**
   *  @brief Replaces the pattern string
   *
   *  'w' is the number of bits to consider of the pattern.
   *  w needs to be between 0 and 32. A width of 0 means a solid pattern.
   */
  void set_pattern (uint32_t bits, unsigned int w);

  /**
   *  @brief Scales the existing pattern
   *
   *  Each bit is stretch into n bits.
   */
  void scale_pattern (unsigned int n);

  /**
   *  @brief Gets a scaled version of the pattern
   */
  const LineStyleInfo &scaled (unsigned int n) const;

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
   *  @brief Gets a value indicating whether the nth bit is set
   */
  bool is_bit_set (unsigned int n) const;

  /**
   *  @brief Gets the width
   */
  unsigned int width () const
  {
    return m_width;
  }

  /**
   *  @brief Load from a string
   */
  void from_string (const std::string &s);

  /**
   *  @brief Convert to string
   */
  std::string to_string () const;

private:
  uint32_t m_pattern [32];
  unsigned int m_width;
  unsigned int m_pattern_stride;
  unsigned int m_order_index;
  std::string m_name;
  mutable std::unique_ptr<std::map<unsigned int, LineStyleInfo> > m_scaled_pattern;

  void assign_no_lock (const LineStyleInfo &other);
};

/**
 *  @brief This class represents the set of line styles available
 *
 *  The main method for accessing the style is through the "style"
 *  method which delivers a LineStyleInfo object. The style can be 
 *  replaced with a new pattern, except for the first styles which
 *  cannot be changed. 
 */
class LAYBASIC_PUBLIC LineStyles :
    public db::Object
{
public:
  typedef std::vector<LineStyleInfo> pattern_vector;
  typedef pattern_vector::const_iterator iterator;

  /**
   *  @brief The default constructor
   *
   *  This method initializes the first 16 pattern.
   */
  LineStyles ();

  /**
   *  @brief The copy constructor
   */
  LineStyles (const LineStyles &d);

  /**
   *  @brief The destructor
   */
  ~LineStyles ();

  /**
   *  @brief Assignment operator
   */
  LineStyles &operator= (const LineStyles &p);

  /**
   *  @brief Equality
   */
  bool operator== (const LineStyles &p) const
  {
    return m_styles == p.m_styles;
  }

  /**
   *  @brief Inequality
   */
  bool operator!= (const LineStyles &p) const
  {
    return m_styles != p.m_styles;
  }

  /**
   *  @brief Deliver the style with the given index
   *
   *  If the index is not valid, an empty style is returned.
   */
  const LineStyleInfo &style (unsigned int i) const;

  /**
   *  @brief Replace the style with the given index
   *
   *  The first style cannot be replaced. In this case, the change
   *  request is simply ignored. 
   *  By replacing the style with one with an order_index of 0, 
   *  the pattern is virtually deleted (such pattern are not shown in the editor)
   */
  void replace_style (unsigned int i, const LineStyleInfo &p);

  /**
   *  @brief Add a new style, searching for a empty slot and returning that index
   *
   *  This method will look for the first style with a order index of 0 
   *  or create a new entry if no such style exists. This entry will be used
   *  to place the style into. The order_index will be set to the highest value
   *  plus one thus placing the new style at the end of the list in the editor.
   */
  unsigned int add_style (const LineStyleInfo &p);

  /**
   *  @brief Renumber the order indices to numbers increasing by 1 only
   *
   *  This method should be called when a style is deleted by setting it's
   *  order_index to 0.
   */
  void renumber ();

  /**
   *  @brief Merge two style lists
   *
   *  *this is filled with all the styles of "other" which are not
   *  member of this list yet. A mapping table is filled, mapping 
   *  an index of "other" to an index inside *this;
   */
  void merge (const LineStyles &other, std::map<unsigned int, unsigned int> &index_map);

  /**
   *  @brief Return the number of styles
   */
  unsigned int count () const
  {
    return (unsigned int) m_styles.size ();
  }

  /**
   *  @brief The begin iterator delivering the custom style objects
   * 
   *  The corresponding end iterator is delivered with end()
   */
  iterator begin_custom () const;

  /**
   *  @brief The begin iterator delivering all style objects
   */
  iterator begin () const 
  {
    return m_styles.begin ();
  }

  /**
   *  @brief The begin iterator delivering the past-the-end style object
   */
  iterator end () const 
  {
    return m_styles.end ();
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
   *  @brief Accessor to the default style (solid)
   */
  static const LineStyles &default_style ();

private:
  std::vector<LineStyleInfo> m_styles;
};

}

#endif

