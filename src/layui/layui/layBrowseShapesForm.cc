
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

#include <vector>
#include <string>

#include <QTreeWidgetItem>
#include <QKeyEvent>

#include "layBrowseShapesForm.h"
#include "layConfigurationDialog.h"

#include "dbCellGraphUtils.h"
#include "tlException.h"
#include "tlString.h"
#include "tlAlgorithm.h"
#include "layQtTools.h"
#include "layMarker.h"
#include "layUtils.h"


namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

const std::string cfg_shb_context_cell ("shb-context-cell");
const std::string cfg_shb_context_mode ("shb-context-mode");
const std::string cfg_shb_window_state ("shb-window-state");
const std::string cfg_shb_window_mode ("shb-window-mode");
const std::string cfg_shb_window_dim ("shb-window-dim");
const std::string cfg_shb_max_inst_count ("shb-max-inst-count");
const std::string cfg_shb_max_shape_count ("shb-max-shape-count");

class BrowseShapesPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_shb_context_cell, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_context_mode, "any-top"));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_window_mode, "fit-marker"));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_max_inst_count, "1000"));
    options.push_back (std::pair<std::string, std::string> (cfg_shb_max_shape_count, "1000"));
  }

  virtual lay::ConfigPage *config_page (QWidget *parent, std::string &title) const
  {
    title = tl::to_string (QObject::tr ("Browsers|Shape Browser"));
    return new BrowseShapesConfigPage (parent); 
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::separator ("browser_group", "tools_menu.end"));
    menu_entries.push_back (lay::menu_item ("browse_shapes::show", "browse_shapes", "tools_menu.end", tl::to_string (QObject::tr ("Browse Shapes"))));
  }
 
  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new BrowseShapesForm (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new BrowseShapesPluginDeclaration (), 10000, "BrowseShapesPlugin");


// ------------------------------------------------------------

static struct {
  BrowseShapesForm::mode_type mode;
  const char *string;
} context_modes [] = {
  { BrowseShapesForm::AnyTop,      "any-top"    },
  { BrowseShapesForm::Local,       "local"      },
  { BrowseShapesForm::ToCellView,  "given-cell" }
};

class BrowseShapesContextModeConverter
{
public:
  void
  from_string (const std::string &value, BrowseShapesForm::mode_type &mode)
  {
    for (unsigned int i = 0; i < sizeof (context_modes) / sizeof (context_modes [0]); ++i) {
      if (value == context_modes [i].string) {
        mode = context_modes [i].mode;
        return;
      }
    }
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid cell browser context mode: ")) + value);
  }

  std::string 
  to_string (BrowseShapesForm::mode_type mode)
  {
    for (unsigned int i = 0; i < sizeof (context_modes) / sizeof (context_modes [0]); ++i) {
      if (mode == context_modes [i].mode) {
        return context_modes [i].string;
      }
    }
    return "";
  }
};

static struct {
  BrowseShapesForm::window_type mode;
  const char *string;
} window_modes [] = {
  { BrowseShapesForm::DontChange,    "dont-change" },
  { BrowseShapesForm::FitCell,       "fit-cell"    },
  { BrowseShapesForm::FitMarker,     "fit-marker"  },
  { BrowseShapesForm::Center,        "center"      },
  { BrowseShapesForm::CenterSize,    "center-size" }
};

class BrowseShapesWindowModeConverter
{
public:
  void
  from_string (const std::string &value, BrowseShapesForm::window_type &mode)
  {
    for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
      if (value == window_modes [i].string) {
        mode = window_modes [i].mode;
        return;
      }
    }
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid cell browser window mode: ")) + value);
  }

  std::string 
  to_string (BrowseShapesForm::window_type mode)
  {
    for (unsigned int i = 0; i < sizeof (window_modes) / sizeof (window_modes [0]); ++i) {
      if (mode == window_modes [i].mode) {
        return window_modes [i].string;
      }
    }
    return "";
  }
};

// ------------------------------------------------------------

BrowseShapesConfigPage::BrowseShapesConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::BrowseShapesConfigPage::setupUi (this);

  connect (cbx_context, SIGNAL (currentIndexChanged (int)), this, SLOT (context_changed (int)));
  connect (cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

void 
BrowseShapesConfigPage::setup (lay::Dispatcher *root)
{
  std::string value;

  //  context cell
  root->config_get (cfg_shb_context_cell, value);
  le_cell_name->setText (tl::to_qstring (value));

  //  context mode
  BrowseShapesForm::mode_type cmode = BrowseShapesForm::AnyTop;
  root->config_get (cfg_shb_context_mode, cmode, BrowseShapesContextModeConverter ());
  cbx_context->setCurrentIndex (int (cmode));

  //  window mode
  BrowseShapesForm::window_type wmode = BrowseShapesForm::FitMarker;
  root->config_get (cfg_shb_window_mode, wmode, BrowseShapesWindowModeConverter ());
  cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  std::string wdim_str;
  root->config_get (cfg_shb_window_dim, wdim_str);
  mrg_window->set_margin (lay::Margin::from_string (wdim_str));
    
  //  max. instance count
  unsigned int max_inst_count = 1000;
  root->config_get (cfg_shb_max_inst_count, max_inst_count);
  le_max_inst->setText (tl::to_qstring (tl::to_string (max_inst_count)));

  //  max. instance count
  unsigned int max_shape_count = 1000;
  root->config_get (cfg_shb_max_shape_count, max_shape_count);
  le_max_shapes->setText (tl::to_qstring (tl::to_string (max_shape_count)));

  //  enable controls
  context_changed (int (cmode));
  window_changed (int (wmode));
}

void
BrowseShapesConfigPage::context_changed (int m)
{
  le_cell_name->setEnabled (m == int (BrowseShapesForm::ToCellView));
}

void
BrowseShapesConfigPage::window_changed (int m)
{
  mrg_window->setEnabled (m == int (BrowseShapesForm::FitMarker) || m == int (BrowseShapesForm::CenterSize));
}

void 
BrowseShapesConfigPage::commit (lay::Dispatcher *root)
{
  unsigned int max_inst_count = 1000;
  tl::from_string_ext (tl::to_string (le_max_inst->text ()), max_inst_count);

  unsigned int max_shape_count = 1000;
  tl::from_string_ext (tl::to_string (le_max_shapes->text ()), max_shape_count);

  root->config_set (cfg_shb_context_cell, tl::to_string (le_cell_name->text ()));
  root->config_set (cfg_shb_context_mode, BrowseShapesForm::mode_type (cbx_context->currentIndex ()), BrowseShapesContextModeConverter ());
  root->config_set (cfg_shb_window_mode, BrowseShapesForm::window_type (cbx_window->currentIndex ()), BrowseShapesWindowModeConverter ());
  root->config_set (cfg_shb_window_dim, mrg_window->get_margin ().to_string ());
  root->config_set (cfg_shb_max_inst_count, max_inst_count);
  root->config_set (cfg_shb_max_shape_count, max_shape_count);
}


// ------------------------------------------------------------

class BrowseShapesFormLVI : public QTreeWidgetItem
{
public:
  BrowseShapesFormLVI (const std::string &text)
    : QTreeWidgetItem (),
      m_value (0.0), m_value_flat (0.0)
  { 
    setText (0, tl::to_qstring (text));
  }  
     
  virtual bool operator< (const QTreeWidgetItem &i) const
  {
    const BrowseShapesFormLVI *other = dynamic_cast <const BrowseShapesFormLVI *> (&i);
    if (other) {
      return m_value < other->m_value;
    }
    return QTreeWidgetItem::operator< (i);
  }

  void set_value (double v) 
  {
    m_value = v;
  }

  void set_value_flat (double v) 
  {
    m_value_flat = v;
  }

private:
  double m_value, m_value_flat;
};

// ------------------------------------------------------------

class BrowseShapesFormLayerLVI : public BrowseShapesFormLVI
{
public:
  BrowseShapesFormLayerLVI (const std::string &text)
    : BrowseShapesFormLVI (text)
  {
    QFont f (font (0));
    f.setBold (true);
    QColor tc (0, 0, 255);

    for (int col = 0; col < 3; ++col) {
      setFont (col, f);
#if QT_VERSION >= 0x60000
      setForeground (col, tc);
#else
      setTextColor (col, tc);
#endif
    }
  }
};

// ------------------------------------------------------------

class BrowseShapesFormCellLVI : public BrowseShapesFormLVI
{
public:
  BrowseShapesFormCellLVI (const std::string &cn, lay::CellView::cell_index_type index, unsigned int lindex)
    : BrowseShapesFormLVI (cn),
      m_index (index), m_lindex (lindex)
  {
    //  .. nothing yet ..
  }

  lay::CellView::cell_index_type index () const
  {
    return m_index;
  }

  unsigned int lindex () const
  {
    return m_lindex;
  }

private:
  lay::CellView::cell_index_type m_index;
  unsigned int m_lindex;
};

// ------------------------------------------------------------

class BrowseShapesFormCellInstanceLVI : public QTreeWidgetItem
{
public:
  BrowseShapesFormCellInstanceLVI (const std::string &text, const std::string &path, 
                                   const db::ICplxTrans &trans, lay::CellView::cell_index_type index)
    : QTreeWidgetItem (),
      m_trans (trans), m_index (index)
  {
    setText (0, tl::to_qstring (text));
    setText (1, tl::to_qstring (path));
  }

  const db::ICplxTrans &trans () const
  {
    return m_trans;
  }

  lay::CellView::cell_index_type index () const
  {
    return m_index;
  }

private:
  db::ICplxTrans m_trans;
  lay::CellView::cell_index_type m_index;
};

// ------------------------------------------------------------

class BrowseShapesFormShapeInstanceLVI : public QTreeWidgetItem
{
public:
  BrowseShapesFormShapeInstanceLVI (const std::string &text, 
                                    const db::ShapeIterator &iter,
                                    const db::ICplxTrans &trans)
    : QTreeWidgetItem (), m_iter (iter), m_trans (trans)
  {
    setText (0, tl::to_qstring (text));
  }

  const db::Shape &shape () const
  {
    return *(m_iter.operator-> ());
  }

  const db::ICplxTrans &trans () const
  {
    return m_trans;
  }

private:
  db::ShapeIterator m_iter;
  db::ICplxTrans m_trans;
};

// ------------------------------------------------------------

BrowseShapesForm::BrowseShapesForm (lay::Dispatcher *root, LayoutViewBase *vw)
  : lay::Browser (root, vw), 
    Ui::BrowseShapesForm (),
    m_cv_index (-1),
    m_cell_changed_enabled (true),
    m_view_changed (false),
    m_cell_inst_changed_enabled (true),
    m_shape_inst_changed_enabled (true),
    m_ef_enabled (true),
    m_mode (AnyTop),
    m_window (FitMarker),
    m_window_dim (),
    m_max_inst_count (0), 
    m_max_shape_count (0)
{
  Ui::BrowseShapesForm::setupUi (this);

  lv_cell->setSortingEnabled (false); // don't enable sorting: this messes up the list completely
  lv_cell->setSelectionMode (QTreeWidget::SingleSelection);
  lv_cell->setUniformRowHeights (true);

  lv_cell_instance->installEventFilter (this);
  lv_cell_instance->setSortingEnabled (false);
  lv_cell_instance->setSelectionMode (QTreeWidget::SingleSelection);
  lv_cell_instance->setUniformRowHeights (true);

  lv_shape_instance->installEventFilter (this);
  lv_shape_instance->setSelectionMode (QTreeWidget::ExtendedSelection);
  lv_shape_instance->setSortingEnabled (false);
  lv_shape_instance->setUniformRowHeights (true);

  update ();
  update_cell_list ();

  // signals and slots connections
  connect (lv_cell, SIGNAL (currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT (cell_changed(QTreeWidgetItem*, QTreeWidgetItem*)));
  connect (lv_cell_instance, SIGNAL (currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT (cell_inst_changed(QTreeWidgetItem*, QTreeWidgetItem*)));
  connect (lv_shape_instance, SIGNAL (itemSelectionChanged()), this, SLOT (shape_inst_changed()));
  connect (pb_next_cell, SIGNAL (clicked()), this, SLOT (next_cell()));
  connect (pb_prev_cell, SIGNAL (clicked()), this, SLOT (prev_cell()));
  connect (pb_next_shape, SIGNAL (clicked()), this, SLOT (next_shape()));
  connect (pb_prev_shape, SIGNAL (clicked()), this, SLOT (prev_shape()));
  connect (pb_next_inst, SIGNAL (clicked()), this, SLOT (next_inst()));
  connect (pb_prev_inst, SIGNAL (clicked()), this, SLOT (prev_inst()));
  connect (configureButton, SIGNAL (clicked ()), this, SLOT (configure ()));
}

void 
BrowseShapesForm::menu_activated (const std::string &symbol)
{
  if (symbol == "browse_shapes::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else {
    lay::Browser::menu_activated (symbol);
  }
}

BrowseShapesForm::~BrowseShapesForm ()
{
  remove_marker ();
}

void
BrowseShapesForm::configure ()
{
  lay::ConfigurationDialog config_dialog (this, root (), "BrowseShapesPlugin");
  config_dialog.exec ();
}

bool 
BrowseShapesForm::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;

  if (name == cfg_shb_context_cell) {

    need_update = lay::test_and_set (m_context_cell, value);

  } else if (name == cfg_shb_window_state) {

    lay::restore_dialog_state (this, value);

  } else if (name == cfg_shb_context_mode) {

    mode_type mode = m_mode;
    BrowseShapesContextModeConverter ().from_string (value, mode);
    need_update = lay::test_and_set (m_mode, mode);

  } else if (name == cfg_shb_window_mode) {

    window_type window = m_window;
    BrowseShapesWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_shb_window_dim) {

    lay::Margin wdim = lay::Margin::from_string (value);
    if (wdim != m_window_dim) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_shb_max_inst_count) {

    unsigned int mic = m_max_inst_count;
    tl::from_string (value, mic);
    need_update = lay::test_and_set (m_max_inst_count, mic);

  } else if (name == cfg_shb_max_shape_count) {

    unsigned int mic = 0;
    tl::from_string (value, mic);
    need_update = lay::test_and_set (m_max_shape_count, mic);

  } else {
    taken = false;
  }

  if (active () && need_update) {
    update ();
  }

  return taken;
}

void
BrowseShapesForm::remove_marker ()
{
  for (std::vector<lay::ShapeMarker *>::iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }
  mp_markers.clear ();
}

void 
BrowseShapesForm::cell_changed (QTreeWidgetItem *item, QTreeWidgetItem *)
{
  lv_cell_instance->clear ();
  lv_shape_instance->clear ();

  if (m_cv_index < 0 || ! m_cell_changed_enabled) {
    return;
  }

  BrowseShapesFormCellLVI *it = dynamic_cast <BrowseShapesFormCellLVI *> (item);
  if (! it) {
    remove_marker ();
    return;
  }

  const db::Layout &layout = m_cellview->layout ();
  double dbu = layout.dbu ();
  unsigned int layer = m_lprops [it->lindex ()]->layer_index ();
  db::ICplxTrans trans = db::VCplxTrans(1.0 / dbu) * m_lprops [it->lindex ()]->trans () [0] * db::CplxTrans(dbu);
  const std::set<db::properties_id_type> *prop_sel = &m_lprops [it->lindex ()]->prop_sel ();
  bool inv_prop_sel = m_lprops [it->lindex ()]->inverse_prop_sel ();
  const db::Cell &cell = layout.cell (it->index ());

  m_cell_inst_changed_enabled = false;

  QList<QTreeWidgetItem *> items;


  //  fill cell instances

  bool shortened = false;
  unsigned int count = 0;
  if (m_mode == AnyTop) {
    shortened = fill_cell_instances (db::ICplxTrans (), layout, &cell, 0, false, std::string (), items, count);
  } else if (m_mode == ToCellView) {
    if (m_cellview.is_valid ()) {
      shortened = fill_cell_instances (db::ICplxTrans (), layout, &cell, m_cellview.cell (), false, std::string (), items, count);
    }
  }

  //  add an entry to indicate that there are more ..
  if (shortened) {
    items.append (new QTreeWidgetItem ());
    items.back ()->setText (0, QString::fromUtf8 ("..."));
  }

  lv_cell_instance->addTopLevelItems (items);

  if (lv_cell_instance->topLevelItemCount () > 0) {
    lv_cell_instance->setCurrentItem (lv_cell_instance->topLevelItem (0));
  }

  m_cell_inst_changed_enabled = true;


  //  fill shape instances

  items.clear ();

  m_shape_inst_changed_enabled = false;

  //  fill the list of shape instances
  count = 0;
  db::ShapeIterator shape;
  for (shape = cell.shapes (layer).begin (db::ShapeIterator::All, prop_sel, inv_prop_sel); !shape.at_end () && count++ < m_max_shape_count; ++shape) {

    db::Box box (shape->bbox ());

    std::string name;
    if (shape->is_polygon ()) {
      name = tl::to_string (QObject::tr ("polygon"));
    } else if (shape->is_edge ()) {
      name = tl::to_string (QObject::tr ("edge"));
    } else if (shape->is_text ()) {
      name = tl::to_string (QObject::tr ("text"));
    } else if (shape->is_box ()) {
      name = tl::to_string (QObject::tr ("box"));
    } else if (shape->is_path ()) {
      name = tl::to_string (QObject::tr ("path"));
    } else {
      name = tl::to_string (QObject::tr ("non-geometric"));
    }

    items.append (new BrowseShapesFormShapeInstanceLVI ((name + std::string (" at (") + tl::micron_to_string (0.5 * dbu * (box.left () + box.right ())) + 
                                                                        "," + tl::micron_to_string (0.5 * dbu * (box.bottom () + box.top ())) + ")"),
                                                        shape,
                                                        trans));

  }

  shortened = !shape.at_end ();

  //  add an entry to indicate that there are more ..
  if (shortened) {
    items.append (new QTreeWidgetItem ());
    items.back ()->setText (0, QString::fromUtf8 ("..."));
  }

  lv_shape_instance->addTopLevelItems (items);

  if (lv_shape_instance->topLevelItemCount () > 0) {
    lv_shape_instance->topLevelItem (0)->setSelected (true);
    lv_shape_instance->setCurrentItem (lv_shape_instance->topLevelItem (0));
  }

  m_shape_inst_changed_enabled = true;

  highlight_current ();

}

void 
BrowseShapesForm::shape_inst_changed ()
{
  if (m_cv_index >= 0 && m_shape_inst_changed_enabled) {
    highlight_current ();
  }
}

void 
BrowseShapesForm::cell_inst_changed (QTreeWidgetItem *, QTreeWidgetItem *)
{
  if (m_cv_index >= 0 && m_cell_inst_changed_enabled) {
    highlight_current ();
  }
}

void 
BrowseShapesForm::activated ()
{
  view ()->save_view (m_display_state);

  std::vector <lay::LayerPropertiesConstIterator> sel_layers = view ()->selected_layers ();

  if (sel_layers.empty ()) {
    throw tl::Exception (tl::to_string (QObject::tr ("No layer selected")));
  }

  m_lprops.clear ();
  m_cellview = lay::CellView ();

  m_cv_index = -1;

  for (std::vector <lay::LayerPropertiesConstIterator>::const_iterator l = sel_layers.begin (); l != sel_layers.end (); ++l) {

    if ((*l)->layer_index () >= 0 && (*l)->cellview_index () >= 0) {

      m_lprops.push_back (*l);

      int cv_index = (*l)->cellview_index ();

      //  check if the cellviews have the same index
      if (m_cv_index >= 0) {
        if (cv_index != m_cv_index) {
          throw tl::Exception (tl::to_string (QObject::tr ("Layers selected for shape browsing must originate from the same cellview")));
        }
      } else { 
        m_cv_index = cv_index;
        m_cellview = view ()->cellview (m_cv_index);
      }

    }

  }

  update ();
  update_cell_list ();

  m_view_changed = false;
}

void 
BrowseShapesForm::update ()
{
  if (m_mode == ToCellView) {
    m_cellview.set_cell (m_context_cell);
  }

  lv_cell_instance->setEnabled (m_mode != Local);
  if (m_mode == Local) {
    lv_cell_instance->clear ();
  } else if (lv_cell->currentItem ()) {
    cell_changed (lv_cell->currentItem (), 0);
  }
}

//  A helper structure to hold all information relevant to the cells to
//  show

struct BrowseShapesCellInfo 
{
  BrowseShapesCellInfo (const std::string &n, size_t s, size_t sf, lay::CellView::cell_index_type i)
    : name (n), shapes (s), shapes_flat (sf), cell_index (i)
  {
    // ..
  }

  std::string name;
  size_t shapes, shapes_flat;
  lay::CellView::cell_index_type cell_index;

  bool operator< (const BrowseShapesCellInfo &d) const 
  {
    return name < d.name;
  }
};

template <class T>
size_t
num_shape_instances (unsigned int layer, const db::Cell &cell)
{
  typename T::tag tag;
  return std::distance (cell.shapes (layer).begin (tag), cell.shapes (layer).end (tag));
}


void 
BrowseShapesForm::update_cell_list ()
{
  BrowseShapesFormCellLVI *sel_item = 0;

  lv_cell->clear ();

  for (unsigned int lindex = 0; lindex < m_lprops.size (); ++lindex) {

    if (m_cv_index >= 0) {

      size_t all_shapes = 0;
      size_t all_shapes_flat = 0;

      BrowseShapesFormLayerLVI *layer_root = new BrowseShapesFormLayerLVI (m_lprops [lindex]->display_string (view (), true /*real*/));
      lv_cell->addTopLevelItem (layer_root);

      const db::Layout &layout = m_cellview->layout ();
      
      m_cell_changed_enabled = false;

      //  obtain all cell names, sort by shape count and fill into the lv_cell  
      std::vector <BrowseShapesCellInfo> cell_info;
      cell_info.reserve (layout.cells ());

      const std::set<db::properties_id_type> *prop_sel = &m_lprops [lindex]->prop_sel ();
      bool inv_prop_sel = m_lprops [lindex]->inverse_prop_sel ();

      db::CellCounter counter (& layout);

      for (db::Layout::const_iterator c = layout.begin (); c != layout.end (); ++c) {
        size_t shapes = 0;
        for (db::ShapeIterator iter = (*c).shapes (m_lprops [lindex]->layer_index ()).begin (db::ShapeIterator::All, prop_sel, inv_prop_sel); !iter.at_end (); ++iter) {
          ++shapes;
        }
        if (shapes > 0) {
          size_t cm = counter.weight (c->cell_index ());
          cell_info.push_back (BrowseShapesCellInfo (layout.cell_name (c->cell_index ()), shapes, shapes * cm, c->cell_index ()));
          all_shapes += shapes;
          all_shapes_flat += shapes * cm;
        }
      }

      tl::sort (cell_info.begin (), cell_info.end ()); 

      //  create the entries.
      QList<QTreeWidgetItem *> items;
      BrowseShapesFormCellLVI *item = 0;
      for (std::vector<BrowseShapesCellInfo>::const_iterator cn = cell_info.end (); cn != cell_info.begin (); ) {
        --cn;
        item = new BrowseShapesFormCellLVI (cn->name, cn->cell_index, lindex);
        item->setText (1, tl::to_qstring (tl::to_string (cn->shapes)));
        item->setText (2, tl::to_qstring (tl::to_string (cn->shapes_flat)));
        sel_item = item;
        items.prepend (item);
      }

      layer_root->addChildren (items);

      m_cell_changed_enabled = true;

      layer_root->setText (1, tl::to_qstring (tl::to_string (all_shapes)));
      layer_root->setText (2, tl::to_qstring (tl::to_string (all_shapes_flat)));
      layer_root->set_value (all_shapes);
      layer_root->set_value_flat (all_shapes);

    }

  }

  if (sel_item) {
    lv_cell->setCurrentItem (sel_item);
    sel_item->setSelected (true);
    lv_cell->scrollToItem (sel_item);
  }
}

void
BrowseShapesForm::deactivated ()
{
  root ()->config_set (cfg_shb_window_state, lay::save_dialog_state (this));

  //  remove the cellview reference and clean up everything that could reference 
  //  database objects
  lv_cell->clear ();
  lv_cell_instance->clear ();
  lv_shape_instance->clear ();
  m_cellview = lay::CellView ();

  if (m_view_changed) {
    view ()->store_state ();
  }
  view ()->goto_view (m_display_state);
  remove_marker ();
}

bool 
BrowseShapesForm::fill_cell_instances (const db::ICplxTrans &t, const db::Layout &layout, const db::Cell *from, const db::Cell *to, bool to_parent, const std::string &path, QList<QTreeWidgetItem *> &items, unsigned int &count)
{
  if (from == to || (! to_parent && to == 0 && from->is_top ())) {

    if (count == m_max_inst_count) {
      return true; //  shorten list
    }
    ++count;

    std::string text;
    if (! path.empty ()) {
      text += t.to_string (true /*lazy*/, layout.dbu ());
    }

    items.append (new BrowseShapesFormCellInstanceLVI (text, path, t, from->cell_index ()));

  } else {

    //  traverse the parents
    for (db::Cell::parent_inst_iterator p = from->begin_parent_insts (); ! p.at_end (); ++p) {

      db::CellInstArray parent_inst = p->inst ();

      db::Vector a, b;
      unsigned long r = 1, c = 1;
      parent_inst.is_regular_array (a, b, r, c);

      std::string aref;
      if (r > 1 || c > 1) {
        aref = "[";
        aref += tl::to_string (c);
        aref += ",";
        aref += tl::to_string (r);
        aref += "]";
      } else if (parent_inst.size () > 1) {
        aref = "(+";
        aref += tl::to_string (parent_inst.size () - 1);
        aref += "x)";
      }

      std::string new_path;
      if (! path.empty ()) {
        new_path = layout.cell_name (p->parent_cell_index ()) + aref + "/" + path;
      } else {
        new_path = layout.cell_name (p->parent_cell_index ()) + aref;
      }

      db::ICplxTrans tt (parent_inst.complex_trans ());
      const db::Cell *cell = & layout.cell (p->parent_cell_index ());
      if (fill_cell_instances (tt.inverted () * t, layout, cell, to_parent ? cell : to, false, new_path, items, count)) {
        return true; // list too long - no more entries possible
      }

    }

  }

  return false;

}

void 
BrowseShapesForm::highlight_current ()
{
  remove_marker ();

  if (m_cv_index < 0) {
    return;
  }

  db::ICplxTrans t;
  lay::CellView::cell_index_type cell_index = 0;

  BrowseShapesFormCellLVI *item = dynamic_cast <BrowseShapesFormCellLVI *> (lv_cell->currentItem ());
  if (! item) {
    return;
  }

  if (m_mode != Local) {
    BrowseShapesFormCellInstanceLVI *ci_item = dynamic_cast <BrowseShapesFormCellInstanceLVI *> (lv_cell_instance->currentItem ());
    if (! ci_item) {
      return;
    }
    t = ci_item->trans ();
    cell_index = ci_item->index ();
  } else {
    cell_index = item->index ();
  }

  db::DBox dbox;

  const db::Layout &layout = m_cellview->layout ();

  //  TODO: the selectedItems () method is somewhat slow for large selections
  QList<QTreeWidgetItem *> selected_items = lv_shape_instance->selectedItems ();
  for (QList<QTreeWidgetItem *>::const_iterator s = selected_items.begin (); s != selected_items.end (); ++s) {

    BrowseShapesFormShapeInstanceLVI *shape_item = dynamic_cast <BrowseShapesFormShapeInstanceLVI *> (*s);
    if (shape_item) {

      //  transform the box into the cell view shown in micron space
      mp_markers.push_back (new lay::ShapeMarker (view (), m_cv_index));
      mp_markers.back ()->set (shape_item->shape (), shape_item->trans () * t);

      dbox += (db::CplxTrans(layout.dbu ()) * shape_item->trans () * t) * shape_item->shape ().bbox ();

    }

  }

  if (! dbox.empty ()) {

    double window_dim = m_window_dim.get (dbox);

    view ()->select_cell (cell_index, m_cv_index);
    if (m_window == FitCell) {
      view ()->zoom_fit ();
    } else if (m_window == FitMarker) {
      view ()->zoom_box (dbox.enlarged (db::DVector (window_dim, window_dim)));
    } else if (m_window == Center) {
      view ()->pan_center (dbox.p1 () + (dbox.p2 () - dbox.p1 ()) * 0.5);
    } else if (m_window == CenterSize) {
      double w = std::max (dbox.width (), window_dim);
      double h = std::max (dbox.height (), window_dim);
      db::DPoint center (dbox.p1 () + (dbox.p2 () - dbox.p1 ()) * 0.5);
      db::DVector d (w * 0.5, h * 0.5);
      view ()->zoom_box (db::DBox (center - d, center + d));
    }

    m_view_changed = true;

  }

}

bool
BrowseShapesForm::adv_cell (bool up)
{
  QTreeWidgetItem *current = lv_cell->currentItem ();
  m_ef_enabled = false; // prevent recursion
  QKeyEvent ke (QEvent::KeyPress, up ? Qt::Key_Up : Qt::Key_Down, Qt::NoModifier);
  ((QObject *)lv_cell)->event (&ke);
  m_ef_enabled = true;

  if (lv_cell->currentItem () != current) {

    BrowseShapesFormLayerLVI *litem = dynamic_cast <BrowseShapesFormLayerLVI *> (lv_cell->currentItem ());
    if (litem) {
      if (! up) {
        while (litem && litem->childCount () == 0) {
          litem = dynamic_cast <BrowseShapesFormLayerLVI *> (lv_cell->topLevelItem (lv_cell->indexOfTopLevelItem (litem) + 1)); // TODO: slow!
        }
        if (litem) {
          QTreeWidgetItem *ni = litem->child (0);
          lv_cell->setCurrentItem (ni);
          ni->setSelected (true);
          lv_cell->scrollToItem (ni);
        }
      } else if (current->parent () == litem) {
        //  determine the layer item that is before the current one
        do {
          int i = lv_cell->indexOfTopLevelItem (litem);
          if (i > 0) {
            litem = dynamic_cast <BrowseShapesFormLayerLVI *> (lv_cell->topLevelItem (i - 1)); // TODO: slow!
          } else {
            litem = 0;
          }
        } while (litem && litem->childCount () == 0);
        if (litem) {
          QTreeWidgetItem *ni = litem->child (litem->childCount () - 1);
          lv_cell->setCurrentItem (ni);
          ni->setSelected (true);
          lv_cell->scrollToItem (ni);
        } else {
          //  revert to the original
          lv_cell->setCurrentItem (current);
          current->setSelected (true);
          lv_cell->scrollToItem (current);
        }
      }
    }

    return true;

  } else {
    return false;
  }

}

bool
BrowseShapesForm::adv_shape (bool up)
{
  QTreeWidgetItem *current = lv_shape_instance->currentItem ();
  m_ef_enabled = false; // prevent recursion
  QKeyEvent ke (QEvent::KeyPress, up ? Qt::Key_Up : Qt::Key_Down, Qt::NoModifier);
  ((QObject *)lv_shape_instance)->event (&ke);
  m_ef_enabled = true;

  if (lv_shape_instance->currentItem () == current) {

    //  if we are at the end of the list, pass the event 
    //  forward to the cell list
    if (adv_cell (up)) {

      //  position at the last shape if required
      if (up) {
        QTreeWidgetItem *ni = lv_shape_instance->topLevelItem (lv_shape_instance->topLevelItemCount () - 1);
        if (ni) {
          lv_shape_instance->setCurrentItem (ni);
          ni->setSelected (true);
          lv_shape_instance->scrollToItem (ni);
        }
      }

      return true;

    } else {
      return false;
    }

  } else {
    return true;
  }

}

bool
BrowseShapesForm::adv_cell_inst (bool up)
{
  QTreeWidgetItem *current = lv_cell_instance->currentItem ();

  m_ef_enabled = false; // prevent recursion
  QKeyEvent ke (QEvent::KeyPress, up ? Qt::Key_Up : Qt::Key_Down, Qt::NoModifier);
  ((QObject *)lv_cell_instance)->event (&ke);
  m_ef_enabled = true;

  if (lv_cell_instance->currentItem () == current) {

    //  if we are at the end of the list, pass the event 
    //  forward to the shape instance list
    if (adv_shape (up)) {

      //  select first or last item
      QTreeWidgetItem *ni;
      if (up) {
        ni = lv_cell_instance->topLevelItem (lv_cell_instance->topLevelItemCount () - 1);
      } else {
        ni = lv_cell_instance->topLevelItem (0);
      }
      if (ni) {
        lv_cell_instance->setCurrentItem (ni);
        ni->setSelected (true);
        lv_cell_instance->scrollToItem (ni);
      }

      return true;

    } else {
      return false;
    }

  } else {
    return false;
  }
}

bool 
BrowseShapesForm::eventFilter (QObject *watched, QEvent *event)
{
  if (m_ef_enabled && event->type () == QEvent::KeyPress) {

    QKeyEvent *ke = dynamic_cast <QKeyEvent *> (event);
    if (ke && (ke->key () == Qt::Key_Up || ke->key () == Qt::Key_Down)) {

      bool up = ke->key () == Qt::Key_Up;

      if (watched == lv_cell) {
        adv_cell (up);
      } else if (watched == lv_shape_instance) {
        adv_shape (up);
      } else if (watched == lv_cell_instance) {
        adv_cell_inst (up);
      }

      return true;

    }

  } 

  return QDialog::eventFilter (watched, event);
}

void 
BrowseShapesForm::next_cell ()
{
  lv_cell->setFocus ();
  adv_cell (false);
}

void 
BrowseShapesForm::prev_cell ()
{
  lv_cell->setFocus ();
  adv_cell (true);
}

void 
BrowseShapesForm::next_shape ()
{
  lv_shape_instance->setFocus ();
  adv_shape (false);
}

void 
BrowseShapesForm::prev_shape ()
{
  lv_shape_instance->setFocus ();
  adv_shape (true);
}

void 
BrowseShapesForm::next_inst ()
{
  lv_cell_instance->setFocus ();
  adv_cell_inst (false);
}

void 
BrowseShapesForm::prev_inst ()
{
  lv_cell_instance->setFocus ();
  adv_cell_inst (true);
}

}

#endif

