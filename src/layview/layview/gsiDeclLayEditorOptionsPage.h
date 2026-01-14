
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

#if !defined(_HDR_gsiDeclLayEditorOptionsPage)
#define _HDR_gsiDeclLayEditorOptionsPage

#if defined(HAVE_QTBINDINGS)

#include "gsiDecl.h"
#include "gsiDeclBasic.h"

#include "layEditorOptionsPageWidget.h"
#include "layLayoutViewBase.h"

namespace gsi
{

class EditorOptionsPageImpl
  : public lay::EditorOptionsPageWidget, public gsi::ObjectBase
{
public:
  EditorOptionsPageImpl (const std::string &title, int index);

  virtual std::string title () const
  {
    return m_title;
  }

  virtual int order () const
  {
    return m_index;
  }

  void call_edited ();
  virtual void apply (lay::Dispatcher *root);
  virtual void setup (lay::Dispatcher *root);
  virtual void cancel ();
  virtual void commit (lay::Dispatcher *root);

  gsi::Callback f_apply;
  gsi::Callback f_setup;
  gsi::Callback f_cancel;
  gsi::Callback f_commit;

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  tl::weak_ptr<lay::Dispatcher> mp_dispatcher;
  std::string m_title;
  int m_index;

  void apply_impl (lay::Dispatcher *root);
  void setup_impl (lay::Dispatcher *root);
  void cancel_impl ();
  void commit_impl (lay::Dispatcher *root);
};

}

#endif

#endif
