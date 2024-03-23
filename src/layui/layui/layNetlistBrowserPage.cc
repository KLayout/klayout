
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

#include "layNetlistBrowserPage.h"
#include "layNetlistBrowserModel.h"
#include "layNetlistBrowserTreeModel.h"
#include "layNetlistLogModel.h"
#include "layItemDelegates.h"
#include "layCellView.h"
#include "layLayoutViewBase.h"
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
#include "dbLog.h"

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
static std::pair<bool, db::DCplxTrans>
trans_for (const Obj *objs, const db::Layout &ly, const db::Cell &cell, db::ContextCache &cc, const db::DCplxTrans &initial = db::DCplxTrans ())
{
  db::DCplxTrans t = initial;
  bool good = true;

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

  //  The circuit may not be instantiated and still not be the top cell.
  //  This happens if the subcell does not have connections. In this case
  //  we look up one instantiation path

  if (circuit && ly.is_valid_cell_index (circuit->cell_index ())) {
    std::pair<bool, db::ICplxTrans> tc = cc.find_layout_context (circuit->cell_index (), cell.cell_index ());
    if (tc.first) {
      t = dbu_trans * tc.second * dbu_trans.inverted () * t;
    } else {
      good = false;
    }
  }

  return std::make_pair (good, t);
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
    mp_last_db (0),
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

  {
    QAction *collapse_all = new QAction (QObject::tr ("Collapse All"), log_view);
    connect (collapse_all, SIGNAL (triggered ()), log_view, SLOT (collapseAll ()));
    log_view->addAction (collapse_all);

    QAction *expand_all = new QAction (QObject::tr ("Expand All"), log_view);
    connect (expand_all, SIGNAL (triggered ()), log_view, SLOT (expandAll ()));
    log_view->addAction (expand_all);
  }

  QTreeView *dt[] = { nl_directory_tree, sch_directory_tree, xref_directory_tree };

  for (int m = 0; m < int (sizeof (dt) / sizeof (dt[0])); ++m) {

    QTreeView *directory_tree = dt[m];

    QAction *color_action = new QAction (QObject::tr ("Colorize Nets"), directory_tree);
    QMenu *menu = new QMenu (directory_tree);
    lay::ColorButton::build_color_menu (menu, this, SLOT (browse_color_for_net ()), SLOT (select_color_for_net ()));
    color_action->setMenu (menu);

    QAction *sep;
    directory_tree->addAction (m_show_all_action);
    QAction *collapse_all = new QAction (QObject::tr ("Collapse All"), directory_tree);
    connect (collapse_all, SIGNAL (triggered ()), directory_tree, SLOT (collapseAll ()));
    directory_tree->addAction (collapse_all);
    //  TODO: this gives a too big tree - confine to single branches?
    //  QAction *expand_all = new QAction (QObject::tr ("Expand All"), directory_tree);
    //  connect (expand_all, SIGNAL (triggered ()), directory_tree, SLOT (expandAll ()));
    //  directory_tree->addAction (actionExpandAll);
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

    for (int i = 0; i < 4; ++i) {
      lay::HTMLItemDelegate *delegate = new lay::HTMLItemDelegate (this);
      delegate->set_text_margin (2);
      delegate->set_anchors_clickable (true);
      connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
      directory_tree->setItemDelegateForColumn (i, delegate);
    }

    directory_tree->installEventFilter (this);

  }

  QTreeView *ht[] = { nl_hierarchy_tree, sch_hierarchy_tree, xref_hierarchy_tree };

  for (int m = 0; m < int (sizeof (ht) / sizeof (ht[0])); ++m) {

    QTreeView *hierarchy_tree = ht[m];

    for (int i = 0; i < 2; ++i) {
      lay::HTMLItemDelegate *delegate = new lay::HTMLItemDelegate (this);
      delegate->set_text_margin (2);
      delegate->set_anchors_clickable (true);
      connect (delegate, SIGNAL (anchor_clicked (const QString &)), this, SLOT (anchor_clicked (const QString &)));
      hierarchy_tree->setItemDelegateForColumn (i, delegate);
    }

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

  connect (actionExportAll, SIGNAL (triggered ()), this, SLOT (export_all ()));
  connect (actionExportSelected, SIGNAL (triggered ()), this, SLOT (export_selected ()));

  connect (mode_tab, SIGNAL (currentChanged (int)), this, SLOT (mode_tab_changed (int)));

  forward->setEnabled (false);
  backward->setEnabled (false);
}

NetlistBrowserPage::~NetlistBrowserPage ()
{
  clear_markers ();
}

bool
NetlistBrowserPage::is_netlist_mode ()
{
  return mode_tab->currentIndex () == 0;
}

void
NetlistBrowserPage::set_dispatcher (lay::Dispatcher *pr)
{
  mp_plugin_root = pr;
}

void
NetlistBrowserPage::set_highlight_style (tl::Color color, int line_width, int vertex_size, int halo, int dither_pattern, int marker_intensity, bool use_original_colors, const lay::ColorPalette *auto_colors)
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
NetlistBrowserPage::set_view (lay::LayoutViewBase *view, int cv_index)
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
  QTreeView *tree = dynamic_cast<QTreeView *> (watched);
  if (tree != nl_directory_tree && tree != sch_directory_tree && tree != xref_directory_tree) {
    return false;
  }

  QKeyEvent *ke = dynamic_cast<QKeyEvent *> (event);
  if (! ke || event->type () != QEvent::KeyPress) {
    return false;
  }

  if (ke->key () == Qt::Key_Escape) {
    tree->clearSelection ();
    return true;
  } else {
    return false;
  }
}

void
NetlistBrowserPage::mode_tab_changed (int)
{
  clear_highlights ();
  dm_update_highlights ();
}

void
NetlistBrowserPage::layer_list_changed (int)
{
  dm_update_highlights ();
}

QTreeView *
NetlistBrowserPage::current_hierarchy_tree ()
{
  switch (mode_tab->currentIndex ()) {
  case 0:
    return nl_hierarchy_tree;
  case 1:
    return sch_hierarchy_tree;
  case 2:
    return xref_hierarchy_tree;
  default:
    return 0;
  }
}

QTreeView *
NetlistBrowserPage::current_directory_tree ()
{
  switch (mode_tab->currentIndex ()) {
  case 0:
    return nl_directory_tree;
  case 1:
    return sch_directory_tree;
  case 2:
    return xref_directory_tree;
  default:
    return 0;
  }
}

void
NetlistBrowserPage::anchor_clicked (const QString &a)
{
  QTreeView *directory_tree = current_directory_tree ();
  NetlistBrowserModel *netlist_model = 0;
  if (directory_tree) {
    netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  }
  if (netlist_model) {
    navigate_to (netlist_model->index_from_url (a), true);
  }
}

void
NetlistBrowserPage::nl_current_tree_index_changed (const QModelIndex &index)
{
  current_tree_index_changed (nl_hierarchy_tree, nl_directory_tree, index);
}

void
NetlistBrowserPage::sch_current_tree_index_changed (const QModelIndex &index)
{
  current_tree_index_changed (sch_hierarchy_tree, sch_directory_tree, index);
}

void
NetlistBrowserPage::xref_current_tree_index_changed (const QModelIndex &index)
{
  current_tree_index_changed (xref_hierarchy_tree, xref_directory_tree, index);
}

void
NetlistBrowserPage::current_tree_index_changed (QTreeView *hierarchy_tree, QTreeView *directory_tree, const QModelIndex &index)
{
  if (index.isValid () && m_signals_enabled) {

    NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
    NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (! tree_model || ! netlist_model) {
      return;
    }

    //  TODO: this could give a path ...
    std::pair<const db::Circuit *, const db::Circuit *> circuits = tree_model->circuits_from_index (index);
    QModelIndex circuit_index = netlist_model->index_from_circuit (circuits);

    m_signals_enabled = false;
    directory_tree->setCurrentIndex (circuit_index);
    m_signals_enabled = true;

  }
}

void
NetlistBrowserPage::nl_current_index_changed (const QModelIndex &index)
{
  current_index_changed (nl_hierarchy_tree, nl_directory_tree, index);
}

void
NetlistBrowserPage::sch_current_index_changed (const QModelIndex &index)
{
  current_index_changed (sch_hierarchy_tree, sch_directory_tree, index);
}

void
NetlistBrowserPage::xref_current_index_changed (const QModelIndex &index)
{
  current_index_changed (xref_hierarchy_tree, xref_directory_tree, index);
}

void
NetlistBrowserPage::current_index_changed (QTreeView *hierarchy_tree, QTreeView *directory_tree, const QModelIndex &index)
{
  if (index.isValid () && m_signals_enabled) {

    NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
    NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (! tree_model || ! netlist_model) {
      return;
    }

    add_to_history (index, true);

    NetlistObjectsPath path = netlist_model->path_from_index (index);
    QModelIndex circuit_index = tree_model->index_from_netpath (path);

    m_signals_enabled = false;
    hierarchy_tree->setCurrentIndex (circuit_index);
    m_signals_enabled = true;

  }
}

void
NetlistBrowserPage::select_net (const db::Net *net)
{
  if (! net || ! net->circuit ()) {

    nl_directory_tree->clearSelection ();
    sch_directory_tree->clearSelection ();
    xref_directory_tree->clearSelection ();

  } else {

    NetlistBrowserModel *model;

    model = dynamic_cast<NetlistBrowserModel *> (nl_directory_tree->model ());
    tl_assert (model != 0);
    nl_directory_tree->setCurrentIndex (model->index_from_net (net));

    model = dynamic_cast<NetlistBrowserModel *> (sch_directory_tree->model ());
    tl_assert (model != 0);
    sch_directory_tree->setCurrentIndex (model->index_from_net (net));

    model = dynamic_cast<NetlistBrowserModel *> (xref_directory_tree->model ());
    tl_assert (model != 0);
    xref_directory_tree->setCurrentIndex (model->index_from_net (net));

  }
}

void
NetlistBrowserPage::select_path (const lay::NetlistObjectsPath &path)
{
  if (path.is_null ()) {

    nl_directory_tree->clearSelection ();
    sch_directory_tree->clearSelection ();
    xref_directory_tree->clearSelection ();

  } else {

    NetlistBrowserModel *model;
    db::LayoutToNetlist *l2ndb = mp_database.get ();
    db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);

    model = dynamic_cast<NetlistBrowserModel *> (nl_directory_tree->model ());
    if (model) {
      nl_directory_tree->setCurrentIndex (model->index_from_path (path));
    }

    model = dynamic_cast<NetlistBrowserModel *> (sch_directory_tree->model ());
    if (model && lvsdb && lvsdb->cross_ref ()) {
      lay::NetlistObjectsPath sch_path = path;
      //  Note: translation helps generating a schematic-netlist index to
      //  naviate to the schematic netlist in case of probing for example (only
      //  works if all path components can be translated)
      if (lay::NetlistObjectsPath::translate (sch_path, *lvsdb->cross_ref ())) {
        sch_directory_tree->setCurrentIndex (model->index_from_path (sch_path));
      }
    }

    model = dynamic_cast<NetlistBrowserModel *> (xref_directory_tree->model ());
    if (model) {
      xref_directory_tree->setCurrentIndex (model->index_from_path (path));
    }

  }
}

std::vector<const db::Net *>
NetlistBrowserPage::selected_nets ()
{
  std::vector<const db::Net *> nets;

  QTreeView *directory_tree = current_directory_tree ();
  if (! directory_tree) {
    return nets;
  }

  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

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
  std::vector<const db::Circuit *> circuits;

  QTreeView *directory_tree = current_directory_tree ();
  if (! directory_tree) {
    return circuits;
  }

  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  QModelIndexList selection = directory_tree->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
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
  std::vector<const db::SubCircuit *> subcircuits;

  QTreeView *directory_tree = current_directory_tree ();
  if (! directory_tree) {
    return subcircuits;
  }

  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

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
  std::vector<const db::Device *> devices;

  QTreeView *directory_tree = current_directory_tree ();
  if (! directory_tree) {
    return devices;
  }

  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

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
NetlistBrowserPage::nl_selection_changed ()
{
  selection_changed (nl_hierarchy_tree, nl_directory_tree);
}

void
NetlistBrowserPage::sch_selection_changed ()
{
  QTreeView *directory_tree = sch_directory_tree;

  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  db::LayoutToNetlist *l2ndb = mp_database.get ();
  db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);
  if (! lvsdb || ! lvsdb->cross_ref ()) {
    return;
  }

  QModelIndexList selected = directory_tree->selectionModel ()->selectedIndexes ();

  std::vector<lay::NetlistObjectsPath> selected_paths;
  selected_paths.reserve (selected.size ());
  for (QModelIndexList::const_iterator i = selected.begin (); i != selected.end (); ++i) {
    if (i->column () == 0) {
      selected_paths.push_back (model->path_from_index (*i));
      //  translate the schematic paths to layout paths (if available)
      if (! lay::NetlistObjectsPath::translate (selected_paths.back (), *lvsdb->cross_ref ())) {
        selected_paths.pop_back ();
      }
    }
  }

  QModelIndex current = directory_tree->selectionModel ()->currentIndex ();
  highlight (model->path_from_index (current), selected_paths);

  selection_changed_event ();
}

void
NetlistBrowserPage::xref_selection_changed ()
{
  selection_changed (xref_hierarchy_tree, xref_directory_tree);
}

void
NetlistBrowserPage::selection_changed (QTreeView * /*hierarchy_tree*/, QTreeView *directory_tree)
{
  NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
  tl_assert (model != 0);

  QModelIndexList selected = directory_tree->selectionModel ()->selectedIndexes ();

  std::vector<lay::NetlistObjectsPath> selected_paths;
  selected_paths.reserve (selected.size ());
  for (QModelIndexList::const_iterator i = selected.begin (); i != selected.end (); ++i) {
    if (i->column () == 0) {
      selected_paths.push_back (model->path_from_index (*i));
    }
  }

  QModelIndex current = directory_tree->selectionModel ()->currentIndex ();
  highlight (model->path_from_index (current), selected_paths);

  selection_changed_event ();
}

void
NetlistBrowserPage::set_color_for_selected_nets (const tl::Color &color)
{
  std::vector<const db::Net *> nets = selected_nets ();

  m_colorizer.begin_changes ();
  for (std::vector<const db::Net *>::const_iterator n = nets.begin (); n != nets.end (); ++n) {
    if (color.is_valid ()) {
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
    set_color_for_selected_nets (tl::Color (c.rgb ()));
  }
}

void
NetlistBrowserPage::select_color_for_net ()
{
  QAction *action = dynamic_cast<QAction *> (sender ());
  if (action) {
    set_color_for_selected_nets (tl::Color (action->data ().value<QColor> ().rgb ()));
  }
}

void
NetlistBrowserPage::navigate_to (const QModelIndex &index, bool fwd)
{
  if (! index.isValid () || ! index.model ()) {
    return;
  }

  QTreeView *directory_tree = 0;
  QTreeView *hierarchy_tree = 0;

  if (index.model () == nl_directory_tree->model ()) {

    directory_tree = nl_directory_tree;
    hierarchy_tree = nl_hierarchy_tree;
    mode_tab->setCurrentIndex (0);

  } else if (index.model () == sch_directory_tree->model ()) {

    directory_tree = sch_directory_tree;
    hierarchy_tree = sch_hierarchy_tree;
    mode_tab->setCurrentIndex (1);

  } else if (index.model () == xref_directory_tree->model ()) {

    directory_tree = xref_directory_tree;
    hierarchy_tree = xref_hierarchy_tree;
    mode_tab->setCurrentIndex (2);

  } else {
    return;
  }

  m_signals_enabled = false;

  try {

    directory_tree->setCurrentIndex (index);

    NetlistBrowserTreeModel *tree_model = dynamic_cast<NetlistBrowserTreeModel *> (hierarchy_tree->model ());
    NetlistBrowserModel *netlist_model = dynamic_cast<NetlistBrowserModel *> (directory_tree->model ());
    if (! tree_model || ! netlist_model) {
      return;
    }

    lay::NetlistObjectsPath path = netlist_model->path_from_index (index);
    QModelIndex circuit_index = tree_model->index_from_netpath (path);
    hierarchy_tree->setCurrentIndex (circuit_index);

  } catch (...) {
  }

  m_signals_enabled = true;

  add_to_history (index, fwd);

  selection_changed (hierarchy_tree, directory_tree);
}

void
NetlistBrowserPage::log_selection_changed ()
{
  clear_highlights ();

  if (! mp_database.get () || ! mp_database->netlist ()) {
    return;
  }

  NetlistLogModel *model = dynamic_cast<NetlistLogModel *> (log_view->model ());
  tl_assert (model != 0);

  QModelIndexList selection = log_view->selectionModel ()->selectedIndexes ();
  for (QModelIndexList::const_iterator i = selection.begin (); i != selection.end (); ++i) {
    if (i->column () == 0) {
      const db::LogEntryData *le = model->log_entry (*i);
      if (le && le->geometry () != db::DPolygon () && ! le->cell_name ().empty ()) {
        const db::Circuit *c = mp_database->netlist ()->circuit_by_name (le->cell_name ());
        if (c) {
          m_markers.push_back (std::make_pair (c, le->geometry ()));
        }
      }
    }
  }

  update_highlights ();
  adjust_view ();
}

void
NetlistBrowserPage::add_to_history (const QModelIndex &index, bool fwd)
{
  if (! fwd) {
    if (m_history_ptr > 1) {
      --m_history_ptr;
      m_history [m_history_ptr - 1] = index;
    }
  } else if (m_history_ptr >= m_history.size ()) {
    m_history.push_back (index);
    m_history_ptr = m_history.size ();
  } else {
    if (m_history [m_history_ptr] != index) {
      m_history.erase (m_history.begin () + m_history_ptr + 1, m_history.end ());
    }
    m_history [m_history_ptr] = index;
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

  if (mp_database.get () && ! mp_database->generator ().empty ()) {

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

static QModelIndex find_next (QTreeView *view, QAbstractItemModel *model, const QRegExp &to_find, const QModelIndex &from)
{
  QModelIndex index = from;

  if (! index.isValid () && model->hasChildren (index)) {
    index = model->index (0, 0, index);
  }

  if (! index.isValid ()) {
    return index;
  }

  int max_depth = 4;
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

  while (int (rows_stack.size ()) > max_depth) {
    rows_stack.pop_back ();
    parent_stack.pop_back ();
  }

  std::vector<std::pair<int, int> > initial_rows_stack = rows_stack;

  tl::AbsoluteProgress progress (tl::to_string (tr ("Searching ...")));

  do {

    ++progress;

    bool has_next = false;

    if (model->hasChildren (current) && int (rows_stack.size ()) < max_depth - 1) {

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
      if (to_find.indexIn (text) >= 0 && ! view->isRowHidden (rows_stack.back ().first, parent_stack.back ())) {
        return current;
      }

    }

  } while (rows_stack != initial_rows_stack);

  return QModelIndex ();
}

void
NetlistBrowserPage::find_button_pressed ()
{
  QRegExp re (find_text->text (),
              actionCaseSensitive->isChecked () ? Qt::CaseSensitive : Qt::CaseInsensitive,
              actionUseRegularExpressions->isChecked () ? QRegExp::RegExp : QRegExp::FixedString);

  QTreeView *directory_tree = current_directory_tree ();
  if (! directory_tree) {
    return;
  }

  QModelIndex next = find_next (directory_tree, directory_tree->model (), re, directory_tree->currentIndex ());
  if (next.isValid ()) {
    navigate_to (next);
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

    NetlistBrowserModel *model = dynamic_cast<NetlistBrowserModel *> (xref_directory_tree->model ());
    if (model) {
      model->set_item_visibility (xref_directory_tree, m_show_all, false /*show warnings only with 'show all'*/);
    }

  }
}

bool
NetlistBrowserPage::set_db (db::LayoutToNetlist *l2ndb)
{
  //  NOTE: mp_last_db mirrors mp_database, but does not automatically fall back to 0 when the DB is deleted. This way we can call
  //  set_db(0) with the correct behavior after the DB has been destroyed.
  if (l2ndb == mp_last_db) {
    //  not change
    return false;
  }

  if (mp_info_dialog) {
    delete mp_info_dialog;
    mp_info_dialog = 0;
  }

  db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);
  mp_database.reset (l2ndb);
  mp_last_db = l2ndb;

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

  bool is_lvsdb = (lvsdb != 0);
  mode_tab->setTabEnabled (0, true);
  mode_tab->setTabEnabled (1, is_lvsdb);
  mode_tab->setTabEnabled (2, is_lvsdb);
  mode_tab->setTabEnabled (3, true);
#if QT_VERSION >= 0x50F00
  mode_tab->setTabVisible (1, is_lvsdb);
  mode_tab->setTabVisible (2, is_lvsdb);
#endif

  if (is_lvsdb) {
    mode_tab->setCurrentIndex (2);
  } else {
    mode_tab->setCurrentIndex (0);
  }

  clear_highlights ();

  m_cell_context_cache = db::ContextCache (mp_database.get () ? mp_database->internal_layout () : 0);

  setup_trees ();

  selection_changed_event ();

  return true;
}

static void
set_abstract_tree_model (QTreeView *view, QAbstractItemModel *new_model)
{
  int columns = view->model () ? view->model ()->columnCount (QModelIndex ()) : 0;
  int new_columns = new_model->columnCount (QModelIndex ());

  delete view->model ();
  view->setModel (new_model);

  view->header ()->show ();
  view->header ()->setStretchLastSection (true);
  view->header ()->setMinimumSectionSize (25);

  if (columns < new_columns) {
    //  makes sure new columns are properly size-adjusted
    for (int i = std::max (0, columns - 1); i < new_columns; ++i) {
      view->header ()->resizeSection (i, i == 1 ? view->header ()->minimumSectionSize () : view->header ()->defaultSectionSize ());
    }
  }
}

static void
set_tree_model (QTreeView *view, NetlistBrowserModel *new_model)
{
  set_abstract_tree_model (view, new_model);

  //  hide the status column if not needed
  view->header ()->setSectionHidden (1, new_model->status_column () < 0);
}

static void
set_tree_model (QTreeView *view, NetlistBrowserTreeModel *new_model)
{
  set_abstract_tree_model (view, new_model);

  //  hide the status column if not needed
  view->header ()->setSectionHidden (1, new_model->status_column () < 0);
}

void
NetlistBrowserPage::setup_trees ()
{
  if (! mp_database.get ()) {

    delete nl_directory_tree->model ();
    nl_directory_tree->setModel (0);
    delete sch_directory_tree->model ();
    sch_directory_tree->setModel (0);
    delete xref_directory_tree->model ();
    xref_directory_tree->setModel (0);
    delete nl_hierarchy_tree->model ();
    nl_hierarchy_tree->setModel (0);
    delete sch_hierarchy_tree->model ();
    sch_hierarchy_tree->setModel (0);
    delete xref_hierarchy_tree->model ();
    xref_hierarchy_tree->setModel (0);
    delete log_view->model ();
    log_view->setModel (0);

    return;

  }

  db::LayoutToNetlist *l2ndb = mp_database.get ();
  db::LayoutVsSchematic *lvsdb = dynamic_cast<db::LayoutVsSchematic *> (l2ndb);

  QIcon log_tab_icon;

  if ((lvsdb && lvsdb->cross_ref ()) || (l2ndb && ! l2ndb->log_entries ().empty ())) {

    NetlistLogModel *new_model = new NetlistLogModel (log_view, lvsdb ? lvsdb->cross_ref () : 0, l2ndb);
    delete log_view->model ();
    log_view->setModel (new_model);

    connect (log_view->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (log_selection_changed ()));

    log_tab_icon = NetlistLogModel::icon_for_severity (new_model->max_severity ());

  } else {

    delete log_view->model ();
    log_view->setModel (0);

  }

  mode_tab->setTabIcon (3, log_tab_icon);

  {
    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserModel *new_model = new NetlistBrowserModel (nl_directory_tree, l2ndb, &m_colorizer);

    set_tree_model (nl_directory_tree, new_model);

    connect (nl_directory_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (nl_current_index_changed (const QModelIndex &)));
    connect (nl_directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (nl_selection_changed ()));

    //  establish visibility according to "show all"
    new_model->set_item_visibility (nl_directory_tree, m_show_all, false /*show warnings only with 'show all'*/);
  }

  if (lvsdb) {

    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserModel *new_model = new NetlistBrowserModel (sch_directory_tree, lvsdb->reference_netlist (), &m_colorizer);

    set_tree_model (sch_directory_tree, new_model);

    connect (sch_directory_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (sch_current_index_changed (const QModelIndex &)));
    connect (sch_directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (sch_selection_changed ()));

    //  establish visibility according to "show all"
    new_model->set_item_visibility (sch_directory_tree, m_show_all, false /*show warnings only with 'show all'*/);

  } else {

    delete sch_directory_tree->model ();
    sch_directory_tree->setModel (0);

  }

  if (lvsdb) {

    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserModel *new_model = new NetlistBrowserModel (xref_directory_tree, lvsdb, &m_colorizer);

    set_tree_model (xref_directory_tree, new_model);

    connect (xref_directory_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (xref_current_index_changed (const QModelIndex &)));
    connect (xref_directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (xref_selection_changed ()));

    //  establish visibility according to "show all"
    new_model->set_item_visibility (xref_directory_tree, m_show_all, false /*show warnings only with 'show all'*/);

  } else {

    delete xref_directory_tree->model ();
    xref_directory_tree->setModel (0);

  }

  {
    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserTreeModel *new_hierarchy_model = new NetlistBrowserTreeModel (nl_hierarchy_tree, l2ndb);
    set_tree_model (nl_hierarchy_tree, new_hierarchy_model);

    connect (nl_hierarchy_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (nl_current_tree_index_changed (const QModelIndex &)));
  }

  if (lvsdb) {

    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserTreeModel *new_hierarchy_model = new NetlistBrowserTreeModel (sch_hierarchy_tree, lvsdb->reference_netlist ());
    set_tree_model (sch_hierarchy_tree, new_hierarchy_model);

    connect (sch_hierarchy_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (sch_current_tree_index_changed (const QModelIndex &)));

  } else {

    delete sch_hierarchy_tree->model ();
    sch_hierarchy_tree->setModel (0);

  }

  if (lvsdb) {

    //  NOTE: with the tree as the parent, the tree will take over ownership of the model
    NetlistBrowserTreeModel *new_hierarchy_model = new NetlistBrowserTreeModel (xref_hierarchy_tree, lvsdb);
    set_tree_model (xref_hierarchy_tree, new_hierarchy_model);

    connect (xref_hierarchy_tree->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (xref_current_tree_index_changed (const QModelIndex &)));

  } else {

    delete xref_hierarchy_tree->model ();
    xref_hierarchy_tree->setModel (0);

  }

  find_text->setText (QString ());
}

void
NetlistBrowserPage::clear_highlights ()
{
  m_current_path = lay::NetlistObjectsPath ();
  m_selected_paths.clear ();
  m_markers.clear ();

  update_highlights ();
}

void
NetlistBrowserPage::highlight (const NetlistObjectsPath &current_path, const std::vector<NetlistObjectsPath> &selected_paths)
{
  if (current_path != m_current_path && selected_paths != m_selected_paths) {

    m_current_path = current_path;
    m_selected_paths = selected_paths;

    update_highlights ();
    adjust_view ();

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

  return layout->cell (device_abstract->cell_index ()).bbox ().transformed (db::CplxTrans (layout->dbu ()).inverted () * trans * db::CplxTrans (layout->dbu ()));
}

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

void
NetlistBrowserPage::adjust_view ()
{
  if (! mp_database.get () || ! mp_database->netlist () || ! mp_view) {
    return;
  }

  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  if (m_window != lay::NetlistBrowserConfig::FitNet && m_window != lay::NetlistBrowserConfig::Center && m_window != lay::NetlistBrowserConfig::CenterSize) {
    return;
  }

  const db::Layout &original_layout = cv->layout ();
  const db::Circuit *top_circuit = mp_database->netlist ()->circuit_by_name (original_layout.cell_name (cv.cell_index ()));
  if (! top_circuit) {
    return;
  }

  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = layout->is_valid_cell_index (top_circuit->cell_index ()) ? &layout->cell (top_circuit->cell_index ()) : mp_database->internal_top_cell ();
  if (! layout || ! cell) {
    return;
  }

  db::DBox bbox;

  for (std::vector<lay::NetlistObjectsPath>::const_iterator path = m_selected_paths.begin (); path != m_selected_paths.end (); ++path) {

    const db::Circuit *circuit = path->root.first;
    if (! circuit) {
      continue;
    }

    std::pair<bool, db::DCplxTrans> tr = trans_for (circuit, *layout, *cell, m_cell_context_cache, cv.context_dtrans ());
    if (! tr.first) {
      continue;
    }

    db::DCplxTrans trans = tr.second;

    for (std::list<std::pair<const db::SubCircuit *, const db::SubCircuit *> >::const_iterator p = path->path.begin (); p != path->path.end () && circuit; ++p) {
      if (p->first) {
        circuit = p->first->circuit_ref ();
        trans = trans * p->first->trans ();
      } else {
        circuit = 0;
      }
    }

    if (! circuit) {
      continue;
    }

    db::Box ebox;

    const db::Device *device = path->device.first;
    const db::Net *net = path->net.first;

    if (device) {

      ebox += bbox_for_device_abstract (layout, device->device_abstract (), db::DCplxTrans ());

      const std::vector<db::DeviceAbstractRef> &oda = device->other_abstracts ();
      for (std::vector<db::DeviceAbstractRef>::const_iterator a = oda.begin (); a != oda.end (); ++a) {
        ebox += bbox_for_device_abstract (layout, a->device_abstract, a->trans);
      }

      trans *= device->trans ();

    } else if (net) {

      db::cell_index_type cell_index = net->circuit ()->cell_index ();
      size_t cluster_id = net->cluster_id ();

      const db::Connectivity &conn = mp_database->connectivity ();
      for (db::Connectivity::all_layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

        db::Box layer_bbox;
        db::recursive_cluster_shape_iterator<db::NetShape> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
        while (! shapes.at_end ()) {
          layer_bbox += shapes->bbox ().transformed (shapes.trans ());
          ++shapes;
        }

        ebox += layer_bbox;

      }

    } else if (circuit) {
      ebox += bbox_for_circuit (layout, circuit);
    }

    bbox += trans * db::CplxTrans (layout->dbu ()) * ebox;

  }

  //  add markers boxes

  for (auto marker = m_markers.begin (); marker != m_markers.end (); ++marker) {

    std::pair<bool, db::DCplxTrans> tr = trans_for (marker->first, *layout, *cell, m_cell_context_cache, cv.context_dtrans ());
    if (tr.first) {
      bbox += (tr.second * marker->second).box ();
    }

  }

  if (! bbox.empty ()) {

    std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

    db::DBox tv_bbox;
    for (std::vector<db::DCplxTrans>::const_iterator t = tv.begin (); t != tv.end (); ++t) {
      tv_bbox += *t * bbox;
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

tl::Color
NetlistBrowserPage::make_valid_color (const tl::Color &color)
{
  if (! color.is_valid () && mp_view) {
    return mp_view->background_color ().to_mono () ? tl::Color (0, 0, 0) : tl::Color (255, 255, 255);
  } else {
    return color;
  }
}

bool
NetlistBrowserPage::produce_highlights_for_device (const db::Device *device, size_t &n_markers, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();

  tl::Color color = make_valid_color (m_colorizer.marker_color ());

  db::Box device_bbox = bbox_for_device_abstract (layout, device->device_abstract (), device->trans ());
  if (! device_bbox.empty ()) {

    if (n_markers == m_max_shape_count) {
      return true;
    }

    ++n_markers;

    mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
    mp_markers.back ()->set (device_bbox, db::ICplxTrans (), tv);
    mp_markers.back ()->set_color (color);
    mp_markers.back ()->set_frame_color (color);
    configure_marker (mp_markers.back (), false);

  }

  const std::vector<db::DeviceAbstractRef> &oda = device->other_abstracts ();
  for (std::vector<db::DeviceAbstractRef>::const_iterator a = oda.begin (); a != oda.end (); ++a) {

    db::Box da_box = bbox_for_device_abstract (layout, a->device_abstract, device->trans () * a->trans);
    if (! da_box.empty ()) {

      if (n_markers == m_max_shape_count) {
        return true;
      }

      ++n_markers;

      mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
      mp_markers.back ()->set (da_box, db::ICplxTrans (), tv);
      mp_markers.back ()->set_color (color);
      mp_markers.back ()->set_frame_color (color);
      configure_marker (mp_markers.back (), false);

    }

  }

  return false;
}

bool
NetlistBrowserPage::produce_highlights_for_circuit (const db::Circuit *circuit, size_t &n_markers, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();

  tl::Color color = make_valid_color (m_colorizer.marker_color ());
  db::Box circuit_bbox = bbox_for_circuit (layout, circuit);
  if (circuit_bbox.empty ()) {
    return false;
  }

  if (n_markers == m_max_shape_count) {
    return true;
  }

  ++n_markers;

  mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
  mp_markers.back ()->set (circuit_bbox, db::ICplxTrans (), tv);
  mp_markers.back ()->set_color (color);
  mp_markers.back ()->set_frame_color (color);
  configure_marker (mp_markers.back (), false);

  return false;
}

bool
NetlistBrowserPage::produce_highlights_for_net (const db::Net *net, size_t &n_markers, const std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> &display_by_lp, const std::vector<db::DCplxTrans> &tv)
{
  const db::Layout *layout = mp_database->internal_layout ();

  db::cell_index_type cell_index = net->circuit ()->cell_index ();
  size_t cluster_id = net->cluster_id ();

  tl::Color net_color = m_colorizer.color_of_net (net);
  tl::Color fallback_color = make_valid_color (m_colorizer.marker_color ());

  const db::Connectivity &conn = mp_database->connectivity ();
  for (db::Connectivity::all_layer_iterator layer = conn.begin_layers (); layer != conn.end_layers (); ++layer) {

    db::LayerProperties lp = layout->get_properties (*layer);
    std::map<db::LayerProperties, lay::LayerPropertiesConstIterator>::const_iterator display = display_by_lp.find (lp);

    db::recursive_cluster_shape_iterator<db::NetShape> shapes (mp_database->net_clusters (), *layer, cell_index, cluster_id);
    while (! shapes.at_end ()) {

      if (shapes->type () != db::NetShape::Polygon) {
        ++shapes;
        continue;
      }

      if (n_markers == m_max_shape_count) {
        return true;
      }

      mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
      mp_markers.back ()->set (shapes->polygon_ref (), shapes.trans (), tv);

      if (net_color.is_valid ()) {

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
  if (! mp_database.get () || ! mp_database->netlist () || ! mp_view) {
    return;
  }

  const lay::CellView &cv = mp_view->cellview (m_cv_index);
  if (! cv.is_valid ()) {
    return;
  }

  const db::Layout &original_layout = cv->layout ();
  const db::Circuit *top_circuit = mp_database->netlist ()->circuit_by_name (original_layout.cell_name (cv.cell_index ()));
  if (! top_circuit) {
    return;
  }

  const db::Layout *layout = mp_database->internal_layout ();
  const db::Cell *cell = layout->is_valid_cell_index (top_circuit->cell_index ()) ? &layout->cell (top_circuit->cell_index ()) : mp_database->internal_top_cell ();
  if (! layout || ! cell) {
    return;
  }

  std::map<db::LayerProperties, lay::LayerPropertiesConstIterator> display_by_lp;
  for (lay::LayerPropertiesConstIterator lp = mp_view->begin_layers (); ! lp.at_end (); ++lp) {
    if (! lp->has_children () && lp->cellview_index () == int (m_cv_index) && lp->layer_index () >= 0 && (unsigned int) lp->layer_index () < original_layout.layers ()) {
      display_by_lp.insert (std::make_pair (original_layout.get_properties (lp->layer_index ()), lp));
    }
  }

  std::vector<db::DCplxTrans> tv = mp_view->cv_transform_variants (m_cv_index);

  size_t n_markers = 0;
  bool not_all_shapes_are_shown = false;

  for (auto path = m_selected_paths.begin (); path != m_selected_paths.end (); ++path) {

    const db::Circuit *circuit = path->root.first;
    if (! circuit) {
      continue;
    }

    //  computes the transformation supplied by the path

    std::pair<bool, db::DCplxTrans> tr = trans_for (circuit, *layout, *cell, m_cell_context_cache, cv.context_dtrans ());
    if (! tr.first) {
      continue;
    }

    db::DCplxTrans trans = tr.second;

    for (std::list<std::pair<const db::SubCircuit *, const db::SubCircuit *> >::const_iterator p = path->path.begin (); p != path->path.end () && circuit; ++p) {
      if (p->first) {
        circuit = p->first->circuit_ref ();
        trans = trans * p->first->trans ();
      } else {
        circuit = 0;
      }
    }

    if (! circuit) {
      continue;
    }

    //  a map of display properties vs. layer properties

    //  correct DBU differences between the storage layout and the original layout
    for (std::vector<db::DCplxTrans>::iterator t = tv.begin (); t != tv.end (); ++t) {
      *t = *t * trans * db::DCplxTrans (layout->dbu () / original_layout.dbu ());
    }

    if (path->net.first) {
      if (produce_highlights_for_net (path->net.first, n_markers, display_by_lp, tv)) {
        not_all_shapes_are_shown = true;
      }
    } else if (path->device.first) {
      if (produce_highlights_for_device (path->device.first, n_markers, tv)) {
        not_all_shapes_are_shown = true;
      }
    } else if (circuit) {
      if (produce_highlights_for_circuit (circuit, n_markers, tv)) {
        not_all_shapes_are_shown = true;
      }
    }

  }

  for (auto marker = m_markers.begin (); marker != m_markers.end (); ++marker) {

    //  computes the transformation supplied by the path

    std::pair<bool, db::DCplxTrans> tr = trans_for (marker->first, *layout, *cell, m_cell_context_cache, cv.context_dtrans ());
    if (! tr.first) {
      continue;
    }

    //  creates a highlight from the marker

    tl::Color color = make_valid_color (m_colorizer.marker_color ());

    mp_markers.push_back (new lay::Marker (mp_view, m_cv_index));
    mp_markers.back ()->set (marker->second, db::DCplxTrans (1.0 / original_layout.dbu ()) * tr.second, tv);
    mp_markers.back ()->set_color (color);
    mp_markers.back ()->set_frame_color (color);

    configure_marker (mp_markers.back (), true);

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

  std::unique_ptr<lay::NetExportDialog> dialog (new lay::NetExportDialog (this));
  if (dialog->exec_dialog (mp_plugin_root)) {

    //  NOTE: mp_view and database might get reset to 0 in create_layout
    lay::LayoutViewBase *view = mp_view;
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
                          db::NPM_AllProperties,
                          dialog->net_propname (),
                          dialog->produce_circuit_cells () ? db::BNH_SubcircuitCells : db::BNH_Flatten,
                          dialog->produce_circuit_cells () ? dialog->circuit_cell_prefix ().c_str () : 0,
                          dialog->produce_device_cells () ? dialog->device_cell_prefix ().c_str () : 0);

    view->zoom_fit ();
    view->max_hier ();
    view->add_missing_layers ();
    view->select_cell (target_top_index, cv_index);

  }
}

}

#endif
