
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



#ifndef HDR_antTemplate
#define HDR_antTemplate

#include "antCommon.h"

#include "antObject.h"
#include "laySnap.h"

#include <string>
#include <vector>

namespace ant {

/**
 *  @brief The template that is used for creating new rulers from
 */
class ANT_PUBLIC Template
{
public:
  typedef ant::Object::style_type style_type;
  typedef ant::Object::outline_type outline_type;
  typedef lay::angle_constraint_type angle_constraint_type;

  /**
   *  @brief Default constructor
   * 
   *  Creates a template with the default settings
   */
  Template ();

  /**
   *  @brief Constructor
   * 
   *  Creates a template with the given format strings and styles
   */
  Template (const std::string &title, const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraints);

  /** 
   *  @brief Copy constructor
   */
  Template (const ant::Template &d);

  /**
   *  @brief Assignment
   */
  Template &operator= (const ant::Template &d);

  /**
   *  @brief Title read accessor
   */
  const std::string &title () const
  {
    return m_title;
  }
  
  /**
   *  @brief Title write accessor
   */
  void title (const std::string &t) 
  {
    m_title = t;
  }
  
  /**
   *  @brief Main format string read accessor
   */
  const std::string &fmt () const
  {
    return m_fmt;
  }

  /**
   *  @brief Main format string write accessor
   *
   *  Every ruler or marker has a main label usually somewhere at the end point.
   *  This label string is derived from this format.
   */
  void fmt (const std::string &s)
  {
    m_fmt = s;
  }

  /**
   *  @brief x axis format string read accessor
   */
  const std::string &fmt_x () const
  {
    return m_fmt_x;
  }

  /**
   *  @brief x axis format string write accessor
   *
   *  If the ruler has a horizontal component (that is in a non-diagonal outline mode),
   *  this component is labelled with a string formatted with this format.
   */
  void fmt_x (const std::string &s)
  {
    m_fmt_x = s;
  }

  /**
   *  @brief y axis format string read accessor
   */
  const std::string &fmt_y () const
  {
    return m_fmt_y;
  }

  /**
   *  @brief y axis format string write accessor
   *
   *  If the ruler has a vertical component (that is in a non-diagonal outline mode),
   *  this component is labelled with a string formatted with this format.
   */
  void fmt_y (const std::string &s)
  {
    m_fmt_y = s;
  }

  /**
   *  @brief Style read accessor
   */ 
  style_type style () const
  {
    return m_style;
  }

  /**
   *  @brief Outline mode write accessor
   *
   *  The outline mode controls how the ruler or marker is drawn. 
   *  The style is either "ruler" (with tick marks), "arrow" in different 
   *  flavours or "plain line".
   */ 
  void style (style_type s) 
  {
    m_style = s;
  }

  /**
   *  @brief Outline mode read accessor
   */ 
  outline_type outline () const
  {
    return m_outline;
  }

  /**
   *  @brief Outline mode write accessor
   *
   *  The outline mode controls how the ruler or marker appears. 
   *  As a ruler it may appear as a diagonal connection between two points, 
   *  as a set of horizonal and vertical lines or as a set of horizontal, vertical 
   *  and diagonal lines. As a marker it may appear as a box.
   */ 
  void outline (outline_type s) 
  {
    m_outline = s;
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
    m_snap = s;
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
   *  for the rulers derived from this template or if the global setting is to be used
   *  (if ant::Service::Global is used for the angle constraint).
   */
  void angle_constraint (lay::angle_constraint_type a)
  {
    m_angle_constraint = a;
  }
  
  /**
   *  @brief Get a list of templates from a string
   */
  static std::vector<Template> from_string (const std::string &s);

  /**
   *  @brief Convert a list of templates to a string
   */
  static std::string to_string (const std::vector<Template> &v);

private:
  std::string m_title;
  std::string m_fmt_x;
  std::string m_fmt_y;
  std::string m_fmt;
  style_type m_style;
  outline_type m_outline;
  bool m_snap;
  lay::angle_constraint_type m_angle_constraint;
};

}

#endif

