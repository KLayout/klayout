
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
  typedef std::vector<db::DPoint> point_list;

  /** 
   *  @brief The ruler style 
   *
   *  STY_ruler: a ruler with tick marks
   *  STY_arrow_end: a line with an arrow at the end
   *  STY_arrow_start: a line with a arrow at the start
   *  STY_arrow_both: a line with an arrow at both ends
   *  STY_cross_end: a cross at the end
   *  STY_cross_start: a cross at the start
   *  STY_cross_both: a cross at both ends
   *  STY_line: a simple line
   *  STY_none: used internally
   */
  enum style_type { STY_ruler = 0, STY_arrow_end = 1, STY_arrow_start = 2, STY_arrow_both = 3, STY_line = 4, STY_cross_end = 5, STY_cross_start = 6, STY_cross_both = 7, STY_none = 8 };

  /**
   *  @brief The outline modes
   *
   *  OL_diag: connecting start and end point
   *  OL_xy: connecting start and end point, horizontal first then vertical
   *  OL_diag_xy: both OL_diag and OL_xy
   *  OL_yx: connecting start and end point, vertical first then horizontal
   *  OL_diag_yx: both OL_diag and OL_yx
   *  OL_box: draw a box defined by start and end point
   *  OL_ellipse: draws an ellipse with p1 and p2 defining the extension (style is ignored)
   *  OL_angle: an angle measurement ruler (first vs. last segment)
   *  OL_radius: a radius measurement ruler
   */
  enum outline_type { OL_diag = 0, OL_xy = 1, OL_diag_xy = 2, OL_yx = 3, OL_diag_yx = 4, OL_box = 5, OL_ellipse = 6, OL_angle = 7, OL_radius = 8 };

  /**
   *  @brief The position type of the main label
   *
   *  POS_auto: automatic
   *  POS_p1: at P1
   *  POS_p2: at P2
   *  POS_center: at mid point between P1 and P2
   */
  enum position_type { POS_auto = 0, POS_p1 = 1, POS_p2 = 2, POS_center = 3 };

  /**
   *  @brief The alignment type
   *
   *  AL_auto: automatic
   *  AL_center: centered
   *  AL_left, AL_bottom, AL_down: left or bottom
   *  AL_right, AL_top, AL_up: right or top
   */
  enum alignment_type { AL_auto = 0, AL_center = 1, AL_down = 2, AL_left = 2, AL_bottom = 2, AL_up = 3, AL_right = 3, AL_top = 3 };

  /**
   *  @brief Default constructor
   */
  Object ();

  /**
   *  @brief Parametrized constructor
   */
  Object (const db::DPoint &p1, const db::DPoint &p2, int id, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint);

  /**
   *  @brief Parametrized constructor and a list of points
   */
  Object (const point_list &points, int id, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint);

  /**
   *  @brief Parametrized constructor from a template
   */
  Object (const db::DPoint &p1, const db::DPoint &p2, int id, const ant::Template &d);

  /**
   *  @brief Parametrized constructor from a template and a list of points
   */
  Object (const point_list &points, int id, const ant::Template &d);

  /**
   *  @brief Copy constructor
   */
  Object (const ant::Object &d);

  /**
   *  @brief Assignment
   */
  Object &operator= (const ant::Object &d);

  /**
   *  @brief Destructor
   */
  ~Object ();

  /**
   *  @brief Less operator
   */
  bool operator< (const ant::Object &b) const;

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
   *  @brief Equality check
   *  This is the generic equality that involves an other object
   *  of any kind.
   */
  virtual bool equals (const db::DUserObjectBase *d) const;

  /**
   *  @brief Less criterion
   *  This is the generic equality that involves an other object
   *  of any kind.
   */
  virtual bool less (const db::DUserObjectBase *d) const;

  /**
   *  @brief Gets the user object class ID
   */
  virtual unsigned int class_id () const;

  /**
   *  @brief Clones the user object
   */
  virtual db::DUserObjectBase *clone () const;

  /**
   *  @brief Returns the bounding box of the object
   */
  virtual db::DBox box () const;

  /**
   *  @brief Transforms the object (in place)
   */
  void transform (const db::ICplxTrans &t)
  {
    transform (db::DCplxTrans (t));
    property_changed ();
  }

  /**
   *  @brief Transforms the object (in place)
   */
  virtual void transform (const db::DCplxTrans &t)
  {
    for (auto p = m_points.begin (); p != m_points.end (); ++p) {
      *p = t * *p;
    }
    property_changed ();
  }

  /**
   *  @brief Transforms the object (in place)
   */
  virtual void transform (const db::DTrans &t)
  {
    for (auto p = m_points.begin (); p != m_points.end (); ++p) {
      *p = t * *p;
    }
    property_changed ();
  }

  /**
   *  @brief Transforms the object (in place)
   */
  virtual void transform (const db::DFTrans &t)
  {
    for (auto p = m_points.begin (); p != m_points.end (); ++p) {
      *p = t * *p;
    }
    property_changed ();
  }

  /**
   *  @brief Returns the transformed object
   */
  template <class Trans>
  ant::Object transformed (const Trans &t) const
  {
    ant::Object obj (*this);
    obj.transform (t);
    return obj;
  }

  /**
   *  @brief Moves the object by the given distance
   */
  Object &move (const db::DVector &d)
  {
    for (auto p = m_points.begin (); p != m_points.end (); ++p) {
      *p += d;
    }
    return *this;
  }

  /**
   *  @brief Returns the moved object
   */
  Object moved (const db::DVector &p) const
  {
    ant::Object d (*this);
    d.move (p);
    return d;
  }

  /**
   *  @brief Gets the category string
   *  The category string is an arbitrary string that can be used to identify an annotation for
   *  a particular purpose.
   */

  const std::string &category () const
  {
    return m_category;
  }

  /**
   *  @brief Sets the category string
   *  See \category for a description of this attribute.
   */
  void set_category (const std::string &cat)
  {
    if (m_category != cat) {
      m_category = cat;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the ruler's definition points
   */
  const point_list &points () const
  {
    return m_points;
  }

  /**
   *  @brief Sets the ruler's definition points
   */
  void set_points (const point_list &points);

  /**
   *  @brief Sets the ruler's definition points without cleaning
   */
  void set_points_exact (const point_list &points);

  /**
   *  @brief Sets the ruler's definition points without cleaning (move semantics)
   */
  void set_points_exact (point_list &&points);

  /**
   *  @brief Cleans the point list
   */
  void clean_points ();

  /**
   *  @brief Gets the first point of the indicated segment
   */
  db::DPoint seg_p1 (size_t seg_index) const;

  /**
   *  @brief Gets the second point of the indicated segment
   */
  db::DPoint seg_p2 (size_t seg_index) const;

  /**
   *  @brief Sets the first point of the indicated segment
   */
  void seg_p1 (size_t seg_index, const db::DPoint &p);

  /**
   *  @brief Sets the second point of the indicated segment
   */
  void seg_p2 (size_t seg_index, const db::DPoint &p);

  /**
   *  @brief Gets the number of segments
   *
   *  The number of segments is at least 1 for backward compatibility.
   */
  size_t segments () const
  {
    return m_points.size () < 2 ? 1 : m_points.size () - 1;
  }

  /**
   *  @brief Gets the first definition point
   *
   *  This method is provided for backward compatibility. Use the point list accessor for generic point retrieval.
   */
  db::DPoint p1 () const
  {
    return seg_p1 (0);
  }

  /**
   *  @brief Gets the second definition point
   *
   *  This method is provided for backward compatibility. Use the point list accessor for generic point retrieval.
   */
  db::DPoint p2 () const
  {
    return seg_p2 (segments () - 1);
  }

  /**
   *  @brief Sets the first definition point
   *
   *  This method is provided for backward compatibility. Use the point list accessor for generic point retrieval.
   */
  void p1 (const db::DPoint &p);

  /**
   *  @brief Sets the second definition point
   *
   *  This method is provided for backward compatibility. Use the point list accessor for generic point retrieval.
   */
  void p2 (const db::DPoint &p);

  /**
   *  @brief Gets the ID of the annotation object
   *  The ID is a unique identifier for the annotation object. The ID is used
   *  by the layout view to identify the object.
   */
  int id () const
  {
    return m_id;
  }

  /**
   *  @brief Sets the ID of the annotation object
   *  This method is provided for use by the layout view.
   */
  void id (int _id)
  {
    m_id = _id;
  }

  /**
   *  @brief Sets the main format string
   *  The central label is placed either at the first or the second point.
   *  \main_position, \main_xalign, \main_yalign control how the
   *  main label is positioned.
   */
  const std::string &fmt () const
  {
    return m_fmt;
  }

  /**
   *  @brief Sets the main format string
   *  See \fmt for details.
   */
  void fmt (const std::string &s)
  {
    if (m_fmt != s) {
      m_fmt = s;
      property_changed ();
    }
  }

  /**
   *  @brief Sets the position of the main label
   *  See the \position_type enum for details.
   */
  void set_main_position (position_type pos)
  {
    if (m_main_position != pos) {
      m_main_position = pos;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the position of the main label
   */
  position_type main_position () const
  {
    return m_main_position;
  }

  /**
   *  @brief Sets the x alignment flag of the main label
   *  See \alignment_type for details.
   */
  void set_main_xalign (alignment_type a)
  {
    if (m_main_xalign != a) {
      m_main_xalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the x alignment flag of the main label
   */
  alignment_type main_xalign () const
  {
    return m_main_xalign;
  }

  /**
   *  @brief Sets the y alignment flag of the main label
   *  See \alignment_type for details.
   */
  void set_main_yalign (alignment_type a)
  {
    if (m_main_yalign != a) {
      m_main_yalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the y alignment flag of the main label
   */
  alignment_type main_yalign () const
  {
    return m_main_yalign;
  }

  /**
   *  @brief Gets the x label format string
   *  The x label is drawn at the x axis for styles that support a x axis.
   *  \xlabel_xalign and \xlabel_yalign control how the x label is
   *  positioned.
   */
  const std::string &fmt_x () const
  {
    return m_fmt_x;
  }

  /**
   *  @brief Gets the x label format string
   *  See \fmt_x for a description of this attribute.
   */
  void fmt_x (const std::string &s)
  {
    if (m_fmt_x != s) {
      m_fmt_x = s;
      property_changed ();
    }
  }

  /**
   *  @brief Sets the x alignment flag of the x axis label
   *  See \alignment_type for details.
   */
  void set_xlabel_xalign (alignment_type a)
  {
    if (m_xlabel_xalign != a) {
      m_xlabel_xalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the x alignment flag of the x axis label
   */
  alignment_type xlabel_xalign () const
  {
    return m_xlabel_xalign;
  }

  /**
   *  @brief Sets the y alignment flag of the x axis label
   *  See \alignment_type for details.
   */
  void set_xlabel_yalign (alignment_type a)
  {
    if (m_xlabel_yalign != a) {
      m_xlabel_yalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the y alignment flag of the x axis label
   */
  alignment_type xlabel_yalign () const
  {
    return m_xlabel_yalign;
  }

  /**
   *  @brief Gets the y label format string
   *  The y label is drawn at the y axis for styles that support a y axis.
   *  \ylabel_xalign and \ylabel_yalign control how the y label is
   *  positioned.
   */
  const std::string &fmt_y () const
  {
    return m_fmt_y;
  }

  /**
   *  @brief Gets the y label format string
   *  See \fmt_y for a description of this attribute.
   */
  void fmt_y (const std::string &s)
  {
    if (m_fmt_y != s) {
      m_fmt_y = s;
      property_changed ();
    }
  }

  /**
   *  @brief Sets the x alignment flag of the y axis label
   *  See \alignment_type for details.
   */
  void set_ylabel_xalign (alignment_type a)
  {
    if (m_ylabel_xalign != a) {
      m_ylabel_xalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the x alignment flag of the y axis label
   */
  alignment_type ylabel_xalign () const
  {
    return m_ylabel_xalign;
  }

  /**
   *  @brief Sets the y alignment flag of the y axis label
   *  See \alignment_type for details.
   */
  void set_ylabel_yalign (alignment_type a)
  {
    if (m_ylabel_yalign != a) {
      m_ylabel_yalign = a;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the y alignment flag of the y axis label
   */
  alignment_type ylabel_yalign () const
  {
    return m_ylabel_yalign;
  }

  /**
   *  @brief Sets the style
   *  See \style_type enum for the various styles available.
   */
  style_type style () const
  {
    return m_style;
  }

  /**
   *  @brief Gets the style
   */
  void style (style_type s)
  {
    if (m_style != s) {
      m_style = s;
      property_changed ();
    }
  }

  /**
   *  @brief Sets the outline type
   *  See \outline_type enum for the various outline types available.
   */
  outline_type outline () const
  {
    return m_outline;
  }

  /**
   *  @brief Gets the outline type
   */
  void outline (outline_type s) 
  {
    if (m_outline != s) {
      m_outline = s;
      property_changed ();
    }
  }

  /**
   *  @brief Gets the snap mode
   *  See \snap for details about this attribute
   */ 
  bool snap () const
  {
    return m_snap;
  }
  
  /**
   *  @brief Sets snap mode
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
   *  @brief Gets the angle constraint
   */ 
  lay::angle_constraint_type angle_constraint () const
  {
    return m_angle_constraint;
  }
  
  /**
   *  @brief Sets the angle constraint
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

  /**
   *  @brief Gets the formatted text for the x label
   */
  std::string text_x (size_t index) const
  {
    return formatted (m_fmt_x, db::DFTrans (), index);
  }

  /**
   *  @brief Gets the formatted text for the y label
   */
  std::string text_y (size_t index) const
  {
    return formatted (m_fmt_y, db::DFTrans (), index);
  }

  /**
   *  @brief Gets the formatted text for the main label
   */
  std::string text (size_t index) const
  {
    return formatted (m_fmt, db::DFTrans (), index);
  }

  /**
   *  @brief Gets the formatted text for the x label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text_x (size_t index, const db::DFTrans &t) const
  {
    return formatted (m_fmt_x, t, index);
  }

  /**
   *  @brief Gets the formatted text for the y label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text_y (size_t index, const db::DFTrans &t) const
  {
    return formatted (m_fmt_y, t, index);
  }

  /**
   *  @brief Gets the formatted text for the main label
   *  @param t The transformation to apply to the vector before producing the text
   */
  std::string text (size_t index, const db::DFTrans &t) const
  {
    return formatted (m_fmt, t, index);
  }

  /**
   *  @brief Gets the class name for the generic user object factory
   */
  virtual const char *class_name () const;

  /**
   *  @brief Initializes the object from a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual void from_string (const char *s, const char *base_dir = 0);

  /**
   *  @brief Converts the object to a string
   *
   *  This method needs to be implemented mainly if the object is to be created from the
   *  generic factory.
   */
  virtual std::string to_string () const;

  /**
   *  @brief Computes the parameters for an angle ruler
   *  @param radius Returns the radius
   *  @param center Returns the center point
   *  @param start_angle Returns the start angle (in radians)
   *  @param stop_angle Returns the stop angle (in radians)
   *  @return True, if the ruler represents an angle measurement
   */
  bool compute_angle_parameters (double &radius, db::DPoint &center, double &start_angle, double &stop_angle) const;

  /**
   *  @brief Computes the parameters for a radius ruler
   *  @param radius Returns the radius
   *  @param center Returns the center point
   *  @param start_angle Returns the start angle (in radians)
   *  @param stop_angle Returns the stop angle (in radians)
   *  @return True, if the ruler represents an angle measurement
   */
  bool compute_interpolating_circle (double &radius, db::DPoint &center, double &start_angle, double &stop_angle) const;

protected:
  /**
   *  @brief A notification method that is called when a property of the annotation has changed
   */
  virtual void property_changed ();

private:
  point_list m_points;
  int m_id;
  std::string m_fmt_x;
  std::string m_fmt_y;
  std::string m_fmt;
  style_type m_style;
  outline_type m_outline;
  bool m_snap;
  lay::angle_constraint_type m_angle_constraint;
  std::string m_category;
  position_type m_main_position;
  alignment_type m_main_xalign, m_main_yalign;
  alignment_type m_xlabel_xalign, m_xlabel_yalign;
  alignment_type m_ylabel_xalign, m_ylabel_yalign;

  std::string formatted (const std::string &fmt, const db::DFTrans &trans, size_t index) const;
};

}

#endif

