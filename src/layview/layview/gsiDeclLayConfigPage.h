
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

#if !defined(_HDR_gsiDeclLayConfigPage)
#define _HDR_gsiDeclLayConfigPage

#if defined(HAVE_QTBINDINGS)

#include "gsiDecl.h"
#include "gsiDeclBasic.h"

#include "layPluginConfigPage.h"
#include "layLayoutViewBase.h"

namespace gsi
{

class ConfigPageImpl
  : public lay::ConfigPage, public gsi::ObjectBase
{
public:
  ConfigPageImpl (const std::string &title);

  virtual std::string title () const
  {
    return m_title;
  }

  void commit_impl (lay::Dispatcher *root);
  virtual void commit (lay::Dispatcher *root);
  void setup_impl (lay::Dispatcher *root);
  virtual void setup (lay::Dispatcher *root);

  gsi::Callback f_commit;
  gsi::Callback f_setup;

private:
  tl::weak_ptr<lay::LayoutViewBase> mp_view;
  tl::weak_ptr<lay::Dispatcher> mp_dispatcher;
  std::string m_title;
  std::string m_index;
};

}

#endif

#endif
