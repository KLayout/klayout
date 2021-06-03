
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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
#include "layDispatcher.h"
#include "layPlugin.h"
#include "tlExceptions.h"
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
#include <QHBoxLayout>

#include <ctype.h>

namespace lay
{

// ---------------------------------------------------------------

#if defined(__APPLE__)

//  On MacOS, the main menu bar and it's decendent children
//  can't be modified using "removeAction", followed by "addAction"
//  to achieve a move operation.If we try to do so, segmentation faults happen
//  in the timer event that presumably tries to merge the menu bar
//  with the application menu.
//  The fallback is to only allow add/delete, not move operations on the
//  menu. In effect, the order of the menu items may not be the one desired
//  if menus are dynamically created. However, this will only happen when
//  new packages or macros are installed.
static const bool s_can_move_menu = false;

#else

static const bool s_can_move_menu = true;

#endif

// ---------------------------------------------------------------
//  Serialization of key bindings and hidden menu state

std::vector<std::pair<std::string, std::string> >
unpack_key_binding (const std::string &packed)
{
  tl::Extractor ex (packed.c_str ());

  std::vector<std::pair<std::string, std::string> > key_bindings;

  while (! ex.at_end ()) {
    ex.test(";");
    key_bindings.push_back (std::make_pair (std::string (), std::string ()));
    ex.read_word_or_quoted (key_bindings.back ().first);
    ex.test(":");
    ex.read_word_or_quoted (key_bindings.back ().second);
  }

  return key_bindings;
}

std::string
pack_key_binding (const std::vector<std::pair<std::string, std::string> > &unpacked)
{
  std::string packed;

  for (std::vector<std::pair<std::string, std::string> >::const_iterator p = unpacked.begin (); p != unpacked.end (); ++p) {
    if (! packed.empty ()) {
      packed += ";";
    }
    packed += tl::to_word_or_quoted_string (p->first);
    packed += ":";
    packed += tl::to_word_or_quoted_string (p->second);
  }

  return packed;
}

std::vector<std::pair<std::string, bool> >
unpack_menu_items_hidden (const std::string &packed)
{
  tl::Extractor ex (packed.c_str ());

  std::vector<std::pair<std::string, bool> > hidden;

  while (! ex.at_end ()) {
    ex.test(";");
    hidden.push_back (std::make_pair (std::string (), false));
    ex.read_word_or_quoted (hidden.back ().first);
    ex.test(":");
    ex.read (hidden.back ().second);
  }

  return hidden;
}

std::string
pack_menu_items_hidden (const std::vector<std::pair<std::string, bool> > &unpacked)
{
  std::string packed;

  for (std::vector<std::pair<std::string, bool> >::const_iterator p = unpacked.begin (); p != unpacked.end (); ++p) {
    if (! packed.empty ()) {
      packed += ";";
    }
    packed += tl::to_word_or_quoted_string (p->first);
    packed += ":";
    packed += tl::to_string (p->second);
  }

  return packed;
}

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

AbstractMenuItem::AbstractMenuItem (Dispatcher *dispatcher)
  : mp_action (new Action ()), mp_dispatcher (dispatcher), m_has_submenu (false), m_remove_on_empty (false)
{
  //  ... nothing yet ..
}

AbstractMenuItem::AbstractMenuItem (const AbstractMenuItem &item)
  : mp_action (new Action ()), mp_dispatcher (item.dispatcher ()), m_has_submenu (false), m_remove_on_empty (false)
{
  //  ... nothing yet ..
}

void
AbstractMenuItem::setup_item (const std::string &pn, const std::string &s, Action *a)
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
AbstractMenuItem::set_action (Action *a, bool copy_properties)
{
  tl_assert (a != 0);

  a->keep ();

  if (copy_properties && mp_action->qaction () && a->qaction ()) {
    a->qaction ()->setIcon (mp_action->qaction ()->icon ());
    a->qaction ()->setToolTip (mp_action->qaction ()->toolTip ());
    a->qaction ()->setShortcut (mp_action->qaction ()->shortcut ());
    a->qaction ()->setIconText (mp_action->qaction ()->iconText ());
  }

  bool enabled = mp_action.get () ? mp_action->is_enabled () : true;
  bool visible = mp_action.get () ? mp_action->is_visible () : true;
  mp_action->set_dispatcher (0);
  mp_action.reset (a);
  mp_action->set_enabled (enabled);
  mp_action->set_visible (visible);
  mp_action->set_dispatcher (mp_dispatcher);
  mp_action->set_object_name (m_basename);

  if (mp_action->menu ()) {
    mp_action->menu ()->setObjectName (tl::to_qstring (m_basename));
  }
}

void
AbstractMenuItem::set_action_title (const std::string &s)
{
  mp_action->set_title (s);
}

void
AbstractMenuItem::set_has_submenu ()
{
  m_has_submenu = true;
}

void
AbstractMenuItem::set_remove_on_empty ()
{
  m_remove_on_empty = true;
}

// ---------------------------------------------------------------
//  Action implementation

static std::set<Action *> *sp_actionHandles = 0;

static void
configure_action_from_title (Action *ah, const std::string &s)
{
  std::string title;
  std::string shortcut;
  std::string res;
  std::string tool_tip;

  parse_menu_title (s, title, shortcut, res, tool_tip);

  ah->qaction ()->setText (tl::to_qstring (title));

  if (! tool_tip.empty ()) {
    ah->qaction ()->setToolTip (tl::to_qstring (tool_tip));
  }

  if (! res.empty ()) {
    ah->qaction ()->setIcon (QIcon (tl::to_qstring (res)));
  }

  if (! shortcut.empty ()) {
    ah->set_default_shortcut (shortcut);
  }
}

/**
 *  @brief A specialization that provides a way to catch ambiguous key shortcuts
 */
class ActionObject
  : public QAction
{
public:
  ActionObject (QObject *parent)
    : QAction (parent)
  {
    static size_t s_id = 0;
    m_id = ++s_id;
  }

  size_t id () const
  {
    return m_id;
  }

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

        for (std::set<Action *>::const_iterator a = sp_actionHandles->begin (); a != sp_actionHandles->end (); ++a) {
          if (! (*a)->qaction ()->shortcut ().isEmpty () && (*a)->qaction ()->shortcut ().matches (se->key ()) != QKeySequence::NoMatch) {
            msg += QChar (0x2022) /*bullet*/;
            msg += tl::to_qstring (" ");
            msg += (*a)->qaction ()->text ();
            msg += tl::to_qstring ("\n");
          }
        }

        QMessageBox::warning (0, QObject::tr ("Warning"), msg);

        return true;

      }

    }

    return QAction::event(e);
  }

private:
  size_t m_id;
};

static size_t
id_from_action (QAction *action)
{
  ActionObject *ao = dynamic_cast<ActionObject *> (action);
  return ao ? ao->id () : 0;
}

Action::Action ()
  : mp_menu (0),
    mp_action (new ActionObject (0)),
    mp_dispatcher (0),
    m_owned (true),
    m_visible (true),
    m_hidden (false),
    m_no_key_sequence (false)
{
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<Action *> ();
  }
  sp_actionHandles->insert (this);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_action, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
  connect (mp_action, SIGNAL (triggered ()), this, SLOT (qaction_triggered ()));
}

Action::Action (QAction *action, bool owned)
  : mp_menu (0),
    mp_action (action),
    mp_dispatcher (0),
    m_owned (owned),
    m_visible (true),
    m_hidden (false),
    m_no_key_sequence (false)
{
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<Action *> ();
  }
  sp_actionHandles->insert (this);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_action, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
  connect (mp_action, SIGNAL (triggered ()), this, SLOT (qaction_triggered ()));
}

Action::Action (QMenu *menu, bool owned)
  : mp_menu (menu),
    mp_action (menu->menuAction ()),
    mp_dispatcher (0),
    m_owned (owned),
    m_visible (true),
    m_hidden (false),
    m_no_key_sequence (false)
{
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<Action *> ();
  }
  sp_actionHandles->insert (this);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_menu, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
  connect (mp_action, SIGNAL (triggered ()), this, SLOT (qaction_triggered ()));
}

Action::Action (const std::string &title)
  : mp_menu (0),
    mp_action (new QAction (0)),
    mp_dispatcher (0),
    m_owned (true),
    m_visible (true),
    m_hidden (false),
    m_no_key_sequence (false)
{
  if (! sp_actionHandles) {
    sp_actionHandles = new std::set<Action *> ();
  }
  sp_actionHandles->insert (this);

  configure_action_from_title (this, title);

  //  catch the destroyed signal to tell if the QAction object is deleted.
  connect (mp_action, SIGNAL (destroyed (QObject *)), this, SLOT (destroyed (QObject *)));
  connect (mp_action, SIGNAL (triggered ()), this, SLOT (qaction_triggered ()));
}

Action::~Action ()
{
  if (sp_actionHandles) {
    sp_actionHandles->erase (this);
    if (sp_actionHandles->empty ()) {
      delete sp_actionHandles;
      sp_actionHandles = 0;
    }
  }

  if (mp_menu) {
    if (m_owned) {
      delete mp_menu;
      m_owned = false;
    }
    mp_menu = 0;
    mp_action = 0;
  } else if (mp_action) {
    if (m_owned) {
      delete mp_action;
      m_owned = false;
    }
    mp_action = 0;
  }
}

void
Action::set_dispatcher (Dispatcher *dispatcher)
{
  if (mp_dispatcher != dispatcher) {
    if (mp_action && m_owned) {
      mp_action->setParent (dispatcher ? dispatcher->menu_parent_widget () : 0);
    }
    mp_dispatcher = dispatcher;
  }
}

void
Action::qaction_triggered ()
{
  BEGIN_PROTECTED
  triggered ();
  END_PROTECTED
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

QAction *
Action::qaction () const
{
  return mp_action;
}

QMenu *
Action::menu () const
{
  return mp_menu;
}

void
Action::destroyed (QObject *obj)
{
  if (obj == mp_action) {
    mp_action = 0;
  }
  if (obj == mp_menu) {
    mp_menu = 0;
    mp_action = 0;
  }
  m_owned = false;
}

void
Action::set_visible (bool v)
{
  if (m_visible != v) {
    m_visible = v;
    if (mp_action) {
      mp_action->setVisible (is_effective_visible ());
      mp_action->setShortcut (get_key_sequence ());
    }
  }
}

void
Action::set_hidden (bool h)
{
  if (m_hidden != h) {
    m_hidden = h;
    if (mp_action) {
      mp_action->setVisible (is_effective_visible ());
      mp_action->setShortcut (get_key_sequence ());
    }
  }
}

bool
Action::is_visible () const
{
  return m_visible;
}

bool
Action::is_hidden () const
{
  return m_hidden;
}

bool
Action::is_effective_visible () const
{
  return m_visible && !m_hidden;
}

void
Action::set_default_shortcut (const std::string &sc)
{
  if (m_default_shortcut != sc) {
    m_default_shortcut = sc;
    m_default_key_sequence = QKeySequence (tl::to_qstring (sc));
    if (mp_action) {
      mp_action->setShortcut (get_key_sequence ());
    }
  }
}

void
Action::set_shortcut (const std::string &sc)
{
  if (m_shortcut != sc) {
    m_shortcut = sc;
    m_no_key_sequence = (sc == Action::no_shortcut ());
    m_key_sequence = m_no_key_sequence ? QKeySequence () : QKeySequence (tl::to_qstring (m_shortcut));
    if (mp_action) {
      mp_action->setShortcut (get_key_sequence ());
    }
  }
}

std::string
Action::get_default_shortcut () const
{
  return tl::to_string (m_default_key_sequence.toString ());
}

std::string
Action::get_shortcut () const
{
  return m_no_key_sequence ? Action::no_shortcut () : tl::to_string (m_key_sequence.toString ());
}

QKeySequence
Action::get_key_sequence () const
{
  if (m_hidden) {
    //  A hidden menu item does not have a key sequence too.
    return QKeySequence ();
  } else if (m_no_key_sequence) {
    return QKeySequence ();
  } else if (m_key_sequence.isEmpty ()) {
    return m_default_key_sequence;
  } else {
    return m_key_sequence;
  }
}

QKeySequence
Action::get_key_sequence_for (const std::string &sc) const
{
  if (m_hidden) {
    //  A hidden menu item does not have a key sequence too.
    return QKeySequence ();
  } else if (sc.empty ()) {
    return m_default_key_sequence;
  } else if (sc == Action::no_shortcut ()) {
    return QKeySequence ();
  } else {
    return QKeySequence::fromString (tl::to_qstring (sc));
  }
}

const std::string &
Action::no_shortcut ()
{
  static const std::string no_shortcut ("none");
  return no_shortcut;
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

std::string
Action::get_effective_shortcut () const
{
  return tl::to_string (get_key_sequence ().toString ());
}

std::string
Action::get_effective_shortcut_for (const std::string &sc) const
{
  return tl::to_string (get_key_sequence_for (sc).toString ());
}

void
Action::add_to_exclusive_group (lay::AbstractMenu *menu, const std::string &group_name)
{
  if (qaction ()) {
    menu->make_exclusive_group (group_name)->addAction (qaction ());
  }
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

ConfigureAction::ConfigureAction ()
  : Action (), m_type (ConfigureAction::setter_type)
{
  //  .. nothing yet ..
}

ConfigureAction::ConfigureAction (const std::string &cname, const std::string &cvalue)
  : Action (), m_cname (cname), m_cvalue (cvalue), m_type (ConfigureAction::setter_type)
{
  if (cvalue == "?") {
    m_type = boolean_type;
    set_checkable (true);
  }
}

ConfigureAction::ConfigureAction (const std::string &title, const std::string &cname, const std::string &cvalue)
  : Action (title), m_cname (cname), m_cvalue (cvalue), m_type (ConfigureAction::setter_type)
{
  if (cvalue == "?") {
    //  A "?" notation indicates a boolean toggle entry
    m_type = boolean_type;
    set_checkable (true);
  } else if (! cvalue.empty () && cvalue[0] == '?') {
    //  A "?value" notation indicates a choice
    m_type = choice_type;
    m_cvalue.erase (m_cvalue.begin (), m_cvalue.begin () + 1);
    set_checkable (true);
  }
}

ConfigureAction::~ConfigureAction ()
{
  //  .. nothing yet ..
}

void
ConfigureAction::triggered ()
{
  if (dispatcher ()) {
    if (m_type == boolean_type) {
      m_cvalue = tl::to_string (is_checked ());
    }
    dispatcher ()->config_set (m_cname, m_cvalue);
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

AbstractMenu::AbstractMenu (Dispatcher *dispatcher)
  : mp_dispatcher (dispatcher), m_root (dispatcher), m_config_actions_valid (false)
{
  //  .. nothing yet ..
}

AbstractMenu::~AbstractMenu ()
{
  //  .. nothing yet ..
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
AbstractMenu::build_detached (const std::string &name, QFrame *mbar)
{
  //  Clean up the menu bar before rebuilding
  if (mbar->layout ()) {
    delete mbar->layout ();
  }
  QObjectList children = mbar->children ();
  for (QObjectList::const_iterator c = children.begin (); c != children.end (); ++c) {
    if (dynamic_cast<QToolButton *> (*c)) {
      delete *c;
    }
  }

  QHBoxLayout *menu_layout = new QHBoxLayout (mbar);
  menu_layout->setMargin (0);
  mbar->setLayout (menu_layout);

  AbstractMenuItem *item = find_item_exact ("@@" + name);
  tl_assert (item != 0);

  for (std::list<AbstractMenuItem>::iterator c = item->children.begin (); c != item->children.end (); ++c) {

    if (c->has_submenu ()) {

      QToolButton *menu_button = new QToolButton (mbar);
      menu_layout->addWidget (menu_button);
      menu_button->setAutoRaise (true);
      menu_button->setPopupMode (QToolButton::MenuButtonPopup);
      menu_button->setText (tl::to_qstring (c->action ()->get_title ()));

      if (c->menu () == 0) {
        QMenu *menu = new QMenu (mp_dispatcher->menu_parent_widget ());
        menu_button->setMenu (menu);
        c->set_action (new Action (menu), true);
      } else {
        menu_button->setMenu (c->menu ());
      }

      build (c->menu (), c->children);

    } else {

      QAction *action = c->action ()->qaction ();

      QToolButton *menu_button = new QToolButton (mbar);
      menu_layout->addWidget (menu_button);
      menu_button->setAutoRaise (true);
      menu_button->setDefaultAction (action);

    }

  }

  menu_layout->addStretch (1);
}

static QAction *insert_action_after (QWidget *widget, QAction *after, QAction *action)
{
  QList<QAction *> actions = widget->actions ();

  QAction *before = 0;
  if (after == 0) {
    if (! actions.isEmpty ()) {
      before = actions.front ();
    }
  } else {
    int index = actions.indexOf (after);
    if (index >= 0 && index + 1 < actions.size ()) {
      before = actions [index + 1];
    }
  }
  widget->insertAction (before, action);

  return action;
}

void
AbstractMenu::build (QMenuBar *mbar, QToolBar *tbar)
{
  m_helper_menu_items.clear ();
  if (tbar) {
    tbar->clear ();
  }

  std::set<std::pair<size_t, QAction *> > present_actions;
  if (mbar) {
    QList<QAction *> a = mbar->actions ();
    for (QList<QAction *>::const_iterator i = a.begin (); i != a.end (); ++i) {
      present_actions.insert (std::make_pair (id_from_action (*i), *i));
    }
  }

  QAction *prev_action = 0;

  for (std::list<AbstractMenuItem>::iterator c = m_root.children.begin (); c != m_root.children.end (); ++c) {

    if (c->has_submenu ()) {

      if (c->name () == "@toolbar") {

        if (tbar) {
          build (tbar, c->children);
        }

      } else if (c->name ().find ("@@") == 0) {

        //  nothing: let build_detached build the menu

      } else if (c->name ().find ("@") == 0) {

        if (c->menu () == 0) {
          QMenu *menu = new QMenu (tl::to_qstring (c->action ()->get_title ()), mp_dispatcher->menu_parent_widget ());
          // HINT: it is necessary to add the menu action to a widget below the main window.
          // Otherwise, the keyboard shortcuts do not work for menu items inside such a
          // popup menu. It seems not to have a negative effect to add the menu to the
          // main widget.
          if (mp_dispatcher->menu_parent_widget ()) {
            mp_dispatcher->menu_parent_widget ()->addAction (menu->menuAction ());
          }
          c->set_action (new Action (menu), true);
        }

        //  prepare a detached menu which can be used as context menus
        build (c->menu (), c->children);

      } else if (mbar) {

        if (c->menu () == 0) {
          QMenu *menu = new QMenu (mp_dispatcher->menu_parent_widget ());
          menu->setTitle (tl::to_qstring (c->action ()->get_title ()));
          c->set_action (new Action (menu), true);
          prev_action = insert_action_after (mbar, prev_action, menu->menuAction ());
        } else {
          //  Move the action to the end if present in the menu already
          std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.find (std::make_pair (id_from_action (c->menu ()->menuAction ()), c->menu ()->menuAction ()));
          if (a != present_actions.end ()) {
            if (s_can_move_menu) {
              mbar->removeAction (a->second);
              insert_action_after (mbar, prev_action, a->second);
            }
            prev_action = a->second;
            present_actions.erase (*a);
          } else {
            prev_action = insert_action_after (mbar, prev_action, c->menu ()->menuAction ());
          }
        }

        build (c->menu (), c->children);

      }

    } else if (mbar) {

      //  Move the action to the end if present in the menu already
      std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.find (std::make_pair (id_from_action (c->action ()->qaction ()), c->action ()->qaction ()));
      if (a != present_actions.end ()) {
        if (s_can_move_menu) {
          mbar->removeAction (a->second);
          insert_action_after (mbar, prev_action, a->second);
        }
        prev_action = a->second;
        present_actions.erase (*a);
      } else {
        prev_action = insert_action_after (mbar, prev_action, c->action ()->qaction ());
      }

    }

  }

  //  Remove all actions that have vanished
  if (mbar) {
    for (std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.begin (); a != present_actions.end (); ++a) {
      mbar->removeAction (a->second);
    }
  }
}

void
AbstractMenu::build (QMenu *m, std::list<AbstractMenuItem> &items)
{
  std::set<std::pair<size_t, QAction *> > present_actions;
  QList<QAction *> a = m->actions ();
  for (QList<QAction *>::const_iterator i = a.begin (); i != a.end (); ++i) {
    present_actions.insert (std::make_pair (id_from_action (*i), *i));
  }

  QAction *prev_action = 0;

  for (std::list<AbstractMenuItem>::iterator c = items.begin (); c != items.end (); ++c) {

    if (c->has_submenu ()) {

      if (! c->menu ()) {
        //  HINT: the action acts as a container for the title. Unfortunately, we cannot create a
        //  menu with a given action. The action is provided by addMenu instead.
        QMenu *menu = new QMenu (mp_dispatcher->menu_parent_widget ());
        menu->setTitle (tl::to_qstring (c->action ()->get_title ()));
        c->set_action (new Action (menu), true);
        prev_action = insert_action_after (m, prev_action, menu->menuAction ());
      } else {
        //  Move the action to the end if present in the menu already
        std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.find (std::make_pair (id_from_action (c->menu ()->menuAction ()), c->menu ()->menuAction ()));
        if (a != present_actions.end ()) {
          if (s_can_move_menu) {
            m->removeAction (a->second);
            insert_action_after (m, prev_action, a->second);
          }
          prev_action = a->second;
          present_actions.erase (*a);
        } else {
          prev_action = insert_action_after (m, prev_action, c->menu ()->menuAction ());
        }
      }

      build (c->menu (), c->children);

    } else {

      //  Move the action to the end if present in the menu already
      std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.find (std::make_pair (id_from_action (c->action ()->qaction ()), c->action ()->qaction ()));
      if (a != present_actions.end ()) {
        if (s_can_move_menu) {
          m->removeAction (a->second);
          insert_action_after (m, prev_action, a->second);
        }
        prev_action = a->second;
        present_actions.erase (*a);
      } else {
        prev_action = insert_action_after (m, prev_action, c->action ()->qaction ());
      }

    }
  }

  //  Remove all actions that have vanished
  for (std::set<std::pair<size_t, QAction *> >::iterator a = present_actions.begin (); a != present_actions.end (); ++a) {
    m->removeAction (a->second);
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
      c->action ()->qaction ()->setMenu (menu);
      t->addAction (c->action ()->qaction ());
      build (menu, c->children);

    } else {
      t->addAction (c->action ()->qaction ());
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
  return item != 0 && item->action ()->is_separator ();
}

const Action *AbstractMenu::action(const std::string &path) const
{
  return (const_cast<AbstractMenu *> (this))->action (path);
}

Action *AbstractMenu::action(const std::string &path)
{
  AbstractMenuItem *item = find_item_exact (path);
  return item ? item->action () : 0;
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
AbstractMenu::insert_item (const std::string &p, const std::string &name, Action *action)
{
  tl::Extractor extr (p.c_str ());
  while (! extr.at_end ()) {

    typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > > path_type;
    path_type path = find_item (extr);
    if (! path.empty ()) {

      AbstractMenuItem *parent = path.back ().first;
      std::list<AbstractMenuItem>::iterator iter = path.back ().second;

      //  insert the new item
      parent->children.insert (iter, AbstractMenuItem (mp_dispatcher));
      --iter;

      iter->setup_item (parent->name (), name, action);

      //  find any items with the same name and remove them
      for (std::list<AbstractMenuItem>::iterator existing = parent->children.begin (); existing != parent->children.end (); ) {
        std::list<AbstractMenuItem>::iterator existing_next = existing;
        ++existing_next;
        if (existing->name () == iter->name () && existing != iter) {
          parent->children.erase (existing);
        }
        existing = existing_next;
      }

    }

  }

  emit_changed ();
}

void
AbstractMenu::insert_separator (const std::string &p, const std::string &name)
{
  typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > > path_type;
  tl::Extractor extr (p.c_str ());
  path_type path = find_item (extr);
  if (! path.empty ()) {

    AbstractMenuItem *parent = path.back ().first;
    std::list<AbstractMenuItem>::iterator iter = path.back ().second;

    parent->children.insert (iter, AbstractMenuItem (mp_dispatcher));
    --iter;
    Action *action = new Action ();
    action->set_separator (true);
    iter->setup_item (parent->name (), name, action);

  }

  emit_changed ();
}

void
AbstractMenu::insert_menu (const std::string &p, const std::string &name, Action *action)
{
  typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > > path_type;
  tl::Extractor extr (p.c_str ());
  path_type path = find_item (extr);
  if (! path.empty ()) {

    AbstractMenuItem *parent = path.back ().first;
    std::list<AbstractMenuItem>::iterator iter = path.back ().second;

    parent->children.insert (iter, AbstractMenuItem (mp_dispatcher));
    --iter;
    iter->setup_item (parent->name (), name, action);
    iter->set_has_submenu ();

    //  find any items with the same name and remove them
    for (std::list<AbstractMenuItem>::iterator existing = parent->children.begin (); existing != parent->children.end (); ) {
      std::list<AbstractMenuItem>::iterator existing_next = existing;
      ++existing_next;
      if (existing->name () == iter->name () && existing != iter) {
        parent->children.erase (existing);
      }
      existing = existing_next;
    }

  }

  emit_changed ();
}

void
AbstractMenu::insert_menu (const std::string &path, const std::string &name, const std::string &title)
{
  insert_menu (path, name, new Action (title));
}

void
AbstractMenu::clear_menu (const std::string &p)
{
  typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > > path_type;
  tl::Extractor extr (p.c_str ());
  path_type path = find_item (extr);
  if (! path.empty () && ! path.back ().second->children.empty ()) {
    path.back ().second->children.clear ();
    emit_changed ();
  }
}

void
AbstractMenu::delete_item (const std::string &p)
{
  typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator > > path_type;
  tl::Extractor extr (p.c_str ());
  path_type path = find_item (extr);
  if (! path.empty ()) {

    for (path_type::const_reverse_iterator p = path.rbegin (); p != path.rend (); ++p) {

      if (p->second == p->first->children.end ()) {
        break;
      } else if (p != path.rbegin () && (! p->second->remove_on_empty () || ! p->second->children.empty ())) {
        //  stop on non-empty parent menus
        break;
      }

      p->first->children.erase (p->second);

    }

  }

  emit_changed ();
}

static void do_delete_items (AbstractMenuItem &parent, Action *action)
{
  for (std::list<AbstractMenuItem>::iterator l = parent.children.begin (); l != parent.children.end (); ) {
    std::list<AbstractMenuItem>::iterator ll = l;
    ++ll;
    if (l->action () == action) {
      parent.children.erase (l);
    } else {
      do_delete_items (*l, action);
      if (l->remove_on_empty () && l->children.empty ()) {
        parent.children.erase (l);
      }
    }
    l = ll;
  }
}

void
AbstractMenu::delete_items (Action *action)
{
  if (action) {
    do_delete_items (m_root, action);
    emit_changed ();
  }
}

const AbstractMenuItem *
AbstractMenu::find_item_exact (const std::string &path) const
{
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

std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator> >
AbstractMenu::find_item (tl::Extractor &extr)
{
  typedef std::vector<std::pair<AbstractMenuItem *, std::list<AbstractMenuItem>::iterator> > path_type;
  path_type path;

  AbstractMenuItem *parent = &m_root;
  std::list<AbstractMenuItem>::iterator iter = m_root.children.end ();

  while (parent && ! extr.at_end ()) {

    if (extr.test (";")) {

      break;

    } else if (extr.test ("#")) {

      unsigned int n = 0;
      extr.try_read (n);
      iter = parent->children.begin ();
      ++n;
      while (--n > 0 && iter != parent->children.end ()) {
        ++iter;
      }
      if (n > 0) {
        return path_type ();
      }

    } else {

      std::string n;
      extr.read (n, ".;+>(");

      if (n.empty ()) {

        //  skip (avoids infinite loops on wrong paths)
        while (! extr.at_end () && *extr != ';') {
          ++extr;
        }

      } else if (n == "begin") {

        iter = parent->children.begin ();

      } else if (n == "end") {

        iter = parent->children.end ();

      } else {

        std::string nn;
        if (extr.test (">")) {
          extr.read (nn, ".;+>(");
        }

        std::string name (parent->name ());
        if (! name.empty ()) {
          name += ".";
        }

        std::string nname;
        nname = name + nn;
        name += n;

        bool after = extr.test ("+");

        std::string ndesc;
        if (! nn.empty () && extr.test ("(")) {
          extr.read_word_or_quoted (ndesc, " _.;$");
          extr.test (")");
        }

        AbstractMenuItem *p = parent;
        parent = 0;

        //  Look for the next path item
        for (std::list<AbstractMenuItem>::iterator c = p->children.begin (); c != p->children.end (); ++c) {
          if (c->name () == name) {
            if (after && nn.empty ()) {
              ++c;
            }
            parent = p;
            iter = c;
            break;
          }
        }

        //  If that's not found, check whether we are supposed to create one:
        //  identify the insert position and create a new entry there.
        if (! parent && ! nn.empty ()) {

          if (nn == "begin") {
            parent = p;
            iter = parent->children.begin ();
          } else if (nn == "end") {
            parent = p;
            iter = parent->children.end ();
          } else {
            for (std::list<AbstractMenuItem>::iterator c = p->children.begin (); c != p->children.end (); ++c) {
              if (c->name () == nname) {
                if (after) {
                  ++c;
                }
                parent = p;
                iter = c;
                break;
              }
            }
          }

          if (parent) {
            parent->children.insert (iter, AbstractMenuItem (mp_dispatcher));
            --iter;
            iter->setup_item (parent->name (), n, new Action ());
            iter->set_has_submenu ();
            iter->set_remove_on_empty ();
            iter->set_action_title (ndesc.empty () ? n : ndesc);
          }

        }

        if (! parent) {
          return path_type ();
        }

      }

    }

    path.push_back (std::make_pair (parent, iter));

    extr.test (".");

    if (iter == parent->children.end ()) {
      parent = 0;
    } else {
      parent = iter.operator-> ();
    }

  }

  return path;
}

std::vector<std::string>
AbstractMenu::group (const std::string &name) const
{
  std::vector<std::string> grp;
  collect_group (grp, name, m_root);
  return grp;
}

std::vector<lay::Action *>
AbstractMenu::group_actions (const std::string &name)
{
  std::vector<std::string> grp = group (name);

  std::vector<lay::Action *> actions;
  actions.reserve (grp.size ());
  for (std::vector<std::string>::const_iterator g = grp.begin (); g != grp.end (); ++g) {
    actions.push_back (action (*g));
  }
  return actions;
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

void
AbstractMenu::emit_changed ()
{
  m_config_actions_valid = false;
  emit changed ();
}

std::vector<ConfigureAction *> AbstractMenu::configure_actions (const std::string &name)
{
  if (! m_config_actions_valid) {

    std::vector<lay::ConfigureAction *> ca;
    collect_configure_actions (ca, m_root);

    m_config_action_by_name.clear ();
    for (std::vector<lay::ConfigureAction *>::const_iterator c = ca.begin (); c != ca.end (); ++c) {
      m_config_action_by_name [(*c)->get_cname ()].push_back (*c);
    }

    m_config_actions_valid = true;

  }

  std::map<std::string, std::vector<lay::ConfigureAction *> >::const_iterator c = m_config_action_by_name.find (name);
  return c == m_config_action_by_name.end () ? std::vector<lay::ConfigureAction *> () : c->second;
}

void
AbstractMenu::collect_configure_actions (std::vector<lay::ConfigureAction *> &ca, AbstractMenuItem &item)
{
  for (std::list<AbstractMenuItem>::iterator c = item.children.begin (); c != item.children.end (); ++c) {
    ConfigureAction *cca = dynamic_cast<ConfigureAction *> (c->action ());
    if (cca) {
      ca.push_back (cca);
    }
    collect_configure_actions (ca, *c);
  }
}

void
AbstractMenu::get_shortcuts (const std::string &root, std::map<std::string, std::string> &bindings, bool with_defaults)
{
  std::vector<std::string> items = this->items (root);
  for (std::vector<std::string>::const_iterator i = items.begin (); i != items.end (); ++i) {
    if (i->size () > 0) {
      if (is_valid (*i) && action (*i)->is_visible ()) {
        if (is_menu (*i)) {
          //  a menu must be listed (so it can be hidden), but does not have a shortcut
          //  but we don't include special menus
          if (i->at (0) != '@') {
            bindings.insert (std::make_pair (*i, std::string ()));
          }
          get_shortcuts (*i, bindings, with_defaults);
        } else if (! is_separator (*i)) {
          bindings.insert (std::make_pair (*i, with_defaults ? action (*i)->get_default_shortcut () : action (*i)->get_effective_shortcut ()));
        }
      }
    }
  }
}

}
