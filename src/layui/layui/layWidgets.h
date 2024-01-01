
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

#if defined(HAVE_QT)

#ifndef HDR_layWidgets
#define HDR_layWidgets

#include "layuiCommon.h"
#include "layMargin.h"

#include "tlObject.h"
#include "tlDeferredExecution.h"

#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QProxyStyle>
#include <QListWidget>
#include <string>

namespace db
{
  class Layout;
  class Library;
  struct LayerProperties;
}

namespace lay
{

class LayoutViewBase;
struct LayerSelectionComboBoxPrivateData;
struct CellViewSelectionComboBoxPrivateData;

/**
 *  @brief A selection button for dither pattern
 */
class LAYUI_PUBLIC DitherPatternSelectionButton
  : public QPushButton
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  DitherPatternSelectionButton (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~DitherPatternSelectionButton ();

  /**
   *  @brief Associate with a view 
   *
   *  This method is required to select the proper dither pattern
   */
  void set_view (lay::LayoutViewBase *view);

  /**
   *  @brief Set the dither pattern index
   */
  void set_dither_pattern (int dp);

  /**
   *  @brief Get the dither pattern index
   */
  int dither_pattern () const;

  /**
   *  @brief Override setText 
   */
  void setText (const QString &) { }

  /**
   *  @brief Override setPixmap 
   */
  void setPixmap (const QPixmap &) { }

signals:
  void dither_pattern_changed (int);

private slots:
  void browse_selected ();
  void menu_selected ();
  void menu_about_to_show ();

private:
  lay::LayoutViewBase *mp_view;
  int m_dither_pattern;

  void update_pattern ();
  void update_menu ();
};

/**
 *  @brief A selection button for dither pattern
 */
class LAYUI_PUBLIC LineStyleSelectionButton
  : public QPushButton
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LineStyleSelectionButton (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~LineStyleSelectionButton ();

  /**
   *  @brief Associate with a view
   *
   *  This method is required to select the proper dither pattern
   */
  void set_view (lay::LayoutViewBase *view);

  /**
   *  @brief Set the line style index
   */
  void set_line_style (int ls);

  /**
   *  @brief Get the line style index
   */
  int line_style () const;

  /**
   *  @brief Override setText
   */
  void setText (const QString &) { }

  /**
   *  @brief Override setPixmap
   */
  void setPixmap (const QPixmap &) { }

signals:
  void line_style_changed (int);

private slots:
  void browse_selected ();
  void menu_selected ();
  void menu_about_to_show ();

private:
  lay::LayoutViewBase *mp_view;
  int m_line_style;

  void update_pattern ();
  void update_menu ();
};

/**
 *  @brief A library selection combo box
 *
 *  This combo box allows selecting a library
 */
class LAYUI_PUBLIC LibrarySelectionComboBox
  : public QComboBox
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LibrarySelectionComboBox (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~LibrarySelectionComboBox ();

  /**
   *  @brief Set the current library
   *
   *  The current library is "none" (local definition) if the pointer is 0.
   */
  void set_current_library (db::Library *lib);

  /**
   *  @brief Get the current library
   *
   *  The pointer is 0 if "none" is selected.
   */
  db::Library *current_library () const;

  /**
   *  @brief Update the list of libraries
   */
  void update_list ();

  /**
   *  @brief Sets the technology filter
   *
   *  If a technology filter is set, only the libraries associated with the given
   *  technology are shown. If enable is false, the technology name is ignored and
   *  all libraries are shown.
   */
  void set_technology_filter (const std::string &tech, bool enable);

private:
  std::string m_tech;
  bool m_tech_set;
};

/**
 *  @brief A layer selection combo box
 *
 *  This combo box allows selecting a (physical) layer from a layout
 */
class LAYUI_PUBLIC LayerSelectionComboBox
  : public QComboBox, public tl::Object
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  LayerSelectionComboBox (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~LayerSelectionComboBox ();

  /**
   *  @brief Associate with a layout
   *
   *  Associates this widget with a certain layout object - this one is being
   *  scanned for layers that are presented in this combo box.
   */
  void set_layout (const db::Layout *layout);

  /**
   *  @brief Associate with a view and cellview index
   *
   *  This method can be used instead of set_layout. If this method is used, more
   *  functionality is available, i.e. the ability to create new layers.
   *  If all_layers is set to true, layers are shown which are in the layer list, but
   *  not created as layers yet.
   */
  void set_view (lay::LayoutViewBase *view, int cv_index, bool all_layers = false);

  /**
   *  @brief Sets a flag indicating whether the "new layer" option is available
   */
  void set_new_layer_enabled (bool f);

  /**
   *  @brief Gets a flag indicating whether the "new layer" option is available
   */
  bool is_new_layer_enabled () const;

  /**
   *  @brief Sets a flag indicating whether "no layer" is available as selection
   */
  void set_no_layer_available (bool f);

  /**
   *  @brief Gets a flag indicating whether "no layer" is available as selection
   */
  bool is_no_layer_available () const;

  /**
   *  @brief Set the current layer (index)
   */
  void set_current_layer (const db::LayerProperties &lp);

  /**
   *  @brief Set the current layer (index)
   */
  void set_current_layer (int l);

  /**
   *  @brief Get the current layer (index)
   */
  int current_layer () const;

  /**
   *  @brief Get the current layer properties
   */
  db::LayerProperties current_layer_props () const;

protected slots:
  void item_selected (int index);

private:
  LayerSelectionComboBoxPrivateData *mp_private;
  tl::DeferredMethod<LayerSelectionComboBox> dm_update_layer_list;

  void on_layer_list_changed (int);
  void update_layer_list ();
  void do_update_layer_list ();
};

/**
 *  @brief A cell view selection combo box
 *
 *  This combo box allows selecting a cellview from a lay::LayoutView
 */
class LAYUI_PUBLIC CellViewSelectionComboBox
  : public QComboBox
{
Q_OBJECT

public:
  CellViewSelectionComboBox (QWidget *parent);
  ~CellViewSelectionComboBox ();

  void set_layout_view (const lay::LayoutViewBase *view);
  const lay::LayoutViewBase *layout_view () const;

  void set_current_cv_index (int l);
  int current_cv_index () const;

private:
  CellViewSelectionComboBoxPrivateData *mp_private;
};

/**
 *  @brief Simple color chooser button
 *
 *  This class implements a special button that can replace a 
 *  usual push button and supplies a color chooser without the
 *  capability to switch to "auto" color mode.
 */
class LAYUI_PUBLIC SimpleColorButton
  : public QPushButton 
{
Q_OBJECT

public:
  SimpleColorButton (QPushButton *&to_replace, const char *name = 0);
  SimpleColorButton (QWidget *parent, const char *name = 0);

  QColor get_color () const;

signals:
  void color_changed (QColor color);

public slots:
  void set_color (QColor color);

private:
  QColor m_color;

  void set_color_internal (QColor color);

private slots:
  virtual void selected ();
};

/**
 *  @brief Margin edit box
 *
 *  This class implements a special widget to edit a lay::Margin object.
 *  This object allows specification of a relative or absolute margin.
 */
class LAYUI_PUBLIC MarginWidget
  : public QFrame
{
Q_OBJECT

public:
  MarginWidget (QWidget *parent, const char *name = 0);

  lay::Margin get_margin () const;
  void set_margin (const lay::Margin &margin);

protected slots:
  void mode_selection_changed ();

private:
  QLineEdit *mp_abs_edit;
  QLineEdit *mp_rel_edit;
  QComboBox *mp_mode_cb;
  lay::Margin m_margin;
};

/**
 *  @brief Color chooser button
 *
 *  This class implements a special button that can replace a 
 *  usual push button and supplies a color chooser with the
 *  capability to switch to "auto" color mode.
 */
class LAYUI_PUBLIC ColorButton
  : public QPushButton 
{
Q_OBJECT

public:
  ColorButton (QPushButton *&to_replace, const char *name = 0);
  ColorButton (QWidget *parent, const char *name = 0);

  QColor get_color () const;
  static void build_color_menu (QMenu *menu, QObject *receiver, const char *browse_slot, const char *selected_slot);

signals:
  void color_changed (QColor color);

public slots:
  void set_color (QColor color);

private:
  QColor m_color;

  void set_color_internal (QColor color);
  void build_menu ();

protected slots:
  virtual void browse_selected ();
  virtual void menu_selected ();

private slots:
  void menu_about_to_show ();
};

/**
 *  @brief An edit box with a clear button and options menu
 */
class LAYUI_PUBLIC DecoratedLineEdit
  : public QLineEdit
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  DecoratedLineEdit (QWidget *parent);

  /**
   *  @brief Destructor
   */
  ~DecoratedLineEdit ();

  /**
   *  @brief Sets a value indicating whether the clear button is enabled
   *  The clear button will be on the right side of the edit box. Pressing the button
   *  will clear the text and emit a "textEdited" and "textChanged" event.
   */
  void set_clear_button_enabled (bool en);

  /**
   *  @brief Gets a value indicating whether the clear button is enabled
   */
  bool is_clear_button_enabled () const
  {
    return m_clear_button_enabled;
  }

  /**
   *  @brief Sets a value indicating whether the options button is enabled
   *  The options button appears to the left. Pressing the button will either show
   *  the options menu as set by "setOptionsMenu" or emit the "optionsButtonClicked"
   *  signal.
   */
  void set_options_button_enabled (bool en);

  /**
   *  @brief Gets a value indicating whether the options button is enabled
   */
  bool is_option_button_enabled () const
  {
    return m_options_button_enabled;
  }

  /**
   *  @brief Sets the options menu to be shown when the options button is clicked.
   *  The DecoratedLineEdit object will not take ownership over the menu.
   */
  void set_options_menu (QMenu *menu);

  /**
   *  @brief Gets the options menu
   */
  QMenu *options_menu () const;

  /**
   *  @brief Sets a value indicating whether the widgets accepts ESC keys and sends an esc_pressed signal for this
   */
  void set_escape_signal_enabled (bool f);

  /**
   *  @brief gets a value indicating whether the widgets accepts ESC keys and sends an esc_pressed signal for this
   */
  bool escape_signal_enabled () const
  {
    return m_escape_signal_enabled;
  }

  /**
   *  @brief Sets a value indicating whether the widgets accepts Tab keys and sends an tab_pressed or backtab_pressed signal for this
   */
  void set_tab_signal_enabled (bool f);

  /**
   *  @brief gets a value indicating whether the widgets accepts Tab keys and sends an tab_pressed or backtab_pressed signal for this
   */
  bool tab_signal_enabled () const
  {
    return m_tab_signal_enabled;
  }

signals:
  void options_button_clicked ();
  void esc_pressed ();
  void tab_pressed ();
  void backtab_pressed ();
  void clear_pressed ();

protected:
  void mousePressEvent (QMouseEvent *event);
  void mouseReleaseEvent (QMouseEvent *event);
  void resizeEvent (QResizeEvent *event);
  void keyPressEvent (QKeyEvent *event);
  bool focusNextPrevChild (bool next);
  bool event (QEvent *event);

private:
  bool m_clear_button_enabled;
  bool m_options_button_enabled;
  bool m_escape_signal_enabled;
  bool m_tab_signal_enabled;
  QLabel *mp_options_label;
  QLabel *mp_clear_label;
  QMenu *mp_options_menu;
  int m_default_left_margin, m_default_right_margin;
};

/**
 *  @brief An interactive list widget which offers slots to delete and move items and interfaces to std::vector<std::string>
 */
class LAYUI_PUBLIC InteractiveListWidget
  : public QListWidget
{
Q_OBJECT

public:
  /**
   *  @brief Constructor
   */
  InteractiveListWidget (QWidget *parent = 0);

  /**
   *  @brief Sets the items in the widget
   */
  void set_values (const std::vector<std::string> &values);

  /**
   *  @brief Gets the items in the widget
   */
  std::vector<std::string> get_values ();

  /**
   *  @brief Adds a value
   */
  void add_value (const std::string &value);

  /**
   *  @brief Adds values
   */
  void add_values (const std::vector<std::string> &values);

private slots:
  /**
   *  @brief Deletes the selected items
   */
  void delete_selected_items ();

  /**
   *  @brief Moves the selected items up
   */
  void move_selected_items_up ();

  /**
   *  @brief Moves the selected items down
   */
  void move_selected_items_down ();

private:
  void refresh_flags ();

  bool m_drag_and_drop_enabled;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
