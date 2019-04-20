
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2019 Matthias Koefferlein

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

namespace lay
{

extern std::string cfg_l2ndb_show_all;

// ----------------------------------------------------------------------------------
//  NetlistBrowserPage implementation

NetlistBrowserPage::NetlistBrowserPage (QWidget * /*parent*/)
  : m_show_all (true),
    m_context (lay::NetlistBrowserConfig::NetlistTop),
    m_window (lay::NetlistBrowserConfig::FitNet),
    m_window_dim (0.0),
    m_max_shape_count (1000),
    m_marker_line_width (-1),
    m_marker_vertex_size (-1),
    m_marker_halo (-1),
    m_marker_dither_pattern (-1),
    mp_view (0),
    m_cv_index (0),
    mp_plugin_root (0)
{
  Ui::NetlistBrowserPage::setupUi (this);

  m_show_all_action = new QAction (QObject::tr ("Show All"), this);
  m_show_all_action->setCheckable (true);
  m_show_all_action->setChecked (m_show_all);

  connect (m_show_all_action, SIGNAL (triggered ()), this, SLOT (show_all_clicked ()));
  connect (filter, SIGNAL (textEdited (const QString &)), this, SLOT (filter_changed ()));
}

NetlistBrowserPage::~NetlistBrowserPage ()
{
  // @@@
}

void
NetlistBrowserPage::set_plugin_root (lay::PluginRoot *pr)
{
  mp_plugin_root = pr;
}

void
NetlistBrowserPage::set_highlight_style (QColor color, int line_width, int vertex_size, int halo, int dither_pattern)
{
  m_marker_color = color;
  m_marker_line_width = line_width;
  m_marker_vertex_size = vertex_size;
  m_marker_halo = halo;
  m_marker_dither_pattern = dither_pattern;
  update_highlights ();
}

void
NetlistBrowserPage::set_view (lay::LayoutView *view, unsigned int cv_index)
{
  mp_view = view;
  m_cv_index = cv_index;
  update_highlights ();
}

void
NetlistBrowserPage::set_window (lay::NetlistBrowserConfig::net_window_type window, double window_dim, lay::NetlistBrowserConfig::net_context_mode_type context)
{
  if (window != m_window || window_dim != m_window_dim || context != m_context) {
    m_window = window;
    m_window_dim = window_dim;
    m_context = context;
  }
}

void
NetlistBrowserPage::set_max_shape_count (size_t max_shape_count)
{
  if (m_max_shape_count != max_shape_count) {
    m_max_shape_count = max_shape_count;
#if 0 // @@@
    update_marker_list (1 /*select first*/);
#endif
  }
}

void
NetlistBrowserPage::filter_changed ()
{
#if 0 // @@@
  MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
  if (tree_model) {
    set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
  }
  update_highlights (2 /*select all*/);
#endif
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

#if 0 // @@@
    MarkerBrowserTreeViewModel *tree_model = dynamic_cast<MarkerBrowserTreeViewModel *> (directory_tree->model ());
    if (tree_model) {
      set_hidden_rec (tree_model, directory_tree, QModelIndex (), m_show_all, cat_filter->text (), cell_filter->text ());
    }
#endif

  }
}

void
NetlistBrowserPage::update_highlights ()
{
#if 0
  if (! m_enable_updates) {
    m_update_needed = true;
    return;
  }

  if (m_recursion_sentinel) {
    return;
  }

  m_recursion_sentinel = true;
  try {
    do_update_markers ();
  } catch (...) {
    m_recursion_sentinel = false;
    throw;
  }
  m_recursion_sentinel = false;
#endif
}

void
NetlistBrowserPage::set_l2ndb (db::LayoutToNetlist *database)
{
#if 0 // @@@
  if (database != mp_database) {

    release_markers ();

    mp_database = database;

    QAbstractItemModel *tree_model = directory_tree->model ();

    MarkerBrowserTreeViewModel *new_model = new MarkerBrowserTreeViewModel ();
    new_model->set_show_empty_ones (true);
    new_model->set_database (database);
    directory_tree->setModel (new_model);
    connect (directory_tree->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (directory_selection_changed (const QItemSelection &, const QItemSelection &)));

    directory_tree->header ()->setSortIndicatorShown (true);

    cat_filter->setText (QString ());
    cell_filter->setText (QString ());
    set_hidden_rec (new_model, directory_tree, QModelIndex (), m_show_all, QString (), QString ());

    if (tree_model) {
      delete tree_model;
    }

    QAbstractItemModel *list_model = markers_list->model ();

    MarkerBrowserListViewModel *new_list_model = new MarkerBrowserListViewModel ();
    new_list_model->set_database (database);
    markers_list->setModel (new_list_model);
    connect (markers_list->selectionModel (), SIGNAL (selectionChanged (const QItemSelection &, const QItemSelection &)), this, SLOT (markers_selection_changed (const QItemSelection &, const QItemSelection &)));
    connect (markers_list->selectionModel (), SIGNAL (currentChanged (const QModelIndex &, const QModelIndex &)), this, SLOT (markers_current_changed (const QModelIndex &, const QModelIndex &)));

    if (list_model) {
      delete list_model;
    }

  }
#endif
}

void
NetlistBrowserPage::enable_updates (bool f)
{
#if 0 // @@@
  if (f != m_enable_updates) {

    m_enable_updates = f;

    if (f && m_update_needed) {
      update_markers ();
      update_info_text ();
    }

    m_update_needed = false;

  }
#endif
}

}

