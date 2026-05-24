
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

#ifndef HDR_antRulerOptionsPage
#define HDR_antRulerOptionsPage

#include "antCommon.h"

#include "layEditorOptionsPageWidget.h"

namespace Ui
{
  class RulerOptions;
}

namespace ant
{

/**
 *  @brief The generic properties page
 */
class RulerOptionsPage
  : public lay::EditorOptionsPageWidget
{
Q_OBJECT

public:
  RulerOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  ~RulerOptionsPage ();

  virtual std::string title () const;
  virtual int order () const { return -10; }
  void apply (lay::Dispatcher *root);
  void setup (lay::Dispatcher *root);

private:
  Ui::RulerOptions *mp_ui;
};

}

#endif

#endif

