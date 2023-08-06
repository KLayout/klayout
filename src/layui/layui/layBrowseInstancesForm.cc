
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

#if defined(HAVE_QT)

#include <vector>
#include <string>

#include <QTreeWidgetItem>
#include <QKeyEvent>

#include "layBrowseInstancesForm.h"
#include "layConfigurationDialog.h"
#include "layCellSelectionForm.h"

#include "dbCellGraphUtils.h"
#include "tlException.h"
#include "tlString.h"
#include "tlAlgorithm.h"
#include "layMarker.h"
#include "layQtTools.h"
#include "layUtils.h"
#include "tlExceptions.h"

namespace lay
{

// ------------------------------------------------------------
//  Declaration of the configuration options

const std::string cfg_cib_context_cell ("cib-context-cell");
const std::string cfg_cib_context_mode ("cib-context-mode");
const std::string cfg_cib_window_state ("cib-window-state");
const std::string cfg_cib_window_mode ("cib-window-mode");
const std::string cfg_cib_window_dim ("cib-window-dim");
const std::string cfg_cib_max_inst_count ("cib-max-inst-count");


class BrowseInstancesPluginDeclaration
  : public lay::PluginDeclaration
{
public:
  virtual void get_options (std::vector < std::pair<std::string, std::string> > &options) const
  {
    options.push_back (std::pair<std::string, std::string> (cfg_cib_context_cell, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_cib_context_mode, "any-top"));
    options.push_back (std::pair<std::string, std::string> (cfg_cib_window_mode, "fit-marker"));
    options.push_back (std::pair<std::string, std::string> (cfg_cib_window_state, ""));
    options.push_back (std::pair<std::string, std::string> (cfg_cib_window_dim, "1.0"));
    options.push_back (std::pair<std::string, std::string> (cfg_cib_max_inst_count, "1000"));
  }

  virtual lay::ConfigPage *config_page (QWidget *parent, std::string &title) const
  {
    title = tl::to_string (QObject::tr ("Browsers|Cell Instance Browser"));
    return new BrowseInstancesConfigPage (parent); 
  }

  virtual void get_menu_entries (std::vector<lay::MenuEntry> &menu_entries) const
  {
    lay::PluginDeclaration::get_menu_entries (menu_entries);
    menu_entries.push_back (lay::menu_item ("browse_instances::show", "browse_instances", "tools_menu.end", tl::to_string (QObject::tr ("Browse Instances"))));
  }

  virtual lay::Plugin *create_plugin (db::Manager *, lay::Dispatcher *root, lay::LayoutViewBase *view) const
  {
    if (lay::has_gui ()) {
      return new BrowseInstancesForm (root, view);
    } else {
      return 0;
    }
  }
};

static tl::RegisteredClass<lay::PluginDeclaration> config_decl (new BrowseInstancesPluginDeclaration (), 11000, "BrowseInstancesPlugin");

// ------------------------------------------------------------

static struct {
  BrowseInstancesForm::mode_type mode;
  const char *string;
} context_modes [] = {
  { BrowseInstancesForm::AnyTop,      "any-top"    },
  { BrowseInstancesForm::Parent,      "parent"     },
  { BrowseInstancesForm::ToCellView,  "given-cell" }
};

class BrowseInstancesContextModeConverter
{
public:
  void
  from_string (const std::string &value, BrowseInstancesForm::mode_type &mode)
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
  to_string (BrowseInstancesForm::mode_type mode)
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
  BrowseInstancesForm::window_type mode;
  const char *string;
} window_modes [] = {
  { BrowseInstancesForm::DontChange,    "dont-change" },
  { BrowseInstancesForm::FitCell,       "fit-cell"    },
  { BrowseInstancesForm::FitMarker,     "fit-marker"  },
  { BrowseInstancesForm::Center,        "center"      },
  { BrowseInstancesForm::CenterSize,    "center-size" }
};

class BrowseInstancesWindowModeConverter
{
public:
  void
  from_string (const std::string &value, BrowseInstancesForm::window_type &mode)
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
  to_string (BrowseInstancesForm::window_type mode)
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

BrowseInstancesConfigPage::BrowseInstancesConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  Ui::BrowseInstancesConfigPage::setupUi (this);

  connect (cbx_context, SIGNAL (currentIndexChanged (int)), this, SLOT (context_changed (int)));
  connect (cbx_window, SIGNAL (currentIndexChanged (int)), this, SLOT (window_changed (int)));
}

void 
BrowseInstancesConfigPage::setup (lay::Dispatcher *root)
{
  std::string value;

  //  context cell
  root->config_get (cfg_cib_context_cell, value);
  le_cell_name->setText (tl::to_qstring (value));

  //  context mode
  BrowseInstancesForm::mode_type cmode = BrowseInstancesForm::AnyTop;
  root->config_get (cfg_cib_context_mode, cmode, BrowseInstancesContextModeConverter ());
  cbx_context->setCurrentIndex (int (cmode));

  //  window mode
  BrowseInstancesForm::window_type wmode = BrowseInstancesForm::FitMarker;
  root->config_get (cfg_cib_window_mode, wmode, BrowseInstancesWindowModeConverter ());
  cbx_window->setCurrentIndex (int (wmode));

  //  window dimension
  std::string wdim_str;
  root->config_get (cfg_cib_window_dim, wdim_str);
  mrg_window->set_margin (lay::Margin::from_string (wdim_str));
    
  //  max. instance count
  unsigned int max_inst_count = 1000;
  root->config_get (cfg_cib_max_inst_count, max_inst_count);
  le_max_count->setText (tl::to_qstring (tl::to_string (max_inst_count)));

  //  enable controls
  context_changed (int (cmode));
  window_changed (int (wmode));
}

void
BrowseInstancesConfigPage::context_changed (int m)
{
  le_cell_name->setEnabled (m == int (BrowseInstancesForm::ToCellView));
}

void
BrowseInstancesConfigPage::window_changed (int m)
{
  mrg_window->setEnabled (m == int (BrowseInstancesForm::FitMarker) || m == int (BrowseInstancesForm::CenterSize));
}

void 
BrowseInstancesConfigPage::commit (lay::Dispatcher *root)
{
  unsigned int max_inst_count = 1000;
  tl::from_string_ext (tl::to_string (le_max_count->text ()), max_inst_count);

  root->config_set (cfg_cib_context_cell, tl::to_string (le_cell_name->text ()));
  root->config_set (cfg_cib_context_mode, BrowseInstancesForm::mode_type (cbx_context->currentIndex ()), BrowseInstancesContextModeConverter ());
  root->config_set (cfg_cib_window_mode, BrowseInstancesForm::window_type (cbx_window->currentIndex ()), BrowseInstancesWindowModeConverter ());
  root->config_set (cfg_cib_window_dim, mrg_window->get_margin ().to_string ());
  root->config_set (cfg_cib_max_inst_count, max_inst_count);
}


// ------------------------------------------------------------

class BrowseInstancesFormCellLVI : public QTreeWidgetItem
{
public:
  BrowseInstancesFormCellLVI (const std::string &cn, lay::CellView::cell_index_type index)
    : QTreeWidgetItem (),
      m_index (index)
  {
    setText (0, tl::to_qstring (cn));
  }

  lay::CellView::cell_index_type index () const
  {
    return m_index;
  }

private:
  lay::CellView::cell_index_type m_index;
};

// ------------------------------------------------------------

class BrowseInstancesFormCellInstanceLVI : public QTreeWidgetItem
{
public:
  BrowseInstancesFormCellInstanceLVI (const std::string &text, const std::string &path,
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

BrowseInstancesForm::BrowseInstancesForm (lay::Dispatcher *root, LayoutViewBase *vw)
  : lay::Browser (root, vw), 
    Ui::BrowseInstancesForm (),
    m_cv_index (0),
    m_cell_index (0),
    m_cell_changed_enabled (true),
    m_view_changed (false),
    m_cell_inst_changed_enabled (true),
    m_ef_enabled (true),
    m_mode (AnyTop),
    m_window (FitMarker),
    m_window_dim (),
    m_max_inst_count (0),
    m_current_count (0)
{
  Ui::BrowseInstancesForm::setupUi (this);

  lv_cell_instance->installEventFilter (this);
  lv_cell_instance->setSortingEnabled (false);
  lv_cell_instance->setSelectionMode (QTreeWidget::ExtendedSelection);
  lv_cell_instance->setUniformRowHeights (true);

  //  signals and slots connections
  connect (lv_cell, SIGNAL (currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT (cell_changed(QTreeWidgetItem*, QTreeWidgetItem*)));
  connect (lv_cell_instance, SIGNAL (itemSelectionChanged()), this, SLOT (cell_inst_changed()));
  connect (pb_next_cell, SIGNAL (clicked()), this, SLOT (next_cell()));
  connect (pb_prev_cell, SIGNAL (clicked()), this, SLOT (prev_cell()));
  connect (pb_next_inst, SIGNAL (clicked()), this, SLOT (next_inst()));
  connect (pb_prev_inst, SIGNAL (clicked()), this, SLOT (prev_inst()));
  connect (configureButton, SIGNAL (clicked ()), this, SLOT (configure ()));
  connect (chooseCellButton, SIGNAL (clicked ()), this, SLOT (choose_cell_pressed ()));
}

void 
BrowseInstancesForm::menu_activated (const std::string &symbol)
{
  if (symbol == "browse_instances::show") {
    view ()->deactivate_all_browsers ();
    activate ();
  } else {
    lay::Browser::menu_activated (symbol);
  }
}

void
BrowseInstancesForm::configure ()
{
  lay::ConfigurationDialog config_dialog (this, root (), "BrowseInstancesPlugin");
  config_dialog.exec ();
}

BrowseInstancesForm::~BrowseInstancesForm ()
{
  remove_marker ();
}

void
BrowseInstancesForm::choose_cell_pressed ()
{
BEGIN_PROTECTED
  CellSelectionForm form (this, view (), "browse_cell", true /*simple mode*/);
  if (form.exec ()) {
    change_cell (form.selected_cellview ().cell_index (), form.selected_cellview_index ());
  }
END_PROTECTED
}

bool 
BrowseInstancesForm::configure (const std::string &name, const std::string &value)
{
  bool need_update = false;
  bool taken = true;

  if (name == cfg_cib_context_cell) {

    need_update = lay::test_and_set (m_context_cell, value);

  } else if (name == cfg_cib_window_state) {

    lay::restore_dialog_state (this, value);

  } else if (name == cfg_cib_context_mode) {

    mode_type mode = m_mode;
    BrowseInstancesContextModeConverter ().from_string (value, mode);
    need_update = lay::test_and_set (m_mode, mode);

  } else if (name == cfg_cib_window_mode) {

    window_type window = m_window;
    BrowseInstancesWindowModeConverter ().from_string (value, window);
    need_update = lay::test_and_set (m_window, window);

  } else if (name == cfg_cib_window_dim) {

    lay::Margin wdim = lay::Margin::from_string (value);
    if (wdim != m_window_dim) {
      m_window_dim = wdim;
      need_update = true;
    }

  } else if (name == cfg_cib_max_inst_count) {

    unsigned int mic = m_max_inst_count;
    tl::from_string (value, mic);
    need_update = lay::test_and_set (m_max_inst_count, mic);

  } else {
    taken = false;
  }

  if (need_update && active () && lv_cell->currentItem ()) {
    if (m_mode == ToCellView) {
      m_context_cv.set_cell (m_context_cell);
    }
    cell_changed (lv_cell->currentItem (), 0);
  }

  return taken;
}

void
BrowseInstancesForm::remove_marker ()
{
  for (std::vector<lay::Marker *>::const_iterator m = mp_markers.begin (); m != mp_markers.end (); ++m) {
    delete *m;
  }
  mp_markers.clear ();
}

void 
BrowseInstancesForm::cell_changed (QTreeWidgetItem *item, QTreeWidgetItem *)
{
  lv_cell_instance->clear ();

  if (! m_cell_changed_enabled) {
    return;
  }

  std::vector <const db::Cell *> parents;

  const db::Layout &layout = m_context_cv->layout ();
  const db::Cell &cell = layout.cell (m_cell_index);

  BrowseInstancesFormCellLVI *it = dynamic_cast <BrowseInstancesFormCellLVI *> (item);
  if (! it) {

    //  "All item" - fetch the parents of all other items
    for (QList<QTreeWidgetItem *>::const_iterator i = m_items.begin (); i != m_items.end (); ++i) {
      BrowseInstancesFormCellLVI *it = dynamic_cast <BrowseInstancesFormCellLVI *> (*i);
      if (it) {
        parents.push_back (& layout.cell (it->index ()));
      }
    }

  } else {
    parents.push_back (& layout.cell (it->index ()));
  }

  m_cell_inst_changed_enabled = false;

  m_current_count = 0;

  QList<QTreeWidgetItem *> items;

  bool shortened = false;

  for (std::vector <const db::Cell *>::const_iterator parent = parents.begin (); parent != parents.end () && ! shortened; ++parent) {
    if (m_mode == AnyTop) {
      shortened = fill_cell_instances (db::ICplxTrans (), layout, *parent, &cell, 0, false, std::string (), items);
    } else if (m_mode == ToCellView) {
      if (m_context_cv.is_valid ()) {
        shortened = fill_cell_instances (db::ICplxTrans (), layout, *parent, &cell, m_context_cv.cell (), false, std::string (), items);
      }
    } else if (m_mode == Parent) {
      shortened = fill_cell_instances (db::ICplxTrans (), layout, *parent, &cell, 0, true, std::string (), items);
    }
  }

  //  add an entry to indicate that there are more ..
  if (shortened) {
    items.append (new QTreeWidgetItem ());
    items.back ()->setText (0, tl::to_qstring ("..."));
  }

  lv_cell_instance->addTopLevelItems (items);

  if (lv_cell_instance->topLevelItemCount () > 0) {
    QTreeWidgetItem *item = lv_cell_instance->topLevelItem (0);
    item->setSelected (true);
    lv_cell_instance->setCurrentItem (item);
    lv_cell_instance->scrollToItem (item);
  }

  m_cell_inst_changed_enabled = true;

  highlight_current ();

}

void 
BrowseInstancesForm::cell_inst_changed ()
{
  if (m_cell_inst_changed_enabled) {
    highlight_current ();
  }
}

//  A helper structure to hold all information relevant to the cells to
//  show

struct BrowseInstancesCellInfo 
{
  BrowseInstancesCellInfo (const std::string &n, lay::CellView::cell_index_type i)
    : name (n), cell_index (i), count (0), count_flat (0)
  {
    // ..
  }

  bool operator< (const BrowseInstancesCellInfo &d) const 
  {
    return name < d.name;
  }

  std::string name;
  lay::CellView::cell_index_type cell_index;
  size_t count, count_flat;
};

void 
BrowseInstancesForm::activated ()
{
  view ()->save_view (m_display_state);

  //  if no cellviews are available, don't do anything
  if (! view ()->cellviews ()) {
    return;
  }

  //  obtain active cellview index and cell index
  int cv_index = view ()->active_cellview_index ();

  lay::LayoutViewBase::cell_path_type path;
  view ()->current_cell_path (path);

  //  no cell to index
  if (path.empty ()) {
    return;
  }

  change_cell (path.back (), cv_index);
}

void 
BrowseInstancesForm::change_cell (db::cell_index_type cell_index, int cv_index)
{
  //  obtain active cellview index and cell index
  m_cv_index = cv_index;
  m_context_cv = view ()->cellview (m_cv_index);

  //  collect the transformation variants for this cellview - this way we can paint
  //  the cell boxes for each global transformation
  m_global_trans = view ()->cv_transform_variants (m_cv_index);

  m_cell_index = cell_index;

  if (m_mode == ToCellView) {
    m_context_cv.set_cell (m_context_cell);
  } else {
    m_context_cv.set_cell (m_cell_index);
  }

  const db::Layout &layout = m_context_cv->layout ();

  setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Browse Instances Of Cell")) + " '" + layout.cell_name (m_cell_index) + "'"));

  //  update the cell list

  BrowseInstancesFormCellLVI *sel_item = 0;

  lv_cell->clear ();

  m_cell_changed_enabled = false;

  //  obtain all cell names, sort and fill into the lv_cell  
  std::vector <BrowseInstancesCellInfo> cell_info;

  db::CellCounter counter (&layout);

  size_t tot_count = 0, tot_count_flat = 0;

  const db::Cell &cell = layout.cell (m_cell_index);
  bool ci_set = false;
  size_t w = 0;
  lay::CellView::cell_index_type ci = 0;
  for (db::Cell::parent_inst_iterator p = cell.begin_parent_insts (); ! p.at_end (); ++p) {
    if (! ci_set || p->parent_cell_index () != ci) { 
      ci = p->parent_cell_index ();
      ci_set = true;
      w = counter.weight (ci);
      cell_info.push_back (BrowseInstancesCellInfo (layout.cell_name (ci), ci));
    }
    size_t c = p->child_inst ().size ();
    cell_info.back ().count += c;
    cell_info.back ().count_flat += c * w;
    tot_count += c;
    tot_count_flat += c * w;
  }

  tl::sort (cell_info.begin (), cell_info.end ()); 

  QTreeWidgetItem *all = new QTreeWidgetItem (lv_cell);
  all->setText (0, QObject::tr ("(All Instances)"));
  all->setText (1, tl::to_qstring (tl::to_string (tot_count)));
  all->setText (2, tl::to_qstring (tl::to_string (tot_count_flat)));

  QFont f (all->font (0));
  f.setBold (true);
  QColor tc (0, 0, 255);

  for (int col = 0; col < 3; ++col) {
    all->setFont (col, f);
#if QT_VERSION >= 0x60000
    all->setForeground (col, tc);
#else
    all->setTextColor (col, tc);
#endif
  }

  //  create the entries.
  m_items.clear ();
  BrowseInstancesFormCellLVI *item = 0;
  for (std::vector<BrowseInstancesCellInfo>::const_iterator cn = cell_info.begin (); cn != cell_info.end (); ++cn) {
    item = new BrowseInstancesFormCellLVI (cn->name, cn->cell_index);
    item->setText (1, tl::to_qstring (tl::to_string (cn->count)));
    item->setText (2, tl::to_qstring (tl::to_string (cn->count_flat)));
    m_items.prepend (item);
    if (! sel_item) {
      sel_item = item;
    }
  }
  lv_cell->addTopLevelItems (m_items);

  if (! sel_item && item) {
    sel_item = item;
  }

  //  make the first the current one
  if (sel_item) {
    lv_cell->setCurrentItem (sel_item);
    sel_item->setSelected (true);
    lv_cell->scrollToItem (sel_item);
  }

  m_cell_changed_enabled = true;

  if (sel_item) {
    cell_changed (sel_item, 0);
  }

  m_view_changed = false;
}

void
BrowseInstancesForm::deactivated ()
{
  root ()->config_set (cfg_cib_window_state, lay::save_dialog_state (this));

  //  remove the cellview reference and clean up everything that could reference 
  //  database objects
  lv_cell->clear ();
  lv_cell_instance->clear ();
  m_context_cv = lay::CellView ();

  if (m_view_changed) {
    view ()->store_state ();
  }
  view ()->goto_view (m_display_state);
  remove_marker ();
}

bool 
BrowseInstancesForm::fill_cell_instances (const db::ICplxTrans &t, const db::Layout &layout, const db::Cell *parent, const db::Cell *from, const db::Cell *to, bool to_parent, const std::string &path, QList<QTreeWidgetItem *> &items)
{
  if (from == to || (! to_parent && to == 0 && from->is_top ())) {

    if (m_current_count == m_max_inst_count) {
      return true; //  shorten list
    }
    ++m_current_count;

    std::string text;
    if (! path.empty ()) {
      text += t.to_string (true /*lazy*/, layout.dbu ());
    }

    items.append (new BrowseInstancesFormCellInstanceLVI (text, path, t, from->cell_index ()));

  } else {

    //  traverse the parents
    for (db::Cell::parent_inst_iterator p = from->begin_parent_insts (); ! p.at_end (); ++p) {

      //  not in scope - continue
      if (parent && p->parent_cell_index () != parent->cell_index ()) {
        continue;
      }

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
      if (fill_cell_instances (tt.inverted () * t, layout, 0, cell, to_parent ? cell : to, false, new_path, items)) {
        return true; // list too long - no more entries possible
      }

    }

  }

  return false;

}

void 
BrowseInstancesForm::highlight_current ()
{
  remove_marker ();

  bool index_set = false;
  lay::CellView::cell_index_type index = 0;
  db::DBox dbox;

  //  TODO: the selectedItems () method is somewhat slow for large selections
  QList<QTreeWidgetItem *> selected_items = lv_cell_instance->selectedItems ();
  for (QList<QTreeWidgetItem *>::const_iterator s = selected_items.begin (); s != selected_items.end (); ++s) {

    BrowseInstancesFormCellInstanceLVI *inst_item = dynamic_cast <BrowseInstancesFormCellInstanceLVI *> (*s);
    if (inst_item) {

      if (! index_set) {
        index = inst_item->index ();
        index_set = true;
      }

      if (index == inst_item->index ()) {

        const db::Layout &layout = m_context_cv->layout ();
        db::Box box = layout.cell (m_cell_index).bbox ();

        lay::Marker *marker = new lay::Marker (view (), m_cv_index);
        marker->set (box, inst_item->trans (), m_global_trans);
        mp_markers.push_back (marker);

        //  compute the bbox of the marker
        for (std::vector<db::DCplxTrans>::const_iterator gt = m_global_trans.begin (); gt != m_global_trans.end (); ++gt) {
          dbox += (*gt * db::CplxTrans (layout.dbu ()) * inst_item->trans ()) * box;
        }

      }
   
    }

  }

  if (index_set) {

    double window_dim = m_window_dim.get (dbox);

    view ()->select_cell (index, m_cv_index);
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
BrowseInstancesForm::adv_cell (bool up)
{
  QTreeWidgetItem *current = lv_cell->currentItem ();
  int i = lv_cell->indexOfTopLevelItem (current);
  if (i >= 0) {
    QTreeWidgetItem *next = lv_cell->topLevelItem (i + (up ? -1 : 1));
    if (next && dynamic_cast <BrowseInstancesFormCellLVI *> (next)) {
      lv_cell->setCurrentItem (next);
      lv_cell->scrollToItem (next);
      return true;
    }
  }
  return false;
}

void
BrowseInstancesForm::next_cell ()
{
  lv_cell->setFocus ();
  adv_cell (false);
}

void
BrowseInstancesForm::prev_cell ()
{
  lv_cell->setFocus ();
  adv_cell (true);
}

bool 
BrowseInstancesForm::eventFilter (QObject *watched, QEvent *event)
{
  if (m_ef_enabled && event->type () == QEvent::KeyPress) {

    QKeyEvent *ke = dynamic_cast <QKeyEvent *> (event);
    if (ke && (ke->key () == Qt::Key_Up || ke->key () == Qt::Key_Down)) {

      bool up = ke->key () == Qt::Key_Up;

      if (watched == lv_cell_instance) {
        adv_cell_inst (up);
      }

      return true;

    }

  } 

  return QDialog::eventFilter (watched, event);
}

bool
BrowseInstancesForm::adv_cell_inst (bool up)
{
  QTreeWidgetItem *current = lv_cell_instance->currentItem ();

  m_ef_enabled = false; // prevent recursion
  QKeyEvent ke (QEvent::KeyPress, up ? Qt::Key_Up : Qt::Key_Down, Qt::NoModifier);
  ((QObject *)lv_cell_instance)->event (&ke);
  m_ef_enabled = true;

  if (lv_cell_instance->currentItem () == current) {

    //  if we are at the end of the list, pass the event 
    //  forward to the shape instance list
    if (adv_cell (up)) {

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
    return true;
  }
}

void
BrowseInstancesForm::next_inst ()
{
  lv_cell_instance->setFocus ();
  adv_cell_inst (false);
}

void
BrowseInstancesForm::prev_inst ()
{
  lv_cell_instance->setFocus ();
  adv_cell_inst (true);
}

}

#endif

