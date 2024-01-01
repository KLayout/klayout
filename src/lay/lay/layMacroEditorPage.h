
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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

#ifndef HDR_layMacroEditorPage
#define HDR_layMacroEditorPage

#include "layCommon.h"

#include "lymMacro.h"
#include "layGenericSyntaxHighlighter.h"
#include "tlVariant.h"

#include <QDialog>
#include <QPixmap>
#include <QRegExp>

#include <set>

#if QT_VERSION >= 0x040400
#  include <QPlainTextEdit>
typedef QPlainTextEdit TextEditWidget;
#else
#  include <QTextEdit>
typedef QTextEdit TextEditWidget;
#endif

class QLabel;
class QSyntaxHighlighter;
class QTimer;
class QWindow;
class QListWidget;
class QVBoxLayout;

namespace lay
{

class MacroEditorPage;

/**
 *  @brief A collection of highlighters 
 */
class MacroEditorHighlighters
{
public:
  MacroEditorHighlighters (QObject *parent);

  QSyntaxHighlighter *highlighter_for (QObject *parent, lym::Macro::Interpreter lang, const std::string &dsl_name, bool initialize);

  GenericSyntaxHighlighterAttributes *attributes_for (lym::Macro::Interpreter lang, const std::string &dsl_name);
  GenericSyntaxHighlighterAttributes *basic_attributes ();

  std::string to_string () const;
  void load (const std::string &s);

  typedef std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::const_iterator const_iterator;
  typedef std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> >::iterator iterator;

  const_iterator begin () const
  {
    return m_attributes.begin ();
  }

  const_iterator end () const
  {
    return m_attributes.end ();
  }

  iterator begin () 
  {
    return m_attributes.begin ();
  }

  iterator end () 
  {
    return m_attributes.end ();
  }

private:
  std::vector<std::pair<std::string, GenericSyntaxHighlighterAttributes> > m_attributes;
  GenericSyntaxHighlighterAttributes m_basic_attributes;

  lay::GenericSyntaxHighlighter *highlighter_for_scheme (QObject *parent, const std::string &scheme, GenericSyntaxHighlighterAttributes *attributes, bool initialize);
  std::string scheme_for (lym::Macro::Interpreter lang, const std::string &dsl_name);
};

/**
 *  @brief The execution model
 *
 *  The execution model stores the breakpoints and current execution line for a given 
 *  file.
 */
class MacroEditorExecutionModel
  : public QObject
{
Q_OBJECT

public:
  MacroEditorExecutionModel (QObject *parent);

  const std::set<int> &breakpoints () const
  {
    return m_breakpoints;
  }

  bool is_breakpoint (int line) const
  {
    return m_breakpoints.find (line) != m_breakpoints.end ();
  }

  void set_interpreter (lym::Macro::Interpreter lang);

  void set_breakpoints (const std::set<int> &b);

  void set_breakpoint (int line);

  void remove_breakpoint (int line);

  void toggle_breakpoint (int line); 

  int current_line () const
  {
    return m_current_line;
  }

  void set_current_line (int l, bool force_event = false);

  bool run_mode () const
  {
    return m_run_mode;
  }

  void set_run_mode (bool run_mode);

signals:
  void breakpoints_changed ();
  void current_line_changed ();
  void run_mode_changed ();

private:
  std::set<int> m_breakpoints;
  int m_current_line;
  bool m_run_mode;
  lym::Macro::Interpreter m_interpreter;
};

/**
 *  @brief A specialization of TextEditWidget which catches the scroll events and generates signals from them
 */
class MacroEditorTextWidget
  : public TextEditWidget
{
Q_OBJECT

public:
  MacroEditorTextWidget (QWidget *parent);

  void paintEvent (QPaintEvent *event);

signals:
  void contentsChanged ();

private:
  QRect m_r;
};

/**
 *  @brief The side panel is the widget that shows the current line and the breakpoints
 */
class MacroEditorSidePanel
  : public QWidget
{
Q_OBJECT 

public:
  MacroEditorSidePanel (QWidget *parent, MacroEditorTextWidget *text, MacroEditorExecutionModel *exec_model);
  
  QSize sizeHint () const;
  void paintEvent (QPaintEvent *event);
  void mousePressEvent (QMouseEvent *event);

  void set_watermark (const QString &wm);
  void set_debugging_on (bool debugging_on);

private slots:
  void redraw ();     

private:
  MacroEditorTextWidget *mp_text;
  MacroEditorExecutionModel *mp_exec_model;
  QPixmap m_breakpoint_pixmap;
  QPixmap m_breakpoint_disabled_pixmap;
  QPixmap m_exec_point_pixmap;
  QString m_watermark;
  bool m_debugging_on;
};

/**
 *  @brief Descriptor for a notification inside the macro editor
 *
 *  Notifications are popups added at the top of the view to indicate need for reloading for example.
 *  Notifications have a name, a title, optional actions (id, title) and a parameter (e.g. file path to reload).
 *  Actions are mapped to QPushButtons.
 */
class MacroEditorNotification
{
public:
  MacroEditorNotification (const std::string &name, const std::string &title, const tl::Variant &parameter = tl::Variant ())
    : m_name (name), m_title (title), m_parameter (parameter)
  {
    //  .. nothing yet ..
  }

  void add_action (const std::string &name, const std::string &title)
  {
    m_actions.push_back (std::make_pair (name, title));
  }

  const std::vector<std::pair<std::string, std::string> > &actions () const
  {
    return m_actions;
  }

  const std::string &name () const
  {
    return m_name;
  }

  const std::string &title () const
  {
    return m_title;
  }

  const tl::Variant &parameter () const
  {
    return m_parameter;
  }

  bool operator<(const MacroEditorNotification &other) const
  {
    if (m_name != other.name ()) {
      return m_name < other.name ();
    }
    return m_parameter < other.parameter ();
  }

  bool operator==(const MacroEditorNotification &other) const
  {
    if (m_name != other.name ()) {
      return false;
    }
    return m_parameter == other.parameter ();
  }

private:
  std::string m_name;
  std::string m_title;
  tl::Variant m_parameter;
  std::vector<std::pair<std::string, std::string> > m_actions;
};

/**
 *  @brief A widget representing a notification
 */
class MacroEditorNotificationWidget
  : public QFrame
{
Q_OBJECT

public:
  MacroEditorNotificationWidget (MacroEditorPage *parent, const MacroEditorNotification *notification);

private slots:
  void action_triggered ();
  void close_triggered ();

private:
  MacroEditorPage *mp_parent;
  const MacroEditorNotification *mp_notification;
  std::map<QObject *, std::string> m_action_buttons;
};

class MacroEditorPage
  : public QWidget
{
Q_OBJECT

public:
  MacroEditorPage (QWidget *parent, MacroEditorHighlighters *highlighters);

  void connect_macro (lym::Macro *macro);

  lym::Macro *macro () const
  {
    return mp_macro;
  }

  const std::string path () const
  {
    return m_path;
  }

  bool is_modified () const
  {
    return m_is_modified;
  }

  void set_error_line (int line);
  void goto_position (int line, int pos);
  void goto_line (int line);

  void set_ntab (int n);

  void set_nindent (int n);

  void set_font (const std::string &family, int size);

  MacroEditorExecutionModel *exec_model () const
  {
    return mp_exec_model;
  }

  int current_line () const;
  int current_pos () const;
  bool has_multi_block_selection () const;

  void set_debugging_on (bool debugging_on);

  void set_search (const QRegExp &text);

  const QRegExp &get_search () const
  {
    return m_current_search;
  }

  void find_reset ();
  bool find_next ();
  bool find_prev ();

  void replace_and_find_next (const QString &replace);

  void replace_all (const QString &replace);

  void apply_attributes ();

  void set_editor_focus ();

  void add_notification (const MacroEditorNotification &notificaton);
  void remove_notification (const MacroEditorNotification &notificaton);

signals:
  void help_requested (const QString &s);
  void search_requested (const QString &s, bool backward);
  void edit_trace (bool);
  void close_requested ();

public slots:
  void commit ();
  void update ();

protected slots:
  void text_changed ();
  void cursor_position_changed ();
  void breakpoints_changed ();
  void current_line_changed ();
  void run_mode_changed ();
  void completer_timer ();
  void hide_completer ();

private:
  friend class MacroEditorNotificationWidget;

  struct CompareNotificationPointers
  {
    bool operator() (const MacroEditorNotification *a, const MacroEditorNotification *b) const
    {
      return *a < *b;
    }
  };

  lym::Macro *mp_macro;
  std::string m_path;
  MacroEditorExecutionModel *mp_exec_model;
  MacroEditorTextWidget *mp_text;
  MacroEditorSidePanel *mp_side_panel;
  QVBoxLayout *mp_layout;
  QLabel *mp_readonly_label;
  bool m_is_modified;
  MacroEditorHighlighters *mp_highlighters;
  QSyntaxHighlighter *mp_highlighter;
  int m_error_line;
  int m_ntab, m_nindent;
  std::set<QTextBlock> m_breakpoints;
  QRegExp m_current_search;
  QTextCursor m_edit_cursor;
  bool m_ignore_cursor_changed_event;
  QTimer *mp_completer_timer;
  QWidget *mp_completer_popup;
  QListWidget *mp_completer_list;
  std::list<MacroEditorNotification> m_notifications;
  std::map<const MacroEditorNotification *, QWidget *, CompareNotificationPointers> m_notification_widgets;

  void update_extra_selections ();
  bool return_pressed ();
  bool backspace_pressed ();
  bool back_tab_key_pressed ();
  bool tab_key_pressed ();
  void fill_completer_list ();
  void complete ();
  QTextCursor get_completer_cursor (int &pos0, int &pos);
  bool select_match_here ();
  void replace_in_selection (const QString &replace, bool first);
  void notification_action (const MacroEditorNotification &notification, const std::string &action);

  bool eventFilter (QObject *watched, QEvent *event);
};

}

#endif

