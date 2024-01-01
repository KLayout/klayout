
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

#ifndef HDR_layLayerControlPanel
#define HDR_layLayerControlPanel


#include "layuiCommon.h"
#include "layCanvasPlane.h"
#include "layViewOp.h"
#include "layLayoutViewBase.h"
#include "layColorPalette.h"
#include "layStipplePalette.h"
#include "layDitherPattern.h"
#include "layLayerProperties.h"
#include "layLayerTreeModel.h"
#include "layWidgets.h"
#include "dbObject.h"
#include "tlDeferredExecution.h"

#include <vector>
#include <string>
#include <set>
#include <algorithm>

#include <QFrame>
#include <QLabel>
#include <QPainter>
#include <QPaintEvent>
#include <QTreeView>

class QTreeView;
class QModelIndex;
class QMenu;
class QLabel;
class QTabBar;
class QCheckBox;

namespace lay 
{

/**
 *  @brief A layer tree widget helper class
 *
 *  A specialization of the TreeView that allows us to control
 *  sizeHint which otherwise is pretty large (around 100 pixel per column)
 *  and attaches the layer tree model to the view.
 */

class LCPTreeWidget : public QTreeView
{
Q_OBJECT 

public:
  LCPTreeWidget (QWidget *parent, lay::LayerTreeModel *model, const char *name);
  ~LCPTreeWidget ();

  virtual QSize sizeHint () const;

  //  overload the double click event, because the default behaviour is
  //  expanding/collapsing the items
  virtual void mouseDoubleClickEvent (QMouseEvent *event);

  void set_selection (const std::vector<lay::LayerPropertiesConstIterator> &sel);
  void set_current (const lay::LayerPropertiesConstIterator &sel);
  void collapse_all ();
  void expand_all ();

signals:
  void double_clicked (const QModelIndex &, Qt::KeyboardModifiers);
  void search_triggered (const QString &t);

protected:
  virtual void keyPressEvent (QKeyEvent *event);
  virtual bool event (QEvent *event);
  virtual bool focusNextPrevChild (bool next);

private:
  lay::LayerTreeModel *mp_model;
};

/**
 *  @brief The layer control panel 
 *
 *  The layer control panel has a layer list and four panels 
 *  for colors, dither pattern and visibility.
 *
 *  The class communicates with a Layout interface for
 *  retrieval and changing of layer properties.
 */
class LAYUI_PUBLIC LayerControlPanel
  : public QFrame, 
    public db::Object,
    public tl::Object
{
Q_OBJECT 

public:
  enum SortOrder { ByName, ByIndexLayerDatatype, ByIndexDatatypeLayer, ByLayerDatatypeIndex, ByDatatypeLayerIndex };
  enum RegroupMode { RegroupByIndex, RegroupByDatatype, RegroupByLayer, RegroupFlatten };

  /**
   *  @brief Constructor
   *
   *  @param layout The Layout interface that the layer
   *         control panel is attached to
   *  @param parent The Qt parent widget
   *  @param name The layer control panel's widget name
   */
  LayerControlPanel (LayoutViewBase *view, db::Manager *manager, QWidget *parent = 0, const char *name = "control_panel");

  /**
   *  @brief Destructor
   */
  ~LayerControlPanel ();

  /** 
   *  @brief Return true, if the tree view has the focus
   */
  bool has_focus () const;

  /**
   *  @brief Tell, if there is something to copy
   */
  bool has_selection () const;

  /**
   *  @brief Cut to clipboards
   */
  void cut ();

  /**
   *  @brief Copy to clipboards
   */
  void copy ();

  /**
   *  @brief Paste from clipboard
   */
  void paste ();

  /**
   *  @brief Enable or disable stipples
   */
  void set_no_stipples (bool ns);

  /**
   *  @brief Changing of the background color
   */
  void set_background_color (tl::Color c);

  /**
   *  @brief Changing of the text color
   */
  void set_text_color (tl::Color c);

  /**
   *  @brief Set the "hide empty layers" flag
   *
   *  If this flag is set, empty layers (either completely empty or no shapes in view - depending on the test_shapes_in_view flag)
   *  are not shown. Otherwise they are shown "faded".
   */
  void set_hide_empty_layers (bool f);

  /**
   *  @brief Get the "hide empty layers" flag
   */
  bool hide_empty_layers ();

  /**
   *  @brief Set the "layer visibility follows selection" flag
   *
   *  If this flag is set, only the selected layers are made visible.
   */
  void set_layer_visibility_follows_selection (bool f);

  /**
   *  @brief Get the "layer visibility follows selection" flag
   */
  bool layer_visibility_follows_selection ();

  /**
   *  @brief Set the "test_shapes_in_view" flag
   *
   *  If this flag is set, layers without a shape in the viewport are shown "empty".
   */
  void set_test_shapes_in_view (bool f);

  /**
   *  @brief Get the "test shapes in view" flag
   */
  bool test_shapes_in_view () 
  {
    return mp_model->get_test_shapes_in_view ();
  }

  /**
   *  @brief Set the animation phase
   */
  void set_phase (int phase);

  /**
   *  @brief Sets highres mode
   */
  void set_highres_mode (bool hrm);

  /**
   *  @brief Sets oversampling mode
   */
  void set_oversampling (int os);

  /**
   *  @brief Tell, if the model has been updated already (true) or if it is still under construction (false)
   */
  bool model_updated ();

  /**
   *  @brief Inform of coming changes
   *  
   *  This method may be called before changes are made to the cell list.
   *  "end_updates" resets this state.
   */
  void begin_updates ();

  /**
   *  @brief Cancel the "begin_update" state
   */
  void cancel_updates ();

  /**
   *  @brief Tells that updates started with begin_updates() have been finished
   *
   *  In result, this method will perform all actions to update the display
   *  and reset the "in_update" state.
   */
  void end_updates ();

  /**
   *  @brief Return the selected layers
   */
  std::vector<lay::LayerPropertiesConstIterator> selected_layers () const;

  /**
   *  @brief Make the given layers selected
   */
  void set_selection (const std::vector<lay::LayerPropertiesConstIterator> &new_sel);

  /**
   *  @brief Sets the current layer
   *
   *  This will also select this layer.
   */
  void set_current_layer (const lay::LayerPropertiesConstIterator &l);

  /**
   *  @brief Return the current layer index
   *
   *  Will return a "null" iterator if no layer is selected currently.
   */
  lay::LayerPropertiesConstIterator current_layer () const;

  /**
   *  @brief Sort the layer list in the given order 
   */
  void sort_layers (SortOrder order);

  /**
   *  @brief Regroup the layer list in the given way 
   */
  void regroup_layers (RegroupMode mode);

  /**
   *  @brief Implementation of the undo operations
   */
  virtual void undo (db::Op *op);

  /**
   *  @brief Implementation of the redo operations
   */
  virtual void redo (db::Op *op);

  using QFrame::setGeometry;

signals:
  void order_changed ();
  void tab_changed ();
  void current_layer_changed (const lay::LayerPropertiesConstIterator &iter);

public slots:
  void cm_new_tab ();
  void cm_remove_tab ();
  void cm_rename_tab ();
  void cm_select_all ();
  void cm_invert_selection ();
  void cm_make_valid ();
  void cm_make_invalid ();
  void cm_hide ();
  void cm_hide_all ();
  void cm_show ();
  void cm_show_all ();
  void cm_toggle_visibility ();
  void cm_show_only ();
  void cm_rename ();
  void cm_delete ();
  void cm_insert ();
  void cm_group ();
  void cm_ungroup ();
  void cm_source ();
  void cm_sort_by_name ();
  void cm_sort_by_ild ();
  void cm_sort_by_idl ();
  void cm_sort_by_ldi ();
  void cm_sort_by_dli ();
  void cm_regroup_by_index ();
  void cm_regroup_by_datatype ();
  void cm_regroup_by_layer ();
  void cm_regroup_flatten ();
  void cm_expand_all ();
  void cm_add_missing ();
  void cm_remove_unused ();
  void tab_selected (int index);
  void double_clicked (const QModelIndex &index, Qt::KeyboardModifiers modifiers);
  void context_menu (const QPoint &pt);  
  void tab_context_menu (const QPoint &pt);  
  void group_collapsed (const QModelIndex &index);
  void group_expanded (const QModelIndex &index);
  void current_index_changed (const QModelIndex &index);
  void selection_changed (const QItemSelection &, const QItemSelection &);
  void up_clicked ();
  void upup_clicked ();
  void down_clicked ();
  void downdown_clicked ();
  void search_triggered (const QString &t);
  void search_edited ();
  void search_editing_finished ();
  void search_next ();
  void search_prev ();

private slots:
  void update_hidden_flags ();

private:
  QTabBar *mp_tab_bar;
  LCPTreeWidget *mp_layer_list;
  std::unique_ptr<QStyle> mp_ll_style;
  LayerTreeModel *mp_model;
  lay::LayoutViewBase *mp_view;
  bool m_needs_update;
  bool m_expanded_state_needs_update;
  bool m_tabs_need_update;
  bool m_hidden_flags_need_update;
  bool m_in_update;
  std::vector<size_t> m_new_sel;
  int m_phase;
  int m_oversampling;
  bool m_hrm;
  tl::DeferredMethod<LayerControlPanel> m_do_update_content_dm;
  tl::DeferredMethod<LayerControlPanel> m_do_update_visibility_dm;
  bool m_no_stipples;
  bool m_layer_visibility_follows_selection;
  QLabel *m_no_stipples_label;
  lay::DecoratedLineEdit *mp_search_edit_box;
  QAction *mp_case_sensitive;
  QAction *mp_use_regular_expressions;
  QAction *mp_filter;
  QFrame *mp_search_frame;
  QCheckBox *mp_search_close_cb;

  void clear_selection ();

  void restore_expanded ();

  void update_required (int f);
  void signal_ll_changed (int index);
  void signal_li_changed (int index);
  void signal_cv_changed ();
  void signal_cv_changed_with_int (int index);
  void signal_vp_changed ();

  void do_update_content ();
  void do_update_visibility ();
  void do_update_hidden_flags ();
  void do_delete ();
  void do_copy ();
  void recover ();
  void do_move (int mode);
};

} // namespace

#endif

#endif  //  defined(HAVE_QT)
