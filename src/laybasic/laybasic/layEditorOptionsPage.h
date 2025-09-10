
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

#ifndef HDR_layEditorOptionsPage
#define HDR_layEditorOptionsPage

#include "laybasicCommon.h"

#include "tlObject.h"

#include <QWidget>

namespace db
{
  struct LayerProperties;
}

namespace lay
{

class PluginDeclaration;
class Dispatcher;
class LayoutViewBase;
class Plugin;
class CellView;
class EditorOptionsPages;

/**
 *  @brief The base class for a object properties page
 */
class LAYBASIC_PUBLIC EditorOptionsPage
  : public QWidget, public tl::Object
{
Q_OBJECT

public:
  EditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  EditorOptionsPage ();
  virtual ~EditorOptionsPage ();

  virtual std::string title () const = 0;
  virtual int order () const = 0;
  virtual void apply (lay::Dispatcher * /*root*/) { }
  virtual void setup (lay::Dispatcher * /*root*/) { }
  virtual void commit_recent (lay::Dispatcher * /*root*/) { }
  virtual void config_recent_for_layer (lay::Dispatcher * /*root*/, const db::LayerProperties & /*lp*/, int /*cv_index*/) { }

  bool is_focus_page () const { return m_focus_page; }
  void set_focus_page (bool f) { m_focus_page = f; }
  void set_focus ();

  bool is_modal_page () const { return m_modal_page; }
  void set_modal_page (bool f) { m_modal_page = f; }

  bool active () const { return m_active; }
  void activate (bool active);
  void set_owner (EditorOptionsPages *owner);

  /**
   *  @brief Shows the editor page
   *  @return -1, if the page is shown non-modal, otherwise 1 or 0 if the dialog was accepted (1) or rejected (0)
   */
  int show ();

  const lay::PluginDeclaration *plugin_declaration () const { return mp_plugin_declaration; }
  void set_plugin_declaration (const lay::PluginDeclaration *pd) { mp_plugin_declaration = pd; }

  void init (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);

  lay::Dispatcher *dispatcher () const
  {
    return mp_dispatcher;
  }

  lay::LayoutViewBase *view () const
  {
    return mp_view;
  }

protected slots:
  void edited ();

protected:
  virtual void active_cellview_changed () { }
  virtual void technology_changed (const std::string & /*tech*/) { }

  virtual bool focusNextPrevChild (bool next);
  virtual void keyPressEvent (QKeyEvent *event);

private:
  EditorOptionsPages *mp_owner;
  bool m_active;
  bool m_focus_page;
  bool m_modal_page;
  const lay::PluginDeclaration *mp_plugin_declaration;
  lay::Dispatcher *mp_dispatcher;
  lay::LayoutViewBase *mp_view;

  void on_active_cellview_changed ();
  void on_technology_changed ();
  void attach_events ();
};

}

#endif

#endif  //  defined(HAVE_QT)
