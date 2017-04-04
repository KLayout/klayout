
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


#ifndef HDR_layMacroEditorDialog
#define HDR_layMacroEditorDialog

#include "layCommon.h"

#include "ui_MacroEditorDialog.h"
#include "layMacroEditorPage.h"
#include "tlDeferredExecution.h"
#include "tlTimer.h"
#include "tlFileSystemWatcher.h"
#include "tlDeferredExecution.h"
#include "layMacro.h"
#include "gsiInterpreter.h"
#include "tlScriptError.h"

#include <QDialog>
#include <QTextCharFormat>
#include <QPixmap>

#include <set>
#include <map>

class QDialog;
class QTreeWidgetItem;

namespace lay
{

class Macro;
class MacroCollection;
class MacroEditorTree;
class BrowserPanel;

class MacroEditorDialog
  : public QDialog, public gsi::Console, private Ui::MacroEditorDialog, public gsi::ExecutionHandler
{
  Q_OBJECT 

  struct EditTrace
  {
    EditTrace ()
      : path (), line (0), pos (0)
    {
      //  .. nothing yet ..
    }

    std::string path;
    int line, pos;
  };

public:
  /**
   *  @brief Constructor
   */
  MacroEditorDialog (QWidget *parent, lay::MacroCollection *root);

  /**
   *  @brief Destructor
   */
  ~MacroEditorDialog ();

  /**
   *  @brief Gets the singleton instance of the macro editor
   */
  static MacroEditorDialog *instance ();

  /**
   *  @brief Reimplementation of gsi::Console:
   */
  void write_str (const char *text, output_stream os);

  /**
   *  @brief Reimplementation of gsi::Console:
   */
  void flush ();

  /**
   *  @brief Reimplementation of gsi::Console:
   */
  bool is_tty ();

  /**
   *  @brief Reimplementation of gsi::Console:
   */
  int columns ();

  /**
   *  @brief Reimplementation of gsi::Console:
   */
  int rows ();

  /**
   *  @brief Perform all operations on application exit and return true if this is possible
   */
  bool can_exit ();

  /**
   *  @brief Override show to bring up a tip dialog initially
   *
   *  Depending on the category, a different tip dialog will be shown.
   *  If "force_add" is true, a new macro will be created, otherwise only
   *  if none exists yet.
   */
  void show (const std::string &cat = std::string (), bool force_add = false);

  /**
   *  @brief Gets the macro which is run
   */
  lay::Macro *run_macro () const
  {
    return mp_run_macro;
  }

  /**
   *  @brief Returns true while the macro is executing
   */
  bool in_exec () const
  {
    return m_in_exec;
  }

  /**
   *  @brief Selects the current category in the tree view
   */
  void select_category (const std::string &cat);

public slots:
  /**
   *  @brief Reloads all macros from the paths registered
   */
  void refresh ();

private slots:
  void help_button_clicked ();
  void add_button_clicked ();
  void close_button_clicked ();
  void delete_button_clicked ();
  void rename_button_clicked ();
  void import_button_clicked ();
  void new_folder_button_clicked ();
  void save_all_button_clicked ();
  void save_button_clicked ();
  void run_button_clicked ();
  void run_this_button_clicked ();
  void single_step_button_clicked ();
  void next_step_button_clicked ();
  void pause_button_clicked ();
  void stop_button_clicked ();
  void properties_button_clicked ();
  void setup_button_clicked ();
  void breakpoint_button_clicked ();
  void add_location ();
  void remove_location ();
  void clear_breakpoints_button_clicked ();
  void item_double_clicked (lay::Macro *macro);
  void move_macro (lay::Macro *source, lay::MacroCollection *target);
  void move_folder (lay::MacroCollection *source, lay::MacroCollection *target);
  void macro_renamed (lay::Macro *macro);
  void folder_renamed (lay::MacroCollection *mc);
  void current_tab_changed (int index);
  void commit ();
  void stack_element_double_clicked (QListWidgetItem *item);
  void search_edited ();
  void tab_close_requested (int);
  void replace_mode_button_clicked ();
  void replace_next_button_clicked ();
  void replace_all_button_clicked ();
  void find_next_button_clicked ();
  void help_requested (const QString &s);
  void macro_changed (Macro *macro);
  void macro_deleted (Macro *macro);
  void add_watch ();
  void edit_watch ();
  void del_watches ();
  void clear_watches ();
  void set_debugging_on (bool on);

  //  edit trace navigation
  void forward ();
  void backward ();
  void add_edit_trace (bool compress);
  void clear_edit_trace ();

protected slots:
  void file_changed_timer ();
  void immediate_command_text_changed (const QString &text);
  void file_changed (const QString &path);
  void file_removed (const QString &path);
  void clear_log ();
  void search_replace ();
  void apply_search ()
  {
    apply_search (false);
  }

protected:
  void showEvent (QShowEvent *);
  void closeEvent (QCloseEvent *);
  void reject ();
  void accept ();

  bool eventFilter (QObject *obj, QEvent *event);
  void execute (const QString &cmd);

private:
  lay::MacroEditorTree *current_macro_tree ();
  lay::Macro *create_macro_here(const char *name = 0);
  void move_subfolder (lay::MacroCollection *source, lay::MacroCollection *target);
  lay::MacroEditorPage *create_page (lay::Macro *macro);
  void ensure_writeable_collection_selected ();
  void update_console_text ();
  void start_exec (gsi::Interpreter *interpreter);
  void end_exec (gsi::Interpreter *interpreter);
  size_t id_for_path (gsi::Interpreter *interpreter, const std::string &path);
  void trace (gsi::Interpreter *interpreter, size_t file_id, int line, const gsi::StackTraceProvider *stack_trace_provider);
  void exception_thrown (gsi::Interpreter *interpreter, size_t file_id, int line, const std::string &eclass, const std::string &emsg, const gsi::StackTraceProvider *stack_trace_provider);
  void enter_exec_mode ();
  void leave_exec_mode ();
  void enter_breakpoint_mode (gsi::Interpreter *interpreter, const gsi::StackTraceProvider *stack_trace_provider);
  void leave_breakpoint_mode ();
  void handle_error (tl::ScriptError &re);
  void set_exec_point (const std::string *file, int line, int eval_context);
  MacroEditorPage *editor_for_file (const std::string &path);
  MacroEditorPage *editor_for_macro (lay::Macro *macro);
  void run (int stop_stack_depth, lay::Macro *macro);
  lay::Macro *current_run_macro ();
  void update_ui_to_run_mode ();
  void set_run_macro (lay::Macro *m);
  void apply_search (bool if_needed);
  void process_events (QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
  bool sync_macros (lay::MacroCollection *current, lay::MacroCollection *actual);
  void sync_file_watcher (lay::MacroCollection *current);
  void do_refresh_file_watcher ();
  void refresh_file_watcher ();
  void reload_macros ();
  void update_inspected ();
  void update_watches ();
  lay::Macro *new_macro ();
  void do_search_edited ();
  void select_trace (size_t index);

  lay::MacroCollection *mp_root;
  bool m_first_show;
  bool m_in_processing;
  bool m_debugging_on;
  lay::Macro *mp_run_macro;
  std::vector<lay::Macro *> m_macro_templates;
  tl::DeferredMethod<MacroEditorDialog> md_update_console_text;
  tl::DeferredMethod<MacroEditorDialog> md_search_edited;
  TextEditWidget *mp_console_text;
  std::map <Macro *, MacroEditorPage *> m_tab_widgets;
  int m_history_index;
  bool m_in_event_handler;
  QString m_edit_text;
  output_stream m_os;
  bool m_new_line;
  QTextCharFormat m_stdout_format;
  QTextCharFormat m_echo_format;
  QTextCharFormat m_stderr_format;
  MacroEditorHighlighters m_highlighters;
  std::vector<std::pair<Macro *, MacroEditorPage *> > m_file_to_widget;
  std::vector<lay::MacroEditorTree *> m_macro_trees;
  bool m_in_exec, m_in_breakpoint;
  gsi::Interpreter *mp_exec_controller, *mp_current_interpreter;
  bool m_continue;
  int m_trace_count;
  int m_current_stack_depth;
  int m_stop_stack_depth;
  int m_eval_context;
  double m_process_events_interval;
  tl::Clock m_last_process_events;
  bool m_window_closed;
  int m_ntab;
  int m_nindent;
  bool m_save_all_on_run;
  bool m_stop_on_exception;
  bool m_file_watcher_enabled;
  std::string m_font_family;
  int m_font_size;
  std::vector<std::pair<std::string, std::string> > m_categories;
  std::vector<std::pair<gsi::Interpreter *, std::string> > m_watch_expressions;
  std::vector<EditTrace> m_edit_trace;
  size_t m_edit_trace_index;
  bool m_add_edit_trace_enabled;
  tl::FileSystemWatcher *m_file_watcher;
  QTimer *m_file_changed_timer;
  std::vector<QString> m_changed_files, m_removed_files;
  tl::DeferredMethod<MacroEditorDialog> dm_refresh_file_watcher;
};

}

#endif

