
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

#ifndef HDR_layEditorOptionsPages
#define HDR_layEditorOptionsPages

#include "layuiCommon.h"
#include "layEditorOptionsPage.h"

#include <tlVariant.h>

#include <QFrame>
#include <vector>
#include <string>

class QTabWidget;
class QLabel;

namespace lay
{

class PluginDeclaration;
class Dispatcher;
class Plugin;

/**
 *  @brief The object properties dialog
 */
class LAYUI_PUBLIC EditorOptionsPages
  : public QFrame
{
Q_OBJECT

public:
  EditorOptionsPages (QWidget *parent, const std::vector<lay::EditorOptionsPage *> &pages, lay::Dispatcher *root);
  ~EditorOptionsPages ();

  void unregister_page (lay::EditorOptionsPage *page);
  void activate_page (lay::EditorOptionsPage *page);
  void focusInEvent (QFocusEvent *event);

  const std::vector <lay::EditorOptionsPage *> &pages () const
  {
    return m_pages;
  }

  bool has_content () const;

public slots:
  void apply ();
  void setup ();

private:
  std::vector <lay::EditorOptionsPage *> m_pages;
  lay::Dispatcher *mp_dispatcher;
  QTabWidget *mp_pages;

  void update (lay::EditorOptionsPage *page);
  void do_apply ();
};

}

#endif

#endif  //  defined(HAVE_QT)
