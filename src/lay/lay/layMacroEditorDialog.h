
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


#ifndef HDR_layMacroEditorDialog
#define HDR_layMacroEditorDialog

#include "layCommon.h"

#include "ui_MacroEditorDialog.h"
#include "layMacroEditorPage.h"
#include "layMacroController.h"
#include "layPlugin.h"
#include "tlDeferredExecution.h"
#include "tlTimer.h"
#include "tlFileSystemWatcher.h"
#include "tlDeferredExecution.h"
#include "tlScriptError.h"
#include "tlInclude.h"
#include "lymMacro.h"
#include "gsiInterpreter.h"

#include <QDialog>
#include <QTextCharFormat>
#include <QPixmap>

#include <set>
#include <map>

class QDialog;
class QTreeWidgetItem;

namespace lay
{

extern const std::string cfg_macro_editor_styles;
extern const std::string cfg_macro_editor_save_all_on_run;
extern const std::string cfg_macro_editor_stop_on_exception;
extern const std::string cfg_macro_editor_file_watcher_enabled;
extern const std::string cfg_macro_editor_font_family;
extern const std::string cfg_macro_editor_font_size;
extern const std::string cfg_macro_editor_tab_width;
extern const std::string cfg_macro_editor_indent;
extern const std::string cfg_macro_editor_window_state;
extern const std::string cfg_macro_editor_console_mru;
extern const std::string cfg_macro_editor_console_interpreter;
extern const std::string cfg_macro_editor_open_macros;
extern const std::string cfg_macro_editor_current_macro;
extern const std::string cfg_macro_editor_active_macro;
extern const std::string cfg_macro_editor_watch_expressions;
extern const std::string cfg_macro_editor_debugging_enabled;
extern const std::string cfg_macro_editor_ignore_exception_list;

class MacroEditorTree;
class BrowserPanel;
class MainWindow;

class MacroEditorDialog
  : public QDialog,
    public lay::Plugin,
    public gsi::Console,
    public gsi::ExecutionHandler,
    private Ui::MacroEditorDialog
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
  MacroEditorDialog (lay::Dispatcher *pr, lym::MacroCollection *root);

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
  lym::Macro *run_macro () const
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
  void save_as_button_clicked ();
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
  void item_double_clicked (lym::Macro *macro);
  void move_macro (lym::Macro *source, lym::MacroCollection *target);
  void move_folder (lym::MacroCollection *source, lym::MacroCollection *target);
  void macro_renamed (lym::Macro *macro);
  void folder_renamed (lym::MacroCollection *mc);
  void current_tab_changed (int index);
  void commit ();
  void stack_element_double_clicked (QListWidgetItem *item);
  void search_edited ();
  void search_editing ();
  void search_finished ();
  void tab_close_requested (int);
  void close_requested ();
  void close_all ();
  void close_all_but_this ();
  void close_all_left ();
  void close_all_right ();
  void replace_mode_button_clicked ();
  void replace_next_button_clicked ();
  void replace_all_button_clicked ();
  void find_next_button_clicked ();
  void find_prev_button_clicked ();
  void help_requested (const QString &s);
  void search_requested (const QString &s, bool prev);
  void macro_changed (lym::Macro *macro);
  void macro_deleted (lym::Macro *macro);
  void macro_collection_deleted (lym::MacroCollection *collection);
  void macro_collection_changed (lym::MacroCollection *collection);
  void add_watch ();
  void edit_watch ();
  void del_watches ();
  void clear_watches ();
  void set_debugging_on (bool on);
  void tabs_menu_about_to_show ();
  void tab_menu_selected ();

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
  lym::Macro *create_macro_here(const char *name = 0);
  void move_subfolder (lym::MacroCollection *source, lym::MacroCollection *target);
  lay::MacroEditorPage *create_page (lym::Macro *macro);
  void open_macro (lym::Macro *macro);
  void close_many (int which_relative_to_current);
  void ensure_writeable_collection_selected ();
  void update_console_text ();
  void do_current_tab_changed ();
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
  MacroEditorPage *editor_for_macro (lym::Macro *macro);
  void run (int stop_stack_depth, lym::Macro *macro);
  lym::Macro *current_run_macro ();
  void update_ui_to_run_mode ();
  void do_update_ui_to_run_mode ();
  void set_run_macro (lym::Macro *m);
  void apply_search (bool if_needed);
  void process_events (QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);
  void sync_file_watcher (lym::MacroCollection *current);
  void do_refresh_file_watcher ();
  void refresh_file_watcher ();
  void reload_macros ();
  void update_inspected ();
  void update_watches ();
  lym::Macro *new_macro ();
  void do_search_edited ();
  void set_editor_focus ();
  void select_trace (size_t index);
  bool configure (const std::string &name, const std::string &value);
  void config_finalize ();
  void translate_pseudo_id (size_t &file_id, int &line);
  void exit_if_needed ();

  lay::Dispatcher *mp_plugin_root;
  lym::MacroCollection *mp_root;
  bool m_first_show;
  QPoint m_mouse_pos;
  bool m_debugging_on;
  lym::Macro *mp_run_macro;
  std::vector<lym::Macro *> m_macro_templates;
  tl::DeferredMethod<MacroEditorDialog> md_update_console_text;
  TextEditWidget *mp_console_text;
  std::map <lym::Macro *, MacroEditorPage *> m_tab_widgets;
  int m_history_index;
  bool m_in_event_handler;
  QString m_edit_text;
  output_stream m_os;
  bool m_new_line;
  QTextCharFormat m_stdout_format;
  QTextCharFormat m_echo_format;
  QTextCharFormat m_stderr_format;
  MacroEditorHighlighters m_highlighters;
  std::vector<std::pair<lym::Macro *, MacroEditorPage *> > m_file_to_widget;
  std::vector<tl::IncludeExpander> m_include_expanders;
  std::map<std::string, size_t> m_include_paths_to_ids;
  std::map<std::pair<size_t, int>, std::pair<size_t, int> > m_include_file_id_cache;
  std::vector<lay::MacroEditorTree *> m_macro_trees;
  bool m_in_exec, m_in_breakpoint, m_ignore_exec_events;
  gsi::Interpreter *mp_exec_controller, *mp_current_interpreter;
  bool m_continue;
  int m_trace_count;
  int m_current_stack_depth;
  int m_stop_stack_depth;
  int m_eval_context;
  double m_process_events_interval;
  tl::Clock m_last_process_events;
  bool m_window_closed;
  bool m_needs_update;
  std::string m_styles;
  int m_ntab;
  int m_nindent;
  bool m_save_all_on_run;
  bool m_stop_on_exception;
  std::set<std::string> m_ignore_exception_list;
  bool m_file_watcher_enabled;
  std::string m_font_family;
  int m_font_size;
  std::vector<lay::MacroController::MacroCategory> m_categories;
  std::vector<std::pair<gsi::Interpreter *, std::string> > m_watch_expressions;
  std::vector<EditTrace> m_edit_trace;
  size_t m_edit_trace_index;
  bool m_add_edit_trace_enabled;
  tl::FileSystemWatcher *m_file_watcher;
  QTimer *m_file_changed_timer;
  std::vector<QString> m_changed_files, m_removed_files;
  tl::DeferredMethod<MacroEditorDialog> dm_refresh_file_watcher;
  tl::DeferredMethod<MacroEditorDialog> dm_update_ui_to_run_mode;
  tl::DeferredMethod<MacroEditorDialog> dm_current_tab_changed;
  QMenu *mp_tabs_menu;
};

}

#endif

