
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

#ifndef HDR_imgWidgets
#define HDR_imgWidgets

#include "layWidgets.h"
#include "tlColor.h"
#include "imgObject.h"

#include <QObject>
#include <QWidget>
#include <QFrame>
#include <QToolButton>
#include <vector>

class QMouseEvent;
class QKeyEvent;
class QPaintEvent;

namespace img
{

/**
 *  @brief A two-color widget
 *
 *  This widget has two color buttons and a "lock" checkbox which makes both colors identical
 */
class TwoColorWidget
  : public QFrame
{
Q_OBJECT

public:
  TwoColorWidget (QWidget *parent);

signals:
  void color_changed (std::pair<QColor, QColor> c);

public slots:
  void set_color (std::pair<QColor, QColor> c);
  void set_single_mode (bool f);

private slots:
  void lcolor_changed (QColor c);
  void rcolor_changed (QColor c);
  void lock_changed (bool checked);

private:
  lay::SimpleColorButton *mp_left, *mp_right;
  QToolButton *mp_lock;
};

/**
 *  @brief A color bar widget
 */
class ColorBar
  : public QWidget
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  ColorBar (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~ColorBar ();

  void mouseMoveEvent (QMouseEvent *event);
  void mousePressEvent (QMouseEvent *event);
  void mouseReleaseEvent (QMouseEvent *event);
  void mouseDoubleClickEvent (QMouseEvent *event);
  void keyPressEvent (QKeyEvent *event);

  void paintEvent (QPaintEvent *event);

  QSize sizeHint () const;

  int selected_node () const
  {
    return m_selected;
  }

  bool has_selection () const
  {
    return m_selected >= 0;
  }

  void set_nodes (const std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > &nodes);

  const std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > &nodes () const
  {
    return m_nodes;
  }

  void set_histogram (const std::vector <size_t> &histogram);

public slots:
  void set_current_color (std::pair<QColor, QColor> c);
  void set_current_position (double x);

signals:
  void color_mapping_changed ();
  void selection_changed ();
  void selection_changed (std::pair<QColor, QColor> c);

private:
  bool m_dragging;
  int m_selected;
  std::vector <std::pair <double, std::pair<tl::Color, tl::Color> > > m_nodes;
  std::vector <size_t> m_histogram;
};

} // namespace img

#endif

#endif
