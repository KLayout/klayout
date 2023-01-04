
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


#include "rdb.h"
#include "rdbReader.h"
#include "tlTimer.h"
#include "tlString.h"
#include "tlClassRegistry.h"
#include "tlProgress.h"
#include "dbTrans.h"
#include "dbPolygon.h"
#include "dbEdge.h"
#include "dbEdgePair.h"

#include <cctype>
#include <cstring>

namespace rdb
{

class RVEReaderException
  : public ReaderException 
{
public:
  RVEReaderException (const std::string &msg, size_t line)
    : ReaderException (tl::sprintf (tl::to_string (tr ("%s (line=%lu)")), msg, line))
  { }
};

class RVEReader 
  : public ReaderBase
{
public:
  RVEReader (tl::InputStream &stream)
    : m_input_stream (stream),
      m_progress (tl::to_string (tr ("Reading RVE DB")), 10000)
  {
    m_progress.set_format (tl::to_string (tr ("%.0f MB")));
    m_progress.set_unit (1024 * 1024);
  }

  virtual void read (Database &db) 
  {
    try {
      do_read (db);
    } catch (tl::Exception &ex) {
      error (ex.msg ()); 
    }
  }

  void do_read (Database &db)
  {
    tl::SelfTimer timer (tl::verbosity () >= 11, "Reading RVE file");

    tl::Extractor ex;
    std::string s;
    double res;
    double dbu;

    std::string cell_name;
    db::DCplxTrans trans;
    db::DCplxTrans shape_trans;
    std::vector <db::DPoint> points;
    std::vector <db::DEdge> edges;

    ex = tl::Extractor (get_line ().c_str ());
    ex.read (s, " ");
    ex.read (res);

    if (res < 0.001 || res > 1e6) {
      error (tl::sprintf (tl::to_string (tr ("Invalid resolution value: %g")), res));
    }

    dbu = 1.0 / res;

    Cell *top_cell = db.create_cell (s);
    db.set_top_cell_name (s);

    std::string cat_name;
    id_type waived_tag_id = db.tags ().tag ("waived").id ();

    while (! at_end ()) {

      //  TODO: check if this is correct: when a new category is started the 
      //  cell name is reset. Any shape not having a specific cell will go into the
      //  top cell then.
      cell_name.clear (); 

      //  Read the category name unless we have some already (that we got when parsing the shapes).
      if (cat_name.empty ()) {
        ex = tl::Extractor (get_line ().c_str ());
        const char *start = ex.skip ();
        if (! *start) {
          break;
        } else {
          const char *end = start + strlen (start);
          while (end > start && isspace (end [-1])) {
            --end;
          }
          if (end > start && end[-1] == '.') {
            --end;
          }
          cat_name = std::string (start, end - start);
        }
      }

      Category *cath = db.create_category (cat_name);
      cat_name.clear ();

      if (at_end ()) {
        error (tl::to_string (tr ("Unexpected end of file")));
      }

      ex = tl::Extractor (get_line ().c_str ());
      size_t n1, n2, n3;
      ex.read (n1);
      ex.read (n2);
      ex.read (n3);

      std::map<size_t, std::string> waivers;

      std::string desc;
      for (size_t i = 0; i < n3; ++i) {

        if (at_end ()) {
          error (tl::to_string (tr ("Unexpected end of file")));
        }

        std::string l = m_input_stream.get_line ();
        if (l.size () > 3 && l[0] == 'W' && l[1] == 'E' && isdigit (l[2])) {

          size_t n = 0;
          const char *cp;
          for (cp = l.c_str () + 2; *cp && isdigit (*cp); ++cp) {
            n = n * 10 + size_t (*cp - '0');
          }
          while (*cp && isspace (*cp)) {
            ++cp;
          }
          waivers.insert (std::make_pair (n, std::string ())).first->second = cp;

        } else {

          if (! desc.empty ()) {
            desc += "\n";
          }
          desc += l;

        }

      }

      cath->set_description (desc);

      for (size_t shape = 0; shape < n1; ++shape) {

        std::map<size_t, std::string>::const_iterator w = waivers.find (shape);
        bool waived = (w != waivers.end ());
        //  TODO: add waiver string somehow ...

        if (at_end ()) {
          warn (tl::to_string (tr ("Unexpected end of file before the specified number of shapes was read - stopping.")));
          break;
        }

        s = get_line ();

        ex = tl::Extractor (s.c_str ());

        char shape_type = tolower (*ex.skip ());
        bool valid = (shape_type == 'p' || shape_type == 'e');

        if (valid) {
          ++ex;
        }

        Values values;

        size_t nshape, npoints;
        valid = valid && ex.try_read (nshape);
        valid = valid && ex.try_read (npoints);

        if (! valid) {
          ex = tl::Extractor (s.c_str ());
          const char *start = ex.skip ();
          if (! *start) {
            warn (tl::to_string (tr ("Unexpected end of file before the specified number of shapes was read - stopping.")));
          } else {
            const char *end = start + strlen (start);
            while (end > start && isspace (end [-1])) {
              --end;
            }
            if (end > start && end[-1] == '.') {
              --end;
            }
            cat_name = std::string (start, end - start);
            warn (tl::to_string (tr ("Obviously reaching end of shapes list before the specified number of shapes was read - parsing next category.")));
          }
          break;
        }

        while (true) {
        
          if (at_end ()) {
            error (tl::to_string (tr ("Unexpected end of file")));
          }

          ex = tl::Extractor (get_line ().c_str ());
          char c = *ex.skip ();

          if (isalpha (c)) {

            std::string prop_name;
            ex.read_word (prop_name);

            if (prop_name == "CN") {

              ex.read_word (cell_name, "_.$-");

              int m11 = 1, m12 = 0, m21 = 0, m22 = 1;
              int64_t x = 0, y = 0;

              bool cspace = (ex.test ("c") || ex.test ("C"));

              if (! ex.at_end ()) {

                ex.read (m11);
                ex.read (m21);
                ex.read (m12);
                ex.read (m22);

                ex.read (x);
                ex.read (y);

              }

              int f = 0;
              if (m11 * m22 - m21 * m12 < 0) {
                f = 4;
                m12 = -m12;
                m22 = -m22;
              }

              if (m11 == 1 && m21 == 0) {
                // r0 or m0
              } else if (m11 == 0 && m21 == 1) {
                // r90 or m45
                f += 1;
              } else if (m11 == -1 && m21 == 0) {
                // r180 or m90
                f += 2;
              } else if (m11 == 0 && m21 == -1) {
                // r270 or m135
                f += 3;
              }

              if (cspace) {
                shape_trans = db::DCplxTrans ();
                trans = db::DCplxTrans (db::DTrans (f, db::DVector (x * dbu, y * dbu)));
              } else {
                shape_trans = db::DCplxTrans (db::DTrans (f, db::DVector (x * dbu, y * dbu)));
                trans = shape_trans.inverted ();
              }

            } else {

              double v = 0.0;
              if (ex.try_read (v)) {
                
                //  custom properties get a tag name with a hash
                id_type tag_id = db.tags ().tag (prop_name, true).id ();

                Value<double> *value = new Value<double> (v);
                values.add (value, tag_id);

              } else {

                //  TODO: other property formats?

              }
            
            }

          } else {
            break;
          }

        }
        
        const Cell *cell;

        //  Use the last cell or the top cell if no cell is specified since the start of the category.
        if (cell_name.empty ()) {

          cell = top_cell;

        } else {

          cell = db.cell_by_qname (cell_name);
          if (! cell) {

            Cell *nc_cell = db.create_cell (cell_name);

            Reference ref (trans, top_cell->id ());
            nc_cell->references ().insert (ref);

            cell = nc_cell;

          }

        }

        m_progress.set (m_input_stream.raw_stream ().pos ());

        if (shape_type == 'p') {

          points.clear ();

          for (size_t point = 0; point < npoints; ++point) {

            if (point > 0) {

              if (at_end ()) {
                error (tl::to_string (tr ("Unexpected end of file")));
              }

              ex = tl::Extractor (get_line ().c_str ());

            }

            int64_t x, y;
            ex.read (x);
            ex.read (y);
            ex.expect_end ();
            points.push_back (db::DPoint (x * dbu, y * dbu));

          }

          Value<db::DPolygon> *value = new Value<db::DPolygon> (db::DPolygon ());
          value->value ().assign_hull (points.begin (), points.end (), shape_trans);
          values.add (value);

        } else if (shape_type == 'e') {

          edges.clear ();

          for (size_t point = 0; point < npoints; ++point) {

            if (point > 0) {

              if (at_end ()) {
                error (tl::to_string (tr ("Unexpected end of file")));
              }

              ex = tl::Extractor (get_line ().c_str ());

            }

            int64_t x1, y1;
            ex.read (x1);
            ex.read (y1);

            int64_t x2, y2;
            ex.read (x2);
            ex.read (y2);

            ex.expect_end ();

            edges.push_back (db::DEdge (db::DPoint (x1 * dbu, y1 * dbu), db::DPoint (x2 * dbu, y2 * dbu)).transformed (shape_trans));

          }

          if (edges.size () == 2) {
            //  produce an edge pair from two edges
            values.add (new Value<db::DEdgePair> (db::DEdgePair (edges [0], edges [1])));
          } else {
            for (std::vector <db::DEdge>::const_iterator e = edges.begin (); e != edges.end (); ++e) {
              values.add (new Value<db::DEdge> (*e));
            }
          }

        } else {
          error (tl::sprintf (tl::to_string (tr ("Invalid shape type: %c")), shape_type));
        }

        Item *item = db.create_item (cell->id (), cath->id ());
        if (waived) {
          db.add_item_tag (item, waived_tag_id);
        }

        item->values ().swap (values);

      }

    }

  }

  virtual const char *format () const 
  {
    return "RVE";
  }

private:
  tl::TextInputStream m_input_stream;
  tl::AbsoluteProgress m_progress;
  std::string m_line;

  bool at_end ()
  {
    return m_input_stream.at_end ();
  }

  const std::string &get_line ()
  {
    m_line.clear ();
    while (! m_input_stream.at_end ()) {
      m_line = m_input_stream.get_line ();
      //  skip lines starting with "//" (#522)
      if (m_line.size () < 2 || m_line[0] != '/' || m_line[1] != '/') {
        break;
      }
      m_line.clear ();
    }
    return m_line;
  }

  void warn (const std::string &msg)
  { 
    tl::warn << tl::sprintf (tl::to_string (tr ("%s (line=%lu)")), msg, m_input_stream.line_number ());
  }

  void error (const std::string &msg)
  {
    throw RVEReaderException (msg, m_input_stream.line_number ());
  }
};

class RVEFormatDeclaration 
  : public FormatDeclaration
{
  virtual std::string format_name () const { return "RVE"; }
  virtual std::string format_desc () const { return "RVE format"; }
  virtual std::string file_format () const { return "RVE files (*.rve *.rve.gz *.db *.db.gz)"; }

  virtual bool detect (tl::InputStream &stream) const
  {
    tl::TextInputStream text_stream (stream);

    std::string l;
    tl::Extractor ex;
    std::string s;
    double d;
    int i;

    //  The first line must be "<cellname> <resolution>"
    if (text_stream.at_end ()) {
      return false;
    }

    l = text_stream.get_line ();
    ex = tl::Extractor (l.c_str ());
    ex.read (s, " ");

    if (! ex.try_read (d)) {
      return false;
    }

    //  If we are at the end, this is probably an empty file. No checks and no results.
    if (text_stream.at_end ()) {
      return true;
    }

    //  The second line is skipped
    l = text_stream.get_line ();

    ex = tl::Extractor (l.c_str ());

    //  If we are at the end, this is probably an empty file. No checks and no results.
    if (text_stream.at_end ()) {
      return ex.at_end ();
    }

    //  The third line must be "<n> <n> <n> <date...>"
    l = text_stream.get_line ();

    ex = tl::Extractor (l.c_str ());
    if (! ex.try_read (i)) {
      return false;
    }

    if (! ex.try_read (i)) {
      return false;
    }

    if (! ex.try_read (i)) {
      return false;
    }
    
    //  That's it.
    return true;
  }

  virtual ReaderBase *create_reader (tl::InputStream &s) const 
  {
    return new RVEReader (s);
  }
};

static tl::RegisteredClass<rdb::FormatDeclaration> format_decl (new RVEFormatDeclaration (), 0, "RVE");

}

