
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2023 Matthias Koefferlein

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

namespace lay
{

/**
 *  @brief A collection of highlighters 
 */
class MacroEditorHighlighters
{
public:
  MacroEditorHighlighters (QObject *parent);

  QSyntaxHighlighter *highlighter_for (QObject *parent, lym::Macro::Interpreter lang, const std::string &dsl_name);

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

  lay::GenericSyntaxHighlighter *highlighter_for_scheme (QObject *parent, const std::string &scheme, GenericSyntaxHighlighterAttributes *attributes);
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

signals:
  void help_requested (const QString &s);
  void search_requested (const QString &s);
  void edit_trace (bool);

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
  lym::Macro *mp_macro;
  MacroEditorExecutionModel *mp_exec_model;
  MacroEditorTextWidget *mp_text;
  MacroEditorSidePanel *mp_side_panel;
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

  void update_extra_selections ();
  bool return_pressed ();
  bool backspace_pressed ();
  bool back_tab_key_pressed ();
  bool tab_key_pressed ();
  void fill_completer_list ();
  void complete ();
  QTextCursor get_completer_cursor (int &pos0, int &pos);
  void replace_in_selection (const QString &replace, bool first);

  bool eventFilter (QObject *watched, QEvent *event);
};

}

#endif

