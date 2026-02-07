
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

#ifndef HDR_layEditorOptionsPageWidget
#define HDR_layEditorOptionsPageWidget

#if defined(HAVE_QT)

#include <QWidget>

#include "layviewCommon.h"
#include "layEditorOptionsPage.h"

namespace lay
{

/**
 *  @brief The base class for a object properties page
 */
class LAYVIEW_PUBLIC EditorOptionsPageWidget
  : public QWidget, public EditorOptionsPage
{
Q_OBJECT

public:
  EditorOptionsPageWidget (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  EditorOptionsPageWidget ();
  virtual ~EditorOptionsPageWidget ();

  virtual void set_focus ();
  virtual bool is_visible () const;
  virtual void set_visible (bool visible);
  virtual EditorOptionsPageWidget *widget () { return this; }

  void set_transparent (bool f);
  bool is_transparent () const { return m_is_transparent; }

protected slots:
  void edited ();

protected:
  virtual bool focusNextPrevChild (bool next);
  virtual void keyPressEvent (QKeyEvent *event);
  virtual void resizeEvent (QResizeEvent *e);
  virtual bool event (QEvent *event);

  bool m_is_transparent;
};

}

#endif

#endif

