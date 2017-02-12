
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#include "layAbstractMenu.h"
#include "layAbstractMenuProvider.h"
#include "tlExceptions.h"
#include "layPlugin.h"
#include "tlAssert.h"
#include "gtf.h"
#include "gsi.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QShortcutEvent>
#include <QToolBar>
#include <QToolButton>
#include <QApplication>
#include <QMessageBox>

#include <ctype.h>

namespace lay
{

// ---------------------------------------------------------------
//  Helper function to parse a title with potential shortcut and 
//  icon specification

static void
parse_menu_title (const std::string &s, std::string &title, std::string &shortcut, std::string &icon_res, std::string &tool_tip)
{
  const char *p = s.c_str ();
  while (*p) {
    if (*p == '\\' && p[1]) {
      ++p;
      title += *p++;
    } else if (*p == '(' || *p == '<' || *p == '{') {
      break;
    } else {
      title += *p++;
    }
  }

  do {

    if (*p == '(') {
      ++p;
      while (*p && *p != ')') {
        shortcut += *p++;
      }
      if (*p == ')') {
        ++p;
      }
    } else if (*p == '{') {
      ++p;
      while (*p && *p != '}') {
        tool_tip += *p++;
      }
      if (*p == '}') {
        ++p;
      }
    } else if (*p == '<') {
      ++p;
      while (*p && *p != '>') {
        icon_res += *p++;
      }
      if (*p == '>') {
        ++p;
      }
    }

    while (*p && isspace (*p)) {
      ++p;
    }

  } while (*p);
}

// ---------------------------------------------------------------
//  AbstractMenuItem implementation

AbstractMenuItem::AbstractMenuItem () 
  : mp_menu (0), m_has_submenu (false)
{
  //  ... nothing yet ..
}

AbstractMenuItem::AbstractMenuItem (const AbstractMenuItem &) 
  : mp_menu (0), m_has_submenu (false)
{ 
  //  ... nothing yet ..
}

void 
AbstractMenuItem::setup_item (const std::string &pn, const std::string &s, const Action &a)
{
  m_basename.clear ();

  tl::Extractor ex (s.c_str ());

  m_name = pn;
  if (! m_name.empty ()) {
    m_name += ".";
  }

  if (! ex.at_end ()) {

    ex.read (m_basename, ":");
    m_name += m_basename;

    while (ex.test (":")) {
      std::string g;
      ex.read (g, ":");
      m_groups.insert (g);
    }

  }

  set_action (a, false);
}

void 
AbstractMenuItem::set_action (const Action &a, bool copy_properties)
{
  Action acopy = a;

  if (copy_properties && m_action.qaction () && acopy.qaction ()) {
    acopy.qaction ()->setIcon (m_action.qaction ()->icon ());
    acopy.qaction ()->setToolTip (m_action.qaction ()->toolTip ());
    acopy.qaction ()->setShortcut (m_action.qaction ()->shortcut ());
    acopy.qaction ()->setIconText (m_action.qaction ()->iconText ());
  }

  bool enabled = m_action.is_enabled ();
  bool visible = m_action.is_visible ();
  m_action = acopy;
  m_action.set_enabled (enabled);
  m_action.set_visible (visible);

  m_action.set_object_name (m_basename);
}

void 
AbstractMenuItem::set_action_title (const std::string &s)
{
  m_action.set_title (s);
}

void  
AbstractMenuItem::set_has_submenu ()
{
  m_has_submenu = true;
}

void  
AbstractMenuItem::set_menu (QMenu *menu)
{
  mp_menu = menu;
  if (mp_menu) {
    mp_menu->setObjectName (tl::to_qstring (m_basename));
  }
}

// ---------------------------------------------------------------
//  Action implementation

static std::set<ActionHandle *> *sp_actionHandles = 0;

/**
 *  @brief A specialization that provides a way to catch ambiguous key shortcuts
 */
class ActionObject
  : public QAction 
{
public:
  ActionObject (QObject *parent) : QAction (parent) { }

  bool event(QEvent *e)
  {
    if (e->type() == QEvent::Shortcut) {

      QShortcutEvent *se = static_cast<QShortcutEvent *>(e);
      if (se->isAmbiguous() && sp_actionHandles) {

        QString msg = QObject::tr ("Keyboard shortcut is ambiguous: ");
        msg += QString (se->key ().toString ());
        msg += tl::to_qstring ("\n\n");
        msg += QObject::tr ("Targets of the that shortcut are:");
        msg += tl::to_qstring ("\n");

        for (std::set<ActionHandle *>::const_iterator a = sp_actionHandles->begin (); a != sp_actionHandles->end (); ++a) {
          if (! (*a)->ptr ()->shortcut ().isEmpty () && (*a)->ptr ()->shortcut ().matches (se->key ()) != QKeySequence::NoMatch) {
            msg += QChar (0x2022) /*bullet*/;
            msg += tl::to_qstring (" ");
            msg += (*a)->ptr ()->text ();
            msg += tl::to_qstring ("\n");
          }
        }

        QMessageBox::warning (0, QObject::tr ("Warning"), msg);

        return true;

      }

    }

    return QAction::event(e);
  }
};

ActionHandle::ActionHandle (QWidget *parent)
  : mp_action (new ActionObject (parent)), 
    m_ref_count (0),
    m_owned (true)
{ 
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<ActionHandle *> ();
  }
  sp_actionHandles->insert (this);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_action, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
}

ActionHandle::ActionHandle (QAction *action, bool owned)
  : mp_action (action), 
    m_ref_count (0), 
    m_owned (owned)
{ 
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<ActionHandle *> ();
  }
  sp_actionHandles->insert (this);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_action, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
}

ActionHandle::~ActionHandle ()
{
  if (sp_actionHandles) {
    sp_actionHandles->erase (this);
    if (sp_actionHandles->empty ()) {
      delete sp_actionHandles;
      sp_actionHandles = 0;
    }
  }

  if (mp_action) {
    if (m_owned) {
      delete mp_action;
      m_owned = false;
    }
    mp_action = 0;
  }
}

void 
ActionHandle::add_ref () 
{
  ++m_ref_count;
}

void 
ActionHandle::remove_ref ()
{
  if (--m_ref_count == 0) {
    delete this;
  }
}

QAction *
ActionHandle::ptr () const
{
  return mp_action;
}

void 
ActionHandle::destroyed (QObject * /*obj*/)
{
  mp_action = 0;
  m_owned = false;
}


// ---------------------------------------------------------------
//  Action implementation

Action::Action ()
  : mp_handle (0)
{
  if (lay::AbstractMenuProvider::instance ()) {
    mp_handle = new ActionHandle (lay::AbstractMenuProvider::instance ()->menu_parent_widget ());
    gtf::action_connect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
    mp_handle->add_ref ();
  }
}

Action::Action (const std::string &title)
{
  mp_handle = AbstractMenu::create_action (title);
  gtf::action_connect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
  mp_handle->add_ref ();
}

Action::Action (ActionHandle *handle)
{
  mp_handle = handle;
  gtf::action_connect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
  mp_handle->add_ref ();
}

Action::Action (const Action &action)
  : QObject ()
{
  mp_handle = action.mp_handle;
  if (mp_handle) {
    gtf::action_connect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
    mp_handle->add_ref ();
  }
}

Action &
Action::operator= (const Action &action)
{
  if (this != &action) {
    if (mp_handle) {
      if (mp_handle->ptr ()) {
        gtf::action_disconnect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
      }
      mp_handle->remove_ref ();
      mp_handle = 0;
    }
    mp_handle = action.mp_handle;
    if (mp_handle) {
      gtf::action_connect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
      mp_handle->add_ref ();
    }
  }
  return *this;
}

Action::~Action ()
{
  if (mp_handle) {
    if (mp_handle->ptr ()) {
      gtf::action_disconnect (mp_handle->ptr (), SIGNAL (triggered ()), this, SLOT (triggered_slot ()));
    }
    mp_handle->remove_ref ();
    mp_handle = 0;
  }
}

void
Action::trigger ()
{
  if (qaction ()) {
    qaction ()->trigger ();
  }
}

void
Action::triggered ()
{
  //  .. no action yet, the reimplementation must provide some ..
}

void
Action::triggered_slot ()
{
  BEGIN_PROTECTED
  triggered ();
  END_PROTECTED
}

void 
Action::set_title (const std::string &t)
{
  if (qaction ()) {
    qaction ()->setText (tl::to_qstring (t));
  }
}

std::string 
Action::get_title () const
{
  if (qaction ()) {
    return tl::to_string (qaction ()->text ());
  } else {
    return std::string ();
  }
}

void 
Action::set_shortcut (const QKeySequence &s)
{
  if (qaction ()) {
    qaction ()->setShortcut (s);
  }
}

void 
Action::set_shortcut (const std::string &s)
{
  if (qaction () && s != get_shortcut ()) {
    qaction ()->setShortcut (QKeySequence (tl::to_qstring (s)));
  }
}

std::string 
Action::get_shortcut () const
{
  if (qaction ()) {
    return tl::to_string (qaction ()->shortcut ().toString ());
  } else {
    return std::string ();
  }
}

QAction *
Action::qaction () const
{
  return mp_handle ? mp_handle->ptr () : 0;
}

void
Action::add_to_exclusive_group (lay::AbstractMenu *menu, const std::string &group_name)
{
  menu->make_exclusive_group (group_name)->addAction (qaction ());
}

bool
Action::is_checkable () const
{
  return qaction () && qaction ()->isCheckable ();
}

bool
Action::is_checked () const
{
  return qaction () && qaction ()->isChecked ();
}

bool
Action::is_enabled () const
{
  return qaction () && qaction ()->isEnabled ();
}

bool
Action::is_visible () const
{
  return qaction () && qaction ()->isVisible ();
}

bool
Action::is_separator () const
{
  return qaction () && qaction ()->isSeparator ();
}

void
Action::set_enabled (bool b)
{
  if (qaction ()) {
    qaction ()->setEnabled (b);
  }
}

void
Action::set_visible (bool v)
{
  if (qaction ()) {
    qaction ()->setVisible (v);
  }
}

void
Action::set_checked (bool c)
{
  if (qaction ()) {
    qaction ()->setChecked (c);
  }
}

void
Action::set_checkable (bool c)
{
  if (qaction ()) {
    qaction ()->setCheckable (c);
  }
}

void
Action::set_separator (bool s)
{
  if (qaction ()) {
    qaction ()->setSeparator (s);
  }
}

void 
Action::set_icon (const std::string &filename)
{
  if (qaction ()) {
    if (filename.empty ()) {
      qaction ()->setIcon (QIcon ());
    } else {
      qaction ()->setIcon (QIcon (tl::to_qstring (filename)));
    }
  }
}

std::string 
Action::get_tool_tip () const
{
  if (qaction ()) {
    return tl::to_string (qaction ()->toolTip ());
  } else {
    return std::string ();
  }
}

void 
Action::set_tool_tip (const std::string &text)
{
  if (qaction ()) {
    if (text.empty ()) {
      qaction ()->setToolTip (QString ());
    } else {
      qaction ()->setToolTip (tl::to_qstring (text));
    }
  }
}

std::string 
Action::get_icon_text () const
{
  if (qaction ()) {
    return tl::to_string (qaction ()->iconText ());
  } else {
    return std::string ();
  }
}

void 
Action::set_icon_text (const std::string &icon_text)
{
  if (qaction ()) {
    if (icon_text.empty ()) {
      qaction ()->setIconText (QString ());
    } else {
      qaction ()->setIconText (tl::to_qstring (icon_text));
    }
  }
}

void 
Action::set_object_name (const std::string &name)
{
  if (qaction ()) {
    qaction ()->setObjectName (tl::to_qstring (name));
  }
}

// ---------------------------------------------------------------
//  ConfigureAction implementation

ConfigureAction::ConfigureAction (lay::PluginRoot *pr)
  : Action (), m_pr (pr), m_type (ConfigureAction::setter_type)
{
  //  .. nothing yet ..
}

ConfigureAction::ConfigureAction (lay::PluginRoot *pr, const std::string &cname, const std::string &cvalue)
  : Action (), m_pr (pr), m_cname (cname), m_cvalue (cvalue), m_type (ConfigureAction::setter_type)
{
  if (cvalue == "?") {
    m_type = boolean_type;
    set_checkable (true);
  }

  reg ();
} 

ConfigureAction::ConfigureAction (lay::PluginRoot *pr, const std::string &title, const std::string &cname, const std::string &cvalue)
  : Action (title), m_pr (pr), m_cname (cname), m_cvalue (cvalue), m_type (ConfigureAction::setter_type)
{
  if (cvalue == "?") {
    //  A "?" notation indicates a boolean toogle entry
    m_type = boolean_type;
    set_checkable (true);
  } else if (! cvalue.empty () && cvalue[0] == '?') {
    //  A "?value" notation indicates a choice
    m_type = choice_type;
    m_cvalue.erase (m_cvalue.begin (), m_cvalue.begin () + 1);
    set_checkable (true);
  }

  reg ();
} 

ConfigureAction::~ConfigureAction ()
{
  unreg ();
}

void 
ConfigureAction::triggered ()
{
  if (m_type == boolean_type) {
    m_cvalue = tl::to_string (is_checked ());
  } 

  m_pr->config_set (m_cname, m_cvalue);
  m_pr->config_end ();
}

void
ConfigureAction::reg ()
{
  if (lay::AbstractMenuProvider::instance ()) {
    lay::AbstractMenuProvider::instance ()->register_config_action (m_cname, this);
  }
}

void
ConfigureAction::unreg ()
{
  if (lay::AbstractMenuProvider::instance ()) {
    lay::AbstractMenuProvider::instance ()->unregister_config_action (m_cname, this);
  }
}

void
ConfigureAction::configure (const std::string &value)
{
  if (m_type == boolean_type) {

    bool f = false;
    tl::from_string (value, f);

    set_checkable (true);
    set_checked (f);

  } else if (m_type == choice_type) {

    set_checkable (true);
    set_checked (m_cvalue == value);

  } 
}

// ---------------------------------------------------------------
//  AbstractMenu implementation

ActionHandle *
AbstractMenu::create_action (const std::string &s)
{
  tl_assert (lay::AbstractMenuProvider::instance () != 0);

  std::string title;
  std::string shortcut;
  std::string res;
  std::string tool_tip;

  parse_menu_title (s, title, shortcut, res, tool_tip);
  
  ActionHandle *ah = new ActionHandle (lay::AbstractMenuProvider::instance ()->menu_parent_widget ());
  ah->ptr ()->setText (tl::to_qstring (title));

  if (! tool_tip.empty ()) {
    ah->ptr ()->setToolTip (tl::to_qstring (tool_tip));
  }

  if (! res.empty ()) {
    ah->ptr ()->setIcon (QIcon (tl::to_qstring (res)));
  }

  if (! shortcut.empty ()) {
    ah->ptr ()->setShortcut (QKeySequence (tl::to_qstring (shortcut)));
  }

  return ah;
}

AbstractMenu::AbstractMenu (AbstractMenuProvider *provider)
  : mp_provider (provider)
{
  tl_assert (mp_provider != 0);
}

AbstractMenu::~AbstractMenu ()
{
  reset_menu_objects (m_root);
}

void 
AbstractMenu::init (const MenuLayoutEntry *layout)
{
  m_root.set_has_submenu ();
  transfer (layout, m_root);
}

void 
AbstractMenu::reset_menu_objects (AbstractMenuItem &item)
{
  for (std::list<AbstractMenuItem>::iterator c = item.children.begin (); c != item.children.end (); ++c) {
    reset_menu_objects (*c);
  }
  item.set_menu (0);
}

QActionGroup *
AbstractMenu::make_exclusive_group (const std::string &name)
{
  std::map<std::string, QActionGroup *>::const_iterator a = m_action_groups.find (name);

  if (a == m_action_groups.end ()) {
    QActionGroup *ag = new QActionGroup (this);
    ag->setExclusive (true);
    a = m_action_groups.insert (std::make_pair (name, ag)).first;
  } 

  return a->second;
}

void 
AbstractMenu::build_detached (const std::string &name, QMenuBar *mbar)
{
  AbstractMenuItem *item = find_item_exact ("@@" + name);
  tl_assert (item != 0);

  mbar->clear ();

  for (std::list<AbstractMenuItem>::iterator c = item->children.begin (); c != item->children.end (); ++c) {

    if (c->has_submenu ()) {

      if (c->menu () == 0) {
        c->set_menu (mbar->addMenu (tl::to_qstring (c->action ().get_title ())));
        c->set_action (Action (new ActionHandle (c->menu ()->menuAction (), false)), true);
      } else {
        mbar->addMenu (c->menu ());
      }

      build (c->menu (), c->children);

    } else {
      mbar->addAction (c->action ().qaction ());
    }

  }
}

void 
AbstractMenu::build (QMenuBar *mbar, QToolBar *tbar)
{
  m_helper_menu_items.clear ();
  mbar->clear ();
  tbar->clear ();

  for (std::list<AbstractMenuItem>::iterator c = m_root.children.begin (); c != m_root.children.end (); ++c) {

    if (c->has_submenu ()) {

      if (c->name () == "@toolbar") {

        build (tbar, c->children);

      } else if (c->name ().find ("@@") == 0) {

        //  nothing: let build_detached build the menu 

      } else if (c->name ().find ("@") == 0) {

        if (c->menu () == 0) {
          QMenu *menu = new QMenu (tl::to_qstring (c->action ().get_title ()), mp_provider->menu_parent_widget ());
          // HINT: it is necessary to add the menu action to a widget below the main window.
          // Otherwise, the keyboard shortcuts do not work for menu items inside such a
          // popup menu. It seems not to have a negative effect to add the menu to the 
          // main widget.
          mp_provider->menu_parent_widget ()->addAction (menu->menuAction ()); 
          c->set_menu (menu);
          c->set_action (Action (new ActionHandle (c->menu ()->menuAction (), false)), true);
        }

        //  prepare a detached menu which can be used as context menues
        build (c->menu (), c->children);

      } else {

        if (c->menu () == 0) {
          c->set_menu (mbar->addMenu (tl::to_qstring (c->action ().get_title ())));
          c->set_action (Action (new ActionHandle (c->menu ()->menuAction (), false)), true);
        } else {
          mbar->addMenu (c->menu ());
        }

        build (c->menu (), c->children);

      }

    } else {
      mbar->addAction (c->action ().qaction ());
    }

  }
}

void 
AbstractMenu::build (QMenu *m, std::list<AbstractMenuItem> &items)
{
  m->clear (); 

  for (std::list<AbstractMenuItem>::iterator c = items.begin (); c != items.end (); ++c) {
    if (c->has_submenu ()) {
      //  HINT: the action acts as a container for the title. Unfortunately, we cannot create a 
      //  menu with a given action. The action is provided by addMenu instead.
      QMenu *menu = m->addMenu (tl::to_qstring (c->action ().get_title ()));
      c->set_action (Action (new ActionHandle (menu->menuAction (), false)), true);
      //  HINT: build must be done before set_menu because set_menu might delete all child QAction's ..
      build (menu, c->children);
      c->set_menu (menu);
    } else {
      m->addAction (c->action ().qaction ());
    }
  }
}

void 
AbstractMenu::build (QToolBar *t, std::list<AbstractMenuItem> &items)
{
  for (std::list<AbstractMenuItem>::iterator c = items.begin (); c != items.end (); ++c) {

    if (! c->children.empty ()) {

      //  To support tool buttons with menu we have to attach a helper menu 
      //  item to the QAction object. 
      //  TODO: this hurts if we use this QAction otherwise. In this case, this
      //  QAction would get a menu too. However, hopefully this usage is constrained
      //  to special toolbar buttons only.
      //  In order to be able to manage the QMenu ourselves, we must not give it a parent.
      QMenu *menu = new QMenu (0);
      m_helper_menu_items.push_back (menu); // will be owned by the stable vector 
      c->action ().qaction ()->setMenu (menu);
      t->addAction (c->action ().qaction ());
      build (menu, c->children);

    } else {
      t->addAction (c->action ().qaction ());
    }

  }

}

QMenu *
AbstractMenu::detached_menu (const std::string &name)
{
  AbstractMenuItem *item = find_item_exact ("@" + name);
  tl_assert (item != 0);
  return item->menu ();
}

QMenu *
AbstractMenu::menu (const std::string &path) 
{
  AbstractMenuItem *item = find_item_exact (path);
  if (item) {
    return item->menu ();
  } else {
    return 0;
  }
}

bool
AbstractMenu::is_valid (const std::string &path) const
{
  const AbstractMenuItem *item = find_item_exact (path);
  return item != 0;
}

bool
AbstractMenu::is_menu (const std::string &path) const
{
  const AbstractMenuItem *item = find_item_exact (path);
  return item != 0 && item->has_submenu ();
}

bool
AbstractMenu::is_separator (const std::string &path) const
{
  const AbstractMenuItem *item = find_item_exact (path);
  return item != 0 && item->action ().is_separator ();
}

Action 
AbstractMenu::action (const std::string &path) const
{
  const AbstractMenuItem *item = find_item_exact (path);
  if (! item) {
    throw tl::Exception (tl::to_string (QObject::tr ("Not a valid menu item path: ")) + path);
  }
  return item->action ();
}

std::vector<std::string> 
AbstractMenu::items (const std::string &path) const
{
  std::vector<std::string> res;

  const AbstractMenuItem *item = find_item_exact (path);
  if (item) {
    res.reserve (item->children.size ());
    for (std::list <AbstractMenuItem>::const_iterator c = item->children.begin (); c != item->children.end (); ++c) {
      res.push_back (c->name ());
    }
  }

  return res;
}

void 
AbstractMenu::insert_item (const std::string &path, const std::string &name, const Action &action)
{
  std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > ins = find_item (path);
  if (ins.first) {

    //  insert the new item
    ins.first->children.insert (ins.second, AbstractMenuItem ());
    --ins.second;

    ins.second->setup_item (ins.first->name (), name, action);

    //  find an item with the same name and remove it
    for (std::list<AbstractMenuItem>::iterator existing = ins.first->children.begin (); existing != ins.first->children.end (); ) {
      std::list<AbstractMenuItem>::iterator existing_next = existing;
      ++existing_next;
      if (existing->name () == ins.second->name () && &*existing != &*ins.second) {
        ins.first->children.erase (existing);
      }
      existing = existing_next;
    }

  }

  emit changed ();
}

void 
AbstractMenu::insert_separator (const std::string &path, const std::string &name)
{
  std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > ins = find_item (path);
  if (ins.first) {
    ins.first->children.insert (ins.second, AbstractMenuItem ());
    --ins.second;
    Action action (new ActionHandle (mp_provider->menu_parent_widget ()));
    action.set_separator (true);
    ins.second->setup_item (ins.first->name (), name, action);
  }
  emit changed ();
}

void 
AbstractMenu::insert_menu (const std::string &path, const std::string &name, const Action &action)
{
  std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > ins = find_item (path);
  if (ins.first) {
    ins.first->children.insert (ins.second, AbstractMenuItem ());
    --ins.second;
    ins.second->setup_item (ins.first->name (), name, action);
    ins.second->set_has_submenu ();
  }
  emit changed ();
}

void 
AbstractMenu::insert_menu (const std::string &path, const std::string &name, const std::string &title)
{
  insert_menu (path, name, Action (create_action (title)));
}

void 
AbstractMenu::delete_item (const std::string &path)
{
  std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > ins = find_item (path);
  if (ins.first) {
    if (ins.second == ins.first->children.end ()) {
      throw tl::Exception (tl::to_string (QObject::tr ("delete_item cannot delete past-the-end item")));
    }
    reset_menu_objects (*ins.second);
    ins.first->children.erase (ins.second);
  }
  emit changed ();
}

static void do_delete_items (std::list<AbstractMenuItem> &list, const Action &action)
{
  for (std::list<AbstractMenuItem>::iterator l = list.begin (); l != list.end (); ) {
    std::list<AbstractMenuItem>::iterator ll = l;
    ++ll;
    if (l->action () == action) {
      list.erase (l);
    } else {
      do_delete_items (l->children, action);
    }
    l = ll;
  }
}

void
AbstractMenu::delete_items (const Action &action)
{
  do_delete_items (m_root.children, action);
  emit changed ();
}

const AbstractMenuItem *
AbstractMenu::find_item_exact (const std::string &path) const
{
  //  Hint: this const_cast saves us having to implement const and non-const versions ..
  return (const_cast<AbstractMenu *> (this))->find_item_exact (path);
}

AbstractMenuItem *
AbstractMenu::find_item_exact (const std::string &path) 
{
  tl::Extractor extr (path.c_str ());
  AbstractMenuItem *item = &m_root;

  while (! extr.at_end ()) {

    if (extr.test ("#")) {

      unsigned int n = 0;
      extr.try_read (n);

      std::list<AbstractMenuItem>::iterator c = item->children.begin ();
      ++n;
      while (--n > 0 && c != item->children.end ()) {
        ++c;
      }
      if (n > 0) {
        return 0;
      } 

      item = &*c;

    } else {

      std::string n;
      extr.read (n, ".");
      std::string name (item->name ());
      if (! name.empty ()) {
        name += ".";
      }
      name += n;

      AbstractMenuItem *p = item;

      item = 0;
      for (std::list<AbstractMenuItem>::iterator c = p->children.begin (); c != p->children.end () && item == 0; ++c) {
        if (c->name () == name) {
          item = &*c;
        }
      }

      if (! item) {
        return 0;
      }

    }

    extr.test (".");

  }

  return item;
}

std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator> 
AbstractMenu::find_item (const std::string &path) 
{
  tl::Extractor extr (path.c_str ());
  AbstractMenuItem *parent = &m_root;
  std::list<AbstractMenuItem>::iterator iter = m_root.children.end ();

  while (true) {

    if (extr.test ("#")) {

      unsigned int n = 0;
      extr.try_read (n);
      iter = parent->children.begin ();
      ++n;
      while (--n > 0 && iter != parent->children.end ()) {
        ++iter;
      }
      if (n > 0) {
        return std::make_pair ((AbstractMenuItem *) 0, m_root.children.end ());
      }

    } else if (extr.test ("begin")) {
      return std::make_pair (parent, parent->children.begin ());
    } else if (extr.test ("end")) {
      return std::make_pair (parent, parent->children.end ());
    } else {

      std::string n;
      extr.read (n, ".+");
      std::string name (parent->name ());
      if (! name.empty ()) {
        name += ".";
      }
      name += n;

      bool after = extr.test ("+");

      AbstractMenuItem *p = parent;
      parent = 0;
      for (std::list<AbstractMenuItem>::iterator c = p->children.begin (); c != p->children.end (); ++c) {

        if (c->name () == name) {

          if (after) {
            ++c;
            if (c == p->children.end ()) {
              return std::make_pair (parent, parent->children.end ());
            }
          }

          parent = p;
          iter = c;
          break;

        }

      }

      if (! parent) {
        return std::make_pair ((AbstractMenuItem *) 0, m_root.children.end ());
      }

    }

    if (extr.at_end ()) {
      return std::make_pair (parent, iter);
    }

    extr.test (".");

    parent = &*iter;

  }

}

void 
AbstractMenu::transfer (const MenuLayoutEntry *layout, AbstractMenuItem &item)
{
  while (layout->name) {

    item.children.push_back (AbstractMenuItem ());
    AbstractMenuItem &new_item = item.children.back ();

    lay::Action a;

    if (layout->slot) {
      // reuse any actions already registered for this slot
      a = mp_provider->action_for_slot (layout->slot);
    } else if (! layout->kv_pair.first.empty ()) {
      a = *mp_provider->create_config_action (layout->kv_pair.first, layout->kv_pair.second);
    } else {
      a = lay::Action (new ActionHandle (mp_provider->menu_parent_widget ()));
    }

    if (! layout->title.empty ()) {

      std::string title;
      std::string shortcut;
      std::string res;
      std::string tool_tip;

      parse_menu_title (layout->title, title, shortcut, res, tool_tip);
      
      a.set_separator (false);
      a.set_title (title);

      if (! shortcut.empty ()) {
        a.set_shortcut (QKeySequence (tl::to_qstring (shortcut)));
      }

      if (! tool_tip.empty ()) {
        a.set_tool_tip (tool_tip);
      }

      if (! res.empty ()) {
        a.set_icon (res);
      }

    } else {
      a.set_separator (true);
    }

    new_item.setup_item (item.name (), layout->name, a);

    if (layout->submenu) {
      new_item.set_has_submenu ();
      transfer (layout->submenu, item.children.back ());
    }

    ++layout;

  }
}

std::vector<std::string> 
AbstractMenu::group (const std::string &name) const
{
  std::vector<std::string> grp;
  collect_group (grp, name, m_root);
  return grp;
}

void 
AbstractMenu::collect_group (std::vector<std::string> &grp, const std::string &name, const AbstractMenuItem &item) const
{
  for (std::list<AbstractMenuItem>::const_iterator c = item.children.begin (); c != item.children.end (); ++c) {
    if (c->groups ().find (name) != c->groups ().end ()) {
      grp.push_back (c->name ());
    }
    collect_group (grp, name, *c);
  }
}

}

