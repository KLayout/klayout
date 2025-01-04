
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

#ifndef HDR_layEditorOptionsFrame
#define HDR_layEditorOptionsFrame

#include "layuiCommon.h"
#include <QFrame>

namespace lay
{

class EditorOptionsPages;
class LayoutViewBase;

class LAYUI_PUBLIC EditorOptionsFrame
  : public QFrame
{
public:
  EditorOptionsFrame (QWidget *parent);
  virtual ~EditorOptionsFrame ();

  void populate (lay::LayoutViewBase *view);

  EditorOptionsPages *pages_widget () const
  {
    return mp_pages;
  }

public:
  EditorOptionsPages *mp_pages;
};

}

#endif

#endif  //  defined(HAVE_QT)
