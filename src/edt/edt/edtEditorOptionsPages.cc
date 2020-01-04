
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


#include "tlInternational.h"
#include "dbLibrary.h"
#include "dbLibraryManager.h"
#include "dbPCellHeader.h"
#include "edtEditorOptionsPages.h"
#include "edtPCellParametersPage.h"
#include "edtConfig.h"
#include "edtService.h"
#include "tlExceptions.h"
#include "layPlugin.h"
#include "layLayoutView.h"
#include "layCellSelectionForm.h"
#include "ui_EditorOptionsDialog.h"
#include "ui_EditorOptionsGeneric.h"
#include "ui_EditorOptionsPath.h"
#include "ui_EditorOptionsText.h"
#include "ui_EditorOptionsInst.h"

namespace edt
{

// ------------------------------------------------------------------
//  EditorOptionsPage implementation

EditorOptionsPage::EditorOptionsPage ()
  : mp_owner (0), m_active (true), mp_plugin_declaration (0)
{
  //  nothing yet ..
}

EditorOptionsPage::~EditorOptionsPage ()
{
  set_owner (0);
}

void 
EditorOptionsPage::set_owner (EditorOptionsPages *owner)
{
  if (mp_owner) {
    mp_owner->unregister_page (this);
  }
  mp_owner = owner;
}

void  
EditorOptionsPage::activate (bool active)
{
  if (m_active != active) {
    m_active = active;
    if (mp_owner) {
      mp_owner->activate_page (this);
    }
  }
}

// ------------------------------------------------------------------
//  EditorOptionsPages implementation

struct EOPCompareOp
{
  bool operator() (edt::EditorOptionsPage *a, edt::EditorOptionsPage *b) const
  {
    return a->order () < b->order ();
  }
};

EditorOptionsPages::EditorOptionsPages (const std::vector<edt::EditorOptionsPage *> &pages, lay::PluginRoot *root)
  : mp_root (root)
{
  mp_ui = new Ui::EditorOptionsDialog ();
  mp_ui->setupUi (this);

  connect (mp_ui->apply_pb, SIGNAL (clicked ()), this, SLOT (apply ()));

  m_pages = pages;
  for (std::vector <edt::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    (*p)->set_owner (this);
  }

  update (0);
  setup ();
}

EditorOptionsPages::~EditorOptionsPages ()
{
  while (m_pages.size () > 0) {
    delete m_pages [0];
  }

  delete mp_ui;
  mp_ui = 0;
}

void  
EditorOptionsPages::unregister_page (edt::EditorOptionsPage *page)
{
  std::vector <edt::EditorOptionsPage *> pages;
  for (std::vector <edt::EditorOptionsPage *>::const_iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if (*p != page) {
      pages.push_back (*p);
    }
  }
  m_pages = pages;
  update (0);
}

void  
EditorOptionsPages::activate_page (edt::EditorOptionsPage *page)
{
  try {
    page->setup (mp_root);
  } catch (...) {
    //  catch any errors related to configuration file errors etc.
  }
  update (page);

  if (isVisible ()) {
    activateWindow ();
    raise ();
  }
}

void   
EditorOptionsPages::update (edt::EditorOptionsPage *page)
{
  std::sort (m_pages.begin (), m_pages.end (), EOPCompareOp ());

  while (mp_ui->pages->count () > 0) {
    mp_ui->pages->removeTab (0);
  }
  int index = -1;
  for (std::vector <edt::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active ()) {
      mp_ui->pages->addTab ((*p)->q_frame (), tl::to_qstring ((*p)->title ()));
      if ((*p) == page) {
        index = int (std::distance (m_pages.begin (), p));
      }
    } else {
      (*p)->q_frame ()->setParent (0);
    }
  }
  if (index < 0) {
    index = mp_ui->pages->currentIndex ();
  }
  if (index >= int (mp_ui->pages->count ())) {
    index = mp_ui->pages->count () - 1;
  }
  mp_ui->pages->setCurrentIndex (index);
}

void 
EditorOptionsPages::setup ()
{
  try {

    for (std::vector <edt::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
      if ((*p)->active ()) {
        (*p)->setup (mp_root);
      }
    }

    //  make the display consistent with the status (this is important for 
    //  PCell parameters where the PCell may be asked to modify the parameters)
    do_apply ();

  } catch (...) {
    //  catch any errors related to configuration file errors etc.
  }
}

void 
EditorOptionsPages::do_apply ()
{
  for (std::vector <edt::EditorOptionsPage *>::iterator p = m_pages.begin (); p != m_pages.end (); ++p) {
    if ((*p)->active ()) {
      (*p)->apply (mp_root);
    }
  }
}

void 
EditorOptionsPages::apply ()
{
BEGIN_PROTECTED
  do_apply ();
END_PROTECTED_W (this)
}

void
EditorOptionsPages::accept ()
{
BEGIN_PROTECTED
  do_apply ();
  QDialog::accept ();
END_PROTECTED_W (this)
}

// ------------------------------------------------------------------
//  EditorOptionsGeneric implementation

EditorOptionsGeneric::EditorOptionsGeneric ()
  : QWidget (), EditorOptionsPage ()
{
  mp_ui = new Ui::EditorOptionsGeneric ();
  mp_ui->setupUi (this);

  connect (mp_ui->grid_cb, SIGNAL (activated (int)), this, SLOT (grid_changed (int)));
}

EditorOptionsGeneric::~EditorOptionsGeneric ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsGeneric::title () const
{
  return tl::to_string (QObject::tr ("Basic Editing"));
}

void  
EditorOptionsGeneric::apply (lay::Plugin *root) 
{
  //  Edit grid
  
  db::DVector eg;
  EditGridConverter egc;
  if (mp_ui->grid_cb->currentIndex () == 0) {
    eg = db::DVector (-1.0, -1.0);
  } else if (mp_ui->grid_cb->currentIndex () == 1) {
    eg = db::DVector ();
  } else {
    egc.from_string_picky (tl::to_string (mp_ui->edit_grid_le->text ()), eg);
  }
  root->config_set (cfg_edit_grid, egc.to_string (eg));

  //  Edit & move angle

  ACConverter acc;
  root->config_set (cfg_edit_move_angle_mode, acc.to_string (lay::angle_constraint_type (mp_ui->move_angle_cb->currentIndex ())));
  root->config_set (cfg_edit_connect_angle_mode, acc.to_string (lay::angle_constraint_type (mp_ui->conn_angle_cb->currentIndex ())));

  root->config_set (cfg_edit_top_level_selection, tl::to_string (mp_ui->hier_sel_cbx->isChecked ()));
  int cpm = mp_ui->hier_copy_mode_cbx->currentIndex ();
  root->config_set (cfg_edit_hier_copy_mode, tl::to_string ((cpm < 0 || cpm > 1) ? -1 : cpm));
  root->config_set (cfg_edit_snap_to_objects, tl::to_string (mp_ui->snap_objects_cbx->isChecked ()));

  unsigned int max_shapes = 1000;
  tl::from_string (tl::to_string (mp_ui->max_shapes_le->text ()), max_shapes);
  root->config_set (cfg_edit_max_shapes_of_instances, tl::to_string (max_shapes));
  root->config_set (cfg_edit_show_shapes_of_instances, tl::to_string (mp_ui->show_shapes_cbx->isChecked ()));
}

void
EditorOptionsGeneric::grid_changed (int grid_mode)
{
  mp_ui->edit_grid_le->setEnabled (grid_mode == 2);
}

void
EditorOptionsGeneric::show_shapes_changed ()
{
  mp_ui->max_shapes_le->setEnabled (mp_ui->show_shapes_cbx->isChecked ());
}

void  
EditorOptionsGeneric::setup (lay::Plugin *root)
{
  //  Edit grid

  EditGridConverter egc;
  db::DVector eg;
  root->config_get (cfg_edit_grid, eg, egc);

  if (eg == db::DVector ()) {
    mp_ui->grid_cb->setCurrentIndex (1);
  } else if (eg.x () < -0.5) {
    mp_ui->grid_cb->setCurrentIndex (0);
  } else {
    mp_ui->grid_cb->setCurrentIndex (2);
    mp_ui->edit_grid_le->setText (tl::to_qstring (egc.to_string (eg)));
  }
  grid_changed (mp_ui->grid_cb->currentIndex ());

  //  edit & move angle

  ACConverter acc;
  lay::angle_constraint_type ac;
  
  ac = lay::AC_Any;
  root->config_get (cfg_edit_move_angle_mode, ac, acc);
  mp_ui->move_angle_cb->setCurrentIndex (int (ac));

  ac = lay::AC_Any;
  root->config_get (cfg_edit_connect_angle_mode, ac, acc);
  mp_ui->conn_angle_cb->setCurrentIndex (int (ac));

  bool top_level_sel = false;
  root->config_get (cfg_edit_top_level_selection, top_level_sel);
  mp_ui->hier_sel_cbx->setChecked (top_level_sel);

  int cpm = -1;
  root->config_get (cfg_edit_hier_copy_mode, cpm);
  mp_ui->hier_copy_mode_cbx->setCurrentIndex ((cpm < 0 || cpm > 1) ? 2 : cpm);

  bool snap_to_objects = false;
  root->config_get (cfg_edit_snap_to_objects, snap_to_objects);
  mp_ui->snap_objects_cbx->setChecked (snap_to_objects);

  unsigned int max_shapes = 1000;
  root->config_get (cfg_edit_max_shapes_of_instances, max_shapes);
  mp_ui->max_shapes_le->setText (tl::to_qstring (tl::to_string (max_shapes)));

  bool show_shapes = true;
  root->config_get (cfg_edit_show_shapes_of_instances, show_shapes);
  mp_ui->show_shapes_cbx->setChecked (show_shapes);
}

// ------------------------------------------------------------------
//  EditorOptionsText implementation

EditorOptionsText::EditorOptionsText ()
  : QWidget (), EditorOptionsPage ()
{
  mp_ui = new Ui::EditorOptionsText ();
  mp_ui->setupUi (this);
}

EditorOptionsText::~EditorOptionsText ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsText::title () const
{
  return tl::to_string (QObject::tr ("Text"));
}

void  
EditorOptionsText::apply (lay::Plugin *root) 
{
  //  Text string
  root->config_set (cfg_edit_text_string, tl::unescape_string (tl::to_string (mp_ui->text_le->text ())));

  //  HAlign
  HAlignConverter hac;
  root->config_set (cfg_edit_text_halign, hac.to_string (db::HAlign (mp_ui->halign_cbx->currentIndex () - 1)));

  //  VAlign
  VAlignConverter vac;
  root->config_set (cfg_edit_text_valign, vac.to_string (db::VAlign (mp_ui->valign_cbx->currentIndex () - 1)));

  //  Text size
  if (mp_ui->size_le->text ().isEmpty ()) {
    root->config_set (cfg_edit_text_size, 0.0);
  } else {
    double sz = 0.0;
    tl::from_string (tl::to_string (mp_ui->size_le->text ()), sz);
    root->config_set (cfg_edit_text_size, sz);
  }
}

void  
EditorOptionsText::setup (lay::Plugin *root)
{
  //  Text string
  std::string s;
  root->config_get (cfg_edit_text_string, s);
  mp_ui->text_le->setText (tl::to_qstring (tl::escape_string (s)));

  //  HAlign
  db::HAlign ha = db::HAlignLeft;
  root->config_get (cfg_edit_text_halign, ha, HAlignConverter ());
  mp_ui->halign_cbx->setCurrentIndex (int (ha) + 1);

  //  VAlign
  db::VAlign va = db::VAlignBottom;
  root->config_get (cfg_edit_text_valign, va, VAlignConverter ());
  mp_ui->valign_cbx->setCurrentIndex (int (va) + 1);

  double sz = 0.0;
  root->config_get (cfg_edit_text_size, sz);
  if (sz > 0.0) {
    mp_ui->size_le->setText (tl::to_qstring (tl::to_string (sz)));
  } else {
    mp_ui->size_le->setText (QString ());
  }
}

// ------------------------------------------------------------------
//  EditorOptionsPath implementation

EditorOptionsPath::EditorOptionsPath ()
  : QWidget (), EditorOptionsPage ()
{
  mp_ui = new Ui::EditorOptionsPath ();
  mp_ui->setupUi (this);

  connect (mp_ui->type_cb, SIGNAL (currentIndexChanged (int)), this, SLOT (type_changed (int)));
}

EditorOptionsPath::~EditorOptionsPath ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsPath::title () const
{
  return tl::to_string (QObject::tr ("Path"));
}

void
EditorOptionsPath::type_changed (int type)
{
  mp_ui->start_ext_le->setEnabled (type == 2);
  mp_ui->end_ext_le->setEnabled (type == 2);
}

void  
EditorOptionsPath::apply (lay::Plugin *root) 
{
  //  width

  double w = 0.0;
  tl::from_string (tl::to_string (mp_ui->width_le->text ()), w);
  root->config_set (cfg_edit_path_width, tl::to_string (w));

  //  path type and extensions 

  if (mp_ui->type_cb->currentIndex () == 0) {

    root->config_set (cfg_edit_path_ext_type, "flush");

  } else if (mp_ui->type_cb->currentIndex () == 1) {

    root->config_set (cfg_edit_path_ext_type, "square");

  } else if (mp_ui->type_cb->currentIndex () == 2) {

    double bgnext = 0.0, endext = 0.0;
    root->config_set (cfg_edit_path_ext_type, "variable");

    tl::from_string (tl::to_string (mp_ui->start_ext_le->text ()), bgnext);
    root->config_set (cfg_edit_path_ext_var_begin, tl::to_string (bgnext));

    tl::from_string (tl::to_string (mp_ui->end_ext_le->text ()), endext);
    root->config_set (cfg_edit_path_ext_var_end, tl::to_string (endext));

  } else if (mp_ui->type_cb->currentIndex () == 3) {

    root->config_set (cfg_edit_path_ext_type, "round");

  }
}

void  
EditorOptionsPath::setup (lay::Plugin *root)
{
  //  width

  double w = 0.0;
  root->config_get (cfg_edit_path_width, w);
  mp_ui->width_le->setText (tl::to_qstring (tl::to_string (w)));

  //  path type and extensions 

  std::string type;
  root->config_get (cfg_edit_path_ext_type, type);
  if (type == "square") {
    mp_ui->type_cb->setCurrentIndex (1);
  } else if (type == "variable") {
    mp_ui->type_cb->setCurrentIndex (2);
  } else if (type == "round") {
    mp_ui->type_cb->setCurrentIndex (3);
  } else {
    mp_ui->type_cb->setCurrentIndex (0);
  }
  type_changed (mp_ui->type_cb->currentIndex ());

  double bgnext = 0.0, endext = 0.0;
  root->config_get (cfg_edit_path_ext_var_begin, bgnext);
  root->config_get (cfg_edit_path_ext_var_end, endext);
  mp_ui->start_ext_le->setText (tl::to_qstring (tl::to_string (bgnext)));
  mp_ui->end_ext_le->setText (tl::to_qstring (tl::to_string (endext)));
}

// ------------------------------------------------------------------
//  EditorOptionsInst implementation

EditorOptionsInst::EditorOptionsInst (lay::PluginRoot *root)
  : QWidget (), EditorOptionsPage (), mp_root (root), mp_pcell_parameters (0)
{
  mp_ui = new Ui::EditorOptionsInst ();
  mp_ui->setupUi (this);

  connect (mp_ui->array_grp, SIGNAL (clicked ()), this, SLOT (array_changed ()));
  connect (mp_ui->browse_pb, SIGNAL (clicked ()), this, SLOT (browse_cell ()));
  connect (mp_ui->lib_cbx, SIGNAL (currentIndexChanged (int)), this, SLOT (library_changed (int)));
  connect (mp_ui->cell_le, SIGNAL (textChanged (const QString &)), this, SLOT (cell_name_changed (const QString &)));

  QHBoxLayout *layout = new QHBoxLayout (mp_ui->pcell_tab);
  layout->setMargin (0);
  mp_ui->pcell_tab->setLayout (layout);

  m_cv_index = -1;
}

EditorOptionsInst::~EditorOptionsInst ()
{
  delete mp_ui;
  mp_ui = 0;
}

std::string 
EditorOptionsInst::title () const
{
  return tl::to_string (QObject::tr ("Instance"));
}

void
EditorOptionsInst::library_changed (int)
{
BEGIN_PROTECTED
  update_pcell_parameters ();
END_PROTECTED
}

void
EditorOptionsInst::cell_name_changed (const QString &)
{
BEGIN_PROTECTED
  update_pcell_parameters ();
END_PROTECTED
}

void
EditorOptionsInst::browse_cell ()
{
BEGIN_PROTECTED

  if (m_cv_index >= 0 && lay::LayoutView::current () && lay::LayoutView::current ()->cellview (m_cv_index).is_valid ()) {

    //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
    //  the library selected
    db::Layout *layout = 0;
    db::Library *lib = 0;
    if (mp_ui->lib_cbx->current_library ()) {
      lib = mp_ui->lib_cbx->current_library ();
      layout = &lib->layout ();
    } else {
      layout = &lay::LayoutView::current ()->cellview (m_cv_index)->layout ();
    }

    bool all_cells = (mp_ui->lib_cbx->current_library () != 0 ? false : true);
    lay::LibraryCellSelectionForm form (this, layout, "browse_lib_cell", all_cells);

    if (lib) {
      form.setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Select Cell - Library: ")) + lib->get_description ()));
    }

    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
    if (pc.first) {
      form.set_selected_pcell_id (pc.second);
    } else {
      std::pair<bool, db::cell_index_type> c = layout->cell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
      if (c.first) {
        form.set_selected_cell_index (c.second);
      }
    }

    if (form.exec ()) {
      if (form.selected_cell_is_pcell ()) {
        mp_ui->cell_le->setText (tl::to_qstring (layout->pcell_header (form.selected_pcell_id ())->get_name ()));
      } else if (layout->is_valid_cell_index (form.selected_cell_index ())) {
        mp_ui->cell_le->setText (tl::to_qstring (layout->cell_name (form.selected_cell_index ())));
      }
      update_pcell_parameters ();
    }

  }

END_PROTECTED
}

void
EditorOptionsInst::array_changed ()
{
  bool array = (mp_ui->array_grp->isChecked ());
  mp_ui->rows_le->setEnabled (array);
  mp_ui->row_x_le->setEnabled (array);
  mp_ui->row_y_le->setEnabled (array);
  mp_ui->columns_le->setEnabled (array);
  mp_ui->column_x_le->setEnabled (array);
  mp_ui->column_y_le->setEnabled (array);
}

void  
EditorOptionsInst::apply (lay::Plugin *root) 
{
  //  cell name
  root->config_set (cfg_edit_inst_cell_name, tl::to_string (mp_ui->cell_le->text ()));

  //  cell name
  if (mp_ui->lib_cbx->current_library ()) {
    root->config_set (cfg_edit_inst_lib_name, mp_ui->lib_cbx->current_library ()->get_name ());
  } else {
    root->config_set (cfg_edit_inst_lib_name, std::string ());
  }

  //  pcell parameters
  std::string param;
  db::Layout *layout = 0;

  if (mp_ui->lib_cbx->current_library ()) {
    layout = &mp_ui->lib_cbx->current_library ()->layout ();
  } else if (m_cv_index >= 0 && lay::LayoutView::current () && lay::LayoutView::current ()->cellview (m_cv_index).is_valid ()) {
    layout = &lay::LayoutView::current ()->cellview (m_cv_index)->layout ();
  }

  if (layout && mp_pcell_parameters) {
    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
    if (pc.first) {
      const db::PCellDeclaration *pc_decl = layout->pcell_declaration (pc.second);
      if (pc_decl) {
        param = pcell_parameters_to_string (pc_decl->named_parameters (mp_pcell_parameters->get_parameters ()));
      }
    }
  }
  
  root->config_set (cfg_edit_inst_pcell_parameters, param);

  //  rotation, scaling
  double angle = 0.0;
  tl::from_string (tl::to_string (mp_ui->angle_le->text ()), angle);
  root->config_set (cfg_edit_inst_angle, tl::to_string (angle));

  bool mirror = mp_ui->mirror_cbx->isChecked ();
  root->config_set (cfg_edit_inst_mirror, tl::to_string (mirror));

  double scale = 1.0;
  tl::from_string (tl::to_string (mp_ui->scale_le->text ()), scale);
  root->config_set (cfg_edit_inst_scale, tl::to_string (scale));

  //  array
  bool array = mp_ui->array_grp->isChecked ();
  root->config_set (cfg_edit_inst_array, tl::to_string (array));

  int rows = 1, columns = 1;
  double row_x = 0.0, row_y = 0.0, column_x = 0.0, column_y = 0.0;
  tl::from_string (tl::to_string (mp_ui->rows_le->text ()), rows);
  tl::from_string (tl::to_string (mp_ui->row_x_le->text ()), row_x);
  tl::from_string (tl::to_string (mp_ui->row_y_le->text ()), row_y);
  tl::from_string (tl::to_string (mp_ui->columns_le->text ()), columns);
  tl::from_string (tl::to_string (mp_ui->column_x_le->text ()), column_x);
  tl::from_string (tl::to_string (mp_ui->column_y_le->text ()), column_y);

  root->config_set (cfg_edit_inst_rows, tl::to_string (rows));
  root->config_set (cfg_edit_inst_row_x, tl::to_string (row_x));
  root->config_set (cfg_edit_inst_row_y, tl::to_string (row_y));
  root->config_set (cfg_edit_inst_columns, tl::to_string (columns));
  root->config_set (cfg_edit_inst_column_x, tl::to_string (column_x));
  root->config_set (cfg_edit_inst_column_y, tl::to_string (column_y));

  //  place origin of cell flag
  bool place_origin = mp_ui->place_origin_cb->isChecked ();
  root->config_set (cfg_edit_inst_place_origin, tl::to_string (place_origin));
}

void  
EditorOptionsInst::setup (lay::Plugin *root)
{
  m_cv_index = -1;
  if (lay::LayoutView::current ()) {
    m_cv_index = lay::LayoutView::current ()->active_cellview_index ();
  }
  mp_ui->lib_cbx->update_list ();
  if (m_cv_index >= 0 && lay::LayoutView::current () && lay::LayoutView::current ()->cellview (m_cv_index).is_valid ()) {
    mp_ui->lib_cbx->set_technology_filter (lay::LayoutView::current ()->cellview (m_cv_index)->tech_name (), true);
  } else {
    mp_ui->lib_cbx->set_technology_filter (std::string (), false);
  }

  //  cell name
  std::string s;
  root->config_get (cfg_edit_inst_cell_name, s);
  mp_ui->cell_le->setText (tl::to_qstring (s));

  //  library
  std::string l;
  root->config_get (cfg_edit_inst_lib_name, l);
  mp_ui->lib_cbx->set_current_library (db::LibraryManager::instance ().lib_ptr_by_name (l));

  //  pcell parameters
  std::string param;
  root->config_get (cfg_edit_inst_pcell_parameters, param);

  db::Layout *layout = 0;
  if (mp_ui->lib_cbx->current_library ()) {
    layout = &mp_ui->lib_cbx->current_library ()->layout ();
  } else if (m_cv_index >= 0 && lay::LayoutView::current () && lay::LayoutView::current ()->cellview (m_cv_index).is_valid ()) {
    layout = &lay::LayoutView::current ()->cellview (m_cv_index)->layout ();
  }

  std::vector<tl::Variant> pv;

  if (layout && mp_pcell_parameters) {

    std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());

    if (pc.first) {

      const db::PCellDeclaration *pc_decl = layout->pcell_declaration (pc.second);
      if (pc_decl) {

        std::map<std::string, tl::Variant> parameters;
        try {
          tl::Extractor ex (param.c_str ());
          ex.test ("!");  //  used to flag PCells
          while (! ex.at_end ()) {
            std::string n;
            ex.read_word_or_quoted (n);
            ex.test (":");
            ex.read (parameters.insert (std::make_pair (n, tl::Variant ())).first->second);
            ex.test (";");
          }
        } catch (...) { }

        const std::vector<db::PCellParameterDeclaration> &pcp = pc_decl->parameter_declarations ();
        for (std::vector<db::PCellParameterDeclaration>::const_iterator pd = pcp.begin (); pd != pcp.end (); ++pd) {
          std::map<std::string, tl::Variant>::const_iterator p = parameters.find (pd->get_name ());
          if (p != parameters.end ()) {
            pv.push_back (p->second);
          } else {
            pv.push_back (pd->get_default ());
          }
        }

      }

    }

  }
  
  try {
    update_pcell_parameters (pv);
  } catch (...) { }

  //  rotation, scaling
  double angle = 0.0;
  root->config_get (cfg_edit_inst_angle, angle);
  mp_ui->angle_le->setText (tl::to_qstring (tl::to_string (angle)));

  bool mirror = false;
  root->config_get (cfg_edit_inst_mirror, mirror);
  mp_ui->mirror_cbx->setChecked (mirror);

  double scale = 1.0;
  root->config_get (cfg_edit_inst_scale, scale);
  mp_ui->scale_le->setText (tl::to_qstring (tl::to_string (scale)));

  //  array
  bool array = false;
  root->config_get (cfg_edit_inst_array, array);
  mp_ui->array_grp->setChecked (array);

  int rows = 1, columns = 1;
  double row_x = 0.0, row_y = 0.0, column_x = 0.0, column_y = 0.0;
  root->config_get (cfg_edit_inst_rows, rows);
  root->config_get (cfg_edit_inst_row_x, row_x);
  root->config_get (cfg_edit_inst_row_y, row_y);
  root->config_get (cfg_edit_inst_columns, columns);
  root->config_get (cfg_edit_inst_column_x, column_x);
  root->config_get (cfg_edit_inst_column_y, column_y);

  mp_ui->rows_le->setText (tl::to_qstring (tl::to_string (rows)));
  mp_ui->row_x_le->setText (tl::to_qstring (tl::to_string (row_x)));
  mp_ui->row_y_le->setText (tl::to_qstring (tl::to_string (row_y)));
  mp_ui->columns_le->setText (tl::to_qstring (tl::to_string (columns)));
  mp_ui->column_x_le->setText (tl::to_qstring (tl::to_string (column_x)));
  mp_ui->column_y_le->setText (tl::to_qstring (tl::to_string (column_y)));

  //  place origin of cell flag
  bool place_origin = false;
  root->config_get (cfg_edit_inst_place_origin, place_origin);
  mp_ui->place_origin_cb->setChecked (place_origin);
}

void  
EditorOptionsInst::update_pcell_parameters ()
{
  update_pcell_parameters (std::vector <tl::Variant> ());
}

void  
EditorOptionsInst::update_pcell_parameters (const std::vector <tl::Variant> &parameters)
{
  db::Layout *layout = 0;

  if (m_cv_index < 0 || !lay::LayoutView::current () || !lay::LayoutView::current ()->cellview (m_cv_index).is_valid ()) {
    mp_ui->param_tab_widget->setTabEnabled (1, false);
    return;
  }

  //  find the layout the cell has to be looked up: that is either the layout of the current instance or 
  //  the library selected
  if (mp_ui->lib_cbx->current_library ()) {
    layout = &mp_ui->lib_cbx->current_library ()->layout ();
  } else {
    layout = &lay::LayoutView::current ()->cellview (m_cv_index)->layout ();
  }

  std::pair<bool, db::pcell_id_type> pc = layout->pcell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());
  std::pair<bool, db::cell_index_type> cc = layout->cell_by_name (tl::to_string (mp_ui->cell_le->text ()).c_str ());

  //  by the way, update the foreground color of the cell edit box as well (red, if not valid)
  QPalette pl = mp_ui->cell_le->palette ();
  if (! pc.first && ! cc.first) {
    pl.setColor (QPalette::Text, Qt::red);
    pl.setColor (QPalette::Base, QColor (Qt::red).lighter (180));
  } else {
    pl.setColor (QPalette::Text, palette ().color (QPalette::Text));
    pl.setColor (QPalette::Base, palette ().color (QPalette::Base));
  }
  mp_ui->cell_le->setPalette (pl);

  PCellParametersPage::State pcp_state;

  //  Hint: we shall not delete the page immediately. This gives a segmentation fault in some cases.
  if (mp_pcell_parameters) {
    pcp_state = mp_pcell_parameters->get_state ();
    mp_pcell_parameters->hide ();
    mp_pcell_parameters->deleteLater ();
  }

  mp_pcell_parameters = 0;

  if (pc.first && layout->pcell_declaration (pc.second)) {

    mp_ui->param_tab_widget->setTabEnabled (1, true);
    lay::LayoutView *view = lay::LayoutView::current ();
    mp_pcell_parameters = new PCellParametersPage (mp_ui->pcell_tab, &view->cellview (m_cv_index)->layout (), view, m_cv_index, layout->pcell_declaration (pc.second), parameters);
    mp_ui->pcell_tab->layout ()->addWidget (mp_pcell_parameters);

    mp_pcell_parameters->set_state (pcp_state);

  } else {
    mp_ui->param_tab_widget->setTabEnabled (1, false);
  }
}

}

