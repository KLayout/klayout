
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

#ifndef HDR_layEditorOptionsPage
#define HDR_layEditorOptionsPage

#include "laybasicCommon.h"

#include "tlObject.h"

#if defined(HAVE_QT)
#  include <QWidget>
#endif

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
class EditorOptionsPage;
class EditorOptionsPageWidget;

/**
 *  @brief An interface managing a collection of EditorOptionPage objects
 */
class LAYBASIC_PUBLIC EditorOptionsPageCollection
{
public:
  EditorOptionsPageCollection ();
  virtual ~EditorOptionsPageCollection () { }

  virtual void unregister_page (EditorOptionsPage *page) = 0;
  virtual bool has_content () const = 0;
  virtual bool has_modal_content () const = 0;
  virtual void make_page_current (EditorOptionsPage *page) = 0;
  virtual void activate_page (EditorOptionsPage *page) = 0;
  virtual void activate (const lay::Plugin *plugin) = 0;
  virtual bool exec_modal (EditorOptionsPage *page) = 0;
  virtual std::vector<lay::EditorOptionsPage *> editor_options_pages (const lay::PluginDeclaration *plugin) = 0;
  virtual std::vector<lay::EditorOptionsPage *> editor_options_pages () = 0;
  virtual lay::EditorOptionsPage *page_with_name (const std::string &name) = 0;
};

/**
 *  @brief The base class for a object properties page
 */
class LAYBASIC_PUBLIC EditorOptionsPage
  : public tl::Object
{
public:
  EditorOptionsPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  EditorOptionsPage ();
  virtual ~EditorOptionsPage ();

  virtual std::string title () const = 0;
  virtual int order () const = 0;
  virtual const char *name () const { return 0; }
  virtual void apply (lay::Dispatcher * /*root*/) { }
  virtual void setup (lay::Dispatcher * /*root*/) { }
  virtual void commit_recent (lay::Dispatcher * /*root*/) { }
  virtual void config_recent_for_layer (lay::Dispatcher * /*root*/, const db::LayerProperties & /*lp*/, int /*cv_index*/) { }
  virtual void set_focus () { }
  virtual void set_visible (bool /*visible*/) { }
  virtual EditorOptionsPageWidget *widget () { return 0; }

  bool is_focus_page () const { return m_focus_page; }
  void set_focus_page (bool f) { m_focus_page = f; }

  bool is_modal_page () const { return m_modal_page; }
  void set_modal_page (bool f) { m_modal_page = f; }

  bool is_toolbox_widget () const { return m_toolbox_widget; }
  void set_toolbox_widget (bool f) { m_toolbox_widget = f; }

  bool active () const { return m_active; }
  void activate (bool active);
  void set_owner (EditorOptionsPageCollection *owner);

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

protected:
  virtual void active_cellview_changed () { }
  virtual void technology_changed (const std::string & /*tech*/) { }
  virtual void activated () { }
  virtual void deactivated () { }

private:
  EditorOptionsPageCollection *mp_owner;
  bool m_active;
  bool m_focus_page;
  bool m_modal_page, m_toolbox_widget;
  const lay::PluginDeclaration *mp_plugin_declaration;
  lay::Dispatcher *mp_dispatcher;
  lay::LayoutViewBase *mp_view;

  void on_active_cellview_changed ();
  void on_technology_changed ();
  void attach_events ();
};

#if defined(HAVE_QT)
/**
 *  @brief The base class for a object properties page
 */
class LAYBASIC_PUBLIC EditorOptionsPageWidget
  : public QWidget, public EditorOptionsPage
{
Q_OBJECT

public:
  EditorOptionsPageWidget (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher);
  EditorOptionsPageWidget ();
  virtual ~EditorOptionsPageWidget ();

  virtual void set_focus ();
  virtual void set_visible (bool visible);
  virtual EditorOptionsPageWidget *widget () { return this; }

protected slots:
  void edited ();

protected:
  virtual bool focusNextPrevChild (bool next);
  virtual void keyPressEvent (QKeyEvent *event);
};
#endif  //  defined(HAVE_QT)

}

#endif

