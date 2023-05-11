
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


#ifndef HDR_dbHershey
#define HDR_dbHershey

#include "dbEdge.h"
#include "dbBox.h"
#include "dbHersheyFont.h"
#include "dbCommon.h"

#include <vector>

namespace db {

struct HersheyFont;

DB_PUBLIC int hershey_font_width (unsigned int f);
DB_PUBLIC int hershey_font_height (unsigned int f);
DB_PUBLIC db::DBox hershey_text_box (const std::string &s, unsigned int f);
DB_PUBLIC void hershey_justify (const std::string &s, unsigned int f, db::DBox bx, HAlign halign, VAlign valign, std::vector<db::DPoint> &linestarts, double &left, double &bottom);
DB_PUBLIC std::vector<std::string> hershey_font_names ();
DB_PUBLIC size_t hershey_count_edges (const std::string &s, unsigned int f);

class DB_PUBLIC basic_hershey_edge_iterator
{
public:
  basic_hershey_edge_iterator (const std::string &s, unsigned int f, const std::vector<db::DPoint> &line_starts);
  bool at_end () const;
  db::DEdge get ();
  void inc ();

private:
  unsigned int m_line;
  const char *mp_cp;
  std::string m_string;
  unsigned int m_edge, m_edge_end;
  std::vector<db::DPoint> m_linestarts;
  db::DPoint m_pos;
  db::DVector m_delta;
  HersheyFont *m_fp;
};

template <class C>
class DB_PUBLIC_TEMPLATE hershey_edge_iterator
  : private basic_hershey_edge_iterator
{
public:
  typedef C coord_type;
  typedef db::coord_traits<C> coord_traits;

  /**
   *  @brief Standard constructor of the hershey edge iterator
   */
  hershey_edge_iterator (const std::string &s, unsigned int f, const std::vector<db::DPoint> &line_starts, double scale)
    : basic_hershey_edge_iterator (s, f, line_starts),
      m_scale (scale)
  {
    //  .. nothing yet.
  }
  
  /**
   *  @brief Test if there are more edges
   *
   *  @return true, if there are more edges to deliver
   */
  bool 
  at_end () const
  {
    return basic_hershey_edge_iterator::at_end ();
  }

  /**
   *  @brief Deliver the current edge
   *
   *  @return The current edge
   */
  edge<C> operator* ()
  {
    db::DEdge e = basic_hershey_edge_iterator::get ();
    return edge<C> (point<C> (coord_traits::rounded (e.p1 ().x () * m_scale), coord_traits::rounded (e.p1 ().y () * m_scale)),
                    point<C> (coord_traits::rounded (e.p2 ().x () * m_scale), coord_traits::rounded (e.p2 ().y () * m_scale)));
  }

  /**
   *  @brief Increment operator
   *
   *  Increment the iterator. Must not be called, if at_end() is true.
   */
  hershey_edge_iterator &
  operator++ ()
  {
    basic_hershey_edge_iterator::inc ();
    return *this;
  }

private:
  double m_scale;
};

/**
 *  @brief A hershey text class
 */

template <class C>
struct DB_PUBLIC_TEMPLATE hershey
{
  typedef C coord_type;
  typedef db::coord_traits<C> coord_traits;
  typedef hershey_edge_iterator<C> edge_iterator;

  /** 
   *  @brief Default constructor
   *
   *  Creates a text object with an empty text
   */
  hershey () 
    : m_string (), 
      m_font (DefaultFont),
      m_scale (1.0),
      m_left (0.0), m_bottom (0.0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Standard constructor
   *
   *  @param s The text
   */
  hershey (const std::string &s, Font f) 
    : m_string (s), 
      m_font (f),
      m_scale (1.0),
      m_left (0.0), m_bottom (0.0)
  {
    // .. nothing yet ..
  }

  /**
   *  @brief Scale the text
   *
   *  @param s The scaling factor
   */
  void scale (double s) 
  {
    m_scale *= s;
  }

  /**
   *  @brief Obtain the current scaling factor
   */
  double scale_factor () const
  {
    return m_scale;
  }

  /**
   *  @brief Obtain the size of the text
   *
   *  @return The bounding box of the text with the scaling and justification applied
   */
  db::DBox bbox () const
  {
    db::DBox b = hershey_text_box (m_string, m_font);
    b.move (db::DVector (m_left, m_bottom));
    return b * m_scale;
  }

  /**
   *  @brief Position the text at the given point
   *
   *  @param p The lower left point of the first characters
   */
  void position (const point<C> &p) 
  {
    m_linestarts.clear ();
    m_linestarts.push_back (db::DPoint (coord_traits::rounded (p.x () / m_scale), coord_traits::rounded (p.y () / m_scale)));
  }

  /**
   *  @brief Justify the text within a given bbox
   *
   *  @param b The target box
   *  @param halign The horizontal alignment mode
   *  @param valign The vertical alignment mode
   *  @param scale true, if the text should be scaled to fit into the box
   *  @param margin The amount of margin to leave around the text when scale = true and the box is a "real" box
   *
   *  If the target box is degenerated (width or height is 0) it specifies the 
   *  height or width of the "M" character instead of the whole text.
   */
  void justify (const box<C> &b, HAlign halign, VAlign valign, bool scale = true, double margin = 0.1)
  {
    m_linestarts.clear ();

    if (m_string.size () > 0) {

      if (! scale) {

        db::DPoint p1 (b.p1 ().x () / m_scale, b.p1 ().y () / m_scale);
        db::DPoint p2 (b.p2 ().x () / m_scale, b.p2 ().y () / m_scale);
        hershey_justify (m_string, m_font, db::DBox (p1, p2), halign, valign, m_linestarts, m_left, m_bottom);

      } else {

        if (coord_traits::less (0, b.width ()) && coord_traits::less (0, b.height ())) {

          db::DBox tbx (hershey_text_box (m_string, m_font));
          double fx = double (b.width ()) / double (tbx.width ());
          double fy = double (b.height ()) / double (tbx.height ());
          double f = std::min (fx, fy);
          m_scale = f * (1.0 - 2.0 * margin);

        } else if (coord_traits::less (0, b.width ())) {

          m_scale = double (b.width ()) / double (hershey_font_width (m_font));

        } else if (coord_traits::less (0, b.height ())) {

          m_scale = double (b.height ()) / double (hershey_font_height (m_font));

        }

        if (m_scale > 1e-6) {
          db::DPoint p1 (b.p1 ().x () / m_scale, b.p1 ().y () / m_scale);
          db::DPoint p2 (b.p2 ().x () / m_scale, b.p2 ().y () / m_scale);
          hershey_justify (m_string, m_font, db::DBox (p1, p2), halign, valign, m_linestarts, m_left, m_bottom);
        }

      }

    }
  }

  /**
   *  @brief Count edges required
   *
   *  @return The number of edges required to display this string
   */
  size_t count_edges () const
  {
    return hershey_count_edges (m_string, m_font);
  }

  /**
   *  @brief Edge iterator
   *
   *  @return The edge iterator delivering all edges of the text in the
   *  text object
   */
  hershey_edge_iterator<C> begin_edges () const
  {
    return hershey_edge_iterator<C> (m_string, (unsigned int) m_font, m_linestarts, m_scale);
  }  

  /**
   *  @brief Get font names
   *
   *  The font names are provided as a vector in the order they are
   *  defined in the enum Font. This means, the index can be cast to
   *  the enum rendering the font enum.
   *
   *  @return a vector with the font names.
   */
  static std::vector<std::string> font_names () 
  {
    return hershey_font_names ();
  }


private:
  std::string m_string;
  Font m_font;
  double m_scale;
  std::vector <db::DPoint> m_linestarts;
  double m_left, m_bottom;
};

/**
 *  @brief Standard typedef for db::Coord
 */

typedef db::hershey<db::Coord> Hershey;

/**
 *  @brief Standard typedef for db::DCoord
 */

typedef db::hershey<db::DCoord> DHershey;

}

#endif

