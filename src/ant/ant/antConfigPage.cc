
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

#include "antConfigPage.h"
#include "ui_RulerConfigPage.h"
#include "ui_RulerConfigPage2.h"
#include "ui_RulerConfigPage3.h"
#include "ui_RulerConfigPage4.h"
#include "antConfig.h"
#include "layConverters.h"
#include "layDispatcher.h"
#include "layQtTools.h"
#include "tlExceptions.h"

#include <QInputDialog>

namespace ant
{

// ------------------------------------------------------------
//  Implementation of the configuration page

ConfigPage::ConfigPage (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::RulerConfigPage ();
  mp_ui->setupUi (this);
}

ConfigPage::~ConfigPage ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
ConfigPage::setup (lay::Dispatcher *root)
{
  //  Snap range
  int snap_range = 0;
  root->config_get (cfg_ruler_snap_range, snap_range);
  mp_ui->ruler_snap_range_edit->setText (tl::to_qstring (tl::to_string (snap_range)));

  //  object and grid snap
  bool f = false;
  root->config_get (cfg_ruler_obj_snap, f);
  mp_ui->ruler_obj_snap_cbx->setChecked (f);
  root->config_get (cfg_ruler_grid_snap, f);
  mp_ui->ruler_grid_snap_cbx->setChecked (f);
}

void 
ConfigPage::commit (lay::Dispatcher *root)
{
  root->config_set (cfg_ruler_obj_snap, mp_ui->ruler_obj_snap_cbx->isChecked ());
  root->config_set (cfg_ruler_grid_snap, mp_ui->ruler_grid_snap_cbx->isChecked ());

  int sr = 0;
  tl::from_string_ext (tl::to_string (mp_ui->ruler_snap_range_edit->text ()), sr);
  if (sr < 1 || sr > 1000) {
    throw tl::Exception (tl::to_string (QObject::tr ("Not a valid pixel value (must be non-zero positive and not too large): %s")), tl::to_string (mp_ui->ruler_snap_range_edit->text ()));
  }
  root->config_set (cfg_ruler_snap_range, sr);
}

// ------------------------------------------------------------
//  Implementation of the configuration page 2

ConfigPage2::ConfigPage2 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::RulerConfigPage2 ();
  mp_ui->setupUi (this);
}

ConfigPage2::~ConfigPage2 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
ConfigPage2::setup (lay::Dispatcher *root)
{
  //  Max. number of rulers
  int max_number_of_rulers = -1;
  root->config_get (cfg_max_number_of_rulers, max_number_of_rulers);
  if (max_number_of_rulers < 0) {
    mp_ui->num_rulers_edit->setText (QString ());
  } else {
    mp_ui->num_rulers_edit->setText (tl::to_qstring (tl::to_string (max_number_of_rulers)));
  }

  //  color
  QColor color;
  root->config_get (cfg_ruler_color, color, lay::ColorConverter ());
  mp_ui->ruler_color_pb->set_color (color);

  //  halo flag
  bool halo = true;
  root->config_get (cfg_ruler_halo, halo);
  mp_ui->halo_cb->setChecked (halo);
}

void 
ConfigPage2::commit (lay::Dispatcher *root)
{
  int mr;
  try {
    tl::from_string_ext (tl::to_string (mp_ui->num_rulers_edit->text ()), mr);
  } catch (...) {
    mr = -1;
  }
  root->config_set (cfg_max_number_of_rulers, mr);

  root->config_set (cfg_ruler_color, mp_ui->ruler_color_pb->get_color (), lay::ColorConverter ());
  root->config_set (cfg_ruler_halo, mp_ui->halo_cb->isChecked ());
}

// ------------------------------------------------------------
//  Implementation of the configuration page 3

ConfigPage3::ConfigPage3 (QWidget *parent)
  : lay::ConfigPage (parent)
{
  mp_ui = new Ui::RulerConfigPage3 ();
  mp_ui->setupUi (this);
}

ConfigPage3::~ConfigPage3 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
ConfigPage3::setup (lay::Dispatcher *root)
{
  //  snap mode
  lay::angle_constraint_type rm = lay::AC_Any;
  root->config_get (cfg_ruler_snap_mode, rm, ACConverter ());
  mp_ui->ruler_any_angle_rb->setChecked (rm == lay::AC_Any);
  mp_ui->ruler_ortho_rb->setChecked (rm == lay::AC_Ortho);
  mp_ui->ruler_diag_rb->setChecked (rm == lay::AC_Diagonal);
  mp_ui->ruler_hor_rb->setChecked (rm == lay::AC_Horizontal);
  mp_ui->ruler_vert_rb->setChecked (rm == lay::AC_Vertical);
}

void 
ConfigPage3::commit (lay::Dispatcher *root)
{
  lay::angle_constraint_type rm = lay::AC_Any;
  if (mp_ui->ruler_any_angle_rb->isChecked ()) {
    rm = lay::AC_Any;
  }
  if (mp_ui->ruler_ortho_rb->isChecked ()) {
    rm = lay::AC_Ortho;
  }
  if (mp_ui->ruler_diag_rb->isChecked ()) {
    rm = lay::AC_Diagonal;
  }
  if (mp_ui->ruler_hor_rb->isChecked ()) {
    rm = lay::AC_Horizontal;
  }
  if (mp_ui->ruler_vert_rb->isChecked ()) {
    rm = lay::AC_Vertical;
  }
  root->config_set (cfg_ruler_snap_mode, rm, ACConverter ());
}

// ------------------------------------------------------------
//  Implementation of the configuration page 4

ConfigPage4::ConfigPage4 (QWidget *parent)
  : lay::ConfigPage (parent),
    m_current_template (0),
    m_current_changed_enabled (true)
{
  mp_ui = new Ui::RulerConfigPage4 ();
  mp_ui->setupUi (this);

  connect (mp_ui->add_templ_pb, SIGNAL (clicked ()), this, SLOT (add_clicked ()));
  connect (mp_ui->del_templ_pb, SIGNAL (clicked ()), this, SLOT (del_clicked ()));
  connect (mp_ui->up_templ_pb, SIGNAL (clicked ()), this, SLOT (up_clicked ()));
  connect (mp_ui->down_templ_pb, SIGNAL (clicked ()), this, SLOT (down_clicked ()));
  connect (mp_ui->template_list, SIGNAL (currentRowChanged (int)), this, SLOT (current_template_changed (int)));
  connect (mp_ui->template_list, SIGNAL (itemDoubleClicked (QListWidgetItem *)), this, SLOT (double_clicked (QListWidgetItem *)));

  lay::activate_help_links (mp_ui->help_label);
}

ConfigPage4::~ConfigPage4 ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
ConfigPage4::setup (lay::Dispatcher *root)
{
  //  templates
  root->config_get (cfg_ruler_templates, m_ruler_templates, TemplatesConverter ());
  m_current_template = 0;
  root->config_get (cfg_current_ruler_template, m_current_template);
  
  //  add one template if the current index is not pointing to a valid one
  if (m_current_template < 0) {
    m_current_template = 0;
  }
  if (m_current_template >= int (m_ruler_templates.size ())) {
    m_current_template = int (m_ruler_templates.size ());
    m_ruler_templates.push_back (ant::Template ());
  }
  
  update_list ();
  show ();
}

void 
ConfigPage4::commit (lay::Dispatcher *root)
{
  commit ();
  
  //  templates
  root->config_set (cfg_ruler_templates, m_ruler_templates, TemplatesConverter ());
  root->config_set (cfg_current_ruler_template, m_current_template);
}

void 
ConfigPage4::add_clicked ()
{
  commit ();
  ant::Template new_one;
  if (m_current_template < 0 || m_current_template >= int (m_ruler_templates.size ())) {
    m_current_template = int (m_ruler_templates.size ());
  } else {
    new_one = m_ruler_templates [m_current_template];
  }
  new_one.category (std::string ());
  m_ruler_templates.insert (m_ruler_templates.begin () + m_current_template, new_one);
  m_ruler_templates [m_current_template].title (tl::to_string (QObject::tr ("New Ruler")));
  update_list ();
  show ();
  double_clicked (0); // to edit the name
}

void  
ConfigPage4::del_clicked ()
{
BEGIN_PROTECTED

  if (m_current_template >= 0 && m_current_template < int (m_ruler_templates.size ())) {
    if (! m_ruler_templates [m_current_template].category ().empty ()) {
      throw tl::Exception (tl::to_string (tr ("This ruler is a built-in template and cannot be deleted")));
    }
    m_ruler_templates.erase (m_ruler_templates.begin () + m_current_template);
    if (m_current_template > 0) {
      --m_current_template;
    }
    if (m_ruler_templates.empty ()) {
      m_ruler_templates.push_back (ant::Template ());
      m_current_template = 0;
    }
    update_list ();
    show ();
  }

END_PROTECTED
}

void  
ConfigPage4::up_clicked ()
{
  if (m_current_template > 0) {
    commit ();
    std::swap (m_ruler_templates [m_current_template], m_ruler_templates [m_current_template - 1]);
    --m_current_template;
    update_list ();
    show ();
  }
}

void  
ConfigPage4::down_clicked ()
{
  if (m_current_template >= 0 && m_current_template < int (m_ruler_templates.size () - 1)) {
    commit ();
    std::swap (m_ruler_templates [m_current_template], m_ruler_templates [m_current_template + 1]);
    ++m_current_template;
    update_list ();
    show ();
  }
}

void 
ConfigPage4::update_list ()
{
  m_current_changed_enabled = false;
  mp_ui->template_list->clear ();
  for (std::vector <ant::Template>::const_iterator t = m_ruler_templates.begin (); t != m_ruler_templates.end (); ++t) {
    mp_ui->template_list->addItem (tl::to_qstring (t->title ()));
    if (! t->category ().empty ()) {
      QListWidgetItem *item = mp_ui->template_list->item (int (t - m_ruler_templates.begin ()));
      QFont font = item->font ();
      font.setItalic (true);
      item->setFont (font);
    }
  }
  mp_ui->template_list->setCurrentRow (m_current_template);
  m_current_changed_enabled = true;
}

void  
ConfigPage4::current_template_changed (int index)
{
  if (m_current_changed_enabled) {
    commit ();
    m_current_template = index;
    show ();
  }
}

void   
ConfigPage4::double_clicked (QListWidgetItem *)
{
  if (m_current_template >= 0 && m_current_template < int (m_ruler_templates.size ())) {
    commit ();
    bool ok = false;
    QString new_title = QInputDialog::getText (this, 
                                               QObject::tr ("Enter New Title"),
                                               QObject::tr ("New Title"),
                                               QLineEdit::Normal,
                                               tl::to_qstring (m_ruler_templates [m_current_template].title ()),
                                               &ok);
    if (ok) {
      m_ruler_templates [m_current_template].title (tl::to_string (new_title));
      update_list ();
      show ();
    }
  }
}

void 
ConfigPage4::show ()
{
  mp_ui->fmt_le->setText (tl::to_qstring (m_ruler_templates [m_current_template].fmt ()));
  mp_ui->fmt_x_le->setText (tl::to_qstring (m_ruler_templates [m_current_template].fmt_x ()));
  mp_ui->fmt_y_le->setText (tl::to_qstring (m_ruler_templates [m_current_template].fmt_y ()));
  
  mp_ui->style_cb->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].style ());
  mp_ui->outline_cb->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].outline ());
  mp_ui->t_angle_cb->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].angle_constraint ());
  mp_ui->t_snap_cbx->setChecked (m_ruler_templates [m_current_template].snap ());
  mp_ui->t_mode_cb->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].mode ());

  mp_ui->main_position->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].main_position ());
  mp_ui->main_xalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].main_xalign ());
  mp_ui->main_yalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].main_yalign ());
  mp_ui->xlabel_xalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].xlabel_xalign ());
  mp_ui->xlabel_yalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].xlabel_yalign ());
  mp_ui->ylabel_xalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].ylabel_xalign ());
  mp_ui->ylabel_yalign->setCurrentIndex ((unsigned int) m_ruler_templates [m_current_template].ylabel_yalign ());
}

void   
ConfigPage4::commit ()
{
  std::string fmt, fmt_x, fmt_y;
  fmt = tl::to_string (mp_ui->fmt_le->text ());
  fmt_x = tl::to_string (mp_ui->fmt_x_le->text ());
  fmt_y = tl::to_string (mp_ui->fmt_y_le->text ());
  m_ruler_templates [m_current_template].fmt (fmt);
  m_ruler_templates [m_current_template].fmt_x (fmt_x);
  m_ruler_templates [m_current_template].fmt_y (fmt_y);

  ant::Object::style_type style = ant::Object::style_type (mp_ui->style_cb->currentIndex ());
  m_ruler_templates [m_current_template].style (style);
  
  ant::Object::outline_type outline = ant::Object::outline_type (mp_ui->outline_cb->currentIndex ());
  m_ruler_templates [m_current_template].outline (outline);
  
  lay::angle_constraint_type ac = lay::angle_constraint_type (mp_ui->t_angle_cb->currentIndex ());
  m_ruler_templates [m_current_template].angle_constraint (ac);

  ant::Template::ruler_mode_type mode = ant::Template::ruler_mode_type (mp_ui->t_mode_cb->currentIndex ());
  m_ruler_templates [m_current_template].set_mode (mode);

  m_ruler_templates [m_current_template].snap (mp_ui->t_snap_cbx->isChecked ());

  m_ruler_templates [m_current_template].set_main_position (Object::position_type (mp_ui->main_position->currentIndex ()));
  m_ruler_templates [m_current_template].set_main_xalign (Object::alignment_type (mp_ui->main_xalign->currentIndex ()));
  m_ruler_templates [m_current_template].set_main_yalign (Object::alignment_type (mp_ui->main_yalign->currentIndex ()));
  m_ruler_templates [m_current_template].set_xlabel_xalign (Object::alignment_type (mp_ui->xlabel_xalign->currentIndex ()));
  m_ruler_templates [m_current_template].set_xlabel_yalign (Object::alignment_type (mp_ui->xlabel_yalign->currentIndex ()));
  m_ruler_templates [m_current_template].set_ylabel_xalign (Object::alignment_type (mp_ui->ylabel_xalign->currentIndex ()));
  m_ruler_templates [m_current_template].set_ylabel_yalign (Object::alignment_type (mp_ui->ylabel_yalign->currentIndex ()));
}

} // namespace ant

#endif
