
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2016 Matthias Koefferlein

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



#ifndef HDR_antObject
#define HDR_antObject

#include "antCommon.h"

#include "dbUserObject.h"
#include "dbBox.h"
#include "dbTrans.h"
#include "laySnap.h"

#include <string>

namespace ant {
  
class Template;

/**
 *  @brief A ruler (database) object
 * 
 *  This class implements the actual rulers or markers.
 *  Since this class derives from db::UserObjectBase, these objects
 *  can be stored within the database.
 */  
class ANT_PUBLIC Object
  : public db::DUserObjectBase
{
public:
  typedef db::coord_traits<coord_type> coord_traits;

  /** 
   *  @brief The ruler style 
   *
   *  STY_ruler: a ruler with tick marks
   *  STY_arrow_end: a line with an arrow at the end
   *  STY_arrow_start: a line with a arrow at the start
   *  STY_arrow_both: a line with an arrow at both ends
   *  STY_line: a simple line
   */
  enum style_type { STY_ruler, STY_arrow_end, STY_arrow_start, STY_arrow_both, STY_line };

  /**
   *  @brief The outline modes
   *
   *  OL_diag: connecting start and end point
   *  OL_xy: connecting start and end point, horizontal first then vertical
   *  OL_diag_xy: both OL_diag and OL_xy
   *  OL_yx: connecting start and end point, vertical first then horizontal
   *  OL_diag_yx: both OL_diag and OL_yx
   *  OL_box: draw a box defined by start and end point
   */
  enum outline_type { OL_diag, OL_xy, OL_diag_xy, OL_yx, OL_diag_yx, OL_box };

  Object ();

  Object (const db::DPoint &p1, const db::DPoint &p2, int id, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint);

  Object (const db::DPoint &p1, const db::DPoint &p2, int id, const ant::Template &d);

  Object (const ant::Object &d);

  Object &operator= (const ant::Object &d);

  virtual bool equals (const db::DUserObjectBase *d) const;

  virtual bool less (const db::DUserObjectBase *d) const;

  virtual unsigned int class_id () const;

  virtual db::DUserObjectBase *clone () const;

  virtual db::DBox box () const;

  void transform (const db::ICplxTrans &t)
  {
    transform (db::DCplxTrans (t));
    property_changed ();
  }

  virtual void transform (const db::DCplxTrans &t)
  {
    *this = ant::Object (t * m_p1, t * m_p2, m_id, m_fmt_x, m_fmt_y, m_fmt, m_style, m_outline, m_snap, m_angle_constraint);
    property_changed ();
  }

  virtual void transform (const db::DTrans &t)
  {
    *this = ant::Object (t * m_p1, t * m_p2, m_id, m_fmt_x, m_fmt_y, m_fmt, m_style, m_outline, m_snap, m_angle_constraint);
    property_changed ();
  }

  virtual void transform (const db::DFTrans &t)
  {
    *this = ant::Object (t * m_p1, t * m_p2, m_id, m_fmt_x, m_fmt_y, m_fmt, m_style, m_outline, m_snap, m_angle_constraint);
    property_changed ();
  }

  template <class Trans>
  ant::Object transformed (const Trans &t) const
  {
    ant::Object obj (*this);
    obj.transform (t);
    return obj;
  }

  Object &move (const db::DVector &p)
  {
    m_p1 += p;
    m_p2 += p;
    return *this;
  }

  Object moved (const db::DVector &p) const
  {
    ant::Object d (*this);
    d.move (p);
    return d;
  }

  const db::DPoint &p1 () const
  {
    return m_p1;
  }

  const db::DPoint &p2 () const
  {
    return m_p2;
  }

  void p1 (const db::DPoint &p)
  {
    if (!m_p1.equal (p)) {
      m_p1 = p;
      property_changed ();
    }
  }

  void p2 (const db::DPoint &p)
  {
    if (!m_p2.equal (p)) {
      m_p2 = p;
      property_changed ();
    }
  }

  int id () const
  {
    return m_id;
  }

  void id (int _id) 
  {
    m_id = _id;
  }

  const std::string &fmt () const
  {
    return m_fmt;
  }

  void fmt (const std::string &s)
  {
    if (m_fmt != s) {
      m_fmt = s;
      property_changed ();
    }
  }

  const std::string &fmt_x () const
  {
    return m_fmt_x;
  }

  void fmt_x (const std::string &s)
  {
    if (m_fmt_x != s) {
      m_fmt_x = s;
      property_changed ();
    }
  }

  const std::string &fmt_y () const
  {
    return m_fmt_y;
  }

  void fmt_y (const std::string &s)
  {
    if (m_fmt_y != s) {
      m_fmt_y = s;
      property_changed ();
    }
  }

  style_type style () const
  {
    return m_style;
  }

  void style (style_type s) 
  {
    if (m_style != s) {
      m_style = s;
      property_changed ();
    }
  }

  outline_type outline () const
  {
    return m_outline;
  }

  void outline (outline_type s) 
  {
    if (m_outline != s) {
      m_outline = s;
      property_changed ();
    }
  }

  /**
   *  @brief Angle constraint flag read accessor
   */ 
  bool snap () const
  {
    return m_snap;
  }
  
  /**
   *  @brief Snap flag write accessor
   * 
   *  The snap flag controls whether snapping to objects (edges and vertices)
   *  is active when this template is selected.
   */
  void snap (bool s)
  {
    if (m_snap != s) {
      m_snap = s;
      property_changed ();
    }
  }

  /**
   *  @brief Angle constraint read accessor
   */ 
  lay::angle_constraint_type angle_constraint () const
  {
    return m_angle_constraint;
  }
  
  /**
   *  @brief Angle constraint write accessor
   * 
   *  The angle constraint flag controls which angle constraint is to be used 
   *  for this ruler or the global setting should be used
   *  (if ant::Service::Global is used for the angle constraint).
   */
  void angle_constraint (lay::angle_constraint_type a)
  {
    if (m_angle_constraint != a) {
      m_angle_constraint = a;
      property_changed ();
    }
  }
  
  bool operator< (const ant::Object &b) const;

  /**
   *  @brief Obtain the formatted text for the x label
   */
  std::string text_x () const
  {
    return formatted (m_fmt_x, db::DFTrans ());
  }

  /**
   *  @brief Obtain the formatted text for the y label
   */
  std::string text_y () const
  {
    return formatted (m_fmt_y, db::DFTrans ());
  }

  /**
   *  @brief Obtain the formatted text for the main label
   */
  std::string text () const
  {
    return formatted (m_fmt, db::DFTrans ());
  }

  /**
   *  @brief Obtain the formatted text for the x label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text_x (const db::DFTrans &t) const
  {
    return formatted (m_fmt_x, t);
  }

  /**
   *  @brief Obtain the formatted text for the y label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text_y (const db::DFTrans &t) const
  {
    return formatted (m_fmt_y, t);
  }

  /**
   *  @brief Obtain the formatted text for the main label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text (const db::DFTrans &t) const
  {
    return formatted (m_fmt, t);
  }

  /**
   *  @brief Equality
   */
  bool operator== (const ant::Object &d) const;

  /**
   *  @brief Inequality
   */
  bool operator!= (const ant::Object &d) const
  {
    return !operator== (d);
  }

  /**
   *  @brief The class name for the generic user object factory 
   */
  virtual const char *class_name () const;

  /**
   *  @brief Fill from a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual void from_string (const char *);

  /**
   *  @brief Convert to a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual std::string to_string () const;

  /**
   *  @brief Return the memory used in bytes
   */
  virtual size_t mem_used () const 
  {
    return sizeof (*this);
  }

  /**
   *  @brief Return the memory required in bytes
   */
  virtual size_t mem_reqd () const 
  {
    return sizeof (*this);
  }

protected:
  /**
   *  @brief A notification method that is called when a property of the annotation has changed
   */
  virtual void property_changed ();

private:
  db::DPoint m_p1, m_p2;
  int m_id;
  std::string m_fmt_x;
  std::string m_fmt_y;
  std::string m_fmt;
  style_type m_style;
  outline_type m_outline;
  bool m_snap;
  lay::angle_constraint_type m_angle_constraint;

  std::string formatted (const std::string &fmt, const db::DFTrans &trans) const;
};

}

#endif

