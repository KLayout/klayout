
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#ifndef HDR_layEditStippleWidget
#define HDR_layEditStippleWidget

#include <QFrame>
#include <stdint.h>

#include "dbObject.h"

namespace lay
{

class EditStippleWidget
  : public QFrame, public db::Object
{
  Q_OBJECT 

public:
  EditStippleWidget (QWidget *parent);

  virtual QSize sizeHint () const;
  virtual QSize minimumSize () const;

  void set_pattern (const uint32_t *pattern, unsigned int sx, unsigned int sy);
  void set_pattern (const uint32_t * const *pattern, unsigned int sx, unsigned int sy);

  void undo (db::Op *op);
  void redo (db::Op *op);

  const uint32_t *pattern () const
  {
    return m_pattern;
  }

  virtual void paintEvent (QPaintEvent *event);
  virtual void mouseMoveEvent (QMouseEvent *event);
  virtual void mousePressEvent (QMouseEvent *event);
  virtual void mouseReleaseEvent (QMouseEvent *event);

  void set_size (unsigned int sx, unsigned int sy);

  void set_readonly (bool ro);

  bool readonly () const
  {
    return m_readonly;
  }

  void clear ();
  void invert ();
  void fliph ();
  void flipv ();
  void rotate (int a);
  void shift (int dx, int dy);

  unsigned int sx () const
  {
    return m_sx;
  }

  unsigned int sy () const
  {
    return m_sy;
  }

signals:
  void changed ();
  void size_changed ();

private:
  unsigned int m_last_mx, m_last_my;
  uint32_t m_last_pattern [32];
  unsigned int m_last_sx, m_last_sy;
  bool m_last_pattern_saved;
  uint32_t m_pattern [32];
  bool m_clearing;
  bool m_readonly;
  unsigned int m_sx, m_sy;

  void set_pixel (unsigned int x, unsigned int y, bool value);
  bool get_pixel (int x, int y);
  bool mouse_to_pixel (const QPoint &pt, unsigned int &x, unsigned int &y);
  void expand_pattern ();
};

}

#endif

#endif  //  defined(HAVE_QT)
