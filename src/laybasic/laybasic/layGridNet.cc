
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "layWidgets.h"
#include "layLayoutView.h"
#include "layConverters.h"
#include "layFixedFont.h"
#include "laySnap.h"
#include "dbTrans.h"
#include "ui_GridNetConfigPage.h"

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

class GridNetStyleConverter
{
public:
  void
  from_string (const std::string &value, lay::GridNet::GridStyle &style)
  {
    for (unsigned int i = 0; i < sizeof (grid_styles) / sizeof (grid_styles [0]); ++i) {
      if (value == grid_styles [i].string) {
        style = grid_styles [i].style;
        return;
      }
    }
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid grid net style: ")) + value);
  }

  std::string 
  to_string (lay::GridNet::GridStyle style)
  {
    for (unsigned int i = 0; i < sizeof (grid_styles) / sizeof (grid_styles [0]); ++i) {
      if (style == grid_styles [i].style) {
        return grid_styles [i].string;
      }
    }
    return "";
  }
};

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

lay::ConfigPage *
GridNetPluginDeclaration::config_page (QWidget *parent, std::string &title) const
{
  title = tl::to_string (QObject::tr ("Display|Background"));
  return new GridNetConfigPage (parent); 
}

lay::Plugin *
GridNetPluginDeclaration::create_plugin (db::Manager *, Dispatcher *, lay::LayoutView *view) const
{
  return new lay::GridNet (view);
}

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new GridNetPluginDeclaration (), 2010, "GridNetPlugin");

// ------------------------------------------------------------
//  Implementation of the configuration page

GridNetConfigPage::GridNetConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::GridNetConfigPage ();
  mp_ui->setupUi (this);

  mp_grid_color_cbtn = new lay::ColorButton (mp_ui->grid_net_color_pb);
  mp_grid_grid_color_cbtn = new lay::ColorButton (mp_ui->grid_grid_color_pb);
  mp_grid_axis_color_cbtn = new lay::ColorButton (mp_ui->grid_axis_color_pb);
  mp_grid_ruler_color_cbtn = new lay::ColorButton (mp_ui->grid_ruler_color_pb);
}

GridNetConfigPage::~GridNetConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
GridNetConfigPage::setup (lay::Dispatcher *root)
{
  std::string value;

  //  Grid visibility
  bool visible = false;
  root->config_get (cfg_grid_visible, visible);
  mp_ui->grid_group->setChecked (visible);

  bool show_ruler = false;
  root->config_get (cfg_grid_show_ruler, show_ruler);
  mp_ui->show_ruler->setChecked (show_ruler);

  QColor color;
  root->config_get (cfg_grid_color, color, ColorConverter ());
  mp_grid_color_cbtn->set_color (color);

  root->config_get (cfg_grid_grid_color, color, ColorConverter ());
  mp_grid_grid_color_cbtn->set_color (color);

  root->config_get (cfg_grid_axis_color, color, ColorConverter ());
  mp_grid_axis_color_cbtn->set_color (color);

  root->config_get (cfg_grid_ruler_color, color, ColorConverter ());
  mp_grid_ruler_color_cbtn->set_color (color);

  lay::GridNet::GridStyle style;

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style0, style, GridNetStyleConverter ());
  mp_ui->style0_cbx->setCurrentIndex (int (style));

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style1, style, GridNetStyleConverter ());
  mp_ui->style1_cbx->setCurrentIndex (int (style));

  style = lay::GridNet::Invisible;
  root->config_get (cfg_grid_style2, style, GridNetStyleConverter ());
  mp_ui->style2_cbx->setCurrentIndex (int (style));
}

void 
GridNetConfigPage::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_grid_visible, mp_ui->grid_group->isChecked ());
  root->config_set (cfg_grid_show_ruler, mp_ui->show_ruler->isChecked ());
  root->config_set (cfg_grid_color, mp_grid_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_grid_color, mp_grid_grid_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_axis_color, mp_grid_axis_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_ruler_color, mp_grid_ruler_color_cbtn->get_color (), lay::ColorConverter ());
  root->config_set (cfg_grid_style0, lay::GridNet::GridStyle (mp_ui->style0_cbx->currentIndex ()), GridNetStyleConverter ());
  root->config_set (cfg_grid_style1, lay::GridNet::GridStyle (mp_ui->style1_cbx->currentIndex ()), GridNetStyleConverter ());
  root->config_set (cfg_grid_style2, lay::GridNet::GridStyle (mp_ui->style2_cbx->currentIndex ()), GridNetStyleConverter ());
}

// ------------------------------------------------------------
//  Implementation of the GridNet object

GridNet::GridNet (lay::LayoutView *view)
  : lay::BackgroundViewObject (view->view_object_widget ()), 
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

    QColor color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_color, color);

  } else if (name == cfg_grid_grid_color) {

    QColor color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_grid_color, color);

  } else if (name == cfg_grid_axis_color) {

    QColor color;
    ColorConverter ().from_string (value, color);
    need_update = test_and_set (m_axis_color, color);

  } else if (name == cfg_grid_ruler_color) {

    QColor color;
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


class ImagePainter
{
public:
  ImagePainter (lay::BitmapViewObjectCanvas &canvas)
    : mp_img (&canvas.bg_image ()), 
      m_resolution (canvas.resolution ()), m_width (canvas.canvas_width ()), m_height (canvas.canvas_height ())
  {
    // .. nothing yet ..
  }

  void set (const QPoint &p, QColor c) 
  {
    if (p.x () >= 0 && p.x () < m_width && p.y () >= 0 && p.y () < m_height) {
      ((unsigned int *) mp_img->scanLine (p.y ())) [p.x ()] = c.rgb ();
    }
  }
    
  void draw_line (const QPoint &p1, const QPoint &p2, QColor c) 
  {
    if (p1.x () == p2.x ()) {

      int x = p1.x ();
      int y1 = std::min (p1.y (), p2.y ());
      int y2 = std::max (p1.y (), p2.y ());
      if ((y2 >= 0 || y1 < m_height) && x >= 0 && x < m_width) {
        y1 = std::max (y1, 0);
        y2 = std::min (y2, m_height - 1);
        for (int y = y1; y <= y2; ++y) {
          ((unsigned int *) mp_img->scanLine (y)) [x] = c.rgb ();
        }
      }

    } else if (p1.y () == p2.y ()) {

      int y = p1.y ();
      int x1 = std::min (p1.x (), p2.x ());
      int x2 = std::max (p1.x (), p2.x ());
      if ((x2 >= 0 || x1 < m_width) && y >= 0 && y < m_height) {
        x1 = std::max (x1, 0);
        x2 = std::min (x2, m_width - 1);
        unsigned int *sl = (unsigned int *) mp_img->scanLine (y) + x1;
        for (int x = x1; x <= x2; ++x) {
          *sl++ = c.rgb ();
        }
      }

    } else {
      // TODO: not implemented yet.
    }
  }
    
  void fill_rect (const QPoint &p1, const QPoint &p2, QColor c) 
  {
    int y1 = std::min (p1.y (), p2.y ());
    int y2 = std::max (p1.y (), p2.y ());
    for (int y = y1; y <= y2; ++y) {
      draw_line (QPoint (p1.x (), y), QPoint (p2.x (), y), c);
    }
  }

  void draw_rect (const QPoint &p1, const QPoint &p2, QColor c) 
  {
    int y1 = std::min (p1.y (), p2.y ());
    int y2 = std::max (p1.y (), p2.y ());
    int x1 = std::min (p1.x (), p2.x ());
    int x2 = std::max (p1.x (), p2.x ());
    draw_line (QPoint (x1, y1), QPoint (x2, y1), c);
    draw_line (QPoint (x1, y2), QPoint (x2, y2), c);
    draw_line (QPoint (x1, y1), QPoint (x1, y2), c);
    draw_line (QPoint (x2, y1), QPoint (x2, y2), c);
  }

  void draw_text (const char *t, const QPoint &p, QColor c, int halign, int valign)
  {
    const lay::FixedFont &ff = lay::FixedFont::get_font (m_resolution);
    int x = p.x (), y = p.y ();

    if (halign < 0) {
      x -= ff.width () * int (strlen (t));
    } else if (halign == 0) {
      x -= ff.width () * int (strlen (t)) / 2;
    }

    if (valign < 0) {
      y += ff.height ();
    } else if (valign == 0) {
      y += ff.height () / 2;
    }

    //  TODO: simple implementation
    for (; *t; ++t) {

      unsigned char ch = *t;

      if (x < -int (ff.width ()) || x >= int (mp_img->width ()) || y < 0 || y >= int (mp_img->height () + ff.height ())) {
        continue;
      }

      if (ch < ff.first_char () || (ch - ff.first_char ()) >= ff.n_chars ()) {
        continue;
      }

      const uint32_t *dc = ff.data () + size_t (ch - ff.first_char ()) * ff.height () * ff.stride ();
      for (unsigned int i = 0; i < ff.height (); ++i, dc += ff.stride ()) {

        int iy = y - ff.height () + i + 1;
        if (iy >= 0 || iy < mp_img->height ()) {

          uint32_t *d = (uint32_t *) mp_img->scanLine (y - ff.height () + i);
          uint32_t m = 1; 
          int ix = x;
          const uint32_t *ds = dc;

          for (unsigned int j = 0; j < ff.width (); ++j, ++ix) {

            if ((*ds & m) && ix >= 0 && ix < mp_img->width ()) {
              d[ix] = c.rgb ();
            }

            m <<= 1;
            //  word wrap
            if (m == 0) {
              ++ds;
              m = 1;
            }

          }

        }

      } 

      x += ff.width ();

    }

  }

private:
  QImage *mp_img;
  double m_resolution;
  int m_width, m_height;
};

void 
GridNet::render_bg (const lay::Viewport &vp, ViewObjectCanvas &canvas)
{
  if (m_visible) {

    QColor color;
    if (m_color.isValid ()) {
      color = m_color;
    } else {
      color = QColor (128, 128, 128); // TODO: this is not a "real" automatic color ..
    }

    QColor grid_color = color, axis_color = color, ruler_color = color;
    if (m_grid_color.isValid ()) {
      grid_color = m_grid_color;
    }
    if (m_axis_color.isValid ()) {
      axis_color = m_axis_color;
    }
    if (m_ruler_color.isValid ()) {
      ruler_color = m_ruler_color;
    }

    // TODO: currently, the grid net can only be rendered to a bitmap canvas ..
    BitmapViewObjectCanvas *bmp_canvas = dynamic_cast<BitmapViewObjectCanvas *> (&canvas);
    if (! bmp_canvas) {
      return;
    }

    ImagePainter painter (*bmp_canvas);

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
    while (dgrid < fw * 4 / bmp_canvas->resolution ()) {
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

      int rh = int (floor (0.5 + fw * 0.8 / bmp_canvas->resolution ()));
      int xoffset = int (floor (0.5 + fw * 2.5 / bmp_canvas->resolution ()));
      int yoffset = int (floor (0.5 + fw * 2.5 / bmp_canvas->resolution ()));

      painter.fill_rect (QPoint (xoffset, vp.height () - yoffset - rh / 2),
                         QPoint (xoffset + int (floor (0.5 + dgrid)), vp.height () - yoffset + rh / 2), 
                         ruler_color);

      painter.draw_rect (QPoint (xoffset + int (floor (0.5 + dgrid)), vp.height () - yoffset - rh / 2),
                         QPoint (xoffset + int (floor (0.5 + 2 * dgrid)), vp.height () - yoffset + rh / 2), 
                         ruler_color);

      painter.draw_text (tl::sprintf ("%g \265m", grid * 2).c_str (), 
                         QPoint (xoffset + int (floor (0.5 + trans.ctrans (2 * grid))), vp.height () - yoffset - rh / 2 - 2), 
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
          QPoint p0 (xoffset + 2 * rh, vp.height () - yoffset - rh * 5);
          QPoint p1 = p0 + QPoint (int (floor (0.5 + (*e).p1 ().x () * 0.1 * rh * 4)), -int (floor (0.5 + (*e).p1 ().y () * 0.1 * rh * 4)));
          QPoint p2 = p0 + QPoint (int (floor (0.5 + (*e).p2 ().x () * 0.1 * rh * 4)), -int (floor (0.5 + (*e).p2 ().y () * 0.1 * rh * 4)));
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
          QPoint p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - QPoint (2, 0), p + QPoint (2, 0), grid_color);
          painter.draw_line (p - QPoint (0, 2), p + QPoint (0, 2), grid_color);
        }
      }

    } else if (style == Lines) {

      int n;

      //  the way we iterate here is safe against integer overflow ..
      n = nx;
      for (db::DCoord x = x1; n > 0; x += grid, --n) {
        QPoint p1 (draw_round (trans * db::DPoint (x, y1), vp.height ()));
        QPoint p2 (draw_round (trans * db::DPoint (x, y2), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
      }

      n = ny;
      for (db::DCoord y = y1; n > 0; y += grid, --n) {
        QPoint p1 (draw_round (trans * db::DPoint (x1, y), vp.height ()));
        QPoint p2 (draw_round (trans * db::DPoint (x2, y), vp.height ()));
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
        QPoint p1 (draw_round (trans * db::DPoint (x, y1), vp.height ()));
        QPoint p2 (draw_round (trans * db::DPoint (x, y2), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
        for (db::DCoord y = y1; y < y2 + g * eps; y += g) {
          QPoint p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - QPoint (2, 0), p + QPoint (2, 0), grid_color);
        }
      }

      n = ny;
      for (db::DCoord y = y1; n > 0; y += grid, --n) {
        QPoint p1 (draw_round (trans * db::DPoint (x1, y), vp.height ()));
        QPoint p2 (draw_round (trans * db::DPoint (x2, y), vp.height ()));
        painter.draw_line (p1, p2, grid_color);
        for (db::DCoord x = x1; x < x2 + g * eps; x += g) {
          QPoint p (draw_round (trans * db::DPoint (x, y), vp.height ()));
          painter.draw_line (p - QPoint (0, 2), p + QPoint (0, 2), grid_color);
        }
      }

    } else if (style == CheckerBoard) {

      for (db::DCoord x = x1; x < x2 + grid * eps; x += grid) {
        for (db::DCoord y = y1; y < y2 + grid * eps; y += grid) {
          double idx = (x + y) / grid + eps;
          if (idx - 2.0 * floor (idx * 0.5) < 0.5) {
            QPoint p1 (draw_round (trans * db::DPoint (x, y), vp.height ()));
            QPoint p2 (draw_round (trans * db::DPoint (x + grid, y + grid), vp.height ()));
            painter.fill_rect (p1, p2 + QPoint (-1, 1), grid_color);
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
          QPoint p (draw_round (trans * db::DPoint (0.0, y), vp.height ()));
          painter.draw_line (p - QPoint (2, 0), p + QPoint (2, 0), axis_color);
          painter.draw_line (p - QPoint (0, 2), p + QPoint (0, 2), axis_color);
        }
        for (db::DCoord x = x1; x < x2 + grid * eps && draw_xaxis; x += grid) {
          QPoint p (draw_round (trans * db::DPoint (x, 0.0), vp.height ()));
          painter.draw_line (p - QPoint (2, 0), p + QPoint (2, 0), axis_color);
          painter.draw_line (p - QPoint (0, 2), p + QPoint (0, 2), axis_color);
        }

      } else if (m_style0 == Lines) {

        //  the way we iterate here is safe against integer overflow ..
        if (draw_yaxis) {
          QPoint p1 (draw_round (trans * db::DPoint (0.0, y1), vp.height ()));
          QPoint p2 (draw_round (trans * db::DPoint (0.0, y2), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
        }

        if (draw_xaxis) {
          QPoint p1 (draw_round (trans * db::DPoint (x1, 0.0), vp.height ()));
          QPoint p2 (draw_round (trans * db::DPoint (x2, 0.0), vp.height ()));
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
          QPoint p1 (draw_round (trans * db::DPoint (0.0, y1), vp.height ()));
          QPoint p2 (draw_round (trans * db::DPoint (0.0, y2), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
          for (db::DCoord y = y1; y < y2 + g * eps; y += g) {
            QPoint p (draw_round (trans * db::DPoint (0.0, y), vp.height ()));
            painter.draw_line (p - QPoint (2, 0), p + QPoint (2, 0), axis_color);
          }
        }

        if (draw_xaxis) {
          QPoint p1 (draw_round (trans * db::DPoint (x1, 0.0), vp.height ()));
          QPoint p2 (draw_round (trans * db::DPoint (x2, 0.0), vp.height ()));
          painter.draw_line (p1, p2, axis_color);
          for (db::DCoord x = x1; x < x2 + g * eps; x += g) {
            QPoint p (draw_round (trans * db::DPoint (x, 0.0), vp.height ()));
            painter.draw_line (p - QPoint (0, 2), p + QPoint (0, 2), axis_color);
          }
        }

      }

    }

  }

}
 

} // namespace lay

