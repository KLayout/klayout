
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


#include "dbLayout.h"

#include "dbShape.h"
#include "tlString.h"
#include "dbTextWriter.h"
#include "dbText.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>

namespace db
{

// ------------------------------------------------------------------
//  TextWriter implementation

TextWriter::TextWriter (tl::OutputStream &stream)
  : m_stream (stream), m_in_cell (false)
{
  //  .. nothing yet ..
}

void
TextWriter::begin_sorted_section ()
{
  m_cc.clear ();
  m_in_cell = true;
  m_cc_line.clear ();
}

void
TextWriter::end_sorted_section ()
{
  std::sort (m_cc.begin (), m_cc.end ());
  for (std::vector<std::string>::const_iterator l = m_cc.begin (); l != m_cc.end (); ++l) {
    m_stream.put (l->c_str (), l->size ());
  }
  m_cc.clear ();
  m_in_cell = false;
}

TextWriter &
TextWriter::operator<< (const std::string &s)
{
  if (m_in_cell) {
    m_cc_line += s;
  } else {
    m_stream.put (s.c_str (), s.size ());
  }

  return *this;
}

TextWriter &
TextWriter::operator<< (endl_t)
{
  *this << endl_str ();
  m_cc.push_back (m_cc_line);
  m_cc_line.clear ();
  return *this;
}

TextWriter &
TextWriter::operator<< (const char *s)
{
  *this << std::string (s);
  return *this;
}

TextWriter &
TextWriter::operator<< (int64_t n)
{
  *this << tl::sprintf ("%d", n);
  return *this;
}

TextWriter &
TextWriter::operator<< (int32_t n)
{
  *this << tl::sprintf ("%d", n);
  return *this;
}

TextWriter &
TextWriter::operator<< (double d)
{
  *this << tl::sprintf ("%.12g", d);
  return *this;
}

TextWriter &
TextWriter::operator<< (db::Point p)
{
  *this << tl::sprintf ("{%d %d}", p.x (), p.y ());
  return *this;
}

TextWriter &
TextWriter::operator<< (db::Vector p)
{
  *this << tl::sprintf ("{%d %d}", p.x (), p.y ());
  return *this;
}

TextWriter::endl_t
TextWriter::endl ()
{
  return endl_t ();
}

const char *
TextWriter::endl_str ()
{
  return "\n";
}

void
TextWriter::write_props (const db::Layout &layout, size_t prop_id)
{
  *this << "set props {" << endl_str ();

  const db::PropertiesRepository::properties_set &props = layout.properties_repository ().properties (prop_id);
  for (db::PropertiesRepository::properties_set::const_iterator p = props.begin (); p != props.end (); ++p) {

    const tl::Variant &name = layout.properties_repository ().prop_name (p->first);

    if (name.is_long () || name.is_ulong ()) {
      *this << "  {" << int (name.to_long ()) << " {" << p->second.to_string () << "}}" << endl_str ();
    } else if (name.is_a_string ()) {
      *this << "  {{" << name.to_string () << "} {" << p->second.to_string () << "}}" << endl_str ();
    }

  }

  *this << "}" << endl_str ();
}

void
TextWriter::write (const db::Layout &layout)
{
  //  write header
  
  std::string pfx = "";

  if (layout.prop_id () != 0) {
    pfx = "p $props";
    write_props (layout, layout.prop_id ());
  }

  *this << "begin_lib" << pfx << " " << layout.dbu () << endl ();

  //  body

  for (db::Layout::bottom_up_const_iterator cell = layout.begin_bottom_up (); cell != layout.end_bottom_up (); ++cell) {

    const db::Cell &cref (layout.cell (*cell));

    //  cell header 

    std::string pfx = "";

    if (cref.prop_id () != 0) {
      pfx = "p $props";
      write_props (layout, cref.prop_id ());
    }

    *this << "begin_cell" << pfx << " {" << layout.cell_name (*cell) << "}" << endl ();

    //  cell body 

    begin_sorted_section ();

    //  instances
    
    for (db::Cell::const_iterator inst = cref.begin (); ! inst.at_end (); ++inst) {

      db::Vector a, b;
      unsigned long amax, bmax;

      bool is_reg = inst->is_regular_array (a, b, amax, bmax);

      for (db::CellInstArray::iterator i = inst->begin (); ! i.at_end (); ++i) {

        std::string pfx = "";

        if (inst->has_prop_id () && inst->prop_id () != 0) {
          pfx = "p $props";
          write_props (layout, inst->prop_id ());
        }

        *this << (is_reg ? "aref" : "sref") << pfx << " {" << layout.cell_name (inst->cell_index ()) << "}";

        db::Trans t = *i;

        if (inst->is_complex ()) {
          db::CellInstArray::complex_trans_type ct = inst->complex_trans (t);
          *this << " " << ct.angle ();
          *this << " " << (ct.is_mirror () ? 1 : 0);
          *this << " " << ct.mag ();
        } else {
          *this << " " << (t.rot () % 4) * 90.0;
          *this << " " << (t.is_mirror () ? 1 : 0);
          *this << " " << 1.0;
        }

        if (is_reg) {
          *this << " " << int (std::max ((unsigned long) 1, amax));
          *this << " " << int (std::max ((unsigned long) 1, bmax));
        }
        *this << " " << t.disp ();
        if (is_reg) {
          *this << " " << (t.disp () + a * (long) amax);
          *this << " " << (t.disp () + b * (long) bmax);
        }
        *this << endl ();

        if (is_reg) {
          break;
        }

      }

    }

    end_sorted_section ();

    //  shapes

    for (unsigned int l = 0; l < layout.layers (); ++l) {
 
      if (layout.is_valid_layer (l)) {

        begin_sorted_section ();

        const LayerProperties &prop = layout.get_properties (l);
        int layer = prop.layer;
        int datatype = prop.datatype;

        db::ShapeIterator shape (cref.shapes (l).begin (db::ShapeIterator::Boxes | db::ShapeIterator::Polygons | db::ShapeIterator::Edges | db::ShapeIterator::Paths | db::ShapeIterator::Texts));
        while (! shape.at_end ()) {

          std::string pfx = "";

          if (shape->has_prop_id () && shape->prop_id () != 0) {
            pfx = "p $props";
            write_props (layout, shape->prop_id ());
          }

          if (shape->is_text ()) {
            
            db::Trans trans = shape->text_trans ();

            *this << "text" << pfx 
                  << " " << layer << " " << datatype 
                  << " " << ((trans.rot () % 4) * 90.0) 
                  << " " << (trans.is_mirror () ? 1 : 0) 
                  << " " << trans.disp () 
                  << " {" << shape->text_string () << "}" << endl ();

          } else if (shape->is_polygon ()) {

            *this << "boundary" << pfx 
                  << " " << layer << " " << datatype;

            db::Shape::point_iterator e (shape->begin_hull ());
            for ( ; e != shape->end_hull (); ++e) {
              *this << " " << *e;
            }

            //  and closing point
            e = shape->begin_hull ();
            if (e != shape->end_hull ()) {
              *this << " " << *e;
            }

            *this << endl ();

          } else if (shape->is_edge ()) {

            db::Edge e (shape->edge ());
            *this << "edge" << pfx 
                  << " " << layer << " " << datatype << " " << e.p1 () << " " << e.p2 () << endl ();

          } else if (shape->is_path ()) {

            //  instantiate the path and draw
            db::Path path;
            shape->path (path);

            *this << "path" << pfx 
                  << " " << layer << " " << datatype 
                  << " " << path.width ()
                  << " " << path.extensions ().first
                  << " " << path.extensions ().second;
            for (db::Path::iterator p = path.begin (); p != path.end (); ++p) {
              *this << " " << *p;
            }
            *this << endl ();

          } else if (shape->is_box ()) {

            db::Box b (shape->box ());
            *this << "box" << pfx 
                  << " " << layer << " " << datatype << " " << b.p1 () << " " << b.p2 () << endl ();

          }

          ++shape;

        }

        end_sorted_section ();

      }

    }

    //  end of cell
    
    *this << "end_cell" << endl ();

  }

  *this << "end_lib" << endl ();

  m_stream.flush ();
}


} // namespace db

