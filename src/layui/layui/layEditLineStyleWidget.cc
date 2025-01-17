
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "layEditLineStyleWidget.h"

#include "tlString.h"

#include <QPainter>
#include <QMouseEvent>

namespace lay
{

const int stipple_pixel_size = 12;
const int full_size = 40;
const int full_height = 9;

struct StyleStorageOp
  : public db::Op
{
  StyleStorageOp (uint32_t s, unsigned int w, bool b)
    : db::Op (), width (w), before (b)
  { 
    style = s;
  }

  unsigned int width;
  bool before;
  uint32_t style;
};

EditLineStyleWidget::EditLineStyleWidget (QWidget *parent)
  : QFrame (parent), db::Object (), m_last_mx (-1),
    m_last_sx (32), m_last_style_saved (false),
    m_clearing (false), m_readonly (false),
    m_sx (32)
{
  m_last_style = 0;
  m_style = 0;
  setBackgroundRole (QPalette::NoRole);
}

QSize 
EditLineStyleWidget::sizeHint () const
{
  return QSize (stipple_pixel_size * full_size + 1, stipple_pixel_size * full_height + 1);
}

QSize 
EditLineStyleWidget::minimumSize () const
{
  return QSize (stipple_pixel_size * full_size + 1, stipple_pixel_size + 1);
}

void
EditLineStyleWidget::expand_style ()
{
  if (m_sx == 0) {
    m_style = 0xffffffff;
  } else if (m_sx < 32) {
    uint32_t w = m_style;
    w &= (1 << m_sx) - 1;
    for (size_t j = m_sx; j < 32; j += m_sx) {
      w |= (w << m_sx);
    }
    m_style = w;
  }
}

bool
EditLineStyleWidget::get_pixel (int x)
{
  if (m_sx == 0) {
    return true;
  }

  while (x < 0) {
    x += m_sx;
  }
  x %= m_sx;

  return (m_style & (1 << x)) != 0;
}

void
EditLineStyleWidget::set_pixel (unsigned int xx, bool value)
{
  if (xx >= 32 || m_sx < 1) {
    return; 
  }

  for (int x = xx; x < 32; x += m_sx) {

    uint32_t w = m_style;
    if (value) {
      w |= (1 << x);
    } else {
      w &= ~(1 << x);
    }
    m_style = w;

  }
}

bool 
EditLineStyleWidget::mouse_to_pixel (const QPoint &pt, unsigned int &x)
{
  int ix = pt.x () / stipple_pixel_size;
  ix -= (full_size - 32) / 2;

  if (ix >= 0 && ix < int (m_sx)) {
    x = (unsigned int) ix;
    return true;
  } else {
    x = 0;
    return false;
  }
}

void 
EditLineStyleWidget::mouseMoveEvent (QMouseEvent *event)
{
  if ((event->buttons () & Qt::LeftButton) != 0 && ! m_readonly) {

    unsigned int mx;
    if (! mouse_to_pixel (event->pos (), mx)) {
      return;
    }

    if (mx != m_last_mx) {

      m_last_mx = mx;

      if (get_pixel (mx) == m_clearing) {
        set_pixel (mx, ! m_clearing);
        emit changed ();
        update ();
      }

    }

  }
}

void 
EditLineStyleWidget::mousePressEvent (QMouseEvent *event)
{
  if ((event->buttons () & Qt::LeftButton) != 0 && ! m_readonly) {

    m_last_style = m_style;
    m_last_sx = m_sx;
    m_last_style_saved = true;

    unsigned int mx;
    if (! mouse_to_pixel (event->pos (), mx)) {
      return;
    }

    m_last_mx = mx;

    m_clearing = get_pixel (mx);

    if (get_pixel (mx) == m_clearing) {
      set_pixel (mx, ! m_clearing);
      emit changed ();
      update ();
    }

  }
}

void 
EditLineStyleWidget::mouseReleaseEvent (QMouseEvent *)
{
  if (m_last_style_saved) {
    m_last_style_saved = false;
    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Edit style")));
      manager ()->queue (this, new StyleStorageOp (m_last_style, m_last_sx, true));
      manager ()->queue (this, new StyleStorageOp (m_style, m_sx, false));
      manager ()->commit ();
    }
  }
}

void 
EditLineStyleWidget::paintEvent (QPaintEvent *)
{
  QPainter painter (this);

  QColor cf = palette ().color (QPalette::AlternateBase);
  QColor c0 = palette ().color (QPalette::Base);
  QColor c1 = palette ().color (QPalette::Text);

  QColor cdis ((c0.red () + c1.red ()) / 2,
               (c0.green () + c1.green ()) / 2,
               (c0.blue () + c1.blue ()) / 2);

  painter.setPen (QPen (cf));

  for (unsigned int i = 0; i < (unsigned int) full_size; ++i) {
    painter.drawLine (i * stipple_pixel_size, 0, i * stipple_pixel_size, full_height * stipple_pixel_size);
  }
  for (unsigned int i = 0; i < (unsigned int) full_height; ++i) {
    painter.drawLine (0, i * stipple_pixel_size, full_size * stipple_pixel_size, i * stipple_pixel_size);
  }

  for (unsigned int i = 0; i < (unsigned int) full_size; ++i) {

    QBrush b0 (c0);
    QBrush b1 (m_readonly ? cdis : c1);
    QBrush bd (cdis);
    QBrush bf (cf);

    for (unsigned int j = 0; j < (unsigned int) full_height; ++j) {
      QRect r (i * stipple_pixel_size + 1, j * stipple_pixel_size + 1, stipple_pixel_size - 1, stipple_pixel_size - 1);
      int bx = i - (full_size - 32) / 2;
      int by = int (j) - full_height / 2;
      if (by == 0 && get_pixel (bx)) {
        if (bx < 0 || bx >= int (m_sx)) {
          painter.fillRect (r, bd);
        } else {
          painter.fillRect (r, b1);
        }
      } else if (((i + j) & 1) == 0) {
        painter.fillRect (r, b0);
      } else {
        painter.fillRect (r, bf);
      }
    }

  }

  painter.drawLine (full_size * stipple_pixel_size, 0, full_size * stipple_pixel_size, full_height * stipple_pixel_size);
  painter.drawLine (0, full_height * stipple_pixel_size, full_size * stipple_pixel_size, full_height * stipple_pixel_size);

  painter.setPen (QPen (c1));

  int fl = stipple_pixel_size * (full_size - 32) / 2;
  int fr = fl + stipple_pixel_size * m_sx;
  int ft = stipple_pixel_size * (full_height / 2);
  int fb = ft + stipple_pixel_size;

  painter.drawLine (fl - 2, ft - 2, fr + 2, ft - 2);
  painter.drawLine (fr + 2, ft - 2, fr + 2, fb + 2);
  painter.drawLine (fr + 2, fb + 2, fl - 2, fb + 2);
  painter.drawLine (fl - 2, fb + 2, fl - 2, ft - 2);
}

void 
EditLineStyleWidget::set_style (uint32_t pattern, unsigned int w)
{
  if (w != m_sx) {
    m_sx = w;
    emit size_changed ();
  }

  m_style = pattern;
  update ();
}

void
EditLineStyleWidget::set_readonly (bool readonly)
{
  if (m_readonly != readonly) {
    m_readonly = readonly;
    update ();
  }
}

void 
EditLineStyleWidget::clear ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
  }

  m_style = 0;
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, false));
  }
}

void 
EditLineStyleWidget::invert ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
  }

  m_style = ~m_style;
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, false));
  }
}

void
EditLineStyleWidget::set_size (unsigned int sx)
{
  if (sx != m_sx) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
    }

    m_sx = sx;

    expand_style ();

    update ();
    emit changed ();

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
    }

  }
}

void
EditLineStyleWidget::fliph ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
  }

  uint32_t w = 0;
  for (unsigned int j = 0; j < m_sx; ++j) {
    w <<= 1;
    w |= ((m_style & (1 << j)) != 0 ? 1 : 0);
  }
  m_style = w;

  expand_style ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, false));
  }
}

void 
EditLineStyleWidget::shift (int dx)
{
  if (m_sx == 0) {
    return;
  }

  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, true));
  }

  uint32_t w = m_style;
  if (dx < 0) {
    for (int k = 0; k < -dx; ++k) {
      uint32_t b0 = ((w & 1) != 0);
      w = (w >> 1) | (b0 << (m_sx - 1));
    }
  } else if (dx != 0) {
    for (int k = 0; k < dx; ++k) {
      uint32_t b0 = ((w & (1 << (m_sx - 1))) != 0);
      w = (w << 1) | b0;
    }
  }
  m_style = w;

  expand_style ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new StyleStorageOp (m_style, m_sx, false));
  }
}

void 
EditLineStyleWidget::undo (db::Op *op)
{
  StyleStorageOp *pop = dynamic_cast <StyleStorageOp *> (op);
  if (pop && pop->before) {
    set_style (pop->style, pop->width);
    emit changed ();
  }
}

void 
EditLineStyleWidget::redo (db::Op *op)
{
  StyleStorageOp *pop = dynamic_cast <StyleStorageOp *> (op);
  if (pop && ! pop->before) {
    set_style (pop->style, pop->width);
    emit changed ();
  }
}

}

#endif
