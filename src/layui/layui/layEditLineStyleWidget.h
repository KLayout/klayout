
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

#ifndef HDR_layEditLineStyleWidget
#define HDR_layEditLineStyleWidget

#include <QFrame>
#include <stdint.h>

#include "dbObject.h"

namespace lay
{

class EditLineStyleWidget
  : public QFrame, public db::Object
{
  Q_OBJECT 

public:
  EditLineStyleWidget (QWidget *parent);

  virtual QSize sizeHint () const;
  virtual QSize minimumSize () const;

  void set_style (uint32_t s, unsigned int sx);

  void undo (db::Op *op);
  void redo (db::Op *op);

  uint32_t style () const
  {
    return m_style;
  }

  virtual void paintEvent (QPaintEvent *event);
  virtual void mouseMoveEvent (QMouseEvent *event);
  virtual void mousePressEvent (QMouseEvent *event);
  virtual void mouseReleaseEvent (QMouseEvent *event);

  void set_size (unsigned int sx);

  void set_readonly (bool ro);

  bool readonly () const
  {
    return m_readonly;
  }

  void clear ();
  void invert ();
  void fliph ();
  void shift (int dx);

  unsigned int sx () const
  {
    return m_sx;
  }

signals:
  void changed ();
  void size_changed ();

private:
  unsigned int m_last_mx;
  uint32_t m_last_style;
  unsigned int m_last_sx;
  bool m_last_style_saved;
  uint32_t m_style;
  bool m_clearing;
  bool m_readonly;
  unsigned int m_sx;

  void set_pixel (unsigned int x, bool value);
  bool get_pixel (int x);
  bool mouse_to_pixel (const QPoint &pt, unsigned int &x);
  void expand_style ();
};

}

#endif

#endif  //  defined(HAVE_QT)
