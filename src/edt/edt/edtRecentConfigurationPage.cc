
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2026 Matthias Koefferlein

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

#include "edtRecentConfigurationPage.h"
#include "edtConfig.h"
#include "layDispatcher.h"
#include "layLayoutViewBase.h"
#include "layLayerTreeModel.h"
#include "layBusy.h"
#include "layEditorUtils.h"
#include "dbLibraryManager.h"
#include "dbLibrary.h"
#include "tlLog.h"

#include <QVBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QMessageBox>

namespace edt
{

static const size_t max_entries = 100;

void
RecentConfigurationPage::init ()
{
  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setContentsMargins (0, 0, 0, 0);

  QLabel *label = new QLabel (this);
  label->setText (tr ("Click to select a recent configuration"));
  ly->addWidget (label);

  mp_tree_widget = new QTreeWidget (this);
  mp_tree_widget->setRootIsDecorated (false);
  mp_tree_widget->setUniformRowHeights (true);
  mp_tree_widget->setSelectionMode (QAbstractItemView::NoSelection);
  mp_tree_widget->setAllColumnsShowFocus (true);
  ly->addWidget (mp_tree_widget);

  connect (mp_tree_widget, SIGNAL (itemClicked (QTreeWidgetItem *, int)), this, SLOT (item_clicked (QTreeWidgetItem *)));
  view ()->layer_list_changed_event.add (this, &RecentConfigurationPage::layers_changed);

  mp_tree_widget->setColumnCount (int (m_cfg.size ()));

  QStringList column_labels;
  for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c) {
    column_labels << tl::to_qstring (c->title);
  }
  mp_tree_widget->setHeaderLabels (column_labels);

  update_list ();
}

RecentConfigurationPage::~RecentConfigurationPage ()
{
  //  .. nothing yet ..
}

std::string RecentConfigurationPage::title () const
{
  return tl::to_string (tr ("Recent"));
}

int RecentConfigurationPage::order () const
{
  return 100;
}

std::list<std::vector<std::string> >
RecentConfigurationPage::get_stored_values () const
{
  std::string serialized_list = dispatcher ()->config_get (m_recent_cfg_name);

  std::list<std::vector<std::string> > values;

  try {

    tl::Extractor ex (serialized_list.c_str ());
    while (! ex.at_end ()) {

      values.push_back (std::vector<std::string> ());
      while (! ex.at_end () && ! ex.test (";")) {
        values.back ().push_back (std::string ());
        ex.read_word_or_quoted (values.back ().back ());
        ex.test (",");
      }

    }

  } catch (tl::Exception &ex) {
    tl::error << tl::to_string (tr ("Error reading configuration item ")) << m_recent_cfg_name << ": " << ex.msg ();
    values.clear ();
  }

  return values;
}

void
RecentConfigurationPage::set_stored_values (const std::list<std::vector<std::string> > &values) const
{
  std::string serialized_list;
  for (std::list<std::vector<std::string> >::const_iterator v = values.begin (); v != values.end (); ++v) {
    if (v != values.begin ()) {
      serialized_list += ";";
    }
    for (std::vector<std::string>::const_iterator s = v->begin (); s != v->end (); ++s) {
      serialized_list += tl::to_word_or_quoted_string (*s);
      serialized_list += ",";
    }
  }

  dispatcher ()->config_set (m_recent_cfg_name, serialized_list);
}

static lay::LayerPropertiesConstIterator
lp_iter_from_string (lay::LayoutViewBase *view, const std::string &s)
{
  //  parse the layer spec (<layer-props>[@<cv-index>])
  db::LayerProperties lp;
  tl::Extractor ex (s.c_str ());
  lp.read (ex);
  int cv_index = view->active_cellview_index ();
  if (ex.test ("@")) {
    ex.read (cv_index);
  }

  //  rename the ones that got shifted.
  lay::LayerPropertiesConstIterator l = view->begin_layers ();
  while (! l.at_end ()) {
    if (l->source (true).cv_index () == int (cv_index) && l->source (true).layer_props ().log_equal (lp)) {
      return l;
    }
    ++l;
  }

  return l;
}


void
RecentConfigurationPage::render_to (QTreeWidgetItem *item, int column, const std::vector<std::string> &values, RecentConfigurationPage::ConfigurationRendering rendering)
{
#if QT_VERSION >= 0x050000
  double dpr = devicePixelRatio ();
#else
  double dpr = 1.0;
#endif

  //  store original value
  item->setData (column, Qt::UserRole, tl::to_qstring (values [column]));

  switch (rendering) {

  case RecentConfigurationPage::ArrayFlag:
  case RecentConfigurationPage::Bool:
    {
      bool f = false;
      try {
        tl::from_string (values [column], f);
      } catch (tl::Exception &ex) {
        tl::error << tl::to_string (tr ("Configuration error (ArrayFlag/Bool): ")) << ex.msg ();
      }
      static QString checkmark = QString::fromUtf8 ("\xe2\x9c\x93");
      item->setText (column, f ? checkmark : QString ()); // "checkmark"
    }
    break;

  case RecentConfigurationPage::Layer:
    {
      int icon_size = item->treeWidget ()->style ()->pixelMetric (QStyle::PM_ButtonIconSize);
      lay::LayerPropertiesConstIterator l;
      try {
        l = lp_iter_from_string (view (), values [column]);
      } catch (tl::Exception &ex) {
        tl::error << tl::to_string (tr ("Configuration error (Layer): ")) << ex.msg ();
      }
      if (! l.is_null () && ! l.at_end ()) {
        item->setIcon (column, lay::LayerTreeModel::icon_for_layer (l, view (), icon_size, icon_size, dpr, 0, true));
        item->setText (column, tl::to_qstring (values [column]));
      } else {
        item->setIcon (column, QIcon ());
        item->setText (column, tl::to_qstring ("(" + values [column] + ")"));
      }
    }
    break;

  case RecentConfigurationPage::Int:
  case RecentConfigurationPage::Double:
  case RecentConfigurationPage::Text:
    item->setText (column, tl::to_qstring (values [column]));
    break;

  case RecentConfigurationPage::CellLibraryName:
    if (values [column].empty ()) {
      item->setText (column, tr ("(local)"));
    } else {
      item->setText (column, tl::to_qstring (values [column]));
    }
    break;

  case RecentConfigurationPage::IntIfArray:
  case RecentConfigurationPage::DoubleIfArray:
    {
      bool is_array = false;
      int flag_column = 0;
      for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c, ++flag_column) {
        if (c->rendering == RecentConfigurationPage::ArrayFlag) {
          try {
            tl::from_string (values [flag_column], is_array);
          } catch (tl::Exception &ex) {
            tl::error << tl::to_string (tr ("Configuration error (IntIfArray/DoubleIfArray): ")) << ex.msg ();
          }
          break;
        }
      }

      if (is_array) {
        item->setText (column, tl::to_qstring (values [column]));
      } else {
        item->setText (column, QString ());
      }
    }
    break;

  case RecentConfigurationPage::CellDisplayName:
    {
      //  search for a libname
      int libname_column = 0;
      const db::Library *lib = 0;
      for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c, ++libname_column) {
        if (c->rendering == RecentConfigurationPage::CellLibraryName) {
          if (view ()->active_cellview ().is_valid ()) {
            lib = db::LibraryManager::instance ().lib_ptr_by_name (values [libname_column], view ()->active_cellview ()->tech_name ());
          } else {
            lib = db::LibraryManager::instance ().lib_ptr_by_name (values [libname_column]);
          }
          break;
        }
      }

      if (lib) {

        //  search for a PCell parameters
        int pcp_column = 0;
        std::map<std::string, tl::Variant> pcp;
        for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c, ++pcp_column) {
          if (c->rendering == RecentConfigurationPage::PCellParameters) {
            pcp = lay::pcell_parameters_from_string (values [pcp_column]);
            break;
          }
        }

        std::pair<bool, db::Layout::pcell_id_type> pcid = lib->layout ().pcell_by_name (values [column].c_str ());
        if (pcid.first) {
          const db::PCellDeclaration *pc_decl = lib->layout ().pcell_declaration (pcid.second);
          if (pc_decl) {
            lay::BusySection busy;  //  do not trigger macro IDE breakpoints and exception handling
            try {
              item->setText (column, tl::to_qstring (pc_decl->get_display_name (pc_decl->map_parameters (pcp))));
            } catch (tl::Exception &ex) {
              item->setText (column, tl::to_qstring (std::string ("ERROR: ") + tl::to_quoted_string (ex.msg ())));
            }
            break;
          }
        }

      }

      item->setText (column, tl::to_qstring (values [column]));
    }
    break;

  case RecentConfigurationPage::PCellParameters:
    {
      std::map<std::string, tl::Variant> pcp;
      try {
        pcp = lay::pcell_parameters_from_string (values [column]);
      } catch (tl::Exception &ex) {
        tl::error << tl::to_string (tr ("Configuration error (PCellParameters): ")) << ex.msg ();
      }
      std::string r;
      for (std::map<std::string, tl::Variant>::const_iterator p = pcp.begin (); p != pcp.end (); ++p) {
        if (p != pcp.begin ()) {
          r += ",";
        }
        r += p->first;
        r += "=";
        r += p->second.to_string ();
      }

      item->setText (column, tl::to_qstring (r));
    }
    break;
  }

}

void
RecentConfigurationPage::layers_changed (int)
{
  dm_update_list ();
}

void
RecentConfigurationPage::technology_changed (const std::string &)
{
  dm_update_list ();
}

void
RecentConfigurationPage::update_list ()
{
  update_list (get_stored_values ());
}

void
RecentConfigurationPage::update_list (const std::list<std::vector<std::string> > &stored_values)
{
  int row = 0;
  for (std::list<std::vector<std::string> >::const_iterator v = stored_values.begin (); v != stored_values.end (); ++v, ++row) {

    QTreeWidgetItem *item = 0;
    if (row < mp_tree_widget->topLevelItemCount ()) {
      item = mp_tree_widget->topLevelItem (row);
    } else {
      item = new QTreeWidgetItem (mp_tree_widget);
      mp_tree_widget->addTopLevelItem (item);
    }

    int column = 0;
    for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c, ++column) {
      if (column < int (v->size ())) {
        render_to (item, column, *v, c->rendering);
      }
    }

  }

  while (mp_tree_widget->topLevelItemCount () > row) {
    delete mp_tree_widget->takeTopLevelItem (row);
  }

  mp_tree_widget->header ()->resizeSections (QHeaderView::ResizeToContents);
}

void
RecentConfigurationPage::item_clicked (QTreeWidgetItem *item)
{
  int column = 0;
  for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c, ++column) {

    std::string v = tl::to_string (item->data (column, Qt::UserRole).toString ());

    if (c->rendering == Layer) {

      //  "getting" a layer means making it current
      db::LayerProperties lp;
      tl::Extractor ex (v.c_str ());
      lp.read (ex);
      int cv_index = view ()->active_cellview_index ();
      if (ex.test ("@")) {
        ex.read (cv_index);
      }

      lay::set_or_request_current_layer (view (), lp, cv_index);

    } else {
      dispatcher ()->config_set (c->cfg_name, v);
    }

  }

  dispatcher ()->config_end ();
}

void
RecentConfigurationPage::commit_recent (lay::Dispatcher *root)
{
  std::vector<std::string> values;
  values.reserve (m_cfg.size ());
  for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c) {

    if (c->rendering == Layer) {

      std::string s;

      if (!(view ()->current_layer ().is_null () || view ()->current_layer ().at_end ()) && view ()->current_layer ()->is_visual ()) {

        int cv_index = view ()->current_layer ()->cellview_index ();
        const lay::CellView &cv = view ()->cellview (cv_index);
        int li = view ()->current_layer ()->layer_index ();
        if (cv.is_valid () && cv->layout ().is_valid_layer (li)) {
          s = cv->layout ().get_properties (li).to_string ();
          if (cv_index != view ()->active_cellview_index ()) {
             s += "@" + tl::to_string (cv_index);
          }
        }

      }

      values.push_back (s);

    } else {
      values.push_back (root->config_get (c->cfg_name));
    }

  }

  std::list<std::vector<std::string> > stored_values = get_stored_values ();

  for (std::list<std::vector<std::string> >::iterator v = stored_values.begin (); v != stored_values.end (); ++v) {
    if (*v == values) {
      stored_values.erase (v);
      break;
    }
  }

  stored_values.push_front (values);

  while (stored_values.size () > max_entries) {
    stored_values.erase (--stored_values.end ());
  }

  set_stored_values (stored_values);

  update_list (stored_values);
}

void
RecentConfigurationPage::config_recent_for_layer (lay::Dispatcher *root, const db::LayerProperties &lp, int cv_index)
{
  std::list<std::vector<std::string> > stored_values = get_stored_values ();

  auto v = stored_values.begin ();
  for ( ; v != stored_values.end (); ++v) {

    bool match = false;
    auto vv = v->begin ();
    for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end () && ! match && vv != v->end (); ++c, ++vv) {

      if (c->rendering == Layer) {

        //  "getting" a layer means making it current
        db::LayerProperties lp_stored;
        tl::Extractor ex (vv->c_str ());
        lp_stored.read (ex);
        int cv_index_stored = view ()->active_cellview_index ();
        if (ex.test ("@")) {
          ex.read (cv_index_stored);
        }

        match = (lp.log_equal (lp_stored) && (cv_index < 0 || cv_index_stored == cv_index));

      }

    }

    if (match) {
      break;
    }

  }

  if (v != stored_values.end ()) {

    auto vv = v->begin ();
    for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end () && vv != v->end (); ++c, ++vv) {
      if (c->rendering != Layer) {
        root->config_set (c->cfg_name, *vv);
      }
    }

    root->config_end ();

  }
}

// ------------------------------------------------------------------
//  Configurations and registrations

namespace {

class RecentShapeConfigurationPage
  : public edt::RecentConfigurationPage
{
public:
  RecentShapeConfigurationPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
    : edt::RecentConfigurationPage (view, dispatcher, "edit-recent-shape-param")
  {
    add (edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer));

    init ();
  }
};

class RecentTextConfigurationPage
  : public edt::RecentConfigurationPage
{
public:
  RecentTextConfigurationPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
    : edt::RecentConfigurationPage (view, dispatcher, "edit-recent-text-param")
  {
    add (edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_string, tl::to_string (tr ("Text")), edt::RecentConfigurationPage::Text));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_size, tl::to_string (tr ("Size")), edt::RecentConfigurationPage::Double));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_halign, tl::to_string (tr ("Hor. align")), edt::RecentConfigurationPage::Text));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_text_valign, tl::to_string (tr ("Vert. align")), edt::RecentConfigurationPage::Text));

    init ();
  }
};

class RecentPathConfigurationPage
  : public edt::RecentConfigurationPage
{
public:
  RecentPathConfigurationPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
    : edt::RecentConfigurationPage (view, dispatcher, "edit-recent-path-param")
  {
    add (edt::RecentConfigurationPage::ConfigurationDescriptor ("", tl::to_string (tr ("Layer")), edt::RecentConfigurationPage::Layer));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_width, tl::to_string (tr ("Width")), edt::RecentConfigurationPage::Double));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_type, tl::to_string (tr ("Ends")), edt::RecentConfigurationPage::Int));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_var_begin, tl::to_string (tr ("Begin ext.")), edt::RecentConfigurationPage::Double));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_path_ext_var_end, tl::to_string (tr ("End ext.")), edt::RecentConfigurationPage::Double));

    init ();
  }
};

class RecentInstConfigurationPage
  : public edt::RecentConfigurationPage
{
public:
  RecentInstConfigurationPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher)
    : edt::RecentConfigurationPage (view, dispatcher, "edit-recent-inst-param")
  {
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_lib_name, tl::to_string (tr ("Library")), edt::RecentConfigurationPage::CellLibraryName));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_cell_name, tl::to_string (tr ("Cell")), edt::RecentConfigurationPage::CellDisplayName));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_angle, tl::to_string (tr ("Angle")), edt::RecentConfigurationPage::Double));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_mirror, tl::to_string (tr ("Mirror")), edt::RecentConfigurationPage::Bool));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_scale, tl::to_string (tr ("Scale")), edt::RecentConfigurationPage::Double));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_array, tl::to_string (tr ("Array")), edt::RecentConfigurationPage::ArrayFlag));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_rows, tl::to_string (tr ("Rows")), edt::RecentConfigurationPage::IntIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_row_x, tl::to_string (tr ("Row step (x)")), edt::RecentConfigurationPage::DoubleIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_row_y, tl::to_string (tr ("Row step (y)")), edt::RecentConfigurationPage::DoubleIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_columns, tl::to_string (tr ("Columns")), edt::RecentConfigurationPage::IntIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_column_x, tl::to_string (tr ("Column step (x)")), edt::RecentConfigurationPage::DoubleIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_column_y, tl::to_string (tr ("Column step (y)")), edt::RecentConfigurationPage::DoubleIfArray));
    add (edt::RecentConfigurationPage::ConfigurationDescriptor (cfg_edit_inst_pcell_parameters, tl::to_string (tr ("PCell parameters")), edt::RecentConfigurationPage::PCellParameters));

    init ();
  }
};

}

static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_polygons (new lay::EditorOptionsPageFactory<RecentShapeConfigurationPage> ("edt::Service(Polygons)"), 0);
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_boxes (new lay::EditorOptionsPageFactory<RecentShapeConfigurationPage> ("edt::Service(Boxes)"), 0);
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_points (new lay::EditorOptionsPageFactory<RecentShapeConfigurationPage> ("edt::Service(Points)"), 0);
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_texts (new lay::EditorOptionsPageFactory<RecentTextConfigurationPage> ("edt::Service(Texts)"), 0);
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_paths (new lay::EditorOptionsPageFactory<RecentPathConfigurationPage> ("edt::Service(Paths)"), 0);
static tl::RegisteredClass<lay::EditorOptionsPageFactoryBase> s_factory_insts (new lay::EditorOptionsPageFactory<RecentInstConfigurationPage> ("edt::Service(CellInstances)"), 0);

}

#endif
