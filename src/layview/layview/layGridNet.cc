
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


#include "laybasicConfig.h"
#include "layGridNet.h"
#include "layConverters.h"
#include "layLayoutViewBase.h"
#include "layFixedFont.h"
#include "layPixelBufferPainter.h"
#include "laySnap.h"
#include "tlColor.h"
#include "dbTrans.h"

#if defined(HAVE_QT)
#  include "layGridNetConfigPage.h"
#endif

namespace lay
{

// ------------------------------------------------------------
//  Helper functions to get and set the configuration

static struct {
  lay::GridNet::GridStyle style;
  const char *string;
} grid_styles [] = {
  { lay::GridNet::Invisible,         "invisible"           },
  { lay::GridNet::Dots,              "dots"                },
  { lay::GridNet::DottedLines,       "dotted-lines"        },
  { lay::GridNet::LightDottedLines,  "light-dotted-lines"  },
  { lay::GridNet::TenthDottedLines,  "tenths-dotted-lines" },
  { lay::GridNet::Crosses,           "crosses"             },
  { lay::GridNet::Lines,             "lines"               },
  { lay::GridNet::TenthMarkedLines,  "tenth-marked-lines"  },
  { lay::GridNet::CheckerBoard,      "checkerboard"        }
};

void
GridNetStyleConverter::from_string (const std::string &value, lay::GridNet::GridStyle &style)
{
  for (unsigned int i = 0; i < sizeof (grid_styles) / sizeof (grid_styles [0]); ++i) {
    if (value == grid_styles [i].string) {
      style = grid_styles [i].style;
      return;
    }
  }
  throw tl::Exception (tl::to_string (tr ("Invalid grid net style: ")) + value);
}

std::string
GridNetStyleConverter::to_string (lay::GridNet::GridStyle style)
{
  for (unsigned int i = 0; i < sizeof (grid_styles) / sizeof (grid_styles [0]); ++i) {
    if (style == grid_styles [i].style) {
      return grid_styles [i].string;
    }
  }
  return "";
}

// ------------------------------------------------------------
//  Implementation of the GridNetPluginDeclaration 

void 
GridNetPluginDeclaration::get_options (std::vector < std::pair<std::string, std::string> > &options) const
{
  options.push_back (std::pair<std::string, std::string> (cfg_grid_color, "auto"));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_ruler_color, "auto"));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_axis_color, "auto"));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_grid_color, "auto"));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_style0, GridNetStyleConverter ().to_string (lay::GridNet::Invisible)));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_style1, GridNetStyleConverter ().to_string (lay::GridNet::Dots)));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_style2, GridNetStyleConverter ().to_string (lay::GridNet::TenthDottedLines)));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_visible, tl::to_string (true)));
  options.push_back (std::pair<std::string, std::string> (cfg_grid_show_ruler, tl::to_string (true)));
  //  grid-micron is not configured here since some other entity is supposed to do this.
}

#if defined(HAVE_QT)
lay::ConfigPage *
GridNetPluginDeclaration::config_page (QWidget *parent, std::string &title) const
{
  title = tl::to_string (QObject::tr ("Display|Background"));
  return new GridNetConfigPage (parent); 
}
#endif

lay::Plugin *
GridNetPluginDeclaration::create_plugin (db::Manager *, Dispatcher *, lay::LayoutViewBase *view) const
{
  return new lay::GridNet (view);
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new GridNetPluginDeclaration (), 2010, "GridNetPlugin");

// ------------------------------------------------------------
//  Implementation of the GridNet object

GridNet::GridNet (LayoutViewBase *view)
  : lay::BackgroundViewObject (view->canvas ()), 
    lay::Plugin (view),
    mp_view (view),
    m_visible (false), m_show_ruler (true), m_grid (1.0),
    m_style0 (Invisible), m_style1 (Invisible), m_style2 (Invisible)
{ 
  // .. nothing yet ..
}

bool 
GridNet::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;

  if (name == cfg_grid_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_color, color);

  } else if (name == cfg_grid_grid_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_grid_color, color);

  } else if (name == cfg_grid_axis_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_axis_color, color);

  } else if (name == cfg_grid_ruler_color) {

    tl::Color color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_ruler_color, color);

  } else if (name == cfg_grid_style0) {

    lay::GridNet::GridStyle style;
    GridNetStyleConverter ().from_string (value, style);
    need_update = test_and_set (m_style0, style);

  } else if (name == cfg_grid_style1) {

    lay::GridNet::GridStyle style;
    GridNetStyleConverter ().from_string (value, style);
    need_update = test_and_set (m_style1, style);

  } else if (name == cfg_grid_style2) {

    lay::GridNet::GridStyle style;
    GridNetStyleConverter ().from_string (value, style);
    need_update = test_and_set (m_style2, style);

  } else if (name == cfg_grid_show_ruler) {

    bool sr = false;
    tl::from_string (value, sr);
    need_update = test_and_set (m_show_ruler, sr);

  } else if (name == cfg_grid_visible) {

    bool vis = false;
    tl::from_string (value, vis);
    need_update = test_and_set (m_visible, vis);

  } else if (name == cfg_grid_micron) {

    double g = 0;
    tl::from_string (value, g);
    if (fabs (g - m_grid) > 1e-6) {
      m_grid = g;
      need_update = true;
    }
    taken = false; // to let others use the grid too.

  } else {
    taken = false;
  }

  if (need_update) {
    widget ()->touch_bg ();
  }

  return taken;
}

void
GridNet::render_bg (const lay::Viewport &vp, ViewObjectCanvas &canvas)
{
  if (m_visible) {

    tl::Color color;
    if (m_color.is_valid ()) {
      color = m_color;
    } else {
      color = tl::Color (128, 128, 128); // TODO: this is not a "real" automatic color ..
    }

    tl::Color grid_color = color, axis_color = color, ruler_color = color;
    if (m_grid_color.is_valid ()) {
      grid_color = m_grid_color;
    }
    if (m_axis_color.is_valid ()) {
      axis_color = m_axis_color;
    }
    if (m_ruler_color.is_valid ()) {
      ruler_color = m_ruler_color;
    }

    // TODO: currently, the grid net can only be rendered to a bitmap canvas ..
    BitmapViewObjectCanvas *bmp_canvas = dynamic_cast<BitmapViewObjectCanvas *> (&canvas);
    if (! bmp_canvas || ! bmp_canvas->bg_image ()) {
      return;
    }

    PixelBufferPainter painter (*bmp_canvas->bg_image (), bmp_canvas->canvas_width (), bmp_canvas->canvas_height (), bmp_canvas->resolution ());

    db::DCplxTrans trans = vp.trans ();
    db::DCplxTrans::inverse_trans trans_inv (trans.inverted ());

    db::DBox dbworld (trans_inv * db::DBox (0.0, 0.0, double (vp.width ()), double (vp.height ())));

    //  fw is the basic unit of the ruler geometry
    const lay::FixedFont &ff = lay::FixedFont::get_font (bmp_canvas->resolution ());
    int fw = ff.width ();

    double dgrid = trans.ctrans (m_grid);
    GridStyle style = m_style1;

    //  compute major grid and switch to secondary style if necessary
    int s = 0;
    while (dgrid < fw * 4) {
      if (s == 0) {
        dgrid *= 2.0;
      } else if (s == 1) {
        dgrid *= 2.5;
      } else if (s == 2) {
        dgrid *= 2.0;
      }
      s = (s + 1) % 3;
      style = m_style2;
    }

    db::DCoord grid = trans_inv.ctrans (dgrid);

    const double eps = 1e-6;

    db::DCoord x1 = floor (dbworld.left () / grid - eps) * grid;
    db::DCoord x2 = ceil (dbworld.right () / grid + eps) * grid;
    db::DCoord y1 = floor (dbworld.bottom () / grid - eps) * grid;
    db::DCoord y2 = ceil (dbworld.top () / grid + eps) * grid;

    bool draw_yaxis = (x1 < 0.0 && x2 > 0.0);
    bool draw_xaxis = (y1 < 0.0 && y2 > 0.0);

    int nx = int (dbworld.width () / grid + eps) + 2;
    int ny = int (dbworld.height () / grid + eps) + 2;

    if (m_show_ruler && dgrid < vp.width () * 0.2) {

      int rh = int (floor (0.5 + fw * 0.8));
      int xoffset = int (floor (0.5 + fw * 2.5));
      int yoffset = int (floor (0.5 + fw * 2.5));

      painter.fill_rect (db::Point (xoffset, vp.height () - yoffset - rh / 2),
                         db::Point (xoffset + int (floor (0.5 + dgrid)), vp.height () - yoffset + rh / 2),
                         ruler_color);

      painter.draw_rect (db::Point (xoffset + int (floor (0.5 + dgrid)), vp.height () - yoffset - rh / 2),
                         db::Point (xoffset + int (floor (0.5 + 2 * dgrid)), vp.height () - yoffset + rh / 2),
                         ruler_color);

      painter.draw_text (tl::sprintf ("%g \265m", grid * 2).c_str (), 
                         db::Point (xoffset + int (floor (0.5 + trans.ctrans (2 * grid))), vp.height () - yoffset - rh / 2 - 2),
                         ruler_color, -1, 1);

      if (mp_view->global_trans ().fp_trans () != db::DFTrans ()) {

        //  draw a small "F" indicating any global transformation
        db::Point pts[] = {
          db::Point (-4, -5),
          db::Point (-4, 5),
          db::Point (4, 5),
          db::Point (4, 3),
          db::Point (-2, 3),
          db::Point (-2, 1),
          db::Point (3, 1),
          db::Point (3, -1),
          db::Point (-2, -1),
          db::Point (-2, -5),
          db::Point (-4, -5)
        };

        db::Polygon poly;
        poly.assign_hull (&pts[0], &pts[0] + (sizeof (pts) / sizeof (pts[0])));
        poly.transform (db::FTrans (mp_view->global_trans ().fp_trans ()));

        for (db::Polygon::polygon_edge_iterator e = poly.begin_edge (); !e.at_end (); ++e) {
          db::Point p0 (xoffset + 2 * rh, vp.height () - yoffset - rh * 5);
          db::Point p1 = p0 + db::Vector (int (floor (0.5 + (*e).p1 ().x () * 0.1 * rh * 4)), -int (floor (0.5 + (*e).p1 ().y () * 0.1 * rh * 4)));
          db::Point p2 = p0 + db::Vector (int (floor (0.5 + (*e).p2 ().x () * 0.1 * rh * 4)), -int (floor (0.5 + (*e).p2 ().y () * 0.1 * rh * 4)));
          painter.draw_line (p1, p2, ruler_color);
        }

      }

    }

    //  draw grid
    if (style == Dots || style == TenthDottedLines || 
        style == DottedLines || style == LightDottedLines) {

      int n;

      double g = grid;
      if (style == TenthDottedLines) {
        g = grid / 10.0;
      } else if (style == DottedLines) {
        g = trans_inv.ctrans (2.0);
      } else if (style == LightDottedLines) {
        g = trans_inv.ctrans (4.0);
      }

      //  the way we iterate here is safe against integer overflow ..
      n = nx;
      for (db::DCoord x = x1; n > 0; x += grid, --n) {
        for (db::DCoord y = y1; y < y2 + g * eps; y += g) {
          painter.set (draw_round (trans * db::DPoint (x, y), vp.height ()), grid_color);
        }
      }

      if (style != Dots) {
        n = ny;
        for (db::DCoord y = y1; n > 0; y += grid, --n) {
          for (db::DCoord x = x1; x < x2 + g * eps; x += g) {
            painter.set (draw_round (trans * db::DPoint (x, y), vp.height ()), grid_color);
          }
        }
      }

    } else if (style == Crosses) {

      for (db::DCoord x = x1; x < x2 + grid * eps; x += grid) {
        for (db::DCoord y = y1; y < y2 + grid * eps; y += grid) {
          db::Point p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - db::Vector (2, 0), p + db::Vector (2, 0), grid_color);
          painter.draw_line (p - db::Vector (0, 2), p + db::Vector (0, 2), grid_color);
        }
      }

    } else if (style == Lines) {

      int n;

      //  the way we iterate here is safe against integer overflow ..
      n = nx;
      for (db::DCoord x = x1; n > 0; x += grid, --n) {
        db::Point p1 (draw_round (trans * db::DPoint (x, y1), vp.height ()));
        db::Point p2 (draw_round (trans * db::DPoint (x, y2), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
      }

      n = ny;
      for (db::DCoord y = y1; n > 0; y += grid, --n) {
        db::Point p1 (draw_round (trans * db::DPoint (x1, y), vp.height ()));
        db::Point p2 (draw_round (trans * db::DPoint (x2, y), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
      }

    } else if (style == TenthMarkedLines) {

      int n;
      double g = grid / 10.0;

      //  reduce grid if too small
      if (trans.ctrans (g) < 2.0) {
        g *= 2.0;
      }
      if (trans.ctrans (g) < 2.0) {
        g *= 2.5;
      }

      //  the way we iterate here is safe against integer overflow ..
      n = nx;
      for (db::DCoord x = x1; n > 0; x += grid, --n) {
        db::Point p1 (draw_round (trans * db::DPoint (x, y1), vp.height ()));
        db::Point p2 (draw_round (trans * db::DPoint (x, y2), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
        for (db::DCoord y = y1; y < y2 + g * eps; y += g) {
          db::Point p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - db::Vector (2, 0), p + db::Vector (2, 0), grid_color);
        }
      }

      n = ny;
      for (db::DCoord y = y1; n > 0; y += grid, --n) {
        db::Point p1 (draw_round (trans * db::DPoint (x1, y), vp.height ()));
        db::Point p2 (draw_round (trans * db::DPoint (x2, y), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
        for (db::DCoord x = x1; x < x2 + g * eps; x += g) {
          db::Point p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - db::Vector (0, 2), p + db::Vector (0, 2), grid_color);
        }
      }

    } else if (style == CheckerBoard) {

      for (db::DCoord x = x1; x < x2 + grid * eps; x += grid) {
        for (db::DCoord y = y1; y < y2 + grid * eps; y += grid) {
          double idx = (x + y) / grid + eps;
          if (idx - 2.0 * floor (idx * 0.5) < 0.5) {
            db::Point p1 (draw_round (trans * db::DPoint (x, y), vp.height ()));
            db::Point p2 (draw_round (trans * db::DPoint (x + grid, y + grid), vp.height ()));
            painter.fill_rect (p1, p2 + db::Vector (-1, 1), grid_color);
          }
        }
      }

    }

    if (m_style0 != Invisible && (draw_xaxis || draw_yaxis)) {

      //  draw grid
      if (m_style0 == Dots || m_style0 == TenthDottedLines || 
          m_style0 == DottedLines || m_style0 == LightDottedLines) {

        int n;

        double g = grid;
        if (m_style0 == TenthDottedLines) {
          g = grid / 10.0;
          //  reduce grid if too small
          if (trans.ctrans (g) < 2.0) {
            g *= 2.0;
          }
          if (trans.ctrans (g) < 2.0) {
            g *= 2.5;
          }
        } else if (m_style0 == DottedLines) {
          g = trans_inv.ctrans (2.0);
        } else if (m_style0 == LightDottedLines) {
          g = trans_inv.ctrans (4.0);
        }

        //  the way we iterate here is safe against integer overflow ..
        n = nx;
        for (db::DCoord x = x1; n > 0 && draw_xaxis; x += grid, --n) {
          painter.set (draw_round (trans * db::DPoint (x, 0.0), vp.height ()), axis_color);
        }
        for (db::DCoord y = y1; y < y2 + g * eps && draw_yaxis; y += g) {
          painter.set (draw_round (trans * db::DPoint (0.0, y), vp.height ()), axis_color);
        }

        if (m_style0 != Dots) {
          n = ny;
          for (db::DCoord y = y1; n > 0 && draw_yaxis; y += grid, --n) {
            painter.set (draw_round (trans * db::DPoint (0.0, y), vp.height ()), axis_color);
          }
          for (db::DCoord x = x1; x < x2 + g * eps && draw_xaxis; x += g) {
            painter.set (draw_round (trans * db::DPoint (x, 0.0), vp.height ()), axis_color);
          }
        }

      } else if (m_style0 == Crosses) {

        for (db::DCoord y = y1; y < y2 + grid * eps && draw_yaxis; y += grid) {
          db::Point p (draw_round (trans * db::DPoint (0.0, y), vp.height ()));
          painter.draw_line (p - db::Vector (2, 0), p + db::Vector (2, 0), axis_color);
          painter.draw_line (p - db::Vector (0, 2), p + db::Vector (0, 2), axis_color);
        }
        for (db::DCoord x = x1; x < x2 + grid * eps && draw_xaxis; x += grid) {
          db::Point p (draw_round (trans * db::DPoint (x, 0.0), vp.height ()));
          painter.draw_line (p - db::Vector (2, 0), p + db::Vector (2, 0), axis_color);
          painter.draw_line (p - db::Vector (0, 2), p + db::Vector (0, 2), axis_color);
        }

      } else if (m_style0 == Lines) {

        //  the way we iterate here is safe against integer overflow ..
        if (draw_yaxis) {
          db::Point p1 (draw_round (trans * db::DPoint (0.0, y1), vp.height ()));
          db::Point p2 (draw_round (trans * db::DPoint (0.0, y2), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
        }

        if (draw_xaxis) {
          db::Point p1 (draw_round (trans * db::DPoint (x1, 0.0), vp.height ()));
          db::Point p2 (draw_round (trans * db::DPoint (x2, 0.0), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
        }

      } else if (m_style0 == TenthMarkedLines) {

        double g = grid / 10.0;

        //  reduce grid if too small
        if (trans.ctrans (g) < 2.0) {
          g *= 2.0;
        }
        if (trans.ctrans (g) < 2.0) {
          g *= 2.5;
        }

        //  the way we iterate here is safe against integer overflow ..
        if (draw_yaxis) {
          db::Point p1 (draw_round (trans * db::DPoint (0.0, y1), vp.height ()));
          db::Point p2 (draw_round (trans * db::DPoint (0.0, y2), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
          for (db::DCoord y = y1; y < y2 + g * eps; y += g) {
            db::Point p (draw_round (trans * db::DPoint (0.0, y), vp.height ()));
            painter.draw_line (p - db::Vector (2, 0), p + db::Vector (2, 0), axis_color);
          }
        }

        if (draw_xaxis) {
          db::Point p1 (draw_round (trans * db::DPoint (x1, 0.0), vp.height ()));
          db::Point p2 (draw_round (trans * db::DPoint (x2, 0.0), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
          for (db::DCoord x = x1; x < x2 + g * eps; x += g) {
            db::Point p (draw_round (trans * db::DPoint (x, 0.0), vp.height ()));
            painter.draw_line (p - db::Vector (0, 2), p + db::Vector (0, 2), axis_color);
          }
        }

      }

    }

  }
}
 
} // namespace lay

