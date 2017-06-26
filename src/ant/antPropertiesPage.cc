
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2017 Matthias Koefferlein

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


#include "antPropertiesPage.h"
#include "layLayoutView.h"
#include "layQtTools.h"

namespace ant
{

// -------------------------------------------------------------------------
//  PropertiesPage implementation

PropertiesPage::PropertiesPage (ant::Service *rulers, QWidget *parent)
  : lay::PropertiesPage (parent, rulers), mp_rulers (rulers), m_enable_cb_callback (true)
{
  mp_rulers->get_selection (m_selection);
  m_pos = m_selection.begin ();

  setupUi (this);

  lay::activate_help_links (help_label);

  mp_rulers->clear_highlights ();
}

PropertiesPage::~PropertiesPage ()
{
  mp_rulers->restore_highlights ();
}

void 
PropertiesPage::back ()
{
  m_pos = m_selection.end ();
}

void 
PropertiesPage::front ()
{
  m_pos = m_selection.begin ();
}

const ant::Object &
PropertiesPage::current () const
{
  const ant::Object *ruler = dynamic_cast <const ant::Object *> ((*m_pos)->ptr ());
  return *ruler;
}

bool 
PropertiesPage::at_begin () const
{
  return (m_pos == m_selection.begin ());
}

bool 
PropertiesPage::at_end () const
{
  return (m_pos == m_selection.end ());
}

void 
PropertiesPage::operator-- ()
{
  --m_pos;
}

void 
PropertiesPage::operator++ ()
{
  ++m_pos;
}

void
PropertiesPage::leave ()
{
  mp_rulers->clear_highlights ();
}

void 
PropertiesPage::update ()
{
  mp_rulers->highlight (std::distance (m_selection.begin (), m_pos));

  fmt_le->setText (tl::to_qstring (current ().fmt ()));
  fmt_x_le->setText (tl::to_qstring (current ().fmt_x ()));
  fmt_y_le->setText (tl::to_qstring (current ().fmt_y ()));
  style_cb->setCurrentIndex (current ().style ());
  outline_cb->setCurrentIndex (current ().outline ());

  x1->setText (tl::to_qstring (tl::micron_to_string (current ().p1 ().x ())));
  x1->setCursorPosition (0);
  x2->setText (tl::to_qstring (tl::micron_to_string (current ().p2 ().x ())));
  x2->setCursorPosition (0);
  y1->setText (tl::to_qstring (tl::micron_to_string (current ().p1 ().y ())));
  y1->setCursorPosition (0);
  y2->setText (tl::to_qstring (tl::micron_to_string (current ().p2 ().y ())));
  y2->setCursorPosition (0);

  main_position->setCurrentIndex (current ().main_position ());
  main_xalign->setCurrentIndex (current ().main_xalign ());
  main_yalign->setCurrentIndex (current ().main_yalign ());
  xlabel_xalign->setCurrentIndex (current ().xlabel_xalign ());
  xlabel_yalign->setCurrentIndex (current ().xlabel_yalign ());
  ylabel_xalign->setCurrentIndex (current ().ylabel_xalign ());
  ylabel_yalign->setCurrentIndex (current ().ylabel_yalign ());

  double sx = (current ().p2 ().x () - current ().p1 ().x ());
  double sy = (current ().p2 ().y () - current ().p1 ().y ());
  dx->setText (tl::to_qstring (tl::micron_to_string (sx)));
  dx->setCursorPosition (0);
  dy->setText (tl::to_qstring (tl::micron_to_string (sy)));
  dy->setCursorPosition (0);
  dd->setText (tl::to_qstring (tl::micron_to_string (sqrt (sx * sx + sy * sy))));
  dd->setCursorPosition (0);
}

bool 
PropertiesPage::readonly ()
{
  return false;
}

void 
PropertiesPage::apply ()
{
  double dx1 = current ().p1 ().x (), dy1 = current ().p1 ().y ();
  double dx2 = current ().p2 ().x (), dy2 = current ().p2 ().y ();

  //  only adjust the values if the text has changed
  if (tl::to_qstring (tl::micron_to_string (dx1)) != x1->text ()) {
    tl::from_string (tl::to_string (x1->text ()), dx1);
  }
  if (tl::to_qstring (tl::micron_to_string (dx2)) != x2->text ()) {
    tl::from_string (tl::to_string (x2->text ()), dx2);
  }
  if (tl::to_qstring (tl::micron_to_string (dy1)) != y1->text ()) {
    tl::from_string (tl::to_string (y1->text ()), dy1);
  }
  if (tl::to_qstring (tl::micron_to_string (dy2)) != y2->text ()) {
    tl::from_string (tl::to_string (y2->text ()), dy2);
  }

  std::string fmt = tl::to_string (fmt_le->text ());
  std::string fmt_x = tl::to_string (fmt_x_le->text ());
  std::string fmt_y = tl::to_string (fmt_y_le->text ());
  Object::style_type style = Object::style_type (style_cb->currentIndex ());
  Object::outline_type outline = Object::outline_type (outline_cb->currentIndex ());

  ant::Object ruler (db::DPoint (dx1, dy1), db::DPoint (dx2, dy2), current ().id (), fmt_x, fmt_y, fmt, style, outline, current ().snap (), current ().angle_constraint ());

  ruler.set_main_position (Object::position_type (main_position->currentIndex ()));
  ruler.set_main_xalign (Object::alignment_type (main_xalign->currentIndex ()));
  ruler.set_main_yalign (Object::alignment_type (main_yalign->currentIndex ()));
  ruler.set_xlabel_xalign (Object::alignment_type (xlabel_xalign->currentIndex ()));
  ruler.set_xlabel_yalign (Object::alignment_type (xlabel_yalign->currentIndex ()));
  ruler.set_ylabel_xalign (Object::alignment_type (ylabel_xalign->currentIndex ()));
  ruler.set_ylabel_yalign (Object::alignment_type (ylabel_yalign->currentIndex ()));

  ruler.set_category (current ().category ());

  mp_rulers->change_ruler (*m_pos, ruler);
}

}

