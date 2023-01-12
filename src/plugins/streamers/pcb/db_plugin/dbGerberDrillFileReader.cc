
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


#include "dbGerberDrillFileReader.h"

namespace db
{

// ---------------------------------------------------------------------------------
//  GerberDrillFileReader implementation

GerberDrillFileReader::GerberDrillFileReader (int warn_level)
  : GerberFileReader (warn_level)
{
  init ();
}

GerberDrillFileReader::~GerberDrillFileReader ()
{
  // .. nothing yet ..
}

bool 
GerberDrillFileReader::does_accept ()
{
  for (int i = 0; i < 100; ++i) {
    tl::Extractor ex (get_block ().c_str ());
    if (ex.test ("M71")) {
      return true;
    } else if (ex.test ("M48")) {
      return true;
    } else if (ex.test (";")) {
      if (ex.test ("Holesize")) {
        return true;
      } else if (ex.test ("T")) {
        return true;
      }
    }
  }

  return false;
}

void
GerberDrillFileReader::init ()
{
  //  Initialize reader:
  m_relative = false;
  m_x = 0.0;
  m_y = 0.0;
  m_xoff = 0.0;
  m_yoff = 0.0;
  m_current_diameter = 0.0;
  m_current_qty = 0;
  m_qty.clear ();
  m_current_tool = -1;
  m_tools.clear ();
  m_recording = false;
  m_record_pattern = false;
  m_holes.clear ();
  m_pattern.clear ();
  m_in_header = false;
  m_end_block = 0;
  m_m02_xoffset = m_m02_yoffset = 0.0;
  m_routing = false;
  m_plunged = false;
  m_linear_interpolation = false;
  m_format_set = false;
}

GerberMetaData
GerberDrillFileReader::do_scan ()
{
  GerberMetaData data;
  data.function = GerberMetaData::Hole;
  return data;
}

void
GerberDrillFileReader::do_read ()
{
  m_buffer.clear ();
  init ();

  stream ().skip ();

  //  Actually read:
  while (! stream ().at_end ()) {
    try {
      process_line (get_block ());
    } catch (tl::Exception &ex) {
      error (ex.msg ());
    }
  }

}

void 
GerberDrillFileReader::process_line (const std::string &s)
{
  tl::Extractor ex (s.c_str ());

  char c = *ex.skip ();
  double xi = m_x, yi = m_y;

  if (ex.at_end ()) {

    //  ignore empty line
 
  } else if (ex.test ("%")) {

    //  blocks starting with a '%' are ignored and end the header
    m_in_header = false;

  } else if (ex.test (";")) {

    int t = -1;
    double d = 0.0;
    long q = 0;
    double u = 0;

    while (! ex.at_end ()) {

      if (ex.test ("Holesize")) {

        double x = 0.0;
        ex.read (x);
        ex.test ("=");
        ex.read (d);

      } else if (ex.test ("Quantity")) {

        ex.test ("=");
        ex.read (q);

      } else if (ex.test ("Tolerance")) {

        ex.test ("=");
        double x = 0.0;
        ex.test ("+");
        if (ex.try_read (x)) {
          ex.test ("/");
          ex.try_read (x);
        }

      } else if (ex.test ("size")) {

        ex.test (":");
        ex.read (d);

      } else if (ex.test ("T")) {

        int x = 0;
        if (ex.try_read (x)) {
          t = x;
        }

      } else if (ex.test ("MM")) {

        u = 1000.0;

      } else if (ex.test ("INCH")) {

        u = 25400.0;

      } else if (ex.test ("MILS")) {

        u = 25.4;

      } else if (ex.test ("FILE_FORMAT")) {

        ex.test ("=");

        int l = -1, t = -1;
        ex.read (l);
        ex.expect (":");
        ex.read (t);

        set_format (l, t);
        m_format_set = true;

      } else {

        std::string s;
        ex.read (s, "");

      }

    }

    if (u == 0.0) {
      u = unit ();
    }

    if (d > 0) {
      d *= u;
      if (t >= 0) {
        m_tools.insert (std::make_pair (t, d));
      } else if (q > 0) {
        m_qty.push_back (std::make_pair (q, d));
      } else {
        m_current_diameter = d;
      }
    }

  } else if (ex.test ("ICI")) {

    if (ex.test (",ON")) {
      m_relative = true;
    } else if (ex.test (",OFF")) {
      m_relative = false;
    }

  } else if (ex.test ("FMAT")) {

    //  TODO: what to do here?

  } else if (ex.test ("INCH")) {

    set_unit (25400.0);

    if (ex.test (",")) {
      if (ex.test ("LZ")) {
        if (m_format_set) {
          set_format (false);
        } else {
          set_format (2, -1, false); // omit trailing zeroes, fixed format for inch system
        }
      } else if (ex.test ("TZ")) {
        if (m_format_set) {
          set_format (true);
        } else {
          set_format (-1, 4, true); // omit leading zeroes, fixed format for inch system
        }
      }
    }

    ex.expect_end ();

  } else if (ex.test ("METRIC")) {

    set_unit (1000.0);

    if (ex.test (",")) {
      if (ex.test ("LZ")) {
        if (m_format_set) {
          set_format (false);
        } else {
          //  TODO: what number of digits to use?
          set_format (3, -1, false); // omit trailing zeroes
        }
      } else if (ex.test ("TZ")) {
        if (m_format_set) {
          set_format (true);
        } else {
          //  TODO: what number of digits to use?
          set_format (-1, 3, true); // omit leading zeroes
        }
      }
    }

    ex.expect_end ();

  } else if (ex.test ("M")) {

    int mcode = 0;
    ex.read (mcode);

    if (mcode == 48) {
      m_in_header = true;
    } else if (mcode == 95) {
      m_in_header = false;
    } else if (mcode == 14 || mcode == 15) {
      m_plunged = true;
    } else if (mcode == 16 || mcode == 17) {
      m_plunged = false;
    } else if (mcode == 25) {
      start_step_and_repeat ();
    } else if (mcode == 8) {
      stop_step_and_repeat ();
    } else if (mcode == 97 || mcode == 98) {

      warn ("Canned text not supported");

      while (! stream ().at_end ()) {
        std::string l;
        read_line (l);
        tl::Extractor lex (l.c_str ());
        if (lex.test (";")) {
          //  comment -> drop
        } else {
          break;
        }
      }

    } else if (mcode == 2) {

      if (! ex.at_end ()) {

        while (true) {
          if (ex.test ("X")) {
            m_m02_xoffset += read_coord (ex);
          } else if (ex.test ("Y")) {
            m_m02_yoffset += read_coord (ex);
          } else {
            break;
          }
        }

        bool swapxy = false;
        double fx = 1.0;
        double fy = 1.0;

        while (ex.test ("M")) {

          int mcode2 = 0;
          ex.read (mcode2);

          if (mcode2 == 70) {
            swapxy = true;
          } else if (mcode2 == 80) {
            if (swapxy) {
              fy = -fy;
            } else {
              fx = -fx;
            }
          } else if (mcode2 == 90) {
            if (swapxy) {
              fx = -fx;
            } else {
              fy = -fy;
            }
          }
          
        }

        repeat_block (m_m02_xoffset, m_m02_yoffset, fx, fy, swapxy);

      }

    } else if (mcode == 1) {

      end_block ();

      m_m02_xoffset = 0.0;
      m_m02_yoffset = 0.0;

    }

  } else if (ex.test ("T")) {

    int tcode = 0;
    if (ex.try_read (tcode)) {

      m_current_tool = tcode;

      c = *ex.skip ();

      if (c == 'F' || c == 'S' || c == 'C') {

        while (true) {
          double d = 0.0;
          if (ex.test ("F") || ex.test ("S")) {
            ex.read (d);
          } else if (ex.test ("C")) {
            ex.read (d);
            d *= unit ();
            m_tools.insert (std::make_pair (tcode, d));
            m_current_diameter = d;
          } else {
            break;
          }
        }

      } else if (ex.test ("size")) {

        double d = 0.0;

        ex.test (":");
        ex.read (d);
        d *= unit ();

        m_tools.insert (std::make_pair (tcode, d));
        m_current_diameter = d;

      } else {

        //  following specs 

        if (m_tools.find (tcode) == m_tools.end ()) {
          if (tcode == 0) {
            //  some file formats indicate "no tool" with this code ..
            m_current_diameter = 0.0;
          } else {
            throw tl::Exception (tl::to_string (tr ("Undefined tool code %d")), tcode);
          }
        } else {
          m_current_diameter = m_tools [tcode];
        }

      }

    }

  } else if (! m_in_header && ex.test ("P")) {

    int n = 0;
    ex.read (n);

    double dx = 0.0, dy = 0.0;

    if (ex.test ("X")) {
      dx = read_coord (ex);
    }

    if (ex.test ("Y")) {
      dy = read_coord (ex);
    }

    for (int i = 1; i <= n; ++i) {
      repeat_pattern (i * dx, i * dy);
    }

  } else if (! m_in_header && ex.test ("R")) {

    int n = 0;
    ex.read (n);

    double dx = 0.0, dy = 0.0;

    bool use_block = false;
    if (ex.test ("M")) {
      int mcode = 0;
      ex.read (mcode);
      if (mcode == 2) {
        use_block = true;
      }
    } 

    if (ex.test ("X")) {
      dx = read_coord (ex);
    }

    if (ex.test ("Y")) {
      dy = read_coord (ex);
    }

    if (use_block) {

      for (int i = 1; i < n; ++i) {
        repeat_block (i * dx, i * dy);
      }

    } else {

      for (int i = 1; i <= n; ++i) {

        next_hole ();
        produce_circle (m_x + m_xoff + i * dx, m_y + m_yoff + i * dy, m_current_diameter * 0.5);

      }

      m_x += n * dx;
      m_y += n * dy;

    }

  } else if (! m_in_header && (c == 'X' || c == 'Y' || c == 'G')) {

    double xb = 0.0, yb = 0.0;
    bool has_xb = false, has_yb = false;

    while (true) {
      if (ex.test ("X")) {
        xb = read_coord (ex);
        has_xb = true;
      } else if (ex.test ("Y")) {
        yb = read_coord (ex);
        has_yb = true;
      } else {
        break;
      }
    }

    int gcode = 0;

    if (has_xb) {
      if (m_relative) {
        m_x += xb;
      } else {
        m_x = xb;
      }
    }

    if (has_yb) {
      if (m_relative) {
        m_y += yb;
      } else {
        m_y = yb;
      }
    }

    if (ex.test ("G")) {

      ex.read (gcode);

      if (gcode == 90) {
        m_relative = false;
      } else if (gcode == 91) {
        m_relative = true;
      } else if (gcode == 93 || gcode == 92) {

        double xa = 0.0, ya = 0.0;
        bool has_xa = false, has_ya = false;

        while (true) {
          if (ex.test ("X")) {
            xa = read_coord (ex);
            has_xa = true;
          } else if (ex.test ("Y")) {
            ya = read_coord (ex);
            has_ya = true;
          } else {
            break;
          }
        }

        if (has_xa) {
          m_xoff += xa; m_x = 0; 
        }

        if (has_ya) {
          m_yoff += ya; m_y = 0;
        }

      } else if (gcode == 2 || gcode == 3) {

        //  TODO: implement
        warn ("Circular interpolation not supported currently.");

      } else if (gcode == 32 || gcode == 33) {

        //  TODO: implement
        warn ("Routed canned circles not supported currently.");

      } else if (gcode == 0) {

        m_routing = true;
        m_linear_interpolation = false;

        //  process rest of line
        process_line (std::string (ex.skip ()));
        return;

      } else if (gcode == 5) {

        m_routing = false;
        m_linear_interpolation = false;

      } else if (gcode == 1) {

        m_linear_interpolation = true;

        //  process rest of line
        process_line (std::string (ex.skip ()));
        return;

      } else if (gcode == 83 || gcode == 81 || gcode == 82) {

        std::vector<std::pair<double, double> > coords;

        while (! stream ().at_end () && coords.size () < 2) {

          std::string l;
          read_line (l);
          tl::Extractor lex (l.c_str ());
          if (lex.test (";")) {
            //  comment -> drop
          } else {

            coords.push_back (std::make_pair (m_x, m_y));

            while (true) {

              if (lex.test ("X")) {

                double d = read_coord (lex);
                if (m_relative) {
                  coords.back ().first += d;
                } else {
                  coords.back ().first = d;
                }

              } else if (lex.test ("Y")) {

                double d = read_coord (lex);
                if (m_relative) {
                  coords.back ().second += d;
                } else {
                  coords.back ().second = d;
                }

              } else {
                break;
              }

            }

          }

        }

        if (coords.size () == 2) {

          begin_pattern ();

          if (gcode == 83) {

            double xc = (coords[0].first + coords[1].first) * 0.5;
            double yc = (coords[0].second + coords[1].second) * 0.5;
            double xr = xc - coords[0].first;
            double yr = yc - coords[0].second;

            for (int i = 0; i < 8; ++i) {

              double a = i * M_PI / 4;

              next_hole ();
              produce_circle (xc + m_xoff + cos (a) * xr - sin (a) * yr, yc + m_yoff + cos (a) * yr + sin (a) * xr, m_current_diameter * 0.5);

            }

          } else {

            double xa = 0.0, ya = 0.0;
            bool has_xa = false, has_ya = false;

            while (true) {
              if (ex.test ("X")) {
                xa = read_coord (ex);
                has_xa = true;
              } else if (ex.test ("Y")) {
                ya = read_coord (ex);
                has_ya = true;
              } else {
                break;
              }
            }

            if (! has_xa || xa < 1e-6) {
              xa = 0.1 * 25400.0;
            } 
            if (! has_ya || ya < 1e-6) {
              ya = 0.3 * 25400.0;
            }

            int n = 0;
            double dx = 0.0, dy = 0.0;

            if (fabs (fabs (coords[0].first - coords[1].first) - fabs (ya)) < fabs (fabs (coords[0].second - coords[1].second) - fabs (ya))) {
              //  vertical
              dx = 0.0;
              dy = xa * (coords[1].second < coords[0].second ? -1.0 : 1.0);
              n = std::max (0, int (floor (0.5 + (coords[1].second - coords[0].second) / xa)));
            } else {
              //  horizontal
              dy = 0.0;
              dx = xa * (coords[1].first < coords[0].first ? -1.0 : 1.0);
              n = std::max (0, int (floor (0.5 + (coords[1].first - coords[0].first) / xa)));
            }

            for (int i = 0; i <= n; ++i) {

              next_hole ();
              produce_circle (coords [0].first + m_xoff, coords [0].second + m_yoff, m_current_diameter * 0.5);
              coords[0].first += dx;
              coords[0].second += dy;

              next_hole ();
              produce_circle (coords [1].first + m_xoff, coords [1].second + m_yoff, m_current_diameter * 0.5);
              coords[1].first -= dx;
              coords[1].second -= dy;

            }

          }

          end_pattern ();

        }

      } else if (gcode == 85) {

        double xa = 0.0, ya = 0.0;
        bool has_xa = false, has_ya = false;

        while (true) {
          if (ex.test ("X")) {
            xa = read_coord (ex);
            has_xa = true;
          } else if (ex.test ("Y")) {
            ya = read_coord (ex);
            has_ya = true;
          } else {
            break;
          }
        }

        double x0 = m_x, y0 = m_y;

        if (has_xa) {
          if (m_relative) {
            m_x += xa;
          } else {
            m_x = xa;
          }
        }

        if (has_ya) {
          if (m_relative) {
            m_y += ya;
          } else {
            m_y = ya;
          }
        }

        next_hole ();

        //  produce the slot
        produce_circle (x0 + m_xoff, y0 + m_yoff, m_current_diameter * 0.5, m_x + m_xoff, m_y + m_yoff);

      }

    } else if (has_xb || has_yb) {

      next_hole ();

      if (!m_routing) {
        produce_circle (m_x + m_xoff, m_y + m_yoff, m_current_diameter * 0.5);
      } else if (m_plunged) {
        if (m_linear_interpolation) {
          produce_circle (xi + m_xoff, yi + m_yoff, m_current_diameter * 0.5, m_x + m_xoff, m_y + m_yoff);
        }
      }

    }

    if (! ex.at_end ()) {
      warn (tl::sprintf (tl::to_string (tr ("Part of line ignored: %s")), ex.skip ()));
    }

  } else if (!m_in_header && c != 0) {
    warn (tl::to_string (tr ("Statement ignored")));
  }
}

void 
GerberDrillFileReader::next_hole ()
{
  if (m_current_tool < 0 && !m_qty.empty ()) {
    if (m_current_qty == 0) {
      m_current_qty = m_qty.front ().first;
      m_current_diameter = m_qty.front ().second;
      m_qty.pop_front ();
    } 
    if (m_current_qty > 0) {
      --m_current_qty;
    }
  } 
}

void 
GerberDrillFileReader::read_line (std::string &b)
{
  progress_checkpoint ();

  b.clear ();

  char c;
  while (! stream ().at_end ()) {
    c = stream ().get_char ();
    if (c == '\n' || c == '\r') {
      break;
    }
    b += c;
  }

  c = stream ().peek_char ();
  if (c == '\n' || c == '\r') {
    stream ().get_char ();
  }
}

const std::string &
GerberDrillFileReader::get_block ()
{
  read_line (m_buffer);
  return m_buffer;
}

void 
GerberDrillFileReader::begin_pattern ()
{
  m_record_pattern = true;
  m_pattern.clear ();
}

void 
GerberDrillFileReader::end_pattern ()
{
  m_record_pattern = false;
}

void
GerberDrillFileReader::repeat_pattern (double dx, double dy)
{
  if (m_record_pattern) {
    return;
  }

  for (size_t i = 0; i < m_pattern.size (); ++i) {
    produce_circle (m_pattern [i].x + dx, m_pattern [i].y + dy, m_pattern [i].r, m_pattern [i].ex + dx, m_pattern [i].ey + dy);
  }
}

void
GerberDrillFileReader::start_step_and_repeat ()
{
  m_holes.clear ();
  m_recording = true;
  m_end_block = 0;
}

void
GerberDrillFileReader::stop_step_and_repeat ()
{
  m_recording = false;

  for (size_t i = 0; i < m_holes.size (); ++i) {
    produce_circle (m_holes [i].x, m_holes [i].y, m_holes [i].r, m_holes [i].ex, m_holes [i].ey);
  }
}

void
GerberDrillFileReader::repeat_block (double dx, double dy, double fx, double fy, bool swapxy)
{
  for (size_t i = 0; i < m_end_block; ++i) {

    m_holes.push_back (m_holes [i]);

    m_holes.back ().x -= m_xoff;
    m_holes.back ().y -= m_yoff;
    m_holes.back ().ex -= m_xoff;
    m_holes.back ().ey -= m_yoff;

    m_holes.back ().x *= fx;
    m_holes.back ().y *= fy;
    m_holes.back ().ex *= fx;
    m_holes.back ().ey *= fy;
    if (swapxy) {
      std::swap (m_holes.back ().x, m_holes.back ().y);
      std::swap (m_holes.back ().ex, m_holes.back ().ey);
    }

    m_holes.back ().x += m_xoff;
    m_holes.back ().y += m_yoff;
    m_holes.back ().ex += m_xoff;
    m_holes.back ().ey += m_yoff;

    m_holes.back ().x += dx;
    m_holes.back ().y += dy;
    m_holes.back ().ex += dx;
    m_holes.back ().ey += dy;

  }
}

void
GerberDrillFileReader::end_block ()
{
  m_end_block = m_holes.size ();
}

void
GerberDrillFileReader::produce_circle (double cx, double cy, double r, double ex, double ey)
{
  if (m_record_pattern) {
    m_pattern.push_back (DrillHoleDescriptor (cx, cy, r, ex, ey));
  }

  if (m_recording) {
    m_holes.push_back (DrillHoleDescriptor (cx, cy, r, ex, ey));
  } else {
    produce_circle_raw (cx, cy, r, ex, ey);
  }
}

void
GerberDrillFileReader::produce_circle_raw (double cx, double cy, double r, double ex, double ey)
{
  double mx = cx - ex;
  double my = cy - ey;
  double m = sqrt (mx * mx + my * my);
  if (m < 1e-6) {
    mx = r;
    my = 0.0;
  } else {
    mx *= r / m;
    my *= r / m;
  }

  double nx = -my;
  double ny = mx;

  std::vector <db::DPoint> points;

  int n_circle = get_circle_points ();

  //  adjust the radius so we get a outer approximation of the circle:
  //  r *= 1.0 / cos (M_PI / double (n_circle));

  int i = 0;

  for (; i < n_circle / 2; ++i) {
    double a = M_PI * 2.0 * (double (i) / double (n_circle));
    points.push_back (db::DPoint (cx + nx * cos (a) + mx * sin (a), cy + ny * cos (a) + my * sin (a)));
  }

  for (; i < n_circle; ++i) {
    double a = M_PI * 2.0 * (double (i) / double (n_circle));
    points.push_back (db::DPoint (ex + nx * cos (a) + mx * sin (a), ey + ny * cos (a) + my * sin (a)));
  }

  db::DPolygon p;
  p.assign_hull (points.begin (), points.end ());

  produce_polygon (p, false);
}

}

