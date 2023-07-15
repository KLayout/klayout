
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

#include "layLayoutPropertiesForm.h"
#include "layLayoutViewBase.h"
#include "tlExceptions.h"
#include "layDialogs.h"
#include "tlInternational.h"
#include "tlString.h"
#include "tlExpression.h"

#include <set>
#include <sstream>

#include <QMessageBox>

namespace lay
{

// ------------------------------------------------------------

LayoutPropertiesForm::LayoutPropertiesForm (QWidget *parent, lay::LayoutViewBase *view, const char *name)
  : QDialog (parent), Ui::LayoutPropertiesForm ()
{
  m_editable = view->is_editable ();
  mp_view = view;

  setObjectName (QString::fromUtf8 (name));

  Ui::LayoutPropertiesForm::setupUi (this);

  //  collect the distinct layout handles 
  std::set <lay::LayoutHandle *> handles;
  for (unsigned int n = 0; n < view->cellviews (); ++n) {
    handles.insert (view->cellview (n).operator-> ());
  }
  
  m_handles.reserve (handles.size ());
  for (unsigned int n = 0; n < view->cellviews (); ++n) {
    lay::LayoutHandle *h = view->cellview (n).operator-> ();
    if (handles.find (h) != handles.end ()) {
      m_handles.push_back (h);
      handles.erase (h);
      layout_cbx->addItem (tl::to_qstring (h->name ()));
    }
  }

  m_index = -1;
  layout_cbx->setCurrentIndex (view->active_cellview_index ());

  connect (layout_cbx, SIGNAL (activated (int)), this, SLOT (layout_selected (int)));
  connect (prop_pb, SIGNAL (clicked ()), this, SLOT (prop_pb_clicked ()));

  dbu_le->setEnabled (m_editable);

  layout_selected (layout_cbx->currentIndex ());
}

void
LayoutPropertiesForm::accept ()
{
BEGIN_PROTECTED
  commit ();
  QDialog::accept ();
END_PROTECTED
}

void
LayoutPropertiesForm::commit ()
{
  if (m_index >= int (m_handles.size ()) || m_index < 0) {
    return;
  }

  db::Layout &layout = m_handles [m_index]->layout ();

  //  get the database unit
  double dbu = 0.001;
  tl::from_string_ext (tl::to_string (dbu_le->text ()), dbu);
  if (dbu < 1e-6 || dbu > 1e3) {
    throw tl::Exception (tl::to_string (QObject::tr ("Invalid database unit")));
  }

  if (fabs (dbu - layout.dbu ()) > 1e-6) {
    if (mp_view->manager ()) {
      mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Change layout's database unit")));
    }
    layout.dbu (dbu);
    if (mp_view->manager ()) {
      mp_view->manager ()->commit ();
    }
  }

  //  get the selected technology name
  std::string technology;
  int technology_index = tech_cbx->currentIndex ();
  const db::Technology *tech = 0;
  if (technology_index >= 0 && technology_index < (int) db::Technologies::instance ()->technologies ()) {
    tech = &(db::Technologies::instance ()->begin () [technology_index]);
    technology = tech->name ();
  }

  if (tech) {

    bool tech_has_changed = (technology != m_handles [m_index]->tech_name ());
    m_handles [m_index]->set_tech_name (technology);

    std::string lyp_file = tech->eff_layer_properties_file ();
    if (tech_has_changed && ! lyp_file.empty ()) {

      //  if the new technology has a layer properties file attached, ask whether to load it
      if (QMessageBox::question (this, QObject::tr ("Load Layer Properties File"), 
                                       tl::to_qstring (tl::to_string (QObject::tr ("The new technology specifies a layer properties file (")) + lyp_file + tl::to_string (QObject::tr (").\nLoad the new layer properties file?"))),
                                       QMessageBox::Yes, QMessageBox::No) == QMessageBox::Yes) {

        //  Interpolate the layer properties file name and load the file
        tl::Eval expr;
        expr.set_var ("layoutfile", m_handles [m_index]->filename ());
        lyp_file = expr.interpolate (lyp_file);
        mp_view->load_layer_props (lyp_file);

      }

    }

  }
}

void 
LayoutPropertiesForm::prop_pb_clicked ()
{
  if (m_index >= int (m_handles.size ()) || m_index < 0) {
    return;
  }

  db::Layout &layout = m_handles [m_index]->layout ();
  db::properties_id_type prop_id = layout.prop_id ();

  lay::UserPropertiesForm props_form (this);
  if (props_form.show (mp_view, m_index, prop_id, layout.begin_meta (), layout.end_meta ())) {

    if (mp_view->manager ()) {
      mp_view->manager ()->transaction (tl::to_string (QObject::tr ("Edit layout's user properties")));
    }
    layout.prop_id (prop_id);
    if (mp_view->manager ()) {
      mp_view->manager ()->commit ();
    }

  }
}

void
LayoutPropertiesForm::layout_selected (int index)
{
BEGIN_PROTECTED
  if (index == m_index) {
    return;
  }

  if (m_index >= 0) {
    try {
      commit ();
    } catch (...) {
      layout_cbx->setCurrentIndex (m_index);
      throw;
    }
    m_index = -1;
  }

  if (index >= int (m_handles.size ()) || index < 0) {
    return;
  }

  m_index = index;

  const db::Layout &layout = m_handles [index]->layout ();

  tech_cbx->clear ();
  unsigned int technology_index = 0;
  for (db::Technologies::const_iterator t = db::Technologies::instance ()->begin (); t != db::Technologies::instance ()->end (); ++t, ++technology_index) {

    tech_cbx->addItem (tl::to_qstring (t->get_display_string ()));
    if (t->name () == m_handles [index]->tech_name ()) {
      tech_cbx->setCurrentIndex (technology_index);
    }

  }

  dbu_le->setText (tl::to_qstring (tl::to_string (layout.dbu ())));

END_PROTECTED
}

}

#endif
