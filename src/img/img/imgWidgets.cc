
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

#if defined(HAVE_QT)

#include "imgWidgets.h"

#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QHBoxLayout>

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

// --------------------------------------------------------------------------------------------------------------------

TwoColorWidget::TwoColorWidget (QWidget *parent)
  : QFrame (parent)
{
  setLayout (new QHBoxLayout (this));

  mp_left = new lay::SimpleColorButton (this);
  layout ()->addWidget (mp_left);
  mp_right = new lay::SimpleColorButton (this);
  layout ()->addWidget (mp_right);
  mp_lock = new QToolButton (this);
  layout ()->addWidget (mp_lock);
  mp_lock->setCheckable (true);
  mp_lock->setAutoRaise (true);
  mp_lock->setIconSize (QSize (16, 16));

  QIcon icon;
  icon.addFile (":/locked_16px.png", QSize (), QIcon::Normal, QIcon::On);
  icon.addFile (":/unlocked_16px.png", QSize (), QIcon::Normal, QIcon::Off);
  mp_lock->setIcon (icon);

  connect (mp_left, SIGNAL (color_changed (QColor)), this, SLOT (lcolor_changed (QColor)));
  connect (mp_right, SIGNAL (color_changed (QColor)), this, SLOT (rcolor_changed (QColor)));
  connect (mp_lock, SIGNAL (clicked (bool)), this, SLOT (lock_changed (bool)));
}

void
TwoColorWidget::set_color (std::pair<QColor, QColor> c)
{
  mp_left->set_color (c.first);
  mp_right->set_color (c.second);
  mp_lock->setChecked (c.first == c.second);
  mp_right->setVisible (! mp_lock->isChecked ());
}

void
TwoColorWidget::set_single_mode (bool f)
{
  mp_lock->setEnabled (! f);
}

void
TwoColorWidget::lcolor_changed (QColor)
{
  if (mp_lock->isChecked ()) {
    mp_right->set_color (mp_left->get_color ());
  }
  emit color_changed (std::make_pair (mp_left->get_color (), mp_right->get_color ()));
}

void
TwoColorWidget::rcolor_changed (QColor)
{
  if (mp_lock->isChecked ()) {
    mp_left->set_color (mp_right->get_color ());
  }
  emit color_changed (std::make_pair (mp_left->get_color (), mp_right->get_color ()));
}

void
TwoColorWidget::lock_changed (bool checked)
{
  if (checked) {

    QColor cl = mp_left->get_color ();
    QColor cr = mp_right->get_color ();

    QColor ca ((cl.red () + cr.red ()) / 2, (cl.green () + cr.green ()) / 2, (cl.blue () + cr.blue ()) / 2);
    set_color (std::make_pair (ca, ca));

    emit color_changed (std::make_pair (mp_left->get_color (), mp_right->get_color ()));

  }

  mp_right->setVisible (! mp_lock->isChecked ());
}

// --------------------------------------------------------------------------------------------------------------------

ColorBar::ColorBar (QWidget *parent)
  : QWidget (parent), m_dragging (false), m_selected (-1)
{
  m_nodes.push_back (std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  m_nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
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
ColorBar::set_current_color (std::pair<QColor, QColor> c)
{
  if (has_selection ()) {
    m_nodes [m_selected].second = std::make_pair (tl::Color (c.first.rgb ()), tl::Color (c.second.rgb ()));
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
    emit selection_changed (std::make_pair (QColor (), QColor ()));
    update ();
  }
}

namespace
{

struct compare_first_of_node
{
  bool operator() (const std::pair <double, std::pair<tl::Color, tl::Color> > &a, const std::pair <double, std::pair<tl::Color, tl::Color> > &b) const
  {
    return a.first < b.first;
  }
};

}

void 
ColorBar::set_nodes (const std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > > &nodes)
{
  m_nodes = nodes;

  std::sort (m_nodes.begin (), m_nodes.end (), compare_first_of_node ());

  if (m_nodes.size () == 0 || fabs (m_nodes[0].first) > epsilon) {
    m_nodes.insert (m_nodes.begin (), std::make_pair (0.0, std::make_pair (tl::Color (0, 0, 0), tl::Color (0, 0, 0))));
  } else {
    m_nodes[0].first = 0.0;
  }

  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > >::iterator w = m_nodes.begin ();
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > >::const_iterator nn = m_nodes.begin ();
  for (std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > >::const_iterator n = m_nodes.begin () + 1; n != m_nodes.end (); ++n) {
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
    m_nodes.push_back (std::make_pair (1.0, std::make_pair (tl::Color (255, 255, 255), tl::Color (255, 255, 255))));
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
    std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > >::const_iterator pmin = m_nodes.end ();
    for (std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > >::const_iterator p = m_nodes.begin (); p != m_nodes.end (); ++p) {
      double d = fabs (p->first - xx);
      if (d < 0.05 && d < dmin) {
        dmin = d;
        pmin = p;
      }
    }

    if (pmin != m_nodes.end ()) {
      m_selected = int (std::distance (std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > >::const_iterator (m_nodes.begin ()), pmin));
      emit selection_changed ();
      std::pair<tl::Color, tl::Color> cp = m_nodes [m_selected].second;
      emit selection_changed (std::make_pair (QColor (cp.first.rgb ()), QColor (cp.second.rgb ())));
      m_dragging = true;
      update ();
    } else {
      m_selected = -1;
      emit selection_changed ();
      emit selection_changed (std::make_pair (QColor (), QColor ()));
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

    std::vector<std::pair<double, std::pair<tl::Color, tl::Color> > >::iterator p = std::lower_bound (m_nodes.begin (), m_nodes.end (), std::make_pair (xx, std::make_pair (tl::Color (), tl::Color ())), compare_first_of_node ());
    if (p != m_nodes.begin () && p != m_nodes.end ()) {
      m_selected = int (std::distance (m_nodes.begin (), p));
      tl::Color ci = interpolated_color (m_nodes, xx);
      m_nodes.insert (p, std::make_pair (xx, std::make_pair (ci, ci)));
      emit selection_changed ();
      std::pair<tl::Color, tl::Color> cp = m_nodes [m_selected].second;
      emit selection_changed (std::make_pair (QColor (cp.first.rgb ()), QColor (cp.second.rgb ())));
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
    tl::Color c = interpolated_color (m_nodes, xx);

    painter.fillRect (x, yb - hbar, 1, hbar + 1, QBrush (QColor (c.rgb ())));

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

#endif

