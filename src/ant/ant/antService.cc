
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


#include "dbUserObject.h"
#include "dbClipboard.h"
#include "tlString.h"
#include "tlAssert.h"
#include "layPlugin.h"
#include "layRenderer.h"
#include "laySnap.h"
#include "layLayoutViewBase.h"
#include "laybasicConfig.h"
#include "layConverters.h"
#include "layLayoutCanvas.h"
#include "layFixedFont.h"
#if defined(HAVE_QT)
#  include "layProperties.h"
#endif
#include "antService.h"
#if defined(HAVE_QT)
#  include "antPropertiesPage.h"
#endif
#include "antConfig.h"

namespace ant
{

double angle_ruler_radius_factor = 0.9;
double ruler_tick_length = 8.0;
double ruler_arrow_width = 8.0;

// -------------------------------------------------------------
//  Convert buttons to an angle constraint

static lay::angle_constraint_type 
ac_from_buttons (unsigned int buttons)
{
  if ((buttons & lay::ShiftButton) != 0) {
    if ((buttons & lay::ControlButton) != 0) {
      return lay::AC_Any;
    } else {
      return lay::AC_Ortho;
    }
  } else {
    if ((buttons & lay::ControlButton) != 0) {
      return lay::AC_Diagonal;
    } else {
      return lay::AC_Global;
    }
  }
}

// -------------------------------------------------------------
//  Functionality to draw a ruler object

static void
tick_spacings (double d, double min_d, int &minor_ticks, double &ticks)
{
  if (min_d > d) {

    minor_ticks = -1;
    ticks = -1;

  } else {

    const double log10 = log (10.0);

    //  as a safety measure, not too many ticks are created.
    min_d = std::max (min_d, 0.001 * d);

    double l1 = log (min_d) / log10;
    double l0 = floor (l1);
    l1 -= l0;

    if (l1 < 0.3) {
      minor_ticks = 5;
    } else if (l1 < 0.7) {
      minor_ticks = 2;
    } else {
      minor_ticks = 1;
    }

    ticks = exp (log10 * l0) * 10.0;

  }
}

/**
 *  @brief Draws a ruler with the given parameters
 *
 *  @param q1 The first point in pixel space
 *  @param q2 The second point in pixel space
 *  @param length_u The ruler length in micron
 *  @param min_spc_u The minimum tick spacing in micron
 *  @param sel True to draw ruler in "selected" mode
 *  @param right True to draw the ruler with ticks to the right (as seem from p1 to p2 in transformed space)
 *  @param style The style with which to draw the ruler
 *  @param pos The position where to draw the text
 *  @param bitmap The bitmap to draw the ruler on
 *  @param renderer The renderer object
 *  @param first_segment True, if we're drawing the first segment
 *  @param last_segment True, if we're drawing the last segment
 */
void 
draw_ruler (const db::DPoint &q1,
            const db::DPoint &q2,
            double length_u,
            double min_spc_u,
            bool sel,
            bool right,
            ant::Object::style_type style,
            lay::CanvasPlane *bitmap,
            lay::Renderer &renderer,
            bool first_segment,
            bool last_segment,
            bool no_line = false)
{
  double arrow_width = ruler_arrow_width / renderer.resolution ();
  double arrow_length = 1.5 * arrow_width;
  double sel_width = 2 / renderer.resolution ();

  if (length_u < 1e-5 /*micron*/ && style != ant::Object::STY_cross_both && style != ant::Object::STY_cross_end && style != ant::Object::STY_cross_start) {

    if (sel) {
      
      db::DBox b (q1 - db::DVector (sel_width * 0.5, sel_width * 0.5),
                  q2 + db::DVector (sel_width * 0.5, sel_width * 0.5));

      renderer.draw (b, bitmap, bitmap, 0, 0);
      
    } else {
      renderer.draw (db::DEdge (q1, q1), 0, bitmap, 0, 0);
    }

  } else {

    //  compute the tick distribution
    double tick_length = (style == ant::Object::STY_ruler ? ruler_tick_length : 0) / renderer.resolution ();

    double ticks = -1.0;
    int minor_ticks = -1;
    
    if (tick_length > 0) {
      tick_spacings (length_u, min_spc_u, minor_ticks, ticks);
    }

    //  normal and unit vector
    
    double len = q1.double_distance (q2);
    if (! no_line && len < double (arrow_length) * 2.4) {
      if ((style == ant::Object::STY_arrow_end || style == ant::Object::STY_arrow_start)) {
        arrow_length = len / 1.2;
        arrow_width = arrow_length * 2.0 / 3.0;
      } else if (style == ant::Object::STY_arrow_both) {
        arrow_length = len / 2.4;
        arrow_width = arrow_length * 2.0 / 3.0;
      }
    }
    
    db::DVector qq (q2.y () - q1.y (), q1.x () - q2.x ());
    if (len > 1e-10) {
      qq *= 1.0 / len;
    } else {
      qq = db::DVector (0.0, 1.0);
    }
    if (!right) {
      qq = -qq;
    }

    db::DVector qu = q2 - q1;
    if (len > 1e-10) {
      qu *= 1.0 / len;
    } else {
      qu = db::DVector (1.0, 0.0);
    }
      
    //  produce line in selected and unselected mode
      
    if (! no_line && style != ant::Object::STY_none) {

      if (sel) {

        db::DVector qw = qq * (sel_width * 0.5);

        db::DVector dq1, dq2;
        if (! first_segment) {
          //  no start indicator if not first segment
        } else if (style == ant::Object::STY_arrow_both || style == ant::Object::STY_arrow_start) {
          dq1 = qu * (arrow_length - 1);
        } else if (style == ant::Object::STY_cross_both || style == ant::Object::STY_cross_start) {
          dq1 = qu * (sel_width * 0.5);
        }
        if (! last_segment) {
          //  no end indicator if not last segment
        } else if (style == ant::Object::STY_arrow_both || style == ant::Object::STY_arrow_end) {
          dq2 = qu * -(arrow_length - 1);
        } else if (style == ant::Object::STY_cross_both || style == ant::Object::STY_cross_end) {
          dq2 = qu * -(sel_width * 0.5);
        }

        db::DPolygon p;
        db::DPoint points[] = {
          db::DPoint (q1 + dq1 + qw),
          db::DPoint (q2 + dq2 + qw),
          db::DPoint (q2 + dq2 - qw),
          db::DPoint (q1 + dq1 - qw),
        };
        p.assign_hull (points, points + sizeof (points) / sizeof (points [0]));
        renderer.draw (p, bitmap, bitmap, 0, 0);

      } else {

        renderer.draw (db::DEdge (q1, q2), 0, bitmap, 0, 0);

      }

    }

    if (! last_segment) {

      //  no end indicator if not last segment

    } else if (style == ant::Object::STY_arrow_end || style == ant::Object::STY_arrow_both) {

      db::DPolygon p;
      db::DPoint points[] = {
        db::DPoint (q2),
        db::DPoint (q2 + qq * double (arrow_width * 0.5) - qu * double (arrow_length)),
        db::DPoint (q2 - qq * double (arrow_width * 0.5) - qu * double (arrow_length)),
      };
      p.assign_hull (points, points + sizeof (points) / sizeof (points [0]));
      renderer.draw (p, bitmap, bitmap, 0, 0);

    } else if (style == ant::Object::STY_cross_end || style == ant::Object::STY_cross_both) {

      db::DPolygon p;
      db::DPoint points[] = {
        db::DPoint (q2),
        db::DPoint (q2 + qq * double (arrow_width)),
        db::DPoint (q2 - qq * double (arrow_width)),
        db::DPoint (q2),
        db::DPoint (q2 + qu * double (arrow_width)),
        db::DPoint (q2 - qu * double (arrow_width)),
      };
      p.assign_hull (points, points + sizeof (points) / sizeof (points [0]), false /*don't compress*/);
      renderer.draw (p, bitmap, bitmap, 0, 0);

    }
    
    if (! first_segment) {

      //  no start indicator if not first segment

    } else if (style == ant::Object::STY_arrow_start || style == ant::Object::STY_arrow_both) {

      db::DPolygon p;
      db::DPoint points[] = {
        db::DPoint (q1),
        db::DPoint (q1 + qq * double (arrow_width * 0.5) + qu * double (arrow_length)),
        db::DPoint (q1 - qq * double (arrow_width * 0.5) + qu * double (arrow_length))
      };
      p.assign_hull (points, points + sizeof (points) / sizeof (points [0]));
      renderer.draw (p, bitmap, bitmap, 0, 0);

    } else if (style == ant::Object::STY_cross_start || style == ant::Object::STY_cross_both) {

      db::DPolygon p;
      db::DPoint points[] = {
        db::DPoint (q1),
        db::DPoint (q1 + qq * double (arrow_width)),
        db::DPoint (q1 - qq * double (arrow_width)),
        db::DPoint (q1),
        db::DPoint (q1 + qu * double (arrow_width)),
        db::DPoint (q1 - qu * double (arrow_width)),
      };
      p.assign_hull (points, points + sizeof (points) / sizeof (points [0]), false /*don't compress*/);
      renderer.draw (p, bitmap, bitmap, 0, 0);

    }

    //  create three tick vectors in tv_text, tv_short and tv_long

    double tf = tick_length;
    db::DVector tv_short = qq * tf * 0.5;
    db::DVector tv_long  = qq * tf;

    if (tick_length > 0) {
      renderer.draw (db::DEdge (q1, q1 + tv_long), 0, bitmap, 0, 0);
      renderer.draw (db::DEdge (q2, q2 + tv_long), 0, bitmap, 0, 0);
    }
    
    if (minor_ticks > 0 && ticks > 0.0) {

      db::DVector q = q2 - q1;

      int nmax = int (floor ((length_u / ticks) * double (minor_ticks) - 1e-6));
      for (int n = 1; n <= nmax; ++n) {

        double r = ticks * double (n) / double (minor_ticks) / length_u;

        db::DPoint qq = q1 + q * r;
        qq = db::DPoint (floor (qq.x () + 0.5), floor (qq.y () + 0.5));

        if (n % minor_ticks == 0) {
          renderer.draw (db::DEdge (qq, qq + tv_long), 0, bitmap, 0, 0);
        } else {
          renderer.draw (db::DEdge (qq, qq + tv_short), 0, bitmap, 0, 0);
        }

      }

    }

  }
}

/**
 *  @brief Draws a text with the given parameters
 *
 *  @param q1 The first point in pixel space
 *  @param q2 The second point in pixel space
 *  @param length_u The ruler length in micron
 *  @param label The label text to draw
 *  @param right True to draw the ruler with ticks to the right (as seem from p1 to p2 in transformed space)
 *  @param style The style with which to draw the ruler
 *  @param pos The position where to draw the text
 *  @param halign The text's horizonal alignment
 *  @param valign The text's vertical alignment
 *  @param bitmap The bitmap to draw the ruler on
 *  @param renderer The renderer object
 */
void
draw_text (const db::DPoint &q1,
           const db::DPoint &q2,
           double length_u,
           const std::string &label,
           bool right,
           ant::Object::style_type style,
           ant::Object::position_type pos,
           ant::Object::alignment_type halign,
           ant::Object::alignment_type valign,
           lay::CanvasPlane *bitmap,
           lay::Renderer &renderer)
{
  if (label.empty ()) {
    return;
  }

  double arrow_width = ruler_arrow_width / renderer.resolution ();
  double arrow_length = 1.5 * arrow_width;

  //  Currently, "auto" means p2.
  if (pos == ant::Object::POS_auto) {
    pos = ant::Object::POS_p2;
  }

  if (length_u < 1e-5 /*micron*/ && style != ant::Object::STY_cross_both && style != ant::Object::STY_cross_end && style != ant::Object::STY_cross_start) {

    renderer.draw (db::DBox (q1, q1),
                     label,
                     db::DefaultFont,
                     db::HAlignLeft,
                     db::VAlignBottom,
                     db::DFTrans (db::DFTrans::r0), 0, 0, 0, bitmap);

  } else {

    //  compute the tick distribution
    double tick_length = (style == ant::Object::STY_ruler ? ruler_tick_length : 0) / renderer.resolution ();

    //  normal and unit vector

    double len = q1.double_distance (q2);
    if ((style == ant::Object::STY_arrow_end || style == ant::Object::STY_arrow_start) && len < double (arrow_length) * 1.2) {
      arrow_length = len / 1.2;
      arrow_width = arrow_length * 2.0 / 3.0;
    } else if (style == ant::Object::STY_arrow_both && len < double (arrow_length) * 2.4) {
      arrow_length = len / 2.4;
      arrow_width = arrow_length * 2.0 / 3.0;
    }

    db::DVector qq (q2.y () - q1.y (), q1.x () - q2.x ());
    if (len > 1e-10) {
      qq *= 1.0 / len;
    } else {
      qq = db::DVector (0.0, 1.0);
    }
    if (!right) {
      qq = -qq;
    }

    db::DVector qu = q2 - q1;
    if (len > 1e-10) {
      qu *= 1.0 / len;
    } else {
      qu = db::DVector (1.0, 0.0);
    }

    db::HAlign text_halign = db::HAlignCenter;
    if (halign == ant::Object::AL_auto) {
      //  Compute a nice alignment depending on the anchor point
      if (fabs (qq.x ()) > 1e-6) {
        text_halign = qq.x () > 0.0 ? db::HAlignLeft : db::HAlignRight;
      } else if (length_u < 1e-5) {
        text_halign = db::HAlignLeft;
      } else if (pos == ant::Object::POS_p2) {
        text_halign = q2.x () < q1.x () ? db::HAlignLeft : db::HAlignRight;
      } else if (pos == ant::Object::POS_p1) {
        text_halign = q1.x () < q2.x () ? db::HAlignLeft : db::HAlignRight;
      } else {
        text_halign = db::HAlignCenter;
      }
    } else if (halign == ant::Object::AL_left) {
      text_halign = db::HAlignLeft;
    } else if (halign == ant::Object::AL_right) {
      text_halign = db::HAlignRight;
    }

    db::VAlign text_valign = db::VAlignCenter;
    if (valign == ant::Object::AL_auto) {
      //  Compute a nice alignment depending on the anchor point
      if (length_u < 1e-5) {
        text_valign = db::VAlignBottom;
      } else if (fabs (qq.y ()) > 1e-6) {
        text_valign = qq.y () > 0.0 ? db::VAlignBottom : db::VAlignTop;
      } else if (pos == ant::Object::POS_p2) {
        text_valign = q1.y () > q2.y () ? db::VAlignBottom : db::VAlignTop;
      } else if (pos == ant::Object::POS_p1) {
        text_valign = q2.y () > q1.y () ? db::VAlignBottom : db::VAlignTop;
      } else {
        text_valign = db::VAlignCenter;
      }
    } else if (valign == ant::Object::AL_bottom) {
      text_valign = db::VAlignBottom;
    } else if (valign == ant::Object::AL_top) {
      text_valign = db::VAlignTop;
    }

    db::DVector tv_text;
    if (style == ant::Object::STY_arrow_start || style == ant::Object::STY_arrow_both || style == ant::Object::STY_arrow_end) {
      tv_text = qq * (arrow_width * 0.5 + 2.0);
    } else if (style == ant::Object::STY_cross_start || style == ant::Object::STY_cross_both || style == ant::Object::STY_cross_end) {
      if (length_u < 1e-5 /*micron*/) {
        if (text_halign == db::HAlignRight) {
          tv_text = (qq - qu) * 2.0;
        } else if (text_halign == db::HAlignLeft) {
          tv_text = (qq + qu) * 2.0;
        } else {
          tv_text = qq * 2.0;
        }
      } else {
        tv_text = qq * (arrow_width + 2.0);
      }
    } else {
      tv_text = qq * (tick_length + 2.0);
    }

    if (text_halign == db::HAlignCenter) {
      tv_text.set_x (0);
    } else if (text_halign == db::HAlignRight) {
      tv_text.set_x (std::min (tv_text.x (), 0.0));
    } else if (text_halign == db::HAlignLeft){
      tv_text.set_x (std::max (tv_text.x (), 0.0));
    }

    if (text_valign == db::VAlignCenter) {
      tv_text.set_y (0);
    } else if (text_valign == db::VAlignTop) {
      tv_text.set_y (std::min (tv_text.y (), 0.0));
    } else if (text_valign == db::VAlignBottom){
      tv_text.set_y (std::max (tv_text.y (), 0.0));
    }

    db::DPoint tp = q2;
    if (pos == ant::Object::POS_center) {
      tp = q1 + (q2 - q1) * 0.5;
    } else if (pos == ant::Object::POS_p1) {
      tp = q1;
    }

    renderer.draw (db::DBox (tp + tv_text, tp + tv_text),
                   label,
                   db::DefaultFont,
                   text_halign,
                   text_valign,
                   db::DFTrans (db::DFTrans::r0), 0, 0, 0, bitmap);

  }
}

/**
 *  @brief Draws an ellipse with the given parameters
 *
 *  @param q1 The first point in pixel space
 *  @param q2 The second point in pixel space
 *  @param length_u The "typical dimension" - used to simplify for very small ellipses
 *  @param sel True to draw ruler in "selected" mode
 *  @param bitmap The bitmap to draw the ruler on
 *  @param renderer The renderer object
 *  @param start_angle The starting angle (in radians)
 *  @param stop_angle The stop angle (in radians)
 */
void
draw_ellipse (const db::DPoint &q1,
              const db::DPoint &q2,
              double length_u,
              bool sel,
              lay::CanvasPlane *bitmap,
              lay::Renderer &renderer,
              double start_angle = 0.0,
              double stop_angle = 2.0 * M_PI)
{
  double sel_width = 2 / renderer.resolution ();

  if (length_u < 1e-5 /*micron*/) {

    if (sel) {

      db::DBox b (q1 - db::DVector (sel_width * 0.5, sel_width * 0.5),
                  q2 + db::DVector (sel_width * 0.5, sel_width * 0.5));

      renderer.draw (b, bitmap, bitmap, 0, 0);

    } else {
      renderer.draw (db::DEdge (q1, q1), 0, bitmap, 0, 0);
    }

  } else {

    int npoints = int (floor (200 * abs (stop_angle - start_angle) / (2.0 * M_PI)));

    double rx = fabs ((q2 - q1).x () * 0.5);
    double ry = fabs ((q2 - q1).y () * 0.5);
    db::DPoint c = q1 + (q2 - q1) * 0.5;

    std::vector<db::DPoint> pts;
    pts.reserve (npoints + 1);

    double da = fabs (stop_angle - start_angle) / double (npoints);
    for (int i = 0; i < npoints + 1; ++i) {
      double a = da * i + start_angle;
      pts.push_back (c + db::DVector (rx * cos (a), ry * sin (a)));
    }

    if (sel) {

      db::DPath p (pts.begin (), pts.end (), sel_width);
      renderer.draw (p, bitmap, bitmap, 0, 0);

    } else {

      for (size_t i = 0; i + 1 < pts.size (); ++i) {
        renderer.draw (db::DEdge (pts [i], pts [i + 1]), 0, bitmap, 0, 0);
      }

    }

  }

}

void
draw_ruler_segment (const ant::Object &ruler, size_t index, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  bool last_segment = (index == ruler.segments () - 1 || index == std::numeric_limits<size_t>::max ());
  bool first_segment = (index == 0 || index == std::numeric_limits<size_t>::max ());

  db::DPoint p1 = ruler.seg_p1 (index), p2 = ruler.seg_p2 (index);

  //  round the starting point, shift both, and round the end point 
  std::pair <db::DPoint, db::DPoint> v = lay::snap (trans * p1, trans * p2);
  db::DPoint q1 = v.first;
  db::DPoint q2 = v.second;
  
  bool xy_swapped = ((trans.rot () % 2) != 0);
  double lu = p1.double_distance (p2);
  int min_tick_spc = int (0.5 + 20 / renderer.resolution ());  //  min tick spacing in canvas units
  double mu = double (min_tick_spc) / trans.ctrans (1.0);

  if (ruler.outline () == Object::OL_diag) {
    draw_ruler (q1, q2, lu, mu, sel, q2.x () < q1.x (), ruler.style (), bitmap, renderer, first_segment, last_segment);
    draw_text (q1, q2, lu, ruler.text (index), q2.x () < q1.x (), ruler.style (), ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);
  }

  if ((!xy_swapped && (ruler.outline () == Object::OL_xy || ruler.outline () == Object::OL_diag_xy)) ||
      ( xy_swapped && (ruler.outline () == Object::OL_yx || ruler.outline () == Object::OL_diag_yx))) {

    bool r = (q2.x () > q1.x ()) ^ (q2.y () < q1.y ());

    if (ruler.outline () == Object::OL_diag_xy || ruler.outline () == Object::OL_diag_yx) {
      draw_ruler (q1, q2, lu, mu, sel, !r, ruler.style (), bitmap, renderer, first_segment, last_segment);
      draw_text (q1, q2, lu, ruler.text (index), !r, ruler.style (), ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);
    }
    draw_ruler (q1, db::DPoint (q2.x (), q1.y ()), lu, mu, sel, r, ruler.style (), bitmap, renderer, false, false);
    draw_text (q1, db::DPoint (q2.x (), q1.y ()), lu, ruler.text_x (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.xlabel_xalign (), ruler.xlabel_yalign (), bitmap, renderer);
    draw_ruler (db::DPoint (q2.x (), q1.y ()), q2, lu, mu, sel, r, ruler.style (), bitmap, renderer, false, false);
    draw_text (db::DPoint (q2.x (), q1.y ()), q2, lu, ruler.text_y (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.ylabel_xalign (), ruler.ylabel_yalign (), bitmap, renderer);

  }

  if ((!xy_swapped && (ruler.outline () == Object::OL_yx || ruler.outline () == Object::OL_diag_yx)) ||
      ( xy_swapped && (ruler.outline () == Object::OL_xy || ruler.outline () == Object::OL_diag_xy))) {

    bool r = (q2.x () > q1.x ()) ^ (q2.y () > q1.y ());

    if (ruler.outline () == Object::OL_diag_xy || ruler.outline () == Object::OL_diag_yx) {
      draw_ruler (q1, q2, lu, mu, sel, !r, ruler.style (), bitmap, renderer, first_segment, last_segment);
      draw_text (q1, q2, lu, ruler.text (index), !r, ruler.style (), ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);
    }
    draw_ruler (q1, db::DPoint (q1.x (), q2.y ()), lu, mu, sel, r, ruler.style (), bitmap, renderer, false, false);
    draw_text (q1, db::DPoint (q1.x (), q2.y ()), lu, ruler.text_y (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.ylabel_xalign (), ruler.ylabel_yalign (), bitmap, renderer);
    draw_ruler (db::DPoint (q1.x (), q2.y ()), q2, lu, mu, sel, r, ruler.style (), bitmap, renderer, false, false);
    draw_text (db::DPoint (q1.x (), q2.y ()), q2, lu, ruler.text_x (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.xlabel_xalign (), ruler.xlabel_yalign (), bitmap, renderer);

  }
}

void
draw_ruler_box (const ant::Object &ruler, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  db::DPoint p1 = ruler.p1 (), p2 = ruler.p2 ();

  //  round the starting point, shift both, and round the end point
  std::pair <db::DPoint, db::DPoint> v = lay::snap (trans * p1, trans * p2);
  db::DPoint q1 = v.first;
  db::DPoint q2 = v.second;

  double lu = p1.double_distance (p2);
  int min_tick_spc = int (0.5 + 20 / renderer.resolution ());  //  min tick spacing in canvas units
  double mu = double (min_tick_spc) / trans.ctrans (1.0);

  bool r = (q2.x () > q1.x ()) ^ (q2.y () < q1.y ());

  size_t index = std::numeric_limits<size_t>::max ();
  draw_ruler (q1, db::DPoint (q2.x (), q1.y ()), lu, mu, sel, r, ruler.style (), bitmap, renderer, true, true);
  draw_text (q1, db::DPoint (q2.x (), q1.y ()), lu, ruler.text_x (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.xlabel_xalign (), ruler.xlabel_yalign (), bitmap, renderer);
  draw_ruler (db::DPoint (q2.x (), q1.y ()), q2, lu, mu, sel, r, ruler.style (), bitmap, renderer, true, true);
  draw_text (db::DPoint (q2.x (), q1.y ()), q2, lu, ruler.text_y (index, trans.fp_trans ()), r, ruler.style (), ant::Object::POS_center, ruler.ylabel_xalign (), ruler.ylabel_yalign (), bitmap, renderer);
  draw_ruler (q1, db::DPoint (q1.x (), q2.y ()), lu, mu, sel, !r, ruler.style (), bitmap, renderer, true, true);
  draw_ruler (db::DPoint (q1.x (), q2.y ()), q2, lu, mu, sel, !r, ruler.style (), bitmap, renderer, true, true);
  draw_text (q1, q2, lu, ruler.text (index), !r, ant::Object::STY_none, ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);
}

void
draw_ruler_ellipse (const ant::Object &ruler, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  db::DPoint p1 = ruler.p1 (), p2 = ruler.p2 ();

  //  round the starting point, shift both, and round the end point
  std::pair <db::DPoint, db::DPoint> v = lay::snap (trans * p1, trans * p2);
  db::DPoint q1 = v.first;
  db::DPoint q2 = v.second;

  double lu = p1.double_distance (p2);

  bool r = (q2.x () > q1.x ()) ^ (q2.y () < q1.y ());

  size_t index = std::numeric_limits<size_t>::max ();
  draw_text (q1, db::DPoint (q2.x (), q1.y ()), lu, ruler.text_x (index, trans.fp_trans ()), r, ant::Object::STY_none, ant::Object::POS_center, ruler.xlabel_xalign (), ruler.xlabel_yalign (), bitmap, renderer);
  draw_text (db::DPoint (q2.x (), q1.y ()), q2, lu, ruler.text_y (index, trans.fp_trans ()), r, ant::Object::STY_none, ant::Object::POS_center, ruler.ylabel_xalign (), ruler.ylabel_yalign (), bitmap, renderer);
  draw_text (q1, q2, lu, ruler.text (index), !r, ant::Object::STY_none, ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);

  draw_ellipse (q1, q2, lu, sel, bitmap, renderer);
}

void
draw_ruler_radius (const ant::Object &ruler, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  //  draw crosses for the support points
  for (auto p = ruler.points ().begin (); p != ruler.points ().end (); ++p) {
    ant::Object supp (*p, *p, 0, std::string (), std::string (), std::string (), ant::Object::STY_cross_start, ant::Object::OL_diag, false, lay::AC_Global);
    draw_ruler_segment (supp, 0, trans, sel, bitmap, renderer);
  }

  double radius = 0.0;
  double start_angle = 0.0, stop_angle = 0.0;
  db::DPoint center;

  //  circle interpolation
  if (ruler.compute_interpolating_circle (radius, center, start_angle, stop_angle)) {

    //  draw circle segment
    db::DVector rr (radius, radius);
    std::pair <db::DPoint, db::DPoint> v = lay::snap (trans * (center - rr), trans * (center + rr));
    draw_ellipse (v.first, v.second, radius * 2.0, sel, bitmap, renderer, start_angle, stop_angle);

    double a = 0.5 * (start_angle + stop_angle);
    db::DPoint rc = center + db::DVector (cos (a), sin (a)) * radius;

#if 0
    //  draw a center marker
    ant::Object center_loc (center, center, 0, std::string (), ruler.fmt_x (), ruler.fmt_y (), ant::Object::STY_cross_start, ant::Object::OL_diag, false, lay::AC_Global);
    draw_ruler_segment (center_loc, 0, trans, sel, bitmap, renderer);
#endif

    //  draw the radius ruler
    ant::Object radius = ruler;
    radius.outline (ant::Object::OL_diag);
    ant::Object::point_list pts;
    pts.push_back (center);
    pts.push_back (rc);
    radius.set_points (pts);
    draw_ruler_segment (radius, 0, trans, sel, bitmap, renderer);

  }
}

void
draw_ruler_angle (const ant::Object &ruler, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  //  draw guiding segments in diag/plain line mode

  for (int p = 0; p < 2; ++p) {

    auto p1 = (p == 0 ? ruler.p1 () : ruler.seg_p1 (ruler.segments () - 1));
    auto p2 = (p == 0 ? ruler.seg_p2 (0) : ruler.p2 ());

    auto v = lay::snap (trans * p1, trans * p2);
    auto q1 = v.first;
    auto q2 = v.second;

    double lu = p1.double_distance (p2);

    draw_ruler (q1, q2, lu, 0.0, sel, false, ant::Object::STY_line, bitmap, renderer, true, true);

  }

  double radius = 0.0, start_angle = 0.0, stop_angle = 0.0;
  db::DPoint center;

  if (! ruler.compute_angle_parameters (radius, center, start_angle, stop_angle)) {
    return;
  }

  double circle_radius = angle_ruler_radius_factor * radius;

  //  draw decorations at start/end

  for (int p = 0; p < 2; ++p) {

    double a = (p == 0 ? start_angle : stop_angle);

    db::DPoint p1 = center + db::DVector (cos (a), sin (a)) * circle_radius;

    auto v = lay::snap (trans * p1, trans * p1);
    db::DVector vn = db::DVector (-sin (a), cos (a));
    auto q1 = v.first + vn * (p == 0 ? 0.0 : -1.0);
    auto q2 = v.second + vn * (p == 0 ? 1.0 : 0.0);

    double lu = abs (circle_radius * (stop_angle - start_angle));

    draw_ruler (q1, q2, lu, 0.0, sel, false, ruler.style (), bitmap, renderer, p == 0, p != 0, true);

  }

  db::DVector rr (circle_radius, circle_radius);
  std::pair <db::DPoint, db::DPoint> v = lay::snap (trans * (center - rr), trans * (center + rr));
  draw_ellipse (v.first, v.second, radius * 2.0, sel, bitmap, renderer, start_angle, stop_angle);

  if (ruler.style () == ant::Object::STY_ruler) {

    //  draw ticks if required - minor at 5 degree, major at 10 degree

    double tick_length = ruler_tick_length / renderer.resolution ();

    double da = 5.0 / 180.0 * M_PI;
    unsigned int major_ticks = 2;

    double n = floor (db::epsilon + std::min (2 * M_PI, stop_angle - start_angle) / da);
    unsigned int ticks = (unsigned int) std::max (1.0, n);

    for (unsigned int i = 0; i <= ticks; ++i) {

      double l = tick_length * ((i % major_ticks) == 0 ? 1.0 : 0.5);

      double a = start_angle + i * da;
      db::DVector tv (cos (a), sin (a));
      db::DPoint p1 = center + tv * circle_radius;

      auto v = lay::snap (trans * p1, trans * p1);
      auto q1 = v.first;
      auto q2 = v.second + tv * l;

      renderer.draw (db::DEdge (q1, q2), 0, bitmap, 0, 0);

    }

  }

  {
    double ta = 0.5 * (stop_angle + start_angle);

    db::DPoint tp = center + db::DVector (cos (ta), sin (ta)) * circle_radius;
    db::DVector tv = db::DVector (-sin (ta), cos (ta));

    auto v = lay::snap (trans * tp, trans * tp);
    auto q1 = v.first + tv;
    auto q2 = v.second - tv;

    double lu = abs (circle_radius * (stop_angle - start_angle));

    draw_text (q1, q2, lu, ruler.text (0), false, ruler.style (), ruler.main_position (), ruler.main_xalign (), ruler.main_yalign (), bitmap, renderer);

  }
}

void
draw_ruler (const ant::Object &ruler, const db::DCplxTrans &trans, bool sel, lay::CanvasPlane *bitmap, lay::Renderer &renderer)
{
  if (ruler.outline () == Object::OL_box) {
    draw_ruler_box (ruler, trans, sel, bitmap, renderer);
  } else if (ruler.outline () == Object::OL_ellipse) {
    draw_ruler_ellipse (ruler, trans, sel, bitmap, renderer);
  } else if (ruler.outline () == Object::OL_angle) {
    draw_ruler_angle (ruler, trans, sel, bitmap, renderer);
  } else if (ruler.outline () == Object::OL_radius) {
    draw_ruler_radius (ruler, trans, sel, bitmap, renderer);
  } else {
    //  other outline styles support segments, so paint them individually
    for (size_t index = 0; index < ruler.segments (); ++index) {
      draw_ruler_segment (ruler, index, trans, sel, bitmap, renderer);
    }
  }
}

static bool
is_selected_by_circle_segment (const ant::Object &ruler, const db::DPoint &pos, double enl, double &distance)
{
  double r = 0.0, a1 = 0.0, a2 = 0.0;
  db::DPoint c;

  bool good;
  if (ruler.outline () == ant::Object::OL_angle) {
    good = ruler.compute_angle_parameters (r, c, a1, a2);
    r *= angle_ruler_radius_factor;
  } else {
    good = ruler.compute_interpolating_circle (r, c, a1, a2);
  }
  if (good && fabs (pos.distance (c) - r) < enl) {

    double a = atan2 ((pos - c).y (), (pos - c).x ()) - 2 * M_PI;
    while (a < a1 - db::epsilon) {
      a += 2 * M_PI;
    }
    if (a < a2 + db::epsilon) {
      distance = std::min (distance, fabs (pos.distance (c) - r));
      return true;
    }

  }

  return false;
}

static bool
is_selected (const ant::Object &ruler, size_t index, const db::DPoint &pos, double enl, double &distance)
{
  ant::Object::outline_type outline = ruler.outline ();

  db::DPoint p1 = ruler.seg_p1 (index), p2 = ruler.seg_p2 (index);
  db::DBox b (p1, p2);

  if (outline == ant::Object::OL_ellipse) {

    //  special handling of the (non-degenerated) ellipse case
    if (b.height () > 1e-6 && b.width () > 1e-6) {

      double dx = (pos.x () - b.center ().x ()) / (b.width () * 0.5);
      double dy = (pos.y () - b.center ().y ()) / (b.height () * 0.5);
      double dd = sqrt (dx * dx + dy * dy);

      if (dd > 1e-6) {
        //  ref is the cutpoint between the ray between pos and the ellipse center and the ellipse itself
        db::DPoint ref = b.center () + db::DVector (dx * b.width () * 0.5 / dd, dy * b.height () * 0.5 / dd);
        double d = ref.distance (pos);
        if (d < enl) {
          distance = std::min (distance, d);
          return true;
        }
      }

      return false;

    }

  }

  //  enlarge this box by some pixels
  b.enlarge (db::DVector (enl, enl));

  if (! b.contains (pos)) {
    return false;
  }

  db::DEdge edges[4];
  unsigned int nedges = 0;

  if (outline == ant::Object::OL_diag ||
      outline == ant::Object::OL_angle ||
      outline == ant::Object::OL_radius ||
      outline == ant::Object::OL_diag_xy ||
      outline == ant::Object::OL_diag_yx) {
    edges [nedges++] = db::DEdge (p1, p2);
  }
  if (outline == ant::Object::OL_xy ||
      outline == ant::Object::OL_diag_xy ||
      outline == ant::Object::OL_box ||
      outline == ant::Object::OL_ellipse) {
    edges [nedges++] = db::DEdge (p1, db::DPoint (p2.x (), p1.y ()));
    edges [nedges++] = db::DEdge (db::DPoint (p2.x (), p1.y ()), p2);
  }
  if (outline == ant::Object::OL_yx ||
      outline == ant::Object::OL_diag_yx ||
      outline == ant::Object::OL_box ||
      outline == ant::Object::OL_ellipse) {
    edges [nedges++] = db::DEdge (p1, db::DPoint (p1.x (), p2.y ()));
    edges [nedges++] = db::DEdge (db::DPoint (p1.x (), p2.y ()), p2);
  }

  for (unsigned int i = 0; i < nedges; ++i) {
    double d = edges [i].distance_abs (pos);
    if (d <= enl) {
      distance = std::min (distance, d);
      return true;
    }
  }

  return false;
}

static bool
is_selected (const ant::Object &ruler, const db::DPoint &pos, double enl, double &distance)
{
  distance = std::numeric_limits<double>::max ();
  bool any = false;

  if (ruler.outline () == ant::Object::OL_box || ruler.outline () == ant::Object::OL_ellipse) {
    return is_selected (ruler, std::numeric_limits<size_t>::max (), pos, enl, distance);
  } else if (ruler.outline () == ant::Object::OL_angle || ruler.outline () == ant::Object::OL_radius) {
    any = is_selected_by_circle_segment (ruler, pos, enl, distance);
  }

  for (size_t index = 0; index < ruler.segments (); ++index) {
    //  NOTE: we check *all* since distance is updated herein.
    if (is_selected (ruler, index, pos, enl, distance)) {
      any = true;
    }
  }
  return any;
}

static bool
is_selected (const ant::Object &ruler, const db::DBox &box, double /*enl*/)
{
  return ruler.box ().inside (box);
}


// -------------------------------------------------------------

View::View (ant::Service *rulers, const ant::Object *ruler, bool selected)
  : lay::ViewObject (rulers->ui ()), 
    mp_rulers (rulers), m_selected (selected), mp_ruler (ruler)
{
  //  .. nothing else ..
}

View::~View ()
{
  //  .. nothing else ..
}

void 
View::transform_by (const db::DCplxTrans &t)
{
  if (m_trans != t) {
    m_trans = t;
    redraw ();
  }
}

void 
View::ruler (const ant::Object *r)
{
  mp_ruler = r;
  redraw ();
}

void 
View::render (const lay::Viewport &vp, lay::ViewObjectCanvas &canvas) 
{ 
  //  .. nothing yet ..
  if (! mp_ruler) {
    return;
  }

  int basic_width = int(0.5 + 1.0 / canvas.resolution ());

  tl::Color c (mp_rulers->color ());
  if (! c.is_valid ()) {
    c = canvas.foreground_color ();
  }

  //  obtain bitmap to render on
  lay::CanvasPlane *plane;
  if (mp_rulers->with_halo ()) {
    std::vector <lay::ViewOp> ops;
    ops.reserve (2);
    //  we use 2 and 3 for the bitmap index. Since selection markers are using 0 and 1, rulers
    //  that are dragged appear in front of them.
    ops.push_back (lay::ViewOp (canvas.background_color ().rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 3 * basic_width, 2));
    ops.push_back (lay::ViewOp (c.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width, 3));
    plane = canvas.plane (ops);
  } else {
    plane = canvas.plane (lay::ViewOp (c.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width));
  }

  draw_ruler (*mp_ruler, vp.trans () * m_trans, m_selected, plane, canvas.renderer ());
}

// -------------------------------------------------------------
//  ant::Service implementation

Service::Service (db::Manager *manager, lay::LayoutViewBase *view)
  : lay::EditorServiceBase (view),
    lay::Drawing (1/*number of planes*/, view->drawings ()),
    db::Object (manager),
    m_halo (true),
    m_snap_mode (lay::AC_Any),
    m_grid (0.001),
    m_grid_snap (false), m_obj_snap (false), m_snap_range (1),
    m_max_number_of_rulers (-1 /*unlimited*/),
    mp_view (view),
    mp_active_ruler (0),
    mp_transient_ruler (0),
    m_drawing (false), m_current (),
    m_move_mode (MoveNone),
    m_seg_index (0),
    m_current_template (0)
{ 
  mp_view->annotations_changed_event.add (this, &Service::annotations_changed);
}

Service::~Service ()
{
  for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
    delete *r;
  }
  m_rulers.clear ();
  clear_transient_selection ();
}

bool 
Service::configure (const std::string &name, const std::string &value)
{
  bool taken = true;

  if (name == cfg_ruler_color) {

    tl::Color color;
    lay::ColorConverter ().from_string (value, color);

    //  make the color available for the dynamic view objects too.
    if (lay::test_and_set (m_color, color)) {
      ui ()->touch ();
    }

  } else if (name == cfg_ruler_halo) {

    bool halo;
    tl::from_string (value, halo);

    //  make the color available for the dynamic view objects too.
    if (lay::test_and_set (m_halo, halo)) {
      ui ()->touch ();
    }

  } else if (name == cfg_ruler_grid_micron) {

    double g = 0;
    tl::from_string (value, g);
    m_grid = g;
    taken = false; // to let others use the grid too.

  } else if (name == cfg_max_number_of_rulers) {

    int n = -1;
    tl::from_string (value, n);
    if (n != m_max_number_of_rulers) {
      m_max_number_of_rulers = n;
      reduce_rulers (n);
    }

  } else if (name == cfg_ruler_snap_range) {

    int n = 0;
    tl::from_string (value, n);
    m_snap_range = n;

  } else if (name == cfg_ruler_obj_snap) {
    tl::from_string (value, m_obj_snap);
  } else if (name == cfg_ruler_grid_snap) {
    tl::from_string (value, m_grid_snap);
  } else if (name == cfg_ruler_snap_mode) {

    lay::angle_constraint_type sm = lay::AC_Any;
    ACConverter ().from_string (value, sm);
    m_snap_mode = sm;

  } else if (name == cfg_ruler_templates) {
    m_ruler_templates = ant::Template::from_string (value); 
  } else if (name == cfg_current_ruler_template) {
    
    int n = 0;
    tl::from_string (value, n);
    m_current_template = n;

  } else {
    lay::EditorServiceBase::configure (name, value);
  }

  return taken;

}

const ant::Template &
Service::current_template () const
{
  if (m_current_template >= m_ruler_templates.size ()) {
    static ant::Template s_def_template;
    return s_def_template;
  } else {
    return m_ruler_templates [m_current_template];
  }
}

void 
Service::config_finalize ()
{
}

void
Service::annotations_changed ()
{
  //  NOTE: right now, we don't differentiate: every annotation change may be a change in an image too.
  //  We just forward this event as a potential image changed event
  annotations_changed_event ();
}

std::vector <lay::ViewOp>
Service::get_view_ops (lay::RedrawThreadCanvas &canvas, tl::Color background, tl::Color foreground, tl::Color /*active*/) const
{
  int basic_width = int(0.5 + 1.0 / canvas.resolution ());

  //  the changing of the view ops is done here since it may depend on the 
  //  background color which might be changed by another configure call later.
  std::vector <lay::ViewOp> view_ops;
  if (m_halo) {
    view_ops.push_back (lay::ViewOp (background.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, 3 * basic_width, 0));
  }
  if (m_color.is_valid ()) {
    view_ops.push_back (lay::ViewOp (m_color.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width, 0));
  } else {
    view_ops.push_back (lay::ViewOp (foreground.rgb (), lay::ViewOp::Copy, 0, 0, 0, lay::ViewOp::Rect, basic_width, 0));
  }

  return view_ops;
}

void 
Service::clear_highlights ()
{
  for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
    (*r)->visible (false);
  }
}

void 
Service::restore_highlights ()
{
  for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
    (*r)->visible (true);
  }
}

void 
Service::highlight (unsigned int n)
{
  for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
    (*r)->visible (n-- == 0);
  }
}

void 
Service::clear_rulers ()
{
  drag_cancel ();
  reduce_rulers (0);
}

double
Service::catch_distance ()
{
  return double (view ()->search_range ()) / ui ()->mouse_event_trans ().mag ();
}

double
Service::catch_distance_box ()
{
  return double (view ()->search_range_box ()) / ui ()->mouse_event_trans ().mag ();
}

void
Service::drag_cancel () 
{
  if (m_drawing) {
    ui ()->ungrab_mouse (this);
    m_drawing = false;
  }

  if (mp_active_ruler) {
    delete mp_active_ruler;
    mp_active_ruler = 0;
  }
}

int
Service::insert_ruler (const ant::Object &ruler, bool limit_number)
{
  //  determine the last id
  int idmax = -1;
  for (lay::AnnotationShapes::iterator r = mp_view->annotation_shapes ().begin (); r != mp_view->annotation_shapes ().end (); ++r) {
    const ant::Object *robj = dynamic_cast <const ant::Object *> (r->ptr ());
    if (robj) {
      if (robj->id () > idmax) {
        idmax = robj->id ();
      }
    }
  }

  //  create the ruler from the template
  ant::Object *new_ruler = new ant::Object (ruler);
  int new_id = idmax + 1;
  new_ruler->id (new_id);
  mp_view->annotation_shapes ().insert (db::DUserObject (new_ruler));

  //  delete surplus rulers
  if (limit_number) {
    reduce_rulers (m_max_number_of_rulers);
  }

  return new_id;
}

static bool
dragging_what_seg (const ant::Object *robj, const db::DBox &search_dbox, ant::Service::MoveMode &mode, db::DPoint &p1, size_t index)
{
  ant::Object::outline_type outline = robj->outline ();

  db::DPoint p12, p21;
  bool has_p12 = false, has_p21 = false;

  db::DPoint p11 = robj->seg_p1 (index), p22 = robj->seg_p2 (index);
  db::DPoint c = p11 + (p22 - p11) * 0.5;

  if (outline == ant::Object::OL_xy || outline== ant::Object::OL_diag_xy || outline == ant::Object::OL_box) {
    p12 = db::DPoint (p22.x (), p11.y ());
    has_p12 = true;
  }

  if (outline == ant::Object::OL_yx || outline == ant::Object::OL_diag_yx || outline == ant::Object::OL_box) {
    p21 = db::DPoint (p11.x (), p22.y ());
    has_p21 = true;
  }

  if (outline == ant::Object::OL_ellipse) {
    db::DVector d = (p22 - p11) * 0.5;
    p12 = c + db::DVector (d.x (), -d.y ());
    p21 = c + db::DVector (-d.x (), d.y ());
    has_p12 = true;
    has_p21 = true;
  }

  //  HINT: this was implemented returning a std::pair<MoveMode, db::DPoint>, but
  //  I was not able to get it to work in gcc 4.1.2 in -O3 mode ...

  if (search_dbox.contains (p11)) {
    p1 = p11;
    mode = ant::Service::MoveP1;
    return true;
  }
  if (search_dbox.contains (p22)) {
    p1 = p22;
    mode = ant::Service::MoveP2;
    return true;
  }
  if (has_p12 && search_dbox.contains (p12)) {
    p1 = p12;
    mode = ant::Service::MoveP12;
    return true;
  }
  if (has_p21 && search_dbox.contains (p21)) {
    p1 = p21;
    mode = ant::Service::MoveP21;
    return true;
  }
  if (has_p12 && search_dbox.touches (db::DBox (p12, p22))) {
    p1 = db::DPoint (p12.x (), search_dbox.center ().y ());
    mode = ant::Service::MoveP2X;
    return true;
  }
  if (has_p21 && search_dbox.touches (db::DBox (p21, p11))) {
    p1 = db::DPoint (p21.x (), search_dbox.center ().y ());
    mode = ant::Service::MoveP1X;
    return true;
  }
  if (has_p12 && search_dbox.touches (db::DBox (p12, p11))) {
    p1 = db::DPoint (search_dbox.center ().x (), p12.y ());
    mode = ant::Service::MoveP1Y;
    return true;
  }
  if (has_p21 && search_dbox.touches (db::DBox (p21, p22))) {
    p1 = db::DPoint (search_dbox.center ().x (), p21.y ());
    mode = ant::Service::MoveP2Y;
    return true;
  }

  return false;
}

/**
 *  @brief Helper function to determine which move mode to choose given a certain search box and ant::Object
 */
static bool
dragging_what (const ant::Object *robj, const db::DBox &search_dbox, ant::Service::MoveMode &mode, db::DPoint &p1, size_t &index)
{
  ant::Object::outline_type outline = robj->outline ();

  if (outline == ant::Object::OL_box || outline == ant::Object::OL_ellipse) {
    index = std::numeric_limits<size_t>::max ();
    return dragging_what_seg (robj, search_dbox, mode, p1, index);
  }

  for (index = 0; index < robj->segments (); ++index) {
    if (dragging_what_seg (robj, search_dbox, mode, p1, index)) {
      return true;
    }
  }
  return false;
}

bool 
Service::begin_move (lay::Editable::MoveMode mode, const db::DPoint &p, lay::angle_constraint_type /*ac*/)
{
  //  cancel any pending move or drag operations, reset mp_active_ruler
  ui ()->drag_cancel (); // KLUDGE: every service does this to the same service manager

  clear_transient_selection ();

  //  choose move mode
  if (mode == lay::Editable::Selected) {

    m_move_mode = MoveSelected;
    m_p1 = p;
    m_trans = db::DTrans (db::DPoint () - m_p1);

    for (std::vector <ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
      (*r)->thaw ();
    }
    return false;

  } else if (mode == lay::Editable::Partial) {
  
    m_move_mode = MoveNone;
    m_seg_index = 0;

    //  compute search box
    double l = catch_distance ();
    db::DBox search_dbox = db::DBox (p, p).enlarged (db::DVector (l, l));

    //  point selection: look for the "closest" ruler

    double dmin = std::numeric_limits <double>::max ();

    const ant::Object *robj_min = 0;
    for (std::map<obj_iterator, unsigned int>::const_iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
      const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r->first).ptr ());
      if (robj) {
        double d;
        if (is_selected (*robj, p, l, d)) {
          if (! robj_min || d < dmin) {
            robj_min = robj;
            dmin = d;
          }
        }
      }
    }

    //  further investigate what part to drag

    for (std::map<obj_iterator, unsigned int>::const_iterator r = m_selected.begin (); r != m_selected.end (); ++r) {

      obj_iterator ri = r->first;
      const ant::Object *robj = dynamic_cast <const ant::Object *> ((*ri).ptr ());
      if (robj && (! robj_min || robj == robj_min)) {
        
        if (dragging_what (robj, search_dbox, m_move_mode, m_p1, m_seg_index) && m_move_mode != MoveRuler) {
          
          //  found anything: make the moved ruler the selection
          clear_selection ();
          m_selected.insert (std::make_pair (ri, 0));
          m_current = *robj;
          m_original = m_current; 
          m_rulers.push_back (new ant::View (this, &m_current, true));
          m_rulers.back ()->thaw ();
          return true;
          
        }

      }
      
    }

    //  nothing was found
    return false;

  } else if (mode == lay::Editable::Any) {
  
    m_move_mode = MoveNone;

    //  compute search box
    double l = catch_distance ();
    db::DBox search_dbox = db::DBox (p, p).enlarged (db::DVector (l, l));

    //  point selection: look for the "closest" ruler

    double dmin = std::numeric_limits <double>::max ();

    lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
    const ant::Object *robj_min = 0;
    while (! r.at_end ()) {
      const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
      if (robj) {
        double d;
        if (is_selected (*robj, p, l, d)) {
          if (! robj_min || d < dmin) {
            robj_min = robj;
            dmin = d;
          }
        }
      }
      ++r;
    }

    //  further investigate what part to drag

    r = mp_view->annotation_shapes ().begin_touching (search_dbox);

    while (m_move_mode == MoveNone && ! r.at_end ()) {

      const ant::Object *robj = dynamic_cast <const ant::Object *> ((*r).ptr ());
      if (robj && (! robj_min || robj == robj_min)) {

        if (dragging_what (robj, search_dbox, m_move_mode, m_p1, m_seg_index)) {

          //  found anything: make the moved ruler the selection
          clear_selection ();
          m_selected.insert (std::make_pair (mp_view->annotation_shapes ().iterator_from_pointer (&*r), 0));
          m_current = *robj;
          m_original = m_current;
          m_rulers.push_back (new ant::View (this, &m_current, true));
          m_rulers.back ()->thaw ();
          return true;

        }

      }

      ++r;

    }

    //  nothing was found
    return false;

  } else {
    return false;
  }
}

void
Service::move_transform (const db::DPoint &p, db::DFTrans tr, lay::angle_constraint_type /*ac*/)
{
  if (m_rulers.empty () || m_selected.empty ()) {
    return;
  }

  if (m_move_mode == MoveRuler) {

    db::DVector dp = p - db::DPoint ();

    m_original.transform (db::DTrans (m_p1 - db::DPoint ()) * db::DTrans (tr) * db::DTrans (db::DPoint () - m_p1));
    m_current.transform (db::DTrans (dp) * db::DTrans (tr) * db::DTrans (-dp));

    //  display current rulers' parameters
    show_message ();

    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveSelected) {

    m_trans *= db::DTrans (m_p1 - db::DPoint ()) * db::DTrans (tr) * db::DTrans (db::DPoint () - m_p1);

    for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
      (*r)->transform_by (db::DCplxTrans (m_trans));
    }

  }

}

void 
Service::move (const db::DPoint &p, lay::angle_constraint_type ac)
{
  if (m_rulers.empty () || m_selected.empty ()) {
    return;
  }

  if (m_move_mode == MoveP1) {
    
    m_current.seg_p1 (m_seg_index, snap2 (m_p1, p, &m_current, ac).second);
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP2) {
    
    m_current.seg_p2 (m_seg_index, snap2 (m_p1, p, &m_current, ac).second);
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP12) {
    
    db::DPoint p12 = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p1 (m_seg_index, db::DPoint (m_current.seg_p1 (m_seg_index).x(), p12.y ()));
    m_current.seg_p2 (m_seg_index, db::DPoint (p12.x (), m_current.seg_p2 (m_seg_index).y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP21) {
    
    db::DPoint p21 = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p1 (m_seg_index, db::DPoint (p21.x (), m_current.seg_p1 (m_seg_index).y ()));
    m_current.seg_p2 (m_seg_index, db::DPoint (m_current.seg_p2 (m_seg_index).x(), p21.y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP1X) {
    
    db::DPoint pc = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p1 (m_seg_index, db::DPoint (pc.x (), m_current.seg_p1 (m_seg_index).y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP2X) {
    
    db::DPoint pc = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p2 (m_seg_index, db::DPoint (pc.x (), m_current.seg_p2 (m_seg_index).y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP1Y) {
    
    db::DPoint pc = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p1 (m_seg_index, db::DPoint (m_current.seg_p1 (m_seg_index).x (), pc.y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveP2Y) {
    
    db::DPoint pc = snap2 (m_p1, p, &m_current, ac).second;
    m_current.seg_p2 (m_seg_index, db::DPoint (m_current.seg_p2 (m_seg_index).x (), pc.y ()));
    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveRuler) {

    //  try two ways of snapping
    db::DVector dp = lay::snap_angle (p - m_p1, ac == lay::AC_Global ? m_snap_mode : ac);

    db::DPoint p1 = m_original.p1 () + dp;
    db::DPoint p2 = m_original.p2 () + dp;

    std::pair<bool, db::DPoint> r1 = snap1 (p1, m_obj_snap && m_original.snap ());
    db::DPoint q1 = r1.second;
    std::pair<bool, db::DPoint> r2 = snap1 (p2, m_obj_snap && m_original.snap ());
    db::DPoint q2 = r2.second;

    if ((!r2.first && r1.first) || ((r1.first || (!r1.first && !r2.first)) && q1.distance (p1) < q2.distance (p2))) {
      q2 = q1 + (m_original.p2 () - m_original.p1 ());
    } else {
      q1 = q2 + (m_original.p1 () - m_original.p2 ());
    }

    m_current.p1 (q1);
    m_current.p2 (q2);

    m_rulers [0]->redraw ();

  } else if (m_move_mode == MoveSelected) {

    db::DVector dp = p - m_p1;
    //  round the drag distance to grid if required: this is the least we can do in this case
    if (m_grid_snap) {
      dp = db::DVector (lay::snap (dp.x (), m_grid), lay::snap (dp.y (), m_grid));
    } 

    dp = lay::snap_angle (dp, ac == lay::AC_Global ? m_snap_mode : ac);

    m_trans = db::DTrans (dp + (m_p1 - db::DPoint ()) - m_trans.disp ()) * m_trans * db::DTrans (db::DPoint () - m_p1);

    for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
      (*r)->transform_by (db::DCplxTrans (m_trans));
    }

  }

  if (m_move_mode != MoveSelected) {
    show_message ();
  }

}

void 
Service::show_message ()
{
  //  display current rulers' parameters
  std::string pos = std::string ("lx: ") + tl::micron_to_string (m_current.p2 ().x () - m_current.p1 ().x ()) 
                      + "  ly: " + tl::micron_to_string (m_current.p2 ().y () - m_current.p1 ().y ()) 
                      + "  l: " + tl::micron_to_string (m_current.p2 ().distance (m_current.p1 ()));
  view ()->message (pos);
}

void 
Service::end_move (const db::DPoint &, lay::angle_constraint_type)
{
  if (! m_rulers.empty () && ! m_selected.empty ()) {

    if (m_move_mode == MoveSelected) {

      //  replace the rulers that were moved:
      for (std::map<obj_iterator, unsigned int>::const_iterator s = m_selected.begin (); s != m_selected.end (); ++s) {

        const ant::Object *robj = dynamic_cast<const ant::Object *> (s->first->ptr ());
        if (robj) {

          //  compute moved object and replace
          ant::Object *rnew = new ant::Object (*robj);
          rnew->transform (m_trans);
          int new_id = rnew->id ();
          mp_view->annotation_shapes ().replace (s->first, db::DUserObject (rnew));
          annotation_changed_event (new_id);

        }

      }

      //  and make selection "visible"
      selection_to_view ();

    } else if (m_move_mode != MoveNone) {

      //  replace the ruler that was moved
      m_current.clean_points ();
      mp_view->annotation_shapes ().replace (m_selected.begin ()->first, db::DUserObject (new ant::Object (m_current)));
      annotation_changed_event (m_current.id ());

      //  clear the selection (that was artifically created before)
      clear_selection ();

    }

  }

  //  termine the operation
  m_move_mode = MoveNone;

}

void
Service::selection_to_view ()
{
  clear_transient_selection ();
  annotation_selection_changed_event ();

  //  the selection objects need to be recreated since we destroyed the old rulers
  for (std::vector<ant::View *>::iterator r = m_rulers.begin (); r != m_rulers.end (); ++r) {
    delete *r;
  }
  m_rulers.clear ();
  m_rulers.reserve (m_selected.size ());
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    r->second = (unsigned int) m_rulers.size ();
    const ant::Object *robj = dynamic_cast<const ant::Object *> (r->first->ptr ());
    m_rulers.push_back (new ant::View (this, robj, true /*selected*/));
  }
}

db::DBox 
Service::selection_bbox ()
{
  db::DBox box;
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    const ant::Object *robj = dynamic_cast<const ant::Object *> (r->first->ptr ());
    if (robj) {
      box += robj->box ();
    }
  }
  return box;
}

void 
Service::transform (const db::DCplxTrans &trans)
{
  //  replace the rulers that were transformed:
  for (std::map<obj_iterator, unsigned int>::const_iterator s = m_selected.begin (); s != m_selected.end (); ++s) {

    const ant::Object *robj = dynamic_cast<const ant::Object *> (s->first->ptr ());
    if (robj) {

      //  compute transformed object and replace
      ant::Object *rnew = new ant::Object (*robj);
      rnew->transform (trans);
      mp_view->annotation_shapes ().replace (s->first, db::DUserObject (rnew));
      annotation_changed_event (rnew->id ());

    }

  }

  selection_to_view ();
}

void 
Service::edit_cancel () 
{
  //  Cancel any move operation
  if (m_move_mode != MoveNone) {

    m_move_mode = MoveNone;
    selection_to_view ();

  }
}

bool 
Service::mouse_press_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{
  return mouse_click_event (p, buttons, prio);
}

void
Service::finish_drawing ()
{
  //  create the ruler object

  //  begin the transaction
  if (manager ()) {
    tl_assert (! manager ()->transacting ());
    manager ()->transaction (tl::to_string (tr ("Create ruler")));
  }

  show_message ();

  insert_ruler (ant::Object (m_current.points (), 0, current_template ()), true);

  //  stop dragging
  drag_cancel ();
  clear_transient_selection ();

  //  end the transaction
  if (manager ()) {
    manager ()->commit ();
  }
}

bool
Service::mouse_double_click_event (const db::DPoint & /*p*/, unsigned int buttons, bool prio)
{
  if (m_drawing && prio && (buttons & lay::LeftButton) != 0) {

    //  ends the current ruler (specifically in multi-segment mode)
    finish_drawing ();
    return true;

  }

  return false;
}

bool
Service::mouse_click_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{
  if (prio && (buttons & lay::LeftButton) != 0) {

    const ant::Template &tpl = current_template ();

    if (! m_drawing) {

      //  cancel any edit operations so far
      m_move_mode = MoveNone;

      //  reset selection
      clear_selection ();
         
      //  set the maximum number of rulers minus 1 to account for the new ruler
      //  and clear surplus rulers
      reduce_rulers (m_max_number_of_rulers - 1);

      //  create and start dragging the ruler
      
      if (tpl.mode () == ant::Template::RulerSingleClick) {

        db::DPoint pt = snap1 (p, m_obj_snap && tpl.snap ()).second;

        //  begin the transaction
        if (manager ()) {
          tl_assert (! manager ()->transacting ());
          manager ()->transaction (tl::to_string (tr ("Create ruler")));
        }

        m_current = ant::Object (pt, pt, 0, tpl);
        show_message ();

        insert_ruler (m_current, true);

        //  end the transaction
        if (manager ()) {
          manager ()->commit ();
        }

      } else if (tpl.mode () == ant::Template::RulerAutoMetric) {

        //  for auto-metric we need some cutline constraint - any or global won't do.
        lay::angle_constraint_type ac = ac_from_buttons (buttons);
        if (ac == lay::AC_Global) {
          ac = tpl.angle_constraint ();
        }
        if (ac == lay::AC_Global) {
          ac = m_snap_mode;
        }
        if (ac == lay::AC_Global) {
          ac = lay::AC_Diagonal;
        }

        db::DVector g;
        if (m_grid_snap) {
          g = db::DVector (m_grid, m_grid);
        }

        double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (m_snap_range);
        snap_range *= 0.5;

        lay::TwoPointSnapToObjectResult ee = lay::obj_snap2 (mp_view, p, g, ac, snap_range, snap_range * 1000.0);
        if (ee.any) {

          //  begin the transaction
          if (manager ()) {
            tl_assert (! manager ()->transacting ());
            manager ()->transaction (tl::to_string (tr ("Create ruler")));
          }

          m_current = ant::Object (ee.first, ee.second, 0, tpl);
          show_message ();

          insert_ruler (m_current, true);

          //  end the transaction
          if (manager ()) {
            manager ()->commit ();
          }

        }

      } else {

        m_p1 = snap1 (p, m_obj_snap && tpl.snap ()).second;

        //  NOTE: generating the ruler this way makes sure we have two points
        ant::Object::point_list pts;
        m_current = ant::Object (pts, 0, tpl);
        pts.push_back (m_p1);
        pts.push_back (m_p1);
        m_current.set_points_exact (pts);

        show_message ();

        if (mp_active_ruler) {
          delete mp_active_ruler;
        }
        mp_active_ruler = new ant::View (this, &m_current, false /*not selected*/);
        mp_active_ruler->thaw ();
        m_drawing = true;

        ui ()->grab_mouse (this, false);

      }

    } else if (tpl.mode () == ant::Template::RulerMultiSegment || tpl.mode () == ant::Template::RulerThreeClicks) {

      ant::Object::point_list pts = m_current.points ();
      tl_assert (! pts.empty ());

      if (tpl.mode () == ant::Template::RulerThreeClicks && pts.size () == 3) {

        finish_drawing ();

      } else {

        //  add a new point
        m_p1 = pts.back ();

        pts.push_back (m_p1);
        m_current.set_points_exact (pts);

      }

    } else {

      finish_drawing ();

    }

    return true;

  }

  return false;
}

ant::Object
Service::create_measure_ruler (const db::DPoint &pt, lay::angle_constraint_type ac)
{
  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (m_snap_range);
  snap_range *= 0.5;

  ant::Template tpl;

  lay::TwoPointSnapToObjectResult ee = lay::obj_snap2 (mp_view, pt, db::DVector (), ac, snap_range, snap_range * 1000.0);
  if (ee.any) {
    return ant::Object (ee.first, ee.second, 0, tpl);
  } else {
    return ant::Object (pt, pt, 0, tpl);
  }
}

bool
Service::mouse_move_event (const db::DPoint &p, unsigned int buttons, bool prio) 
{
  if (prio) {

    lay::PointSnapToObjectResult snap_details;
    if (m_drawing) {
      snap_details = snap2_details (m_p1, p, mp_active_ruler->ruler (), ac_from_buttons (buttons));
    } else {
      const ant::Template &tpl = current_template ();
      snap_details = snap1_details (p, m_obj_snap && tpl.snap ());
    }

    mouse_cursor_from_snap_details (snap_details);

  }

  if (m_drawing && prio) {

    set_cursor (lay::Cursor::cross);

    //  NOTE: we use the direct access path so we do not encounter cleanup by the p1 and p2 setters
    //  otherwise we risk manipulating p1 too.
    ant::Object::point_list pts = m_current.points ();
    if (! pts.empty ()) {
      pts.back () = snap2 (m_p1, p, mp_active_ruler->ruler (), ac_from_buttons (buttons)).second;
    }
    m_current.set_points_exact (pts);

    mp_active_ruler->redraw ();
    show_message ();

  }

  return false;
}

void 
Service::deactivated ()
{
  lay::EditorServiceBase::deactivated ();

  drag_cancel ();
  clear_transient_selection ();
}

lay::PointSnapToObjectResult
Service::snap1_details (const db::DPoint &p, bool obj_snap)
{
  db::DVector g;
  if (m_grid_snap) {
    g = db::DVector (m_grid, m_grid);
  }

  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (m_snap_range);
  return lay::obj_snap (obj_snap ? mp_view : 0, p, g, snap_range);
}

std::pair<bool, db::DPoint>
Service::snap1 (const db::DPoint &p, bool obj_snap)
{
  lay::PointSnapToObjectResult res = snap1_details (p, obj_snap);
  return std::make_pair (res.object_snap != lay::PointSnapToObjectResult::NoObject, res.snapped_point);
}


lay::PointSnapToObjectResult
Service::snap2_details (const db::DPoint &p1, const db::DPoint &p2, const ant::Object *obj, lay::angle_constraint_type ac)
{
  db::DVector g;
  if (m_grid_snap) {
    g = db::DVector (m_grid, m_grid);
  }

  double snap_range = ui ()->mouse_event_trans ().inverted ().ctrans (m_snap_range);
  lay::angle_constraint_type snap_mode = ac == lay::AC_Global ? (obj->angle_constraint () == lay::AC_Global ? m_snap_mode : obj->angle_constraint ()) : ac;

  return lay::obj_snap (m_obj_snap && obj->snap () ? mp_view : 0, p1, p2, g, snap_mode, snap_range);
}

std::pair <bool, db::DPoint>
Service::snap2 (const db::DPoint &p1, const db::DPoint &p2, const ant::Object *obj, lay::angle_constraint_type ac)
{
  lay::PointSnapToObjectResult res = snap2_details (p1, p2, obj, ac);
  return std::make_pair (res.object_snap != lay::PointSnapToObjectResult::NoObject, res.snapped_point);
}


struct RulerIdComp 
{
  bool operator() (const lay::AnnotationShapes::iterator &a, const lay::AnnotationShapes::iterator &b) const
  {
    return dynamic_cast<const ant::Object &>(*a->ptr ()).id () < dynamic_cast<const ant::Object &>(*b->ptr ()).id ();
  }
};

void
Service::reduce_rulers (int num)
{
  clear_transient_selection ();

  lay::AnnotationShapes::iterator rfrom = mp_view->annotation_shapes ().begin (); 
  lay::AnnotationShapes::iterator rto = mp_view->annotation_shapes ().end ();

  size_t n = std::distance (rfrom, rto);
  if (num >= 0 && int (n) > num) {

    //  clear selection
    clear_selection ();

    //  extract all rulers and other objects

    std::vector <lay::AnnotationShapes::iterator> positions;
    positions.reserve (n);

    for (lay::AnnotationShapes::iterator r = rfrom; r != rto; ++r) {
      const ant::Object *robj = dynamic_cast <const ant::Object *> (r->ptr ());
      if (robj) {
        positions.push_back (r);
      }
    }

    //  sort so we find the ones that are too old, remove them and sort the 
    //  remaining positions
    tl::sort (positions.begin (), positions.end (), RulerIdComp ());  // HINT: must be tl::sort, not std::sort because gcc 3.2.3 has some strange namespace resolution problems ..
    positions.erase (positions.begin () + (positions.size () - num), positions.end ());
    tl::sort (positions.begin (), positions.end ());  // HINT: must be tl::sort, not std::sort because gcc 3.2.3 has some strange namespace resolution problems ..

    //  now we can erase these positions
    mp_view->annotation_shapes ().erase_positions (positions.begin (), positions.end ());

  }

}

void 
Service::cut ()
{
  if (has_selection ()) {

    //  copy & delete the selected rulers
    copy_selected ();
    del_selected ();

  }
}

void 
Service::copy ()
{
  //  copy the selected rulers
  copy_selected ();
}

void
Service::copy_selected ()
{
  //  extract all selected rulers and paste in "micron" space
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    r->second = (unsigned int) m_rulers.size ();
    const ant::Object *robj = dynamic_cast<const ant::Object *> (r->first->ptr ());
    if (robj) {
      db::Clipboard::instance () += new db::ClipboardValue<ant::Object> (*robj);
    }
  }
}

void 
Service::paste ()
{
  if (db::Clipboard::instance ().begin () != db::Clipboard::instance ().end ()) {

    //  determine the last id
    int idmax = -1;
    for (lay::AnnotationShapes::iterator r = mp_view->annotation_shapes ().begin (); r != mp_view->annotation_shapes ().end (); ++r) {
      const ant::Object *robj = dynamic_cast <const ant::Object *> (r->ptr ());
      if (robj && robj->id () > idmax) {
        idmax = robj->id ();
      }
    }

    for (db::Clipboard::iterator c = db::Clipboard::instance ().begin (); c != db::Clipboard::instance ().end (); ++c) {
      const db::ClipboardValue<ant::Object> *value = dynamic_cast<const db::ClipboardValue<ant::Object> *> (*c);
      if (value) {
        ant::Object *ruler = new ant::Object (value->get ());
        ruler->id (++idmax);
        mp_view->annotation_shapes ().insert (db::DUserObject (ruler));
      }
    }

  }

}

void 
Service::del ()
{
  if (has_selection ()) {

    //  delete the selected rulers
    del_selected ();

  }
}

void
Service::del_selected ()
{
  //  positions will hold a set of iterators that are to be erased
  std::vector <lay::AnnotationShapes::iterator> positions;
  positions.reserve (m_selected.size ());
  for (std::map<obj_iterator, unsigned int>::iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    positions.push_back (r->first);
  }

  //  clear selection
  clear_selection ();

  //  erase all and insert the ones that we want to keep 
  tl::sort (positions.begin (), positions.end ());  // HINT: must be tl::sort, not std::sort because gcc 3.2.3 has some strange namespace resolution problems ..
  mp_view->annotation_shapes ().erase_positions (positions.begin (), positions.end ());
}

bool
Service::has_selection ()
{
  return ! m_selected.empty ();
}

size_t
Service::selection_size ()
{
  return m_selected.size ();
}

bool
Service::has_transient_selection ()
{
  return mp_transient_ruler != 0;
}

bool 
Service::select (obj_iterator obj, lay::Editable::SelectionMode mode)
{
  if (mode == lay::Editable::Replace || mode == lay::Editable::Add) {
    //  select
    if (m_selected.find (obj) == m_selected.end ()) {
      m_selected.insert (std::make_pair (obj, 0));
      return true;
    }
  } else if (mode == lay::Editable::Reset) {
    //  unselect
    if (m_selected.find (obj) != m_selected.end ()) {
      m_selected.erase (obj);
      return true;
    }
  } else {
    //  invert selection
    if (m_selected.find (obj) != m_selected.end ()) {
      m_selected.erase (obj);
    } else {
      m_selected.insert (std::make_pair (obj, 0));
    }
    return true;
  }
  return false;
}

void
Service::clear_selection ()
{
  select (db::DBox (), lay::Editable::Reset);
}

double
Service::click_proximity (const db::DPoint &pos, lay::Editable::SelectionMode mode)
{
  //  compute search box
  double l = catch_distance ();
  db::DBox search_dbox = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const std::map<obj_iterator, unsigned int> *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selected;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  //  point selection: look for the "closest" ruler
  double dmin = std::numeric_limits <double>::max ();
  bool any_found = false;

  lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
  while (! r.at_end ()) {
    const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
    if (robj && (! exclude || exclude->find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == exclude->end ())) {
      double d;
      if (is_selected (*robj, pos, l, d)) {
        if (! any_found || d < dmin) {
          dmin = d;
        }
        any_found = true;
      }
    }
    ++r;
  }

  //  return the proximity value
  if (any_found) {
    return dmin;
  } else {
    return lay::Editable::click_proximity (pos, mode); 
  } 
}

bool
Service::transient_select (const db::DPoint &pos)
{
  clear_transient_selection ();

  //  if in move mode (which also receives transient_select requests) the move will take the selection,
  //  hence don't do a transient selection if there is one.
  if (view ()->has_selection () && view ()->is_move_mode ()) {
    return false;
  }

  bool any_selected = false;

  //  compute search box
  double l = catch_distance ();
  db::DBox search_dbox = db::DBox (pos, pos).enlarged (db::DVector (l, l));

  //  point selection: look for the "closest" ruler
  double dmin = std::numeric_limits <double>::max ();

  lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
  lay::AnnotationShapes::touching_iterator rmin (r);
  while (! r.at_end ()) {
    const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
    if (robj && m_previous_selection.find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == m_previous_selection.end ()) {
      double d;
      if (is_selected (*robj, pos, l, d)) {
        if (! any_selected || d < dmin) {
          rmin = r;
          dmin = d;
        }
        any_selected = true;
      }
    }
    ++r;
  }

  //  create the transient marker for the object found 
  if (any_selected) {
    const ant::Object *robj = dynamic_cast<const ant::Object *> ((*rmin).ptr ());
    //  HINT: there is no special style for "transient selection on rulers"
    mp_transient_ruler = new ant::View (this, robj, true /*selected*/);
  }

  if (any_selected && ! editables ()->has_selection ()) {
    display_status (true);
  }

  return any_selected;
}

void
Service::clear_transient_selection ()
{
  if (mp_transient_ruler) {
    delete mp_transient_ruler;
    mp_transient_ruler = 0;
  }
}

void
Service::transient_to_selection ()
{
  if (mp_transient_ruler) {
    for (lay::AnnotationShapes::iterator r = mp_view->annotation_shapes ().begin (); r != mp_view->annotation_shapes ().end (); ++r) {
      const ant::Object *robj = dynamic_cast <const ant::Object *> (r->ptr ());
      if (robj == mp_transient_ruler->ruler ()) {
        m_selected.insert (std::make_pair (r, 0));
        selection_to_view ();
        return;
      }
    }
  }
}

void
Service::clear_previous_selection ()
{
  m_previous_selection.clear ();
}

bool
Service::select (const db::DBox &box, lay::Editable::SelectionMode mode)
{
  bool needs_update = false;
  bool any_selected = false;

  //  clear before unless "add" is selected
  if (mode == lay::Editable::Replace) {
    if (! m_selected.empty ()) {
      m_selected.clear ();
      needs_update = true;
    }
  }

  //  for single-point selections either exclude the current selection or the
  //  accumulated previous selection from the search.
  const std::map<obj_iterator, unsigned int> *exclude = 0;
  if (mode == lay::Editable::Replace) {
    exclude = &m_previous_selection;
  } else if (mode == lay::Editable::Add) {
    exclude = &m_selected;
  } else if (mode == lay::Editable::Reset) {
    //  TODO: the finder should favor the current selection in this case.
  }

  if (box.empty ()) {

    //  unconditional selection
    if (mode == lay::Editable::Reset) {
      if (! m_selected.empty ()) {
        m_selected.clear ();
        needs_update = true;
      }
    } else {

      lay::AnnotationShapes::iterator rfrom = mp_view->annotation_shapes ().begin (); 
      lay::AnnotationShapes::iterator rto = mp_view->annotation_shapes ().end ();

      //  extract all rulers
      for (lay::AnnotationShapes::iterator r = rfrom; r != rto; ++r) {
        const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
        if (robj) {
          any_selected = true;
          if (select (r, mode)) {
            needs_update = true;
          }
        }
      }
    }

  } else {

    //  compute search box
    double l = box.is_point () ? catch_distance () : catch_distance_box ();
    db::DBox search_dbox = box.enlarged (db::DVector (l, l));

    if (! box.is_point ()) {

      //  box-selection
      lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
      while (! r.at_end ()) {
        const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
        if (robj && (! exclude || exclude->find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == exclude->end ())) {
          if (is_selected (*robj, box, l)) {
            any_selected = true;
            if (select (mp_view->annotation_shapes ().iterator_from_pointer (&*r), mode)) {
              needs_update = true;
            }
          }
        }
        ++r;
      }

    } else {

      //  point selection: look for the "closest" ruler
      double dmin = std::numeric_limits <double>::max ();

      lay::AnnotationShapes::touching_iterator r = mp_view->annotation_shapes ().begin_touching (search_dbox);
      lay::AnnotationShapes::touching_iterator rmin (r);
      while (! r.at_end ()) {
        const ant::Object *robj = dynamic_cast<const ant::Object *> ((*r).ptr ());
        if (robj && (! exclude || exclude->find (mp_view->annotation_shapes ().iterator_from_pointer (&*r)) == exclude->end ())) {
          double d;
          if (is_selected (*robj, box.p1 (), l, d)) {
            if (! any_selected || d < dmin) {
              rmin = r;
              dmin = d;
            }
            any_selected = true;
          }
        }
        ++r;
      }

      //  select the one that was found
      if (any_selected) {
        select (mp_view->annotation_shapes ().iterator_from_pointer (&*rmin), mode);
        m_previous_selection.insert (std::make_pair (mp_view->annotation_shapes ().iterator_from_pointer (&*rmin), mode));
        needs_update = true;
      }

    }

  }

  //  if required, update the list of ruler objects to display the selection
  if (needs_update) {
    selection_to_view ();
  }

  if (any_selected) {
    display_status (false);
  }

  //  return true if at least one element was selected
  return any_selected;
}

void 
Service::display_status (bool transient)
{
  View *selected_view = transient ? mp_transient_ruler : (m_rulers.size () == 1 ? m_rulers [0] : 0);
  if (! selected_view) {
    view ()->message (std::string ());
  } else {

    const ant::Object *ruler = selected_view->ruler ();

    std::string msg;
    if (! transient) {
      msg = tl::to_string (tr ("selected: "));
    }
    if (ruler->segments () > 1) {
      msg += tl::sprintf (tl::to_string (tr ("annotation(d=%s x=%s y=%s ...)")), ruler->text (0), ruler->text_x (0), ruler->text_y (0));
    } else {
      msg += tl::sprintf (tl::to_string (tr ("annotation(d=%s x=%s y=%s)")), ruler->text (0), ruler->text_x (0), ruler->text_y (0));
    }
    view ()->message (msg);

  }
}

#if defined(HAVE_QT)
std::vector<lay::PropertiesPage *>
Service::properties_pages (db::Manager *manager, QWidget *parent)
{
  std::vector<lay::PropertiesPage *> pages;
  pages.push_back (new PropertiesPage (this, manager, parent));
  return pages;
}
#endif

void 
Service::get_selection (std::vector <obj_iterator> &sel) const
{
  sel.clear ();
  sel.reserve (m_selected.size ());

  //  positions will hold a set of iterators that are to be erased
  for (std::map<obj_iterator, unsigned int>::const_iterator r = m_selected.begin (); r != m_selected.end (); ++r) {
    sel.push_back (r->first);
  }
}

void
Service::delete_ruler (obj_iterator pos)
{
  //  delete the object
  m_selected.erase (pos);
  mp_view->annotation_shapes ().erase (pos);

  //  and make selection "visible"
  selection_to_view ();
}

void
Service::change_ruler (obj_iterator pos, const ant::Object &to)
{
  //  replace the object, keep the ID:
  ant::Object *new_ruler = new ant::Object (to);
  const ant::Object *current_ruler = dynamic_cast <const ant::Object *> (pos->ptr ());
  tl_assert (current_ruler != 0);

  int new_id = current_ruler->id ();
  new_ruler->id (new_id);
  mp_view->annotation_shapes ().replace (pos, db::DUserObject (new_ruler));

  annotation_changed_event (new_id);

  //  and make selection "visible"
  selection_to_view ();
}

void 
Service::paint_on_planes (const db::DCplxTrans &trans,
                          const std::vector <lay::CanvasPlane *> &planes,
                          lay::Renderer &renderer)
{
  if (planes.empty ()) {
    return;
  }

  db::DBox vp = db::DBox (trans.inverted () * db::DBox (db::DPoint (0.0, 0.0), db::DPoint (renderer.width (), renderer.height ())));

  lay::AnnotationShapes::touching_iterator user_object = mp_view->annotation_shapes ().begin_touching (vp);
  while (! user_object.at_end ()) {
    const ant::Object *ruler = dynamic_cast <const ant::Object *> ((*user_object).ptr ());
    if (ruler) {
      draw_ruler (*ruler, trans, false /*not selected*/, planes.front (), renderer);
    }
    ++user_object;
  }
}

AnnotationIterator
Service::begin_annotations () const
{
  return AnnotationIterator (mp_view->annotation_shapes ().begin (), mp_view->annotation_shapes ().end ());
}

void 
Service::menu_activated (const std::string &symbol)
{
  if (symbol == "ant::clear_all_rulers_internal") {
    clear_rulers ();
  } else if (symbol == "ant::clear_all_rulers") {
    if (manager ()) {
      manager ()->transaction (tl::to_string (tr ("Clear all rulers")));
    }
    clear_rulers ();
    if (manager ()) {
      manager ()->commit ();
    }
  } else {
    lay::Plugin::menu_activated (symbol);
  }
}

// -------------------------------------------------------------

} // namespace ant


