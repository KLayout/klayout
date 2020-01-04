
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "imgWidgets.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>

#include <algorithm>
#include <math.h>

namespace img
{

const int frame_width = 5;
const int hframe_width = 10;
const int indicator_height = 8;
const int indicator_spacing = 4;
const int nominal_bar_height = 32;
const int min_bar_height = 4;

const double min_value_interval = 1e-3;
const double epsilon = 1e-6;

struct compare_first_of_node
{
  bool operator() (const std::pair <double, QColor> &a, const std::pair <double, QColor> &b) const
  {
    return a.first < b.first;
  }
};

QColor
interpolated_color (const std::vector<std::pair <double, QColor> > &nodes, double x)
{
  if (nodes.size () < 1) {
    return QColor ();
  } else if (nodes.size () < 2) {
    return nodes[0].second;
  } else {

    std::vector<std::pair<double, QColor> >::const_iterator p = std::lower_bound (nodes.begin (), nodes.end (), std::make_pair (x, QColor ()), compare_first_of_node ());
    if (p == nodes.end ()) {
      return nodes.back ().second;
    } else if (p == nodes.begin ()) {
      return nodes.front ().second;
    } else {

      double x1 = p[-1].first;
      double x2 = p->first;

      int h1 = 0, s1 = 0, v1 = 0;
      p[-1].second.getHsv (&h1, &s1, &v1);

      int h2 = 0, s2 = 0, v2 = 0;
      p->second.getHsv (&h2, &s2, &v2);

      int h = int (0.5 + h1 + double(x - x1) * double (h2 - h1) / double(x2 - x1));
      int s = int (0.5 + s1 + double(x - x1) * double (s2 - s1) / double(x2 - x1));
      int v = int (0.5 + v1 + double(x - x1) * double (v2 - v1) / double(x2 - x1));

      QColor r;
      r.setHsv (h, s, v);
      return r;

    }

  }
}

ColorBar::ColorBar (QWidget *parent)
  : QWidget (parent), m_dragging (false), m_selected (-1)
{
  m_nodes.push_back (std::make_pair (0.0, QColor (0, 0, 0)));
  m_nodes.push_back (std::make_pair (1.0, QColor (255, 255, 255)));
}

ColorBar::~ColorBar ()
{
  // .. nothing yet ..
}

void 
ColorBar::mouseMoveEvent (QMouseEvent *event)
{
  if (m_dragging && m_selected > 0 && m_selected < int (m_nodes.size ()) - 1) {

    int xl = hframe_width;
    int xr = width () - hframe_width;

    const double min_distance = 0.005; // stay away 0.5% from neighboring nodes.

    double xx = double (event->x () - xl) / double (xr - xl);
    xx = std::min (xx, m_nodes[m_selected + 1].first - min_distance);
    xx = std::max (xx, m_nodes[m_selected - 1].first + min_distance);
    m_nodes[m_selected].first = xx;
    emit color_mapping_changed ();
    update ();

  }
}

void 
ColorBar::set_current_color (QColor c) 
{
  if (has_selection ()) {
    m_nodes [m_selected].second = c;
    emit color_mapping_changed ();
    update ();
  }
}

void 
ColorBar::set_current_position (double x) 
{
  if (has_selection () && x > min_value_interval && x < 1.0 - min_value_interval) {

    //  some heuristics to keep the list ordered and to avoid small intervals
    m_nodes [m_selected].first = x;

    while (m_selected > 0 && m_nodes [m_selected].first < m_nodes [m_selected - 1].first) {
      std::swap (m_nodes [m_selected], m_nodes [m_selected - 1]);
      --m_selected;
    }

    while (m_selected < int (m_nodes.size () - 1) && m_nodes [m_selected].first > m_nodes [m_selected + 1].first) {
      std::swap (m_nodes [m_selected], m_nodes [m_selected + 1]);
      ++m_selected;
    }

    while (m_selected < int (m_nodes.size () - 1) && fabs (m_nodes[m_selected].first - m_nodes[m_selected + 1].first) < min_value_interval) {
      m_nodes.erase (m_nodes.begin () + (m_selected + 1));
    }

    while (m_selected > 0 && fabs (m_nodes[m_selected].first - m_nodes[m_selected - 1].first) < min_value_interval) {
      m_nodes.erase (m_nodes.begin () + (m_selected - 1));
      --m_selected;
    }

    m_nodes.front ().first = 0.0;
    m_nodes.back ().first = 1.0;

    emit color_mapping_changed ();
    update ();

  }
}

void
ColorBar::keyPressEvent (QKeyEvent *event)
{
  if (event->key () == Qt::Key_Delete && has_selection () && m_selected > 0 && m_selected < int (m_nodes.size ()) - 1) {
    m_nodes.erase (m_nodes.begin () + m_selected);
    m_selected = -1;
    emit selection_changed ();
    emit selection_changed (QColor ());
    update ();
  }
}

void 
ColorBar::set_nodes (const std::vector <std::pair <double, QColor> > &nodes)
{
  m_nodes = nodes;

  std::sort (m_nodes.begin (), m_nodes.end (), compare_first_of_node ());

  if (m_nodes.size () == 0 || fabs (m_nodes[0].first) > epsilon) {
    m_nodes.insert (m_nodes.begin (), std::make_pair (0.0, QColor (0, 0, 0)));
  } else {
    m_nodes[0].first = 0.0;
  }

  std::vector <std::pair <double, QColor> >::iterator w = m_nodes.begin ();
  std::vector <std::pair <double, QColor> >::const_iterator nn = m_nodes.begin ();
  for (std::vector <std::pair <double, QColor> >::const_iterator n = m_nodes.begin () + 1; n != m_nodes.end (); ++n) {
    if (fabs (nn->first - n->first) > min_value_interval) {
      *w++ = *nn;
      nn = n;
    }
  }

  *w++ = *nn;
  m_nodes.erase (w, m_nodes.end ());

  if (m_nodes.back ().first > 1.0 - min_value_interval) {
    m_nodes.back ().first = 1.0;
  } else {
    m_nodes.push_back (std::make_pair (1.0, QColor (255, 255, 255)));
  }

  m_selected = -1;

  emit selection_changed ();
  emit color_mapping_changed ();
  update ();
}

void 
ColorBar::mousePressEvent (QMouseEvent *event)
{
  setFocus ();

  int xl = hframe_width;
  int xr = width () - hframe_width;

  int yb = height () - (frame_width + indicator_height + indicator_spacing);

  if (event->x () > xl - 5 && event->x () < xr + 5 && event->y () > yb - 5 && event->y () < yb + 5 + indicator_height + indicator_spacing) {

    double xx = double (event->x () - xl) / double (xr - xl);

    double dmin = 100.0;
    std::vector<std::pair<double, QColor> >::const_iterator pmin = m_nodes.end ();
    for (std::vector<std::pair<double, QColor> >::const_iterator p = m_nodes.begin (); p != m_nodes.end (); ++p) {
      double d = fabs (p->first - xx);
      if (d < 0.05 && d < dmin) {
        dmin = d;
        pmin = p;
      }
    }

    if (pmin != m_nodes.end ()) {
      m_selected = int (std::distance (std::vector<std::pair<double, QColor> >::const_iterator (m_nodes.begin ()), pmin));
      emit selection_changed ();
      emit selection_changed (m_nodes [m_selected].second);
      m_dragging = true;
      update ();
    } else {
      m_selected = -1;
      emit selection_changed ();
      emit selection_changed (QColor ());
      update ();
    }

  }

}

void 
ColorBar::mouseReleaseEvent (QMouseEvent *)
{
  if (m_dragging) {
    m_dragging = false;
  }
}

void 
ColorBar::mouseDoubleClickEvent (QMouseEvent *event)
{
  int xl = hframe_width;
  int xr = width () - hframe_width;

  int yb = height () - (frame_width + indicator_height + indicator_spacing);

  if (event->x () > xl && event->x () < xr && event->y () > yb - 5 && event->y () < yb + 5 + indicator_height + indicator_spacing) {

    double xx = double (event->x () - xl) / double (xr - xl);

    std::vector<std::pair<double, QColor> >::iterator p = std::lower_bound (m_nodes.begin (), m_nodes.end (), std::make_pair (xx, QColor ()), compare_first_of_node ());
    if (p != m_nodes.begin () && p != m_nodes.end ()) {
      m_selected = int (std::distance (m_nodes.begin (), p));
      m_nodes.insert (p, std::make_pair (xx, interpolated_color (m_nodes, xx)));
      emit selection_changed ();
      emit selection_changed (m_nodes [m_selected].second);
      emit color_mapping_changed ();
      update ();
    }

  }

}

QSize 
ColorBar::sizeHint () const
{
  return QSize (100, frame_width * 2 + indicator_height + indicator_spacing + nominal_bar_height);
}

void
ColorBar::set_histogram (const std::vector <size_t> &histogram) 
{
  m_histogram = histogram;
  update ();
}

void 
ColorBar::paintEvent (QPaintEvent *)
{
  QPainter painter (this);

  int yb = height () - (frame_width + indicator_height + indicator_spacing);
  int yt = frame_width;

  int xl = hframe_width;
  int xr = width () - hframe_width;

  size_t h_max = 0;
  for (std::vector <size_t>::const_iterator h = m_histogram.begin (); h != m_histogram.end (); ++h) {
    if (*h > h_max) {
      h_max = *h;
    }
  }
  
  for (int x = xl; x <= xr; ++x) {

    int hbar = yb - yt;

    if (m_histogram.size () > 0 && xr > xl) {

      size_t hi = int ((m_histogram.size () - 1) * (x - xl)) / (xr - xl);
      size_t hi_next = int ((m_histogram.size () - 1) * (x + 1 - xl)) / (xr - xl);

      if (hi_next == hi) {
        hi_next = hi + 1;
      }
      if (hi_next > m_histogram.size ()) {
        hi_next = m_histogram.size ();
      }

      size_t h = 0;
      size_t n = 0;
      while (hi < hi_next) {
        h += m_histogram [hi++];
        n += h_max;
      }

      if (n == 0) {
        n = h = 1;
      }

      hbar = int ((hbar - min_bar_height) * double (h) / double (n) + 0.5 + min_bar_height);

    }

    double xx = 0.0;
    if (xr != xl) {
      xx = double (x - xl) / double (xr - xl);
    }
    QColor c = interpolated_color (m_nodes, xx);

    painter.fillRect (x, yb - hbar, 1, hbar + 1, QBrush (c));

  }

  for (unsigned int i = 0; i < m_nodes.size (); ++i) {

    int x = int (xl + 0.5 + m_nodes[i].first * (xr - xl));

    QPoint points[3] = {
      QPoint (x, yb + indicator_spacing), 
      QPoint (x - indicator_height / 2, yb + indicator_spacing + indicator_height), 
      QPoint (x + indicator_height / 2, yb + indicator_spacing + indicator_height), 
    };

    if (int (i) == m_selected) {

      painter.setBrush (QBrush ());
      QPen outerPen (palette ().color (QPalette::Highlight));
      outerPen.setWidth (3);
      painter.setPen (outerPen);
      painter.drawPolygon (points, 3);

      painter.setBrush (palette ().color (QPalette::WindowText));
      painter.setPen (QPen ());
      painter.drawPolygon (points, 3);

      painter.setBrush (QBrush ());
      painter.setPen (palette ().color (QPalette::HighlightedText));
      painter.drawPolygon (points, 3);

    } else {
      painter.setBrush (QBrush ());
      painter.setPen (palette ().color (QPalette::WindowText));
      painter.drawPolygon (points, 3);
    }

  }
}

}

