
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2020 Matthias Koefferlein

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


#include "layNetlistBrowserPage.h"
#include "layNetlistBrowserModel.h"
#include "layNetlistBrowserTreeModel.h"
#include "layItemDelegates.h"
#include "layCellView.h"
#include "layLayoutView.h"
#include "layMarker.h"
#include "layNetInfoDialog.h"
#include "layNetExportDialog.h"
#include "tlProgress.h"
#include "tlExceptions.h"
#include "tlRecipe.h"
#include "dbLayoutToNetlist.h"
#include "dbNetlistDeviceClasses.h"
#include "dbCellMapping.h"
#include "dbLayerMapping.h"
#include "dbCell.h"

#include <QUrl>
#include <QPainter>
#include <QColorDialog>
#include <QRegExp>
#include <QKeyEvent>
#if QT_VERSION >= 0x050000
#  include <QUrlQuery>
#endif

namespace lay
{

extern const std::string cfg_l2ndb_show_all;

// ----------------------------------------------------------------------------------
//  NetlistBrowserPage implementation

template <class Obj>
inline const db::Circuit *deref_circuit (const Obj *obj)
{
  return obj->circuit ();
}

template <>
inline const db::Circuit *deref_circuit (const db::Circuit *obj)
{
  return obj;
}

template <class Obj>
static db::ICplxTrans
trans_for (const Obj *objs, const db::Layout &ly, const db::Cell &cell, db::ContextCache &cc, const db::DCplxTrans &initial = db::DCplxTrans ())
{
  db::DCplxTrans t = initial;

  const db::Circuit *circuit = deref_circuit (objs);
  while (circuit) {
    if (circuit->cell_index () == cell.cell_index ()) {
      circuit = 0;
      break;
    } else if (circuit->begin_refs () != circuit->end_refs ()) {
      const db::SubCircuit &ref = *circuit->begin_refs ();
      t = ref.trans () * t;
      circuit = ref.circuit ();
    } else {
      break;
    }
  }

  db::CplxTrans dbu_trans (ly.dbu ());
  db::ICplxTrans it = dbu_trans.inverted () * t * dbu_trans;

  //  The circuit may not be instantiated and still not be the top cell.
  //  This happens if the subcell does not have connections. In this case
  //  we look up one instantiation path

  if (circuit && ly.is_valid_cell_index (circuit->cell_index ())) {
    std::pair<bool, db::ICplxTrans> tc = cc.find_layout_context (circuit->cell_index (), cell.cell_index ());
    if (tc.first) {
      it = tc.second * it;
    }
  }

  return it;
}

NetlistBrowserPage::NetlistBrowserPage (QWidget * /*parent*/)
  : m_show_all (true),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (1000),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    m_marker_intensity (0),
    m_use_original_colors (false),
    mp_view (0),
    m_cv_index (0),
    mp_plugin_root (0),
    m_history_ptr (0),
    m_signals_enabled (true),
    m_enable_updates (true),
    m_update_needed (true),
    mp_info_dialog (0),
    dm_update_highlights (this, &NetlistBrowserPage::update_highlights),
    dm_rerun_macro (this, &NetlistBrowserPage::rerun_macro),
    m_cell_context_cache (0)
{
  Ui::NetlistBrowserPage::setupUi (this);

  m_show_all_action = new QAction (QObject::tr ("Show All"), this);
  m_show_all_action->setCheckable (true);
  m_show_all_action->setChecked (m_show_all);

  QAction *color_action = new QAction (QObject::tr ("Colorize Nets"), directory_tree);
  QMenu *menu = new QMenu (directory_tree);
  lay::ColorButton::build_color_menu (menu, this, SLOT (browse_color_for_net ()), SLOT (select_color_for_net ()));
  color_action->setMenu (menu);

  QAction *sep;
  directory_tree->addAction (m_show_all_action);
  directory_tree->addAction (actionCollapseAll);
  directory_tree->addAction (actionExpandAll);
  sep = new QAction (directory_tree);
  sep->setSeparator (true);
  directory_tree->addAction (sep);
  directory_tree->addAction (actionUnselectAll);
  sep = new QAction (directory_tree);
  sep->setSeparator (true);
  directory_tree->addAction (sep);
  directory_tree->addAction (color_action);
  sep = new QAction (directory_tree);
  sep->setSeparator (true);
  directory_tree->addAction (sep);
  directory_tree->addAction (actionExportSelected);
  directory_tree->addAction (actionExportAll);

  directory_tree->header ()->setDefaultSectionSize (150);

  lay::HTMLItemDelegate *delegate;

  for (int i = 0; i < 4; ++i) {
    delegate = new lay::HTMLItemDelegate (this);
    delegate->set_text_margin (2);
    delegate->set_anchors_clickable (true);
    connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
    directory_tree->setItemDelegateForColumn (i, delegate);
  }

  for (int i = 0; i < 2; ++i) {
    delegate = new lay::HTMLItemDelegate (this);
    delegate->set_text_margin (2);
    delegate->set_anchors_clickable (true);
    connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
    hierarchy_tree->setItemDelegateForColumn (i, delegate);
  }

  QMenu *find_edit_menu = new QMenu (find_text);
  find_edit_menu->addAction (actionUseRegularExpressions);
  find_edit_menu->addAction (actionCaseSensitive);

  find_text->set_clear_button_enabled (true);
  find_text->set_options_button_enabled (true);
  find_text->set_options_menu (find_edit_menu);
#if QT_VERSION >= 0x40700
  find_text->setPlaceholderText (tr ("Find text ..."));
#endif

  connect (m_show_all_action, SIGNAL (triggered ()), this, SLOT (show_all_clicked ()));
  connect (info_button, SIGNAL (pressed ()), this, SLOT (info_button_pressed ()));
  connect (rerun_button, SIGNAL (pressed ()), this, SLOT (rerun_button_pressed ()));
  connect (find_button, SIGNAL (pressed ()), this, SLOT (find_button_pressed ()));
  connect (forward, SIGNAL (clicked ()), this, SLOT (navigate_forward ()));
  connect (backward, SIGNAL (clicked ()), this, SLOT (navigate_back ()));

  connect (show_netlist, SIGNAL (clicked ()), this, SLOT (mode_changed ()));
  connect (show_xref, SIGNAL (clicked ()), this, SLOT (mode_changed ()));

  connect (actionExportAll, SIGNAL (triggered ()), this, SLOT (export_all ()));
  connect (actionExportSelected, SIGNAL (triggered ()), this, SLOT (export_selected ()));

  forward->setEnabled (false);
  backward->setEnabled (false);

  directory_tree->installEventFilter (this);
}

NetlistBrowserPage::~NetlistBrowserPage ()
{
  clear_markers ();
}

bool
NetlistBrowserPage::is_netlist_mode ()
{
  return show_netlist->isChecked ();
}

void
NetlistBrowserPage::set_dispatcher (lay::Dispatcher *pr)
{
  mp_plugin_root = pr;
}

void
NetlistBrowserPage::set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern, int marker_intensity, bool use_original_colors, const lay::ColorPalette *auto_colors)
{
  m_colorizer.configure (color, auto_colors);
  m_marker_line_width = line_width;
  m_marker_vertex_size = vertex_size;
  m_marker_halo = halo;
  m_marker_dither_pattern = dither_pattern;
  m_marker_intensity = marker_intensity;
  m_use_original_colors = use_original_colors;
  update_highlights ();
}

void
NetlistBrowserPage::set_view (lay::LayoutView *view, int cv_index)
{
  if (mp_view) {
    mp_view->layer_list_changed_event.remove (this, &NetlistBrowserPage::layer_list_changed);
  }

  if (cv_index < 0) {
    mp_view = 0;
    m_cv_index = 0;
  } else {
    mp_view = view;
    m_cv_index = (unsigned int) cv_index;
  }

  if (mp_view) {
    mp_view->layer_list_changed_event.add (this, &NetlistBrowserPage::layer_list_changed);
  }

  update_highlights ();
}

void
NetlistBrowserPage::set_window (lay::NetlistBrowserConfig::net_window_type window, double window_dim)
{
  if (window != m_window || window_dim != m_window_dim) {
    m_window = window;
    m_window_dim = window_dim;
  }
}

void
NetlistBrowserPage::set_max_shape_count (size_t max_shape_count)
{
  if (m_max_shape_count != max_shape_count) {
    m_max_shape_count = max_shape_count;
    update_highlights ();
  }
}

bool
NetlistBrowserPage::eventFilter (QObject *watched, QEvent *event)
{
  if (watched != directory_tree) {
    return false;
  }

  QKeyEvent *ke = dynamic_cast<QKeyEvent *> (event);
  if (! ke || event->type () != QEvent::KeyPress) {
    return false;
  }

  if (ke->key () == Qt::Key_Escape) {
    directory_tree->clearSelection ();
    return true;
  } else {
    return false;
  }
}

void
NetlistBrowserPage::layer_list_changed (int)
{
  dm_update_highlights ();
}

void
NetlistBrowserPage::anchor_clicked (const QString &a)
{
  QUrl url (a);

  QString ids;
#if QT_VERSION >= 0x050000
  ids = QUrlQuery (url.query ()).queryItemValue (QString::fromUtf8 ("id"));
#else
  ids = url.queryItemValue (QString::fromUtf8 ("id"));
#endif

  if (ids.isEmpty ()) {
    return;
  }

  void *id = reinterpret_cast<void *> (ids.toULongLong ());
  navigate_to (id, true);
}

void
NetlistBrowserPage::current_tree_index_changed (const QModelIndex &index)
{
  if (index.isValid () && m_signals_enabled) {

    NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
    NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (! tree_model || ! netlist_model) {
      return;
    }

    std::pair<const db::Circuit *, const db::Circuit *> circuits = tree_model->circuits_from_index (index);
    QModelIndex circuit_index = netlist_model->index_from_circuit (circuits);

    m_signals_enabled = false;
    directory_tree->setCurrentIndex (circuit_index);
    m_signals_enabled = true;

  }
}

void
NetlistBrowserPage::current_index_changed (const QModelIndex &index)
{
  if (index.isValid () && m_signals_enabled) {

    NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
    NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (! tree_model || ! netlist_model) {
      return;
    }

    void *id = index.internalPointer ();
    add_to_history (id, true);

    std::pair<const db::Circuit *, const db::Circuit *> circuits = netlist_model->circuit_from_index (index);
    QModelIndex circuit_index = tree_model->index_from_circuits (circuits);

    m_signals_enabled = false;
    hierarchy_tree->setCurrentIndex (circuit_index);
    m_signals_enabled = true;

  }
}

void
NetlistBrowserPage::select_net (const db::Net *net)
{
  if (! net || ! net->circuit ()) {
    directory_tree->clearSelection ();
  } else {
    NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    tl_assert (model != 0);
    directory_tree->setCurrentIndex (model->index_from_net (net));
  }
}

std::vector<const db::Net *>
NetlistBrowserPage::selected_nets ()
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  std::vector<const db::Net *> nets;

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
      const db::Net *net = model->net_from_index (*i).first;
      if (net) {
        nets.push_back (net);
      }
    }
  }

  return nets;
}

std::vector<const db::Circuit *>
NetlistBrowserPage::selected_circuits ()
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  std::vector<const db::Circuit *> circuits;

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0 && model->is_circuit_index (*i)) {
      const db::Circuit *circuit = model->circuit_from_index (*i).first;
      if (circuit) {
        circuits.push_back (circuit);
      }
    }
  }

  return circuits;
}

std::vector<const db::SubCircuit *>
NetlistBrowserPage::selected_subcircuits ()
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  std::vector<const db::SubCircuit *> subcircuits;

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
      const db::SubCircuit *subcircuit = model->subcircuit_from_index (*i).first;
      if (subcircuit) {
        subcircuits.push_back (subcircuit);
      }
    }
  }

  return subcircuits;
}

std::vector<const db::Device *>
NetlistBrowserPage::selected_devices ()
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  std::vector<const db::Device *> devices;

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
      const db::Device *device = model->device_from_index (*i).first;
      if (device) {
        devices.push_back (device);
      }
    }
  }

  return devices;
}

void
NetlistBrowserPage::selection_changed ()
{
  std::vector<const db::Net *> nets = selected_nets ();
  if (mp_info_dialog) {
    mp_info_dialog->set_nets (mp_database.get (), nets);
  }

  std::vector<const db::Device *> devices = selected_devices ();

  std::vector<const db::SubCircuit *> subcircuits = selected_subcircuits ();

  std::vector<const db::Circuit *> circuits = selected_circuits ();

  highlight (nets, devices, subcircuits, circuits);
}

void
NetlistBrowserPage::set_color_for_selected_nets (const QColor &color)
{
  std::vector<const db::Net *> nets = selected_nets ();

  m_colorizer.begin_changes ();
  for (std::vector<const db::Net *>::const_iterator n = nets.begin (); n != nets.end (); ++n) {
    if (color.isValid ()) {
      m_colorizer.set_color_of_net (*n, color);
    } else {
      m_colorizer.reset_color_of_net (*n);
    }
  }
  m_colorizer.end_changes ();

  update_highlights ();
}

void
NetlistBrowserPage::browse_color_for_net ()
{
  QColor c = QColorDialog::getColor (QColor (), this);
  if (c.isValid ()) {
    set_color_for_selected_nets (c);
  }
}

void
NetlistBrowserPage::select_color_for_net ()
{
  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {
    set_color_for_selected_nets (action->data ().value<QColor> ());
  }
}

void
NetlistBrowserPage::navigate_to (void *id, bool fwd)
{
  NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
  NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  if (! tree_model || ! netlist_model) {
    return;
  }

  QModelIndex index = netlist_model->index_from_id (id, 0);
  if (! index.isValid ()) {
    return;
  }

  m_signals_enabled = false;
  try {

    directory_tree->setCurrentIndex (index);

    std::pair<const db::Circuit *, const db::Circuit *> circuits = netlist_model->circuit_from_index (index);
    QModelIndex circuit_index = tree_model->index_from_circuits (circuits);
    hierarchy_tree->setCurrentIndex (circuit_index);

  } catch (...) {
  }
  m_signals_enabled = true;

  add_to_history (id, fwd);

  selection_changed ();
}

void
NetlistBrowserPage::add_to_history (void *id, bool fwd)
{
  if (! fwd) {
    if (m_history_ptr > 1) {
      --m_history_ptr;
      m_history [m_history_ptr - 1] = id;
    }
  } else if (m_history_ptr >= m_history.size ()) {
    m_history.push_back (id);
    m_history_ptr = m_history.size ();
  } else {
    if (m_history [m_history_ptr] != id) {
      m_history.erase (m_history.begin () + m_history_ptr + 1, m_history.end ());
    }
    m_history [m_history_ptr] = id;
    ++m_history_ptr;
  }

  backward->setEnabled (m_history_ptr > 1);
  forward->setEnabled (m_history_ptr < m_history.size ());
}

void
NetlistBrowserPage::navigate_back ()
{
  if (m_history_ptr > 1) {
    navigate_to (m_history [m_history_ptr - 2], false);
  }
}

void
NetlistBrowserPage::navigate_forward ()
{
  if (m_history_ptr < m_history.size ()) {
    navigate_to (m_history [m_history_ptr]);
  }
}

void
NetlistBrowserPage::rerun_button_pressed ()
{
  //  NOTE: we use deferred execution, because otherwise the button won't get repainted properly
  dm_rerun_macro ();
}

void
NetlistBrowserPage::rerun_macro ()
{
BEGIN_PROTECTED

  if (! mp_database->generator ().empty ()) {

    std::map<std::string, tl::Variant> add_pars;

    for (unsigned int i = 0; i < mp_view->num_l2ndbs (); ++i) {
      if (mp_view->get_l2ndb (i) == mp_database.get ()) {
        add_pars["l2ndb_index"] = tl::Variant (int (i));
        break;
      }
    }

    tl::Recipe::make (mp_database->generator (), add_pars);

  }

END_PROTECTED
}

void
NetlistBrowserPage::info_button_pressed ()
{
  if (! mp_info_dialog) {
    mp_info_dialog = new lay::NetInfoDialog (this);
  }

  mp_info_dialog->set_nets (mp_database.get (), selected_nets ());
  mp_info_dialog->show ();
}

void
NetlistBrowserPage::mode_changed ()
{
  setup_trees ();
}

static QModelIndex find_next (QTreeView *view, QAbstractItemModel *model, const QRegExp &to_find, const QModelIndex &from)
{
  QModelIndex index = from;

  if (! index.isValid () && model->hasChildren (index)) {
    index = model->index (0, 0, index);
  }

  if (! index.isValid ()) {
    return index;
  }

  QModelIndex current = index;

  std::vector<QModelIndex> parent_stack;
  std::vector<std::pair<int, int> > rows_stack;

  while (index.isValid ()) {

    parent_stack.push_back (model->parent (index));
    rows_stack.push_back (std::make_pair (index.row (), model->rowCount (parent_stack.back ())));

    index = parent_stack.back ();

  }

  std::reverse (parent_stack.begin (), parent_stack.end ());
  std::reverse (rows_stack.begin (), rows_stack.end ());

  tl::AbsoluteProgress progress (tl::to_string (tr ("Searching ...")));

  do {

    ++progress;

    bool has_next = false;

    if (model->hasChildren (current) && rows_stack.size () < 2) {

      int row_count = model->rowCount (current);
      if (row_count > 0) {

        parent_stack.push_back (current);
        rows_stack.push_back (std::make_pair (0, row_count));

        current = model->index (0, 0, current);
        has_next = true;

      }

    }

    while (! has_next && ! rows_stack.empty ()) {

      ++rows_stack.back ().first;

      if (rows_stack.back ().first >= rows_stack.back ().second) {

        //  up
        current = parent_stack.back ();
        rows_stack.pop_back ();
        parent_stack.pop_back ();

      } else {

        current = model->index (rows_stack.back ().first, 0, parent_stack.back ());
        has_next = true;

      }

    }

    if (has_next) {

      QString text = model->data (current, Qt::UserRole).toString ();
      if (text.indexOf (to_find) >= 0 && ! view->isRowHidden (rows_stack.back ().first, parent_stack.back ())) {
        return current;
      }

    }

  } while (current.internalPointer () != from.internalPointer () || current.row () != from.row ());

  return QModelIndex ();
}

void
NetlistBrowserPage::find_button_pressed ()
{
  QRegExp re (find_text->text (),
              actionCaseSensitive->isChecked () ? Qt::CaseSensitive : Qt::CaseInsensitive,
              actionUseRegularExpressions->isChecked () ? QRegExp::RegExp : QRegExp::FixedString);

  QModelIndex next = find_next (directory_tree, directory_tree->model (), re, directory_tree->currentIndex ());
  if (next.isValid ()) {
    navigate_to (next.internalPointer ());
  }
}

void
NetlistBrowserPage::show_all_clicked ()
{
  if (mp_plugin_root) {
    mp_plugin_root->config_set (cfg_l2ndb_show_all, tl::to_string (m_show_all_action->isChecked ()));
  }
}

void
NetlistBrowserPage::show_all (bool f)
{
  if (f != m_show_all) {

    m_show_all = f;
    m_show_all_action->setChecked (f);

    NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (model) {
      model->set_item_visibility (directory_tree, m_show_all, false /*show warnings only with 'show all'*/);
    }

  }
}

void
NetlistBrowserPage::set_db (db::LayoutToNetlist *l2ndb)
{
  if (l2ndb == mp_database.get ()) {
    //  not change
    return;
  }

  if (mp_info_dialog) {
    delete mp_info_dialog;
    mp_info_dialog = 0;
  }

  db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);
  mp_database.reset (l2ndb);

  rerun_button->setEnabled (mp_database.get () && ! mp_database->generator ().empty ());
  if (rerun_button->isEnabled ()) {
    QString shortcut;
    if (! rerun_button->shortcut ().isEmpty ()) {
      shortcut = QString::fromUtf8 (" (%1)").arg (rerun_button->shortcut ().toString ());
    }
    rerun_button->setToolTip (tl::to_qstring (tl::to_string (tr ("Run ")) + mp_database->generator ()) + shortcut);
  } else {
    rerun_button->setToolTip (QString ());
  }

  show_netlist->setVisible (lvsdb != 0);
  show_xref->setVisible (lvsdb != 0);

  bool se = m_signals_enabled;
  m_signals_enabled = false;
  show_netlist->setChecked (lvsdb == 0);
  show_xref->setChecked (lvsdb != 0);
  m_signals_enabled = se;

  clear_markers ();
  highlight (std::vector<const db::Net *> (), std::vector<const db::Device *> (), std::vector<const db::SubCircuit *> (), std::vector<const db::Circuit *> ());

  m_cell_context_cache = db::ContextCache (mp_database.get () ? mp_database->internal_layout () : 0);

  setup_trees ();
}

void
NetlistBrowserPage::setup_trees ()
{
  if (! mp_database.get ()) {
    delete directory_tree->model ();
    directory_tree->setModel (0);
    delete hierarchy_tree->model ();
    hierarchy_tree->setModel (0);
    return;
  }

  db::LayoutToNetlist *l2ndb = mp_database.get ();
  db::LayoutVsSchematic *lvsdb = show_netlist->isChecked () ? 0 : dynamic_cast<db::LayoutVsSchematic *> (l2ndb);

  {
    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserModel *new_model = 0;
    if (lvsdb) {
      new_model = new NetlistBrowserModel (directory_tree, lvsdb, &m_colorizer);
    } else {
      new_model = new NetlistBrowserModel (directory_tree, l2ndb, &m_colorizer);
    }

    int columns = directory_tree->model () ? directory_tree->model ()->columnCount (QModelIndex ()) : 0;
    int new_columns = new_model->columnCount (QModelIndex ());

    delete directory_tree->model ();
    directory_tree->setModel (new_model);
    connect (directory_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_index_changed (const QModelIndex &)));
    connect (directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (selection_changed ()));

    directory_tree->header ()->show ();
    directory_tree->header ()->setStretchLastSection (true);
    directory_tree->header ()->setMinimumSectionSize (25);

    if (columns < new_columns) {
      //  makes sure new columns are properly size-adjusted
      for (int i = std::max (0, columns - 1); i < new_columns; ++i) {
        directory_tree->header ()->resizeSection (i, i == 1 ? directory_tree->header ()->minimumSectionSize () : directory_tree->header ()->defaultSectionSize ());
      }
    }

    //  hide the status column if not needed
    directory_tree->header ()->setSectionHidden (1, new_model->status_column () < 0);

    //  establish visibility according to "show all"
    new_model->set_item_visibility (directory_tree, m_show_all, false /*show warnings only with 'show all'*/);
  }

  {
    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserTreeModel *new_hierarchy_model = 0;
    if (lvsdb) {
      new_hierarchy_model = new NetlistBrowserTreeModel (hierarchy_tree, lvsdb);
    } else {
      new_hierarchy_model = new NetlistBrowserTreeModel (hierarchy_tree, l2ndb);
    }

    int columns = hierarchy_tree->model () ? hierarchy_tree->model ()->columnCount (QModelIndex ()) : 0;
    int new_columns = new_hierarchy_model->columnCount (QModelIndex ());

    delete hierarchy_tree->model ();
    hierarchy_tree->setModel (new_hierarchy_model);
    connect (hierarchy_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (current_tree_index_changed (const QModelIndex &)));

    hierarchy_tree->header ()->show ();
    hierarchy_tree->header ()->setStretchLastSection (true);
    hierarchy_tree->header ()->setMinimumSectionSize (25);

    if (columns < new_columns) {
      //  makes sure new columns are properly size-adjusted
      for (int i = std::max (0, columns - 1); i < new_columns; ++i) {
        hierarchy_tree->header ()->resizeSection (i, i == 1 ? hierarchy_tree->header ()->minimumSectionSize () : hierarchy_tree->header ()->defaultSectionSize ());
      }
    }

    //  hide the status column if not needed
    hierarchy_tree->header ()->setSectionHidden (1, new_hierarchy_model->status_column () < 0);
  }

  find_text->setText (QString ());
}

void
NetlistBrowserPage::highlight (const std::vector<const db::Net *> &nets, const std::vector<const db::Device *> &devices, const std::vector<const db::SubCircuit *> &subcircuits, const std::vector<const db::Circuit *> &circuits)
{
  if (nets != m_current_nets || devices != m_current_devices || subcircuits != m_current_subcircuits || circuits != m_current_circuits) {

    m_current_nets = nets;
    m_current_devices = devices;
    m_current_subcircuits = subcircuits;
    m_current_circuits = circuits;

    clear_markers ();

    if (! nets.empty () || ! devices.empty () || ! subcircuits.empty () || ! circuits.empty ()) {
      adjust_view ();
      update_highlights ();
    }

  }
}

void
NetlistBrowserPage::enable_updates (bool f)
{
  if (f != m_enable_updates) {

    m_enable_updates = f;

    if (f && m_update_needed) {
      update_highlights ();
    }

    m_update_needed = false;

  }
}

static db::Box
bbox_for_device_abstract (const db::Layout *layout, const db::DeviceAbstract *device_abstract, const db::DCplxTrans &trans)
{
  if (! device_abstract || ! layout->is_valid_cell_index (device_abstract->cell_index ())) {
    return db::Box ();
  }

  return layout->cell (device_abstract->cell_index ()).bbox ().transformed (db::CplxTrans (layout->dbu ()).inverted () * trans * db::CplxTrans (layout->dbu ()));}

static db::Box
bbox_for_circuit (const db::Layout *layout, const db::Circuit *circuit)
{

  if (! circuit || ! layout->is_valid_cell_index (circuit->cell_index ())) {
    return db::Box ();
  }

  if (circuit->boundary ().vertices () > 0) {
    return db::CplxTrans (layout->dbu ()).inverted () * circuit->boundary ().box ();
  }

  return layout->cell (circuit->cell_index ()).bbox ();
}

static db::Box
bbox_for_subcircuit (const db::Layout *layout, const db::SubCircuit *subcircuit)
{
  return bbox_for_circuit (layout, subcircuit->circuit_ref ());
}

void
NetlistBrowserPage::adjust_view ()
{
  if (! mp_database.get () || ! mp_view) {
    return;
  }

  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (m_window != lay::NetlistBrowserConfig::FitNet && m_window != lay::NetlistBrowserConfig::Center && m_window != lay::NetlistBrowserConfig::CenterSize) {
    return;
  }

  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = mp_database->internal_top_cell ();
  if (! layout || ! cell) {
    return;
  }


  db::Box bbox;

  for (std::vector<const db::Net *>::const_iterator net = m_current_nets.begin (); net != m_current_nets.end (); ++net) {

    db::ICplxTrans net_trans = trans_for (*net, *layout, *cell, m_cell_context_cache);

    db::cell_index_type cell_index = (*net)->circuit ()->cell_index ();
    size_t cluster_id = (*net)->cluster_id ();

    const db::Connectivity &conn = mp_database->connectivity ();
    for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

      db::Box layer_bbox;
      db::recursive_cluster_shape_iterator<db::PolygonRef> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
      while (! shapes.at_end ()) {
        layer_bbox += shapes.trans () * shapes->box ();
        ++shapes;
      }

      bbox += net_trans * layer_bbox;

    }

  }

  for (std::vector<const db::Device *>::const_iterator device = m_current_devices.begin (); device != m_current_devices.end (); ++device) {

    db::ICplxTrans trans = trans_for (*device, *layout, *cell, m_cell_context_cache, (*device)->trans ());

    bbox += trans * bbox_for_device_abstract (layout, (*device)->device_abstract (), db::DCplxTrans ());

    const std::vector<db::DeviceAbstractRef> &oda = (*device)->other_abstracts ();
    for (std::vector<db::DeviceAbstractRef>::const_iterator a = oda.begin (); a != oda.end (); ++a) {
      bbox += trans * bbox_for_device_abstract (layout, a->device_abstract, a->trans);
    }

  }

  for (std::vector<const db::SubCircuit *>::const_iterator subcircuit = m_current_subcircuits.begin (); subcircuit != m_current_subcircuits.end (); ++subcircuit) {
    bbox += trans_for (*subcircuit, *layout, *cell, m_cell_context_cache, (*subcircuit)->trans ()) * bbox_for_subcircuit (layout, *subcircuit);
  }

  for (std::vector<const db::Circuit *>::const_iterator circuit = m_current_circuits.begin (); circuit != m_current_circuits.end (); ++circuit) {
    bbox += trans_for (*circuit, *layout, *cell, m_cell_context_cache, db::DCplxTrans ()) * bbox_for_circuit (layout, *circuit);
  }

  if (! bbox.empty ()) {

    std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

    db::DBox tv_bbox;
    db::DBox dbu_bbox = db::CplxTrans (layout->dbu ()) * bbox;
    for (std::vector<db::DCplxTrans>::const_iterator t = tv.begin (); t != tv.end (); ++t) {
      tv_bbox += *t * dbu_bbox;
    }

    if (m_window == lay::NetlistBrowserConfig::FitNet) {

      mp_view->zoom_box (tv_bbox.enlarged (db::DVector (m_window_dim, m_window_dim)));

    } else if (m_window == lay::NetlistBrowserConfig::Center) {

      mp_view->pan_center (tv_bbox.p1 () + (tv_bbox.p2 () - tv_bbox.p1 ()) * 0.5);

    } else if (m_window == lay::NetlistBrowserConfig::CenterSize) {

      double w = std::max (tv_bbox.width (), m_window_dim);
      double h = std::max (tv_bbox.height (), m_window_dim);
      db::DPoint center (tv_bbox.p1() + (tv_bbox.p2 () - tv_bbox.p1 ()) * 0.5);
      db::DVector d (w * 0.5, h * 0.5);
      mp_view->zoom_box (db::DBox (center - d, center + d));

    }

  }
}

QColor
NetlistBrowserPage::make_valid_color (const QColor &color)
{
  if (! color.isValid () && mp_view) {
    return mp_view->background_color ().green () < 128 ? QColor (Qt::white) : QColor (Qt::black);
  } else {
    return color;
  }
}

bool
NetlistBrowserPage::produce_highlights_for_device (const db::Device *device, size_t &n_markers, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = mp_database->internal_top_cell ();
  db::ICplxTrans device_trans = trans_for (device, *layout, *cell, m_cell_context_cache, device->trans ());

  QColor color = make_valid_color (m_colorizer.marker_color ());

  db::Box device_bbox = bbox_for_device_abstract (layout, device->device_abstract (), db::DCplxTrans ());
  if (! device_bbox.empty ()) {

    if (n_markers == m_max_shape_count) {
      return true;
    }

    ++n_markers;

    mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
    mp_markers.back ()->set (device_bbox, device_trans, tv);
    mp_markers.back ()->set_color (color);
    mp_markers.back ()->set_frame_color (color);
    configure_marker (mp_markers.back (), false);

  }

  const std::vector<db::DeviceAbstractRef> &oda = device->other_abstracts ();
  for (std::vector<db::DeviceAbstractRef>::const_iterator a = oda.begin (); a != oda.end (); ++a) {

    db::Box da_box = bbox_for_device_abstract (layout, a->device_abstract, a->trans);
    if (! da_box.empty ()) {

      if (n_markers == m_max_shape_count) {
        return true;
      }

      ++n_markers;

      mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
      mp_markers.back ()->set (da_box, device_trans, tv);
      mp_markers.back ()->set_color (color);
      mp_markers.back ()->set_frame_color (color);
      configure_marker (mp_markers.back (), false);

    }

  }

  return false;
}

bool
NetlistBrowserPage::produce_highlights_for_subcircuit (const db::SubCircuit *subcircuit, size_t &n_markers, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = mp_database->internal_top_cell ();
  db::ICplxTrans subcircuit_trans = trans_for (subcircuit, *layout, *cell, m_cell_context_cache, subcircuit->trans ());

  QColor color = make_valid_color (m_colorizer.marker_color ());
  db::Box circuit_bbox = bbox_for_subcircuit (layout, subcircuit);
  if (circuit_bbox.empty ()) {
    return false;
  }

  if (n_markers == m_max_shape_count) {
    return true;
  }

  ++n_markers;

  mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
  mp_markers.back ()->set (circuit_bbox, subcircuit_trans, tv);
  mp_markers.back ()->set_color (color);
  mp_markers.back ()->set_frame_color (color);
  configure_marker (mp_markers.back (), false);

  return false;
}

bool
NetlistBrowserPage::produce_highlights_for_circuit (const db::Circuit *circuit, size_t &n_markers, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = mp_database->internal_top_cell ();
  db::ICplxTrans circuit_trans = trans_for (circuit, *layout, *cell, m_cell_context_cache, db::DCplxTrans ());

  QColor color = make_valid_color (m_colorizer.marker_color ());
  db::Box circuit_bbox = bbox_for_circuit (layout, circuit);
  if (circuit_bbox.empty ()) {
    return false;
  }

  if (n_markers == m_max_shape_count) {
    return true;
  }

  ++n_markers;

  mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
  mp_markers.back ()->set (circuit_bbox, circuit_trans, tv);
  mp_markers.back ()->set_color (color);
  mp_markers.back ()->set_frame_color (color);
  configure_marker (mp_markers.back (), false);

  return false;
}

bool
NetlistBrowserPage::produce_highlights_for_net (const db::Net *net, size_t &n_markers, const std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> &display_by_lp, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = mp_database->internal_top_cell ();
  db::ICplxTrans net_trans = trans_for (net, *layout, *cell, m_cell_context_cache);

  db::cell_index_type cell_index = net->circuit ()->cell_index ();
  size_t cluster_id = net->cluster_id ();

  QColor net_color = m_colorizer.color_of_net (net);
  QColor fallback_color = make_valid_color (m_colorizer.marker_color ());

  const db::Connectivity &conn = mp_database->connectivity ();
  for (db::Connectivity::layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

    db::LayerProperties lp = layout->get_properties (*layer);
    std::map<db::LayerProperties, lay::LayerPropertiesConstIterator>::const_iterator display = display_by_lp.find (lp);

    db::recursive_cluster_shape_iterator<db::PolygonRef> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
    while (! shapes.at_end ()) {

      if (n_markers == m_max_shape_count) {
        return true;
      }

      mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
      mp_markers.back ()->set (*shapes, net_trans * shapes.trans (), tv);

      if (net_color.isValid ()) {

        mp_markers.back ()->set_color (net_color);
        mp_markers.back ()->set_frame_color (net_color);

      } else if (! m_use_original_colors || display == display_by_lp.end ()) {

        mp_markers.back ()->set_color (fallback_color);
        mp_markers.back ()->set_frame_color (fallback_color);

      } else if (display != display_by_lp.end ()) {

        mp_markers.back ()->set_line_width (display->second->width (true));
        mp_markers.back ()->set_vertex_size (1);
        mp_markers.back ()->set_dither_pattern (display->second->dither_pattern (true));
        if (mp_view->background_color ().green () < 128) {
          mp_markers.back ()->set_color (display->second->eff_fill_color_brighter (true, (m_marker_intensity * 255) / 100));
          mp_markers.back ()->set_frame_color (display->second->eff_frame_color_brighter (true, (m_marker_intensity * 255) / 100));
        } else {
          mp_markers.back ()->set_color (display->second->eff_fill_color_brighter (true, (-m_marker_intensity * 255) / 100));
          mp_markers.back ()->set_frame_color (display->second->eff_frame_color_brighter (true, (-m_marker_intensity * 255) / 100));
        }

      }

      configure_marker (mp_markers.back (), true);

      ++shapes;
      ++n_markers;

    }

  }

  return false;
}

void
NetlistBrowserPage::configure_marker (Marker *marker, bool with_fill)
{
  if (m_marker_line_width >= 0) {
    marker->set_line_width (m_marker_line_width);
  }

  if (m_marker_vertex_size >= 0) {
    marker->set_vertex_size (m_marker_vertex_size);
  }

  if (m_marker_halo >= 0) {
    marker->set_halo (m_marker_halo);
  }

  if (m_marker_dither_pattern >= 0 && with_fill) {
    marker->set_dither_pattern (m_marker_dither_pattern);
  }
}

void
NetlistBrowserPage::update_highlights ()
{
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  clear_markers ();
  if (! mp_database.get () || ! mp_view) {
    return;
  }

  const db::Layout &original_layout = mp_view->cellview (m_cv_index)->layout ();

  const db::Layout *layout = mp_database->internal_layout ();
  if (! layout) {
    return;
  }

  //  a map of display properties vs. layer properties

  std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> display_by_lp;
  for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {
    if (! lp->has_children () && lp->cellview_index () == int (m_cv_index) && lp->layer_index () >= 0 && (unsigned int) lp->layer_index () < original_layout.layers ()) {
      display_by_lp.insert (std::make_pair (original_layout.get_properties (lp->layer_index ()), lp));
    }
  }

  std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

  //  correct DBU differences between the storage layout and the original layout
  for (std::vector<db::DCplxTrans>::iterator t = tv.begin (); t != tv.end (); ++t) {
    *t = *t * db::DCplxTrans (layout->dbu () / original_layout.dbu ());
  }

  size_t n_markers = 0;
  bool not_all_shapes_are_shown = false;

  for (std::vector<const db::Net *>::const_iterator net = m_current_nets.begin (); net != m_current_nets.end () && ! not_all_shapes_are_shown; ++net) {
    if ((*net)->circuit ()) {
      not_all_shapes_are_shown = produce_highlights_for_net (*net, n_markers, display_by_lp, tv);
    }
  }

  for (std::vector<const db::Device *>::const_iterator device = m_current_devices.begin (); device != m_current_devices.end () && ! not_all_shapes_are_shown; ++device) {
    if ((*device)->circuit ()) {
      not_all_shapes_are_shown = produce_highlights_for_device (*device, n_markers, tv);
    }
  }

  for (std::vector<const db::SubCircuit *>::const_iterator subcircuit = m_current_subcircuits.begin (); subcircuit != m_current_subcircuits.end () && ! not_all_shapes_are_shown; ++subcircuit) {
    if ((*subcircuit)->circuit ()) {
      not_all_shapes_are_shown = produce_highlights_for_subcircuit (*subcircuit, n_markers, tv);
    }
  }

  for (std::vector<const db::Circuit *>::const_iterator circuit = m_current_circuits.begin (); circuit != m_current_circuits.end () && ! not_all_shapes_are_shown; ++circuit) {
    not_all_shapes_are_shown = produce_highlights_for_circuit (*circuit, n_markers, tv);
  }

  if (not_all_shapes_are_shown) {
    info_label->setText (tl::to_qstring ("<html><p style=\"color:red; font-weight: bold\">" +
        tl::to_string (QObject::tr ("Not all shapes are highlighted")) +
        "</p></html>"));
    info_label->show ();
  } else {
    info_label->hide ();
  }

}

void
NetlistBrowserPage::clear_markers ()
{
  for (std::vector <lay::Marker *>::iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }

  mp_markers.clear ();
}

void
NetlistBrowserPage::export_selected ()
{
BEGIN_PROTECTED

  std::vector<const db::Net *> nets = selected_nets ();
  if (nets.empty ()) {
    return;
  }

  export_nets (&nets);

END_PROTECTED
}

void
NetlistBrowserPage::export_all ()
{
BEGIN_PROTECTED
  export_nets (0);
END_PROTECTED
}

void
NetlistBrowserPage::export_nets (const std::vector<const db::Net *> *nets)
{
  if (! mp_view || ! mp_database.get () || ! mp_database->internal_layout ()) {
    return;
  }

  const db::Layout &source_layout = *mp_database->internal_layout ();
  if (source_layout.begin_top_down () == source_layout.end_top_cells ()) {
    //  nothing to export
    return;
  }

  const db::Cell &source_top = source_layout.cell (*source_layout.begin_top_down ());

  std::auto_ptr<lay::NetExportDialog> dialog (new lay::NetExportDialog (this));
  if (dialog->exec_dialog (mp_plugin_root)) {

    //  NOTE: mp_view and database might get reset to 0 in create_layout
    lay::LayoutView *view = mp_view;
    db::LayoutToNetlist *database = mp_database.get ();

    unsigned int cv_index = view->create_layout (true);
    db::Layout &target_layout = view->cellview (cv_index)->layout ();

    db::cell_index_type target_top_index = target_layout.add_cell (source_layout.cell_name (source_top.cell_index ()));

    db::CellMapping cm;
    if (! nets) {
      cm = database->cell_mapping_into (target_layout, target_layout.cell (target_top_index));
    } else {
      cm = database->cell_mapping_into (target_layout, target_layout.cell (target_top_index), *nets);
    }

    std::map<unsigned int, const db::Region *> lm = database->create_layermap (target_layout, dialog->start_layer_number ());

    database->build_nets (nets, cm, target_layout, lm,
                          dialog->net_prefix ().empty () ? 0 : dialog->net_prefix ().c_str (),
                          dialog->net_propname (),
                          dialog->produce_circuit_cells () ? db::LayoutToNetlist::BNH_SubcircuitCells : db::LayoutToNetlist::BNH_Flatten,
                          dialog->produce_circuit_cells () ? dialog->circuit_cell_prefix ().c_str () : 0,
                          dialog->produce_device_cells () ? dialog->device_cell_prefix ().c_str () : 0);

    view->zoom_fit ();
    view->max_hier ();
    view->add_missing_layers ();
    view->select_cell (target_top_index, cv_index);

  }
}

}

