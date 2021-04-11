
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2021 Matthias Koefferlein

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


#include "layPropertiesDialog.h"

#include "tlLog.h"
#include "layEditable.h"
#include "layProperties.h"
#include "tlExceptions.h"

#include <QStackedLayout>

namespace lay
{

PropertiesDialog::PropertiesDialog (QWidget * /*parent*/, db::Manager *manager, lay::Editables *editables)
  : QDialog (0 /*parent*/),
    mp_manager (manager), mp_editables (editables), m_index (-1), m_auto_applied (false), m_transaction_id (0)
{
  mp_editables->enable_edits (false);

  setObjectName (QString::fromUtf8 ("properties_dialog"));

  Ui::PropertiesDialog::setupUi (this);

  mp_stack = new QStackedLayout;

  for (lay::Editables::iterator e = mp_editables->begin (); e != mp_editables->end (); ++e) {
    mp_properties_pages.push_back (e->properties_page (mp_manager, content_frame));
    if (mp_properties_pages.back ()) {
      mp_stack->addWidget (mp_properties_pages.back ());
      connect (mp_properties_pages.back (), SIGNAL (edited ()), this, SLOT (apply ()));
    }
  }

  //  Necessary to maintain the page order for UI regression testing of 0.18 vs. 0.19 (because tl::Collection has changed to order) ..
  std::reverse (mp_properties_pages.begin (), mp_properties_pages.end ());

  //  Add a label as a dummy 
  QLabel *dummy = new QLabel (QObject::tr ("No object with properties to display"), content_frame);
  dummy->setAlignment (Qt::AlignHCenter | Qt::AlignVCenter);
  mp_stack->addWidget (dummy);

  content_frame->setLayout (mp_stack);

  //  disable the apply button for first ..
  apply_to_all_cbx->setEnabled (false);
  relative_cbx->setEnabled (false);
  ok_button->setEnabled (false);

  //  as a proposal, the start button can be enabled in most cases
  prev_button->setEnabled (true);

  //  count the total number of objects
  m_objects = mp_editables->selection_size ();
  m_current_object = 0;

  update_title ();

  //  look for next usable editable 
  while (m_index < int (mp_properties_pages.size ()) && 
         (m_index < 0 || mp_properties_pages [m_index] == 0 || mp_properties_pages [m_index]->at_end ())) {
    ++m_index;
  }

  prev_button->setEnabled (false);

  //  if at end disable the "Next" button and return (this may only happen at the first call)
  if (m_index >= int (mp_properties_pages.size ())) {

    next_button->setEnabled (false);
    mp_stack->setCurrentWidget (dummy);
    apply_to_all_cbx->setEnabled (false);
    apply_to_all_cbx->setChecked (false);
    relative_cbx->setEnabled (false);
    relative_cbx->setChecked (false);
    ok_button->setEnabled (false);

  } else {

    next_button->setEnabled (any_next ());
    mp_properties_pages [m_index]->update ();
    mp_stack->setCurrentWidget (mp_properties_pages [m_index]);
    apply_to_all_cbx->setEnabled (! mp_properties_pages [m_index]->readonly () && mp_properties_pages [m_index]->can_apply_to_all ());
    apply_to_all_cbx->setChecked (false);
    relative_cbx->setEnabled (apply_to_all_cbx->isEnabled () && apply_to_all_cbx->isChecked ());
    relative_cbx->setChecked (true);
    ok_button->setEnabled (! mp_properties_pages [m_index]->readonly ());

  }

  connect (ok_button, SIGNAL (clicked ()), this, SLOT (ok_pressed ()));
  connect (cancel_button, SIGNAL (clicked ()), this, SLOT (cancel_pressed ()));
  connect (prev_button, SIGNAL (clicked ()), this, SLOT (prev_pressed ()));
  connect (next_button, SIGNAL (clicked ()), this, SLOT (next_pressed ()));
}

PropertiesDialog::~PropertiesDialog ()
{
  disconnect ();
}

void 
PropertiesDialog::disconnect ()
{
  mp_editables->enable_edits (true);
  
  for (std::vector <lay::PropertiesPage *>::iterator p = mp_properties_pages.begin (); p != mp_properties_pages.end (); ++p) {
    delete *p;
  }
  mp_properties_pages.clear ();
}

void 
PropertiesDialog::next_pressed ()
{
BEGIN_PROTECTED

  if (! mp_properties_pages [m_index]->readonly ()) {
    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);
    mp_properties_pages [m_index]->apply ();
    m_transaction_id = t.id ();
  }

  //  advance the current entry
  ++(*mp_properties_pages [m_index]);

  //  look for next usable editable if at end
  if (mp_properties_pages [m_index]->at_end ()) {
    mp_properties_pages [m_index]->leave ();
    ++m_index;
    while (m_index < int (mp_properties_pages.size ()) && 
           (mp_properties_pages [m_index] == 0 || ! mp_properties_pages [m_index]->front_checked ())) {
      ++m_index;
    }
    //  because we checked that there are any further elements, this should not happen:
    if (m_index >= int (mp_properties_pages.size ())) {
      return;
    }
    mp_stack->setCurrentWidget (mp_properties_pages [m_index]);
  }

  ++m_current_object;
  update_title ();

  prev_button->setEnabled (true);
  next_button->setEnabled (any_next ());
  apply_to_all_cbx->setEnabled (! mp_properties_pages [m_index]->readonly () && mp_properties_pages [m_index]->can_apply_to_all ());
  relative_cbx->setEnabled (apply_to_all_cbx->isEnabled () && apply_to_all_cbx->isChecked ());
  ok_button->setEnabled (! mp_properties_pages [m_index]->readonly ());
  mp_properties_pages [m_index]->update ();

END_PROTECTED
}

void 
PropertiesDialog::prev_pressed ()
{
BEGIN_PROTECTED

  if (! mp_properties_pages [m_index]->readonly ()) {
    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);
    mp_properties_pages [m_index]->apply ();
    m_transaction_id = t.id ();
  }

  if (mp_properties_pages [m_index]->at_begin ()) {

    //  look for last usable editable if at end
    mp_properties_pages [m_index]->leave ();
    --m_index;
    while (m_index >= 0 && 
           (mp_properties_pages [m_index] == 0 || ! mp_properties_pages [m_index]->back_checked ())) {
      --m_index;
    }
    //  because we checked that there are any further elements, this should not happen:
    if (m_index < 0) {
      return;
    }
    mp_stack->setCurrentWidget (mp_properties_pages [m_index]);
   
  } 

  //  decrement the current entry
  --(*mp_properties_pages [m_index]);

  --m_current_object;
  update_title ();

  next_button->setEnabled (true);
  prev_button->setEnabled (any_prev ());
  apply_to_all_cbx->setEnabled (! mp_properties_pages [m_index]->readonly () && mp_properties_pages [m_index]->can_apply_to_all ());
  relative_cbx->setEnabled (apply_to_all_cbx->isEnabled () && apply_to_all_cbx->isChecked ());
  ok_button->setEnabled (! mp_properties_pages [m_index]->readonly ());
  mp_properties_pages [m_index]->update ();

END_PROTECTED
}

void
PropertiesDialog::update_title ()
{
  setWindowTitle (tl::to_qstring (tl::to_string (QObject::tr ("Object Properties - ")) + tl::to_string (m_current_object + 1) + tl::to_string (QObject::tr (" of ")) + tl::to_string (m_objects)));
}

bool
PropertiesDialog::any_next () const
{
  //  test-advance
  int index = m_index;
  ++(*mp_properties_pages [index]);
  if (mp_properties_pages [index]->at_end ()) {
    ++index;
    while (index < int (mp_properties_pages.size ()) && 
           (mp_properties_pages [index] == 0 || ! mp_properties_pages [index]->front_checked ())) {
      ++index;
    }
  }

  //  return true, if not at end
  bool ret = (index < int (mp_properties_pages.size ()));
  --(*mp_properties_pages [m_index]);
  return ret;
}

bool
PropertiesDialog::any_prev () const
{
  //  test-decrement
  int index = m_index;
  if (mp_properties_pages [index]->at_begin ()) {
    --index;
    while (index >= 0 && 
           (mp_properties_pages [index] == 0 || ! mp_properties_pages [index]->back_checked ())) {
      --index;
    }
  }

  //  return true, if not at the beginning
  return (index >= 0);
}

void
PropertiesDialog::apply ()
{
BEGIN_PROTECTED

  db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

  try {

    if (apply_to_all_cbx->isChecked () && mp_properties_pages [m_index]->can_apply_to_all ()) {
      mp_properties_pages [m_index]->apply_to_all (relative_cbx->isChecked ());
    } else {
      mp_properties_pages [m_index]->apply ();
    }
    mp_properties_pages [m_index]->update ();

  } catch (tl::Exception &) {
    //  we assume the page somehow indicates the error and does not apply the values
  }

  m_transaction_id = t.id ();

END_PROTECTED
}

void 
PropertiesDialog::cancel_pressed ()
{
  //  undo whatever we've done so far
  if (m_transaction_id > 0) {

    //  because undo does not maintain a valid selection we clear it
    mp_editables->clear_selection ();

    mp_manager->undo ();
    m_transaction_id = 0;

  }

  //  make sure that the property pages are no longer used ..
  disconnect ();
  //  close the dialog
  done (0);
}

void 
PropertiesDialog::ok_pressed ()
{
BEGIN_PROTECTED

  if (! mp_properties_pages [m_index]->readonly ()) {

    db::Transaction t (mp_manager, tl::to_string (QObject::tr ("Apply changes")), m_transaction_id);

    mp_properties_pages [m_index]->apply ();
    mp_properties_pages [m_index]->update ();

    m_transaction_id = t.id ();

  }

  //  make sure that the property pages are no longer used ..
  disconnect ();
  QDialog::accept ();

END_PROTECTED
}

void 
PropertiesDialog::reject ()
{
  //  make sure that the property pages are no longer used ..
  disconnect ();
  QDialog::reject ();
}

}

