
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


#include "antTemplate.h"
#include "antConfig.h"
#include "tlInternational.h"
#include "tlException.h"
#include "tlLog.h"

namespace ant
{

int
Template::current_version ()
{
  return 1;
}

ant::Template
Template::from_object (const ant::Object &a, const std::string &title, int mode)
{
  ant::Template t;

  t.angle_constraint (a.angle_constraint ());
  t.category (a.category ());
  t.fmt (a.fmt ());
  t.fmt_x (a.fmt_x ());
  t.fmt_y (a.fmt_y ());
  t.set_main_position (a.main_position ());
  t.set_main_xalign (a.main_xalign ());
  t.set_main_yalign (a.main_yalign ());
  t.set_xlabel_xalign (a.xlabel_xalign ());
  t.set_xlabel_yalign (a.xlabel_yalign ());
  t.set_ylabel_xalign (a.ylabel_xalign ());
  t.set_ylabel_yalign (a.ylabel_yalign ());
  t.outline (a.outline ());
  t.style (a.style ());
  t.title (title);

  t.set_mode (ant::Template::ruler_mode_type (mode));

  return t;
}

Template::Template ()
  : m_version (current_version ()),
    m_title (tl::to_string (tr ("Ruler"))),
    m_fmt_x ("$X"), m_fmt_y ("$Y"), m_fmt ("$D"),
    m_style (ant::Object::STY_ruler), m_outline (ant::Object::OL_diag),
    m_snap (true), m_angle_constraint (lay::AC_Global),
    m_main_position (ant::Object::POS_auto),
    m_main_xalign (ant::Object::AL_auto), m_main_yalign (ant::Object::AL_auto),
    m_xlabel_xalign (ant::Object::AL_auto), m_xlabel_yalign (ant::Object::AL_auto),
    m_ylabel_xalign (ant::Object::AL_auto), m_ylabel_yalign (ant::Object::AL_auto),
    m_mode (ant::Template::RulerNormal)
{
  //  .. nothing yet ..
}

Template::Template (const std::string &title,
                    const std::string &fmt_x, const std::string &fmt_y, const std::string &fmt, 
                    style_type style, outline_type outline, bool snap, lay::angle_constraint_type angle_constraint,
                    const std::string &cat)
  : m_version (current_version ()),
    m_title (title),
    m_category (cat),
    m_fmt_x (fmt_x), m_fmt_y (fmt_y), m_fmt (fmt),
    m_style (style), m_outline (outline),
    m_snap (snap), m_angle_constraint (angle_constraint),
    m_main_position (ant::Object::POS_auto),
    m_main_xalign (ant::Object::AL_auto), m_main_yalign (ant::Object::AL_auto),
    m_xlabel_xalign (ant::Object::AL_auto), m_xlabel_yalign (ant::Object::AL_auto),
    m_ylabel_xalign (ant::Object::AL_auto), m_ylabel_yalign (ant::Object::AL_auto),
    m_mode (ant::Template::RulerNormal)
{
  //  .. nothing else ..
}

Template::Template (const ant::Template &d)
  : m_version (d.m_version),
    m_title (d.m_title),
    m_category (d.m_category),
    m_fmt_x (d.m_fmt_x), m_fmt_y (d.m_fmt_y), m_fmt (d.m_fmt),
    m_style (d.m_style), m_outline (d.m_outline),
    m_snap (d.m_snap), m_angle_constraint (d.m_angle_constraint),
    m_main_position (d.m_main_position),
    m_main_xalign (d.m_main_xalign), m_main_yalign (d.m_main_yalign),
    m_xlabel_xalign (d.m_xlabel_xalign), m_xlabel_yalign (d.m_xlabel_yalign),
    m_ylabel_xalign (d.m_ylabel_xalign), m_ylabel_yalign (d.m_ylabel_yalign),
    m_mode (d.m_mode)
{
  //  .. nothing else ..
}

Template &
Template::operator= (const ant::Template &d)
{
  if (this != &d) {
    m_version = d.m_version;
    m_title = d.m_title;
    m_category = d.m_category;
    m_fmt_x = d.m_fmt_x;
    m_fmt_y = d.m_fmt_y;
    m_fmt = d.m_fmt;
    m_style = d.m_style;
    m_outline = d.m_outline;
    m_snap = d.m_snap;
    m_angle_constraint = d.m_angle_constraint;
    m_main_position = d.m_main_position;
    m_main_xalign = d.m_main_xalign;
    m_main_yalign = d.m_main_yalign;
    m_xlabel_xalign = d.m_xlabel_xalign;
    m_xlabel_yalign = d.m_xlabel_yalign;
    m_ylabel_xalign = d.m_ylabel_xalign;
    m_ylabel_yalign = d.m_ylabel_yalign;
    m_mode = d.m_mode;
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
      r.back ().version (0);

      while (! ex.at_end ()) {

        std::string key, s;
        ex.read_word_or_quoted (key);
        ex.expect ("=");
        ex.read_word_or_quoted (s);

        if (key == "version") {

          int v = 0;
          tl::from_string (s, v);
          r.back ().version (v);

        } else if (key == "mode") {

          ant::RulerModeConverter mc;
          ant::Template::ruler_mode_type mode;
          mc.from_string (s, mode);
          r.back ().set_mode (mode);

        } else if (key == "title") {
          
          r.back ().title (s);
          
        } else if (key == "category") {

          r.back ().category (s);

        } else if (key == "fmt") {

          r.back ().fmt (s);

        } else if (key == "fmt_x") {

          r.back ().fmt_x (s);

        } else if (key == "fmt_y") {

          r.back ().fmt_y (s);

        } else if (key == "position") {

          ant::PositionConverter pc;
          ant::Object::position_type pos;
          pc.from_string (s, pos);
          r.back ().set_main_position (pos);

        } else if (key == "xalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_main_xalign (a);

        } else if (key == "yalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_main_yalign (a);

        } else if (key == "xlabel_xalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_xlabel_xalign (a);

        } else if (key == "xlabel_yalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_xlabel_yalign (a);

        } else if (key == "ylabel_xalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_ylabel_xalign (a);

        } else if (key == "ylabel_yalign") {

          ant::AlignmentConverter ac;
          ant::Object::alignment_type a;
          ac.from_string (s, a);
          r.back ().set_ylabel_yalign (a);

        } else if (key == "style") {

          ant::StyleConverter sc;
          ant::Object::style_type st;
          sc.from_string (s, st);
          r.back ().style (st);

        } else if (key == "outline") {

          ant::OutlineConverter oc;
          ant::Object::outline_type ot;
          oc.from_string (s, ot);
          r.back ().outline (ot);

        } else if (key == "snap") {

          bool f = false;
          tl::from_string (s, f);
          r.back ().snap (f);

        } else if (key == "angle_constraint") {

          ant::ACConverter sc;
          lay::angle_constraint_type sm;
          sc.from_string (s, sm);
          r.back ().angle_constraint (sm);

        }

        ex.test (",");
        
        if (ex.test (";")) {

          r.push_back (Template ());
          r.back ().version (0);

        }

      }

      //  downgrade version
      if (r.back ().version () > current_version ()) {
        r.back ().version (current_version ());
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

    r += "mode=";
    ant::RulerModeConverter mc;
    r += mc.to_string (t->mode ());
    r += ",";

    r += "title=";
    r += tl::to_word_or_quoted_string (t->title ());
    r += ",";
    r += "category=";
    r += tl::to_word_or_quoted_string (t->category ());
    r += ",";
    r += "version=";
    r += tl::to_string (t->version ());
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
    
    r += "position=";
    ant::PositionConverter pc;
    r += pc.to_string (t->main_position ());
    r += ",";

    ant::AlignmentConverter ac;
    r += "xalign=";
    r += ac.to_string (t->main_xalign ());
    r += ",";
    r += "yalign=";
    r += ac.to_string (t->main_yalign ());
    r += ",";
    r += "xlabel_xalign=";
    r += ac.to_string (t->xlabel_xalign ());
    r += ",";
    r += "xlabel_yalign=";
    r += ac.to_string (t->xlabel_yalign ());
    r += ",";
    r += "ylabel_xalign=";
    r += ac.to_string (t->ylabel_xalign ());
    r += ",";
    r += "ylabel_yalign=";
    r += ac.to_string (t->ylabel_yalign ());
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
    ant::ACConverter acc;
    r += acc.to_string (t->angle_constraint ());

  }
 
  return r;
}


} // namespace ant

