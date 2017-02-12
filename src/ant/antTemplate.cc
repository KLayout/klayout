
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


#include "antTemplate.h"
#include "antConfig.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlLog.h"

namespace ant
{

Template::Template ()
  : m_title (tl::to_string (QObject::tr ("Ruler"))),
    m_fmt_x ("$X"), m_fmt_y ("$Y"), m_fmt ("$D"),
    m_style (ant::Object::STY_ruler), m_outline (ant::Object::OL_diag),
    m_snap (true), m_angle_constraint (lay::AC_Global)
{
  //  .. nothing yet ..
}

Template::Template (const std::string &title,
                    const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, 
                    style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint)
  : m_title (title),
    m_fmt_x (fmt_x), m_fmt_y (fmt_y), m_fmt (fmt),
    m_style (style), m_outline (outline),
    m_snap (snap), m_angle_constraint (angle_constraint)
{
  //  .. nothing else ..
}

Template::Template (const ant::Template &d)
  : m_title (d.m_title),
    m_fmt_x (d.m_fmt_x), m_fmt_y (d.m_fmt_y), m_fmt (d.m_fmt),
    m_style (d.m_style), m_outline (d.m_outline),
    m_snap (d.m_snap), m_angle_constraint (d.m_angle_constraint)
{
  //  .. nothing else ..
}

Template &
Template::operator= (const ant::Template &d)
{
  if (this != &d) {
    m_title = d.m_title;
    m_fmt_x = d.m_fmt_x;
    m_fmt_y = d.m_fmt_y;
    m_fmt = d.m_fmt;
    m_style = d.m_style;
    m_outline = d.m_outline;
    m_snap = d.m_snap;
    m_angle_constraint = d.m_angle_constraint;
  }
  return *this;
}

std::vector<Template> 
Template::from_string (const std::string &s)
{
  std::vector<Template> r;
  
  try {
    
    tl::Extractor ex (s.c_str ());

    if (! ex.at_end ()) {

      r.push_back (Template ());

      while (! ex.at_end ()) {

        if (ex.test ("title=")) {
          
          std::string s;
          ex.read_word_or_quoted (s);
          r.back ().title (s);
          ex.test (",");
          
        } else if (ex.test ("fmt=")) {

          std::string s;
          ex.read_word_or_quoted (s);
          r.back ().fmt (s);
          ex.test (",");

        } else if (ex.test ("fmt_x=")) {

          std::string s;
          ex.read_word_or_quoted (s);
          r.back ().fmt_x (s);
          ex.test (",");

        } else if (ex.test ("fmt_y=")) {

          std::string s;
          ex.read_word_or_quoted (s);
          r.back ().fmt_y (s);
          ex.test (",");

        } else if (ex.test ("style=")) {

          std::string s;
          ex.read_word (s);
          ant::StyleConverter sc;
          ant::Object::style_type st;
          sc.from_string (s, st);
          r.back ().style (st);
          ex.test (",");

        } else if (ex.test ("outline=")) {

          std::string s;
          ex.read_word (s);
          ant::OutlineConverter oc;
          ant::Object::outline_type ot;
          oc.from_string (s, ot);
          r.back ().outline (ot);
          ex.test (",");

        } else if (ex.test ("snap=")) {

          bool f = false;
          ex.read (f);
          r.back ().snap (f);
          ex.test (",");

        } else if (ex.test ("angle_constraint=")) {

          std::string s;
          ex.read_word (s);
          ant::ACConverter sc;
          lay::angle_constraint_type sm;
          sc.from_string (s, sm);
          r.back ().angle_constraint (sm);
          ex.test (",");

        } else {

          ex.expect (";");
          r.push_back (Template ());

        }

      }

    }
    
  } catch (tl::Exception &ex) {
    tl::error << ex.msg ();
    r.clear ();
  } catch (...) {
    r.clear ();
  }

  return r;
}

std::string
Template::to_string (const std::vector<Template> &v)
{
  std::string r;
  
  for (std::vector<Template>::const_iterator t = v.begin (); t != v.end (); ++t) {
    
    if (! r.empty ()) {
      r += ";";
    }

    r += "title=";
    r += tl::to_word_or_quoted_string (t->title ());
    r += ",";
    r += "fmt=";
    r += tl::to_word_or_quoted_string (t->fmt ());
    r += ",";
    r += "fmt_x=";
    r += tl::to_word_or_quoted_string (t->fmt_x ());
    r += ",";
    r += "fmt_y=";
    r += tl::to_word_or_quoted_string (t->fmt_y ());
    r += ",";
    
    r += "style=";
    ant::StyleConverter sc;
    r += sc.to_string (t->style ());
    r += ",";
    
    r += "outline=";
    ant::OutlineConverter oc;
    r += oc.to_string (t->outline ());
    r += ",";
    
    r += "snap=";
    r += tl::to_string (t->snap ());
    r += ",";

    r += "angle_constraint=";
    ant::ACConverter ac;
    r += ac.to_string (t->angle_constraint ());

  }
 
  return r;
}


} // namespace ant

