
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#include "layEditStippleWidget.h"

#include "tlString.h"

#include <QPainter>
#include <QMouseEvent>

namespace lay
{

const int stipple_pixel_size = 12;
const int full_size = 40;

struct PatternStorageOp 
  : public db::Op
{
  PatternStorageOp (const uint32_t *p, unsigned int w, unsigned int h, bool b)
    : db::Op (), width (w), height (h), before (b)
  { 
    memcpy (pattern, p, sizeof (pattern));    
  }

  unsigned int width, height;
  bool before;
  uint32_t pattern [32];
};

EditStippleWidget::EditStippleWidget (QWidget *parent)
  : QFrame (parent), db::Object (), m_last_mx (-1), m_last_my (0), 
    m_last_sx (32), m_last_sy (32), m_last_pattern_saved (false),
    m_clearing (false), m_readonly (false),
    m_sx (32), m_sy (32)
{
  memset (m_last_pattern, 0, sizeof (m_last_pattern));
  memset (m_pattern, 0, sizeof (m_pattern));
  setBackgroundRole (QPalette::NoRole);
}

QSize 
EditStippleWidget::sizeHint () const
{
  return QSize (stipple_pixel_size * full_size + 1, stipple_pixel_size * full_size + 1);
}

QSize 
EditStippleWidget::minimumSize () const
{
  return QSize (stipple_pixel_size * full_size + 1, stipple_pixel_size * full_size + 1);
}

void
EditStippleWidget::expand_pattern ()
{
  if (m_sx < 32) {
    for (size_t i = 0; i < m_sy; ++i) {
      uint32_t w = m_pattern [i];
      w &= (1 << m_sx) - 1;
      for (size_t j = m_sx; j < 32; j += m_sx) {
        w |= (w << m_sx);
      }
      m_pattern [i] = w;
    }
  }

  for (size_t i = m_sy; i < 32; ++i) {
    m_pattern [i] = m_pattern [i - m_sy];
  }
}

bool
EditStippleWidget::get_pixel (int x, int y)
{
  while (x < 0) {
    x += m_sx;
  }
  x %= m_sx;

  while (y < 0) {
    y += m_sy;
  }
  y %= m_sy;

  uint32_t w = m_pattern [y];
  return (w & (1 << x)) != 0;
}

void
EditStippleWidget::set_pixel (unsigned int xx, unsigned int yy, bool value)
{
  if (xx >= 32 || yy >= 32) {
    return; 
  }

  for (int x = xx; x < 32; x += m_sx) {
    for (int y = yy; y < 32; y += m_sy) {

      uint32_t w = m_pattern [y];

      if (value) {
        w |= (1 << x);
      } else {
        w &= ~(1 << x);
      }

      m_pattern [y] = w;

    }
  }
}

bool 
EditStippleWidget::mouse_to_pixel (const QPoint &pt, unsigned int &x, unsigned int &y)
{
  int ix = pt.x () / stipple_pixel_size;
  int iy = (height () - 1 - pt.y ()) / stipple_pixel_size;

  ix -= (full_size - 32) / 2;
  iy -= (full_size - 32) / 2;

  if (ix >= 0 && ix < int (m_sx) && iy >= 0 && iy < int (m_sy)) {
    x = (unsigned int) ix;
    y = (unsigned int) iy;
    return true;
  } else {
    x = y = 0;
    return false;
  }
}

void 
EditStippleWidget::mouseMoveEvent (QMouseEvent *event)
{
  if ((event->buttons () & Qt::LeftButton) != 0 && ! m_readonly) {

    unsigned int mx, my;
    if (! mouse_to_pixel (event->pos (), mx, my)) {
      return;
    }

    if (mx != m_last_mx || my != m_last_my) {

      m_last_mx = mx;
      m_last_my = my;

      if (get_pixel (mx, my) == m_clearing) {
        set_pixel (mx, my, ! m_clearing);
        emit changed ();
        update ();
      }

    }

  }
}

void 
EditStippleWidget::mousePressEvent (QMouseEvent *event)
{
  if ((event->buttons () & Qt::LeftButton) != 0 && ! m_readonly) {

    memcpy (m_last_pattern, m_pattern, sizeof (m_pattern));
    m_last_sx = m_sx;
    m_last_sy = m_sy;
    m_last_pattern_saved = true;

    unsigned int mx, my;
    if (! mouse_to_pixel (event->pos (), mx, my)) {
      return;
    }

    m_last_mx = mx;
    m_last_my = my;

    m_clearing = get_pixel (mx, my);

    if (get_pixel (mx, my) == m_clearing) {
      set_pixel (mx, my, ! m_clearing);
      emit changed ();
      update ();
    }

  }
}

void 
EditStippleWidget::mouseReleaseEvent (QMouseEvent *)
{
  if (m_last_pattern_saved) {
    m_last_pattern_saved = false;
    if (manager ()) {
      manager ()->transaction (tl::to_string (QObject::tr ("Edit pattern")));
      manager ()->queue (this, new PatternStorageOp (m_last_pattern, m_last_sx, m_last_sy, true));
      manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
      manager ()->commit ();
    }
  }
}

void 
EditStippleWidget::paintEvent (QPaintEvent *)
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

    painter.drawLine (i * stipple_pixel_size, 0, i * stipple_pixel_size, full_size * stipple_pixel_size);
    painter.drawLine (0, i * stipple_pixel_size, full_size * stipple_pixel_size, i * stipple_pixel_size);

    QBrush b0 (c0);
    QBrush b1 (m_readonly ? cdis : c1);
    QBrush bd (cdis);
    QBrush bf (cf);

    for (unsigned int j = 0; j < (unsigned int) full_size; ++j) {
      QRect r (i * stipple_pixel_size + 1, j * stipple_pixel_size + 1, stipple_pixel_size - 1, stipple_pixel_size - 1);
      int bx = i - (full_size - 32) / 2;
      int by = 32 - (j - (full_size - 32) / 2) - 1;
      if (get_pixel (bx, by)) {
        if (bx < 0 || bx >= int (m_sx) || by < 0 || by >= int (m_sy)) {
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

  painter.drawLine (full_size * stipple_pixel_size, 0, full_size * stipple_pixel_size, full_size * stipple_pixel_size);
  painter.drawLine (0, full_size * stipple_pixel_size, full_size * stipple_pixel_size, full_size * stipple_pixel_size);

  painter.setPen (QPen (c1));

  int fl = stipple_pixel_size * (full_size - 32) / 2;
  int fr = fl + stipple_pixel_size * m_sx;
  int fb = height () - 1 - stipple_pixel_size * (full_size - 32) / 2;
  int ft = fb - stipple_pixel_size * m_sy;

  painter.drawLine (fl - 2, ft - 2, fr + 2, ft - 2);
  painter.drawLine (fr + 2, ft - 2, fr + 2, fb + 2);
  painter.drawLine (fr + 2, fb + 2, fl - 2, fb + 2);
  painter.drawLine (fl - 2, fb + 2, fl - 2, ft - 2);

}

void 
EditStippleWidget::set_pattern (const uint32_t * const *pattern, unsigned int w, unsigned int h)
{
  if (w != m_sx || h != m_sy) {
    m_sx = w;
    m_sy = h;
    emit size_changed ();
  }

  for (size_t i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
    m_pattern [i] = *(pattern [i]);
  }
  update ();
}

void
EditStippleWidget::set_pattern (const uint32_t *pattern, unsigned int w, unsigned int h)
{
  if (w != m_sx || h != m_sy) {
    m_sx = w;
    m_sy = h;
    emit size_changed ();
  }

  for (size_t i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
    m_pattern [i] = pattern [i];
  }
  update ();
}

void
EditStippleWidget::set_readonly (bool readonly)
{
  if (m_readonly != readonly) {
    m_readonly = readonly;
    update ();
  }
}

void 
EditStippleWidget::clear ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  for (unsigned int i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
    m_pattern [i] = 0;
  }
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void 
EditStippleWidget::invert ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  for (unsigned int i = 0; i < sizeof (m_pattern) / sizeof (m_pattern [0]); ++i) {
    m_pattern [i] = ~m_pattern [i];
  }
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void
EditStippleWidget::set_size (unsigned int sx, unsigned int sy)
{
  if (sx != m_sx || sy != m_sy) {

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
    }

    m_sx = sx;
    m_sy = sy;

    expand_pattern ();

    update ();
    emit changed ();

    if (manager () && manager ()->transacting ()) {
      manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
    }

  }
}

void
EditStippleWidget::fliph ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  for (unsigned int i = 0; i < m_sy; ++i) {
    uint32_t w = 0;
    for (unsigned int j = 0; j < m_sx; ++j) {
      w <<= 1;
      w |= ((m_pattern [i] & (1 << j)) != 0 ? 1 : 0); 
    }
    m_pattern [i] = w;
  }

  expand_pattern ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void 
EditStippleWidget::flipv ()
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  uint32_t p[32];
  memcpy (p, m_pattern, sizeof (p));

  for (unsigned int i = 0; i < m_sy; ++i) {
    m_pattern [m_sy - i - 1] = p[i];
  }

  expand_pattern ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void 
EditStippleWidget::rotate (int a)
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  while (a < 0) {
    a += 360;
  }
  while (a >= 360) {
    a -= 360;
  }

  unsigned int dim = std::max (m_sx, m_sy);

  while (a > 0) {

    uint32_t bits [32];

    for (unsigned int i = 0; i < dim; ++i) {
      uint32_t w = 0;
      for (unsigned int j = 0; j < dim; ++j) {
        w <<= 1;
        w |= ((m_pattern [j] & (1 << i)) != 0 ? 1 : 0); 
      }
      bits [i] = w;
    }

    memcpy (m_pattern, bits, sizeof (m_pattern));

    a -= 90;

  } 

  expand_pattern ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void 
EditStippleWidget::shift (int dx, int dy)
{
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, true));
  }

  uint32_t bits [32];

  for (unsigned int i = 0; i < m_sy; ++i) {
    uint32_t w = m_pattern [i];
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
    bits [((unsigned int)(m_sy + i + dy)) % m_sy] = w;
  }

  memcpy (m_pattern, bits, sizeof (m_pattern));

  expand_pattern ();
  update ();
  emit changed ();
  
  if (manager () && manager ()->transacting ()) {
    manager ()->queue (this, new PatternStorageOp (m_pattern, m_sx, m_sy, false));
  }
}

void 
EditStippleWidget::undo (db::Op *op)
{
  PatternStorageOp *pop = dynamic_cast <PatternStorageOp *> (op);
  if (pop && pop->before) {
    set_pattern (pop->pattern, pop->width, pop->height);
    emit changed ();
  }
}

void 
EditStippleWidget::redo (db::Op *op)
{
  PatternStorageOp *pop = dynamic_cast <PatternStorageOp *> (op);
  if (pop && ! pop->before) {
    set_pattern (pop->pattern, pop->width, pop->height);
    emit changed ();
  }
}

}

#endif
