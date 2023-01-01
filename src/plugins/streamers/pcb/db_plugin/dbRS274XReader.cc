
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


#include "dbRS274XReader.h"
#include "dbRS274XApertures.h"

namespace db
{

// ---------------------------------------------------------------------------------
//  RS274XReader implementation

RS274XReader::RS274XReader (int warn_level)
  : GerberFileReader (warn_level)
{
  init ();
}

RS274XReader::~RS274XReader ()
{
  for (std::vector<RS274XApertureBase *>::const_iterator a = m_apertures.begin (); a != m_apertures.end (); ++a) {
    if (*a) {
      delete *a;
    }
  }

  m_apertures.clear ();
}

bool 
RS274XReader::does_accept ()
{
  return true;
}

bool
RS274XReader::is_clear_polarity ()
{
  //  Now that we have used the polarity, we not longer guess it.
  m_guess_polarity = false;
  return m_neg_polarity ? !m_clear : m_clear;
}

void
RS274XReader::init ()
{
  //  Initialize reader:
  m_clear = false;
  m_net_name.clear ();
  m_guess_polarity = true;
  m_neg_polarity = false;
  m_relative = false;
  m_x = m_y = 0.0;
  m_current_gcode = -1;
  m_current_dcode = -1;
  m_polygon_mode = false;
  m_axis_mapping = ab_xy;
  m_current_aperture = 0;
  m_360deg_circular = false;
  m_buffer.clear ();
  m_polygon_points.clear ();

  for (std::vector<RS274XApertureBase *>::const_iterator a = m_apertures.begin (); a != m_apertures.end (); ++a) {
    if (*a) {
      delete *a;
    }
  }
  m_apertures.clear ();
  m_aperture_macros.clear ();
  m_current_aperture = 0;
}

static GerberMetaData::Position
parse_position (tl::Extractor &ex)
{
  if (ex.test ("Bot")) {
    return GerberMetaData::Bottom;
  } else if (ex.test ("Top")) {
    return GerberMetaData::Top;
  } else if (ex.test ("Inr")) {
    return GerberMetaData::Inner;
  } else {
    return GerberMetaData::NoPosition;
  }
}


GerberMetaData
RS274XReader::do_scan ()
{
  GerberMetaData data;

  char c;

  //  Actually read:
  while ((c = stream ().skip ()) != 0 && !stream ().at_end ()) {

    if (c == '%') {

      stream ().get_char ();

      while (! stream ().at_end () && (c = stream ().skip ()) != '%') {

        std::string param;
        param += stream ().get_char ();
        if (! stream ().at_end ()) {
          param += stream ().get_char ();
        }

        std::string bl = get_block ();

        if (param == "TF") {

          //  Extract information
          tl::Extractor ex (bl);

          if (ex.test (".ProjectId")) {

            ex.test (",");
            data.project_id = ex.get ();

          } else if (ex.test (".CreationDate")) {

            ex.test (",");
            data.creation_date = ex.get ();

          } else if (ex.test (".GenerationSoftware")) {

            ex.test (",");
            data.generation_software = ex.get ();

          } else if (ex.test (".FileFunction")) {

            ex.test (",");
            bool plated = false;

            if (ex.test ("Copper")) {

              data.function = GerberMetaData::Copper;

              ex.test (",");
              ex.test ("L");
              ex.read (data.cu_layer_number);
              ex.test (",");
              data.position = parse_position (ex);

            } else if (ex.test ("Profile")) {

              data.function = GerberMetaData::Profile;

            } else if (ex.test ("Soldermask")) {

              data.function = GerberMetaData::SolderMask;
              ex.test (",");
              data.position = parse_position (ex);

            } else if (ex.test ("Legend")) {

              data.function = GerberMetaData::Legend;
              ex.test (",");
              data.position = parse_position (ex);

            } else if ((plated = ex.test ("Plated")) || ex.test ("NonPlated")) {

              data.function = plated ? GerberMetaData::PlatedHole : GerberMetaData::NonPlatedHole;

              ex.test (",");
              ex.read (data.from_cu);
              ex.test (",");
              ex.read (data.to_cu);

            } else {
              data.function = GerberMetaData::NoFunction;
            }

          }

        }

      }

      //  eat trailing '%'
      if (! stream ().at_end ()) {
        stream ().get_char ();
      }

    } else {
      get_block ();
    }

  }

  return data;
}

void
RS274XReader::do_read ()
{
  init ();

  char c;

  //  Actually read:
  while ((c = stream ().skip ()) != 0 && !stream ().at_end ()) {

    if (c == '%') {

      stream ().get_char ();

      while (! stream ().at_end () && (c = stream ().skip ()) != '%') {
        
        std::string param;
        param += stream ().get_char ();

        if (stream ().at_end ()) {
          throw tl::Exception (tl::to_string (tr ("Unexpected EOF")));
        }

        param += stream ().get_char ();

        if (param == "AS") {
          read_as_parameter (get_block ());
        } else if (param == "FS") {
          read_fs_parameter (get_block ());
        } else if (param == "MI") {
          read_mi_parameter (get_block ());
        } else if (param == "MO") {
          read_mo_parameter (get_block ());
        } else if (param == "OF") {
          read_of_parameter (get_block ());
        } else if (param == "SF") {
          read_sf_parameter (get_block ());
        } else if (param == "IJ") {
          read_ij_parameter (get_block ());
        } else if (param == "IN") {
          read_in_parameter (get_block ());
        } else if (param == "IO") {
          read_io_parameter (get_block ());
        } else if (param == "IP") {
          read_ip_parameter (get_block ());
        } else if (param == "IR") {
          read_ir_parameter (get_block ());
        } else if (param == "PF") {
          read_pf_parameter (get_block ());
        } else if (param == "AD") {
          read_ad_parameter (get_block ());
        } else if (param == "TO") {

          std::string net_name;
          if (read_net_name (get_block (), net_name)) {
            if (! m_net_name.empty ()) {
              flush (m_net_name);
            }
            m_net_name = net_name;
          }

        } else if (param == "TA" || param == "TD" || param == "TF") {

          //  TA, TD and TF paramters are skipped for layout
          get_block ();

        } else if (param == "AB") {

          std::string dcode = get_block ();
          if (dcode.empty ()) {

            if (graphics_stack_empty ()) {
              throw tl::Exception (tl::to_string (tr ("AB closed without initial opening AB command")));
            } else {
              db::Region region;
              collect (region);
              std::string ap = pop_state ();
              install_block_aperture (ap, region);
            }

          } else if (m_polygon_mode) {
            warn (tl::to_string (tr ("AB command inside polygon sequence (G36/G37) - polygon ignored")));
          } else {
            push_state (dcode);
          }

        } else if (param == "AM") {

          //  AM parameters can span multiple data blocks, so collect them

          std::string am_string;
          while (! stream ().at_end () && stream ().skip () != '%') {
            am_string += get_block ();
            am_string += "*";
          }

          read_am_parameter (am_string);

        } else if (param == "KO") {
          read_ko_parameter (get_block ());
        } else if (param == "LN") {
          read_ln_parameter (get_block ());
        } else if (param == "LP") {
          read_lp_parameter (get_block ());
        } else if (param == "LM") {
          read_lm_parameter (get_block ());
        } else if (param == "LR") {
          read_lr_parameter (get_block ());
        } else if (param == "LS") {
          read_ls_parameter (get_block ());
        } else if (param == "SR") {
          read_sr_parameter (get_block ());
        } else if (param == "IF") {
          read_if_parameter (get_block ());
        } else {
          get_block ();
          warn (tl::to_string (tr ("Parameter ignored: ")) + param);
        }

      }
      
      //  eat trailing '%'
      stream ().get_char ();

    } else {

      //  process block
      bool has_coord = false;
      double x = m_x, y = m_y;
      double i = 0.0, j = 0.0;

      tl::Extractor ex (get_block ().c_str ());

      while (! ex.at_end ()) {

        c = *ex.skip ();
        ++ex;

        if (c == 'M') {

          int mcode = 0;
          ex.read (mcode);
          process_mcode (mcode);

        } else if (c == 'N') {

          int ncode = 0;
          ex.read (ncode);
          //  currently N codes are ignored.

        } else if (c == 'G') {

          int gcode = -1;
          ex.read (gcode);

          if (gcode == 4) {

            // .. G04 - ignore rest of block
            break;

          } else if (gcode == 36) {

            // .. G36 - enter polygon mode
            m_polygon_mode = true;
            m_polygon_points.clear ();
            m_current_gcode = 1;
            m_current_dcode = -1;

          } else if (gcode == 37) {

            // .. G37 - leave polygon mode
            m_polygon_mode = false;

            if (m_polygon_points.size () >= 3) {
              db::DPolygon poly;
              poly.assign_hull (m_polygon_points.begin (), m_polygon_points.end ());
              produce_polygon (poly, is_clear_polarity ());
            }

            m_current_gcode = -1;
            m_current_dcode = -1;

          } else if (gcode == 54) {

            // .. G54 - tool prepare
            m_current_gcode = -1;
            m_current_dcode = -1;

          } else if (gcode == 70) {

            // .. G70 - specify inches 
            set_unit (25400);

          } else if (gcode == 71) {

            // .. G71 - specify millimeters 
            set_unit (1000);

          } else if (gcode == 74) {

            // .. G74 - disable 360 degree circular interpolation
            m_360deg_circular = false;

          } else if (gcode == 75) {

            // .. G74 - enable 360 degree circular interpolation
            m_360deg_circular = true;

          } else if (gcode == 90) {

            // .. G90 - absolute mode
            m_relative = false;

          } else if (gcode == 91) {

            // .. G91 - relative mode
            m_relative = false;

          } else if (gcode == 0) {

            // .. G0 - move
            m_current_gcode = gcode;

          } else if (gcode == 2 ||
                     gcode == 3) {

            // .. G2, G3 - circular interpolation
            m_current_gcode = gcode;

          } else if (gcode == 1 ||
                     gcode == 10 ||
                     gcode == 11 ||
                     gcode == 12) {

            // TODO: Handle G10, G11, G12 correctly?
            // .. G1 - linear interpolation
            m_current_gcode = 1;

          } else if (gcode >= 0) {
            warn (tl::sprintf (tl::to_string (tr ("Invalid 'G' code %d - ignored")), gcode));
          }

        } else if (c == 'X') {

          double d = read_coord (ex);

          if (m_relative) {
            x += d;
          } else {
            x = d;
          }

          has_coord = true;

        } else if (c == 'Y') {

          double d = read_coord (ex);

          if (m_relative) {
            y += d;
          } else {
            y = d;
          }

          has_coord = true;

        } else if (c == 'I') {
          i = read_coord (ex);
        } else if (c == 'J') {
          j = read_coord (ex);
        } else if (c == 'D') {

          int dcode = -1;
          ex.read (dcode);

          if (dcode >= 10) {

            //  set current aperture
            if (dcode >= int (m_apertures.size ()) || m_apertures[dcode] == 0) {
              throw tl::Exception (tl::to_string (tr ("Aperture code D%d is invalid or undefined")), dcode);
            }

            m_current_aperture = m_apertures[dcode];

          } else if (dcode <= 3) {

            m_current_dcode = dcode;

            if (dcode == 3) {
              //  force a flash here even if there is no explicit coordinate 
              has_coord = true;
            }

          } else {
            warn (tl::sprintf (tl::to_string (tr ("Invalid D code %d ignored")), dcode));
          }

        } else {
          throw tl::Exception (tl::to_string (tr ("Invalid function code '%c'")), c);
        }

      }

      if (has_coord) {

        if (m_current_dcode == 2) {

          //  move
          if (m_polygon_mode) {

            //  D02 strokes close the polygon (and restart a new one)

            if (m_polygon_points.size () >= 3) {
              db::DPolygon poly;
              poly.assign_hull (m_polygon_points.begin (), m_polygon_points.end ());
              produce_polygon (poly, is_clear_polarity ());
            }

            m_polygon_points.clear ();
            m_polygon_points.push_back (db::DPoint (x, y));

          }

        } else if (m_current_dcode == 3) {

          //  flash
          if (! m_current_aperture) {
            throw tl::Exception (tl::to_string (tr ("No aperture defined (missing G54 block)")));
          }

          if (m_polygon_mode) {
            warn (tl::to_string (tr ("D03 blocks are ignored in polygon mode")));
          } else {
            m_current_aperture->produce_flash (db::DCplxTrans (db::DVector (x, y)) * object_trans (), *this, ep (), is_clear_polarity ());
          }

        } else { // only if m_current_dcode == 1?

          //  move with "light" on
          if (m_current_gcode == 2 || m_current_gcode == 3) {

            // .. circular interpolation
            db::DPoint to = db::DPoint (x, y);
            db::DPoint from = db::DPoint (m_x, m_y);

            double rx = sqrt (i * i + j * j);
            double ry = (m_current_gcode == 3) ? rx : -rx;

            if (rx > 1e-12) {

              double a0 = 0.0, a1 = 0.0;
              db::DPoint center;

              bool has_center = false;
              double dmin = 0.0;

              if (! m_360deg_circular) {

                //  look for a good center point
                for (int v = 0; v < 4; ++v) {

                  db::DPoint c = from + db::DVector ((((v & 1) != 0) ? -i : i), (((v & 2) != 0) ? -j : j));

                  double aa0 = atan2 ((from.y () - c.y ()) / ry, (from.x () - c.x ()) / rx);
                  double aa1 = atan2 ((to.y () - c.y ()) / ry, (to.x () - c.x ()) / rx);

                  while (aa1 < aa0 - 1e-12) {
                    aa1 += M_PI * 2.0;
                  }

                  //  this is the single quadrant interpolation, so we can choose the one which is 
                  //  properly located for one quadrant.
                  if (aa1 - aa0 - 1e-6 < 0.5 * M_PI) {

                    double d = fabs (c.distance (to) - rx);
                    if (d < dmin || ! has_center) {
                      dmin = d;
                      center = c;
                      a0 = aa0;
                      a1 = aa1;
                      has_center = true;
                    }

                  }

                }

                if (! has_center) {
                  warn (tl::sprintf (tl::to_string (tr ("No suitable center point found for G%d code: P1=%s P2=%s I=%g J=%g")),
                                     m_current_gcode, from.to_string (), to.to_string (), i, j));
                }

              } else {

                center = from + db::DVector (i, j);

                a0 = atan2 ((from.y () - center.y ()) / ry, (from.x () - center.x ()) / rx);
                a1 = atan2 ((to.y () - center.y ()) / ry, (to.x () - center.x ()) / rx);

                // multi quadrant interpolation
                while (a1 < a0 + 1e-12) {
                  a1 += M_PI * 2.0;
                }

                has_center = true;

              }

              if (has_center) {

                // TODO: 16 is an arbitrary choice (32 points/full circle)
                int n = int (ceil (fabs (a1 - a0) / (M_PI / 16.0) - 1e-4));
                double da = (a1 - a0) / n;

                for (int i = 0; i < n; ++i) {

                  double ae = a0 + (i + 1) * da;
                  db::DPoint pe = center + db::DVector (rx * cos (ae), ry * sin (ae));

                  if (m_polygon_mode) {
                    m_polygon_points.push_back (pe);
                  } else {

                    if (! m_current_aperture) {
                      throw tl::Exception (tl::to_string (tr ("No aperture defined (missing G54 block)")));
                    }

                    m_current_aperture->produce_linear (db::DCplxTrans (db::DVector (m_x, m_y)) * object_trans (), pe - db::DPoint (m_x, m_y), *this, ep (), is_clear_polarity ());

                  }

                  m_x = pe.x ();
                  m_y = pe.y ();

                }

              }

            }

          } else if (m_current_gcode == 0) {

            //  is it correct to ignore G00?
            warn (tl::to_string (tr ("Block with G00 interpolation mode is ignored")));

          } else if (m_current_gcode == 1 || m_current_gcode < 0) {

            // .. linear interpolation
            if (m_polygon_mode) {
              m_polygon_points.push_back (db::DPoint (x, y));
            } else {
              
              if (! m_current_aperture) {
                throw tl::Exception (tl::to_string (tr ("No aperture defined (missing G54 block)")));
              }

              m_current_aperture->produce_linear (db::DCplxTrans (db::DVector (m_x, m_y)) * object_trans (), db::DPoint (x, y) - db::DPoint (m_x, m_y), *this, ep (), is_clear_polarity ());

            }

          } else {
            throw tl::Exception (tl::to_string (tr ("G00 or unspecified 'G' code requires D03")));
          }

        }

        m_x = x;
        m_y = y;

      }

    }

  }

  if (! m_net_name.empty ()) {
    flush (m_net_name);
  }

  if (! graphics_stack_empty ()) {
    throw tl::Exception (tl::to_string (tr ("AB block not closed")));
  }
}

void
RS274XReader::process_mcode (int /*mcode*/)
{
  //  no processing for M codes currently.
}

const std::string &
RS274XReader::get_block ()
{
  progress_checkpoint ();

  m_buffer.clear ();

  char c;
  while (! stream ().at_end () && (c = stream ().get_char ()) != '*') {
    m_buffer += c;
  }

  return m_buffer;
}

void
RS274XReader::read_as_parameter (const std::string &block)
{
  if (block == "AXBY") {
    m_axis_mapping = ab_xy;
  } else if (block == "AYBX") {
    m_axis_mapping = ab_yx;
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid argument '%s' for AS parameter")), block);
  }
}

void
RS274XReader::read_fs_parameter (const std::string &block)
{
  bool omit_lz = true;
  int ld = -1;
  int td = -1;

  tl::Extractor ex (block.c_str ());
  if (ex.test ("L")) {
    omit_lz = true;
  } else if (ex.test ("T")) {
    omit_lz = false;
  } else if (ex.test ("D")) {
    // TODO: clarify what to do in that case ..
  }

  if (ex.test ("A")) {
    m_relative = false;
  } else if (ex.test ("I")) {
    m_relative = true;
  }

  int i;
  if (ex.test ("N")) {
    ex.read (i);
  }
  if (ex.test ("G")) {
    ex.read (i);
  }

  ex.expect ("X");
  ex.read (i);
  ld = i / 10;
  td = i % 10;
  
  int j = i;
  ex.expect ("Y");
  ex.read (j);
  if (i != j) {
    throw tl::Exception (tl::to_string (tr ("X and Y format must be identical currently")));
  }

  if (ex.test ("D")) {
    ex.read (i);
  }
  if (ex.test ("M")) {
    ex.read (i);
  }

  ex.expect_end ();

  set_format (ld, td, omit_lz);
}

void
RS274XReader::read_mi_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  ex.expect ("A");
  int ma = 0;
  ex.read (ma);
  ex.expect ("B");
  int mb = 0;
  ex.read (mb);
  ex.expect_end ();

  bool mx = (ma != 0);
  bool my = (mb != 0);

  if (m_axis_mapping != ab_xy) {
    std::swap (mx, my);
  }

  update_local_mirror (mx, my);
}

void
RS274XReader::read_mo_parameter (const std::string &block)
{
  if (block == "IN") {
    set_unit (25400);
  } else if (block == "MM") {
    set_unit (1000);
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid argument of M0 parameter - must be 'IN' or 'MM', not '%s'")), block);
  }
}

void
RS274XReader::read_of_parameter (const std::string &block)
{
  // TODO: relationship to IO paramter???
  tl::Extractor ex (block.c_str ());

  ex.expect ("A");
  double ao = 0.0;
  ex.read (ao);
  ao *= unit ();
  ex.expect ("B");
  double bo = 0.0;
  ex.read (bo);
  bo *= unit ();
  ex.expect_end ();

  double ox = ao;
  double oy = bo;

  if (m_axis_mapping != ab_xy) {
    std::swap (ox, oy);
  }

  update_local_offset (ox, oy);
}

void
RS274XReader::read_sf_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  ex.expect ("A");
  double sx = 1.0;
  ex.read (sx);
  ex.expect ("B");
  double sy = 1.0;
  ex.read (sy);
  ex.expect_end ();

  if (m_axis_mapping != ab_xy) {
    std::swap (sx, sy);
  }

  if (fabs (sx - sy) > 1e-6) {
    throw tl::Exception (tl::to_string (tr ("Different scalings for x and y axis is not supported currently.")));
  }

  update_local_scale (sx);
}

void
RS274XReader::read_ls_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  double s = 1.0;
  ex.read (s);

  update_object_scale (s);
}

void
RS274XReader::read_lr_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  double a = 0.0;
  ex.read (a);

  update_object_angle (a);
}

void
RS274XReader::read_lm_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  bool omx = false, omy = false;
  while (! ex.at_end ()) {
    if (ex.test ("X")) {
      omy = true;  //  "my == mirror at y axis" is "X == mirror along x axis"
    } else if (ex.test ("Y")) {
      omx = true;  //  "mx == mirror at x axis" is "Y == mirror along y axis"
    } else {
      break;
    }
  }

  update_object_mirror (omx, omy);
}

void
RS274XReader::read_ij_parameter (const std::string & /*block*/)
{
  warn (tl::to_string (tr ("IJ parameters are ignored currently")));
}

void
RS274XReader::read_in_parameter (const std::string & /*block*/)
{
  // image name ignored currently
}

void
RS274XReader::read_io_parameter (const std::string &block)
{
  // TODO: clarify: relationship to OF paramter???
  tl::Extractor ex (block.c_str ());

  ex.expect ("A");
  double ao = 0.0;
  ex.read (ao);
  ao *= unit ();
  ex.expect ("B");
  double bo = 0.0;
  ex.read (bo);
  bo *= unit ();
  ex.expect_end ();

  double ox = ao;
  double oy = bo;

  if (m_axis_mapping != ab_xy) {
    std::swap (ox, oy);
  }

  update_local_offset (ox, oy);
}

void
RS274XReader::read_ip_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  if (ex.test ("POS")) {
    set_inverse (false);
  } else if (ex.test ("NEG")) {
    set_inverse (true);
  } 

  ex.expect_end ();
}

void
RS274XReader::read_ir_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  double rot = 0.0;
  ex.read (rot);
  update_local_angle (rot);
}

void
RS274XReader::read_pf_parameter (const std::string & /*block*/)
{
  warn (tl::to_string (tr ("PF parameters are ignored")));
}

bool
RS274XReader::read_net_name (const std::string &block, std::string &net_name) const
{
  tl::Extractor ex (block.c_str ());

  ex.test (".");

  if (ex.test ("N")) {

    //  only parse net names
    ex.test (",");

    std::string n = ex.get ();
    if (! n.empty () && n != "N/C") {
      net_name = n;
      return true;
    }
  }

  return false;
}

void
RS274XReader::read_ad_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  if (ex.at_end ()) {
    //  ignore "%AD*" commands ..
    return;
  }

  ex.expect ("D");
  int dcode = 0;
  ex.read (dcode);

  if (dcode < 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid D code for AD parameter")));
  }

  while (int (m_apertures.size ()) <= dcode) {
    m_apertures.push_back (0);
  }

  std::string name;
  while (*ex && *ex != '*' && *ex != ',') {
    name += *ex;
    ++ex;
  }

  if (name == "C") {
    m_apertures[dcode] = new RS274XCircleAperture (*this, ex);
  } else if (name == "R") {
    m_apertures[dcode] = new RS274XRectAperture (*this, ex);
  } else if (name == "O") {
    m_apertures[dcode] = new RS274XOvalAperture (*this, ex);
  } else if (name == "P") {
    m_apertures[dcode] = new RS274XRegularAperture (*this, ex);
  } else if (m_aperture_macros.find (name) != m_aperture_macros.end ()) {
    m_apertures[dcode] = new RS274XMacroAperture (*this, name, m_aperture_macros[name], ex);
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid aperture name '%s' (not a macro name and not a standard aperture) for AD parameter")), name);
  }
}

void
RS274XReader::install_block_aperture (const std::string &d, const db::Region &region)
{
  int dcode = 0;

  try {
    tl::Extractor ex (d.c_str ());
    ex.expect ("D");
    ex.read (dcode);
    ex.expect_end ();
  } catch (...) {
    throw tl::Exception (tl::to_string (tr ("Invalid aperture code string for AB command")));
  }

  if (dcode < 0) {
    throw tl::Exception (tl::to_string (tr ("Invalid D code for AB command")));
  }

  while (int (m_apertures.size ()) <= dcode) {
    m_apertures.push_back (0);
  }

  m_apertures[dcode] = new RS274XRegionAperture (region);
}

void
RS274XReader::read_am_parameter (const std::string &block)
{
  tl::Extractor ex (block.c_str ());

  std::string name;
  while (*ex && *ex != '*') {
    name += *ex;
    ++ex;
  }

  ex.expect ("*");

  m_aperture_macros.insert (std::make_pair (name, std::string (ex.skip ())));
}

void
RS274XReader::read_ko_parameter (const std::string & /*block*/)
{
  warn (tl::to_string (tr ("KO parameters are not supported currently")));
}

void
RS274XReader::read_ln_parameter (const std::string & /*block*/)
{
  // TODO: implement layer name
}

void
RS274XReader::read_lp_parameter (const std::string &block)
{
  if (block == "C") {
    //  when we encounter the first LP parameter, and it is a clear layer, we
    //  guess negative polarity (as do some viewers)
    if (m_guess_polarity) {
      m_neg_polarity = true;
      m_guess_polarity = false;
    }
    m_clear = true;
  } else if (block == "D") {
    if (m_guess_polarity) {
      m_neg_polarity = false;
      m_guess_polarity = false;
    }
    m_clear = false;
  } else {
    throw tl::Exception (tl::to_string (tr ("Invalid argument '%s' for LP parameter")), block);
  }
}

void
RS274XReader::read_sr_parameter (const std::string &block)
{
  reset_step_and_repeat ();

  tl::Extractor ex (block.c_str ());

  if (ex.at_end ()) {
    //  empty %SR* comment: just reset
    return;
  }

  int nx = 1, ny = 1;
  double dx = 0.0, dy = 0.0;

  while (! ex.at_end ()) {
    if (ex.test ("X")) {
      ex.read (nx);
    } else if (ex.test ("Y")) {
      ex.read (ny);
    } else if (ex.test ("I")) {
      ex.read (dx);
    } else if (ex.test ("J")) {
      ex.read (dy);
    } else {
      break;
    }
  }
  ex.expect_end ();

  if (nx > 1 || ny > 1) {

    dx *= unit ();
    dy *= unit ();

    std::vector <db::DVector> steps;
    steps.reserve (nx * ny);
    for (int i = 0; i < nx; ++i) {
      for (int j = 0; j < ny; ++j) {
        steps.push_back (db::DVector (i * dx, j * dy));
      }
    }

    step_and_repeat (steps);

  }
}

void
RS274XReader::read_if_parameter (const std::string & /*block*/)
{
  warn (tl::to_string (tr ("IF parameters are not supported currently")));
}

}

