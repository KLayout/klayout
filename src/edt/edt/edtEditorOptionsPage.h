
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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

#ifndef HDR_edtEditorOptionsPage
#define HDR_edtEditorOptionsPage

#include <QWidget>

namespace lay
{
  class PluginDeclaration;
  class Dispatcher;
  class Plugin;
}

namespace edt
{

class EditorOptionsPages;

/**
 *  @brief The base class for a object properties page
 */
class EditorOptionsPage
  : public QWidget
{
Q_OBJECT

public:
  EditorOptionsPage (lay::Dispatcher *dispatcher);
  virtual ~EditorOptionsPage ();

  virtual std::string title () const = 0;
  virtual int order () const = 0;
  virtual void apply (lay::Dispatcher * /*root*/) { }
  virtual void setup (lay::Dispatcher * /*root*/) { }
  virtual void commit_recent (lay::Dispatcher * /*root*/) { }

  bool active () const { return m_active; }
  void activate (bool active);
  void set_owner (EditorOptionsPages *owner);

  const lay::PluginDeclaration *plugin_declaration () const { return mp_plugin_declaration; }
  void set_plugin_declaration (const lay::PluginDeclaration *pd) { mp_plugin_declaration = pd; }

protected slots:
  void edited ()
  {
    apply (dispatcher ());
  }

protected:
  lay::Dispatcher *dispatcher () const
  {
    return mp_dispatcher;
  }

private:
  EditorOptionsPages *mp_owner;
  bool m_active;
  const lay::PluginDeclaration *mp_plugin_declaration;
  lay::Dispatcher *mp_dispatcher;
};

}

#endif
