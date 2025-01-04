
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2025 Matthias Koefferlein

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

#include "laySelectStippleForm.h"
#include "ui_SelectStippleForm.h"

#include "tlString.h"

#include <QBitmap>

#include <algorithm>

namespace lay
{

SelectStippleForm::SelectStippleForm (QWidget *parent, const lay::DitherPattern &pattern, bool include_nil)
  : QDialog (parent), m_selected (-1), m_pattern (pattern), m_include_nil (include_nil)
{
  mp_ui = new Ui::SelectStippleForm ();

  mp_ui->setupUi (this);

  mp_ui->stipple_items->setUniformItemSizes (true);

  update ();

  connect (mp_ui->stipple_items, SIGNAL (currentItemChanged(QListWidgetItem*, QListWidgetItem*)), 
           this, SLOT (sel_changed (QListWidgetItem *, QListWidgetItem *)));
}

SelectStippleForm::~SelectStippleForm ()
{
  delete mp_ui;
  mp_ui = 0;
}

void 
SelectStippleForm::set_selected (int selected)
{
  if (selected != m_selected) {
    m_selected = selected;
    mp_ui->stipple_items->setCurrentRow (m_include_nil ? (selected < 0 ? 0 : selected + 1) : selected);
  }
}

namespace {
  struct display_order
  {
    bool operator () (lay::DitherPattern::iterator a, lay::DitherPattern::iterator b)
    {
      return a->order_index () < b->order_index ();
    }
  };
}

void 
SelectStippleForm::update ()
{
#if QT_VERSION >= 0x050000
  double dpr = devicePixelRatio ();
#else
  double dpr = 1.0;
#endif

  mp_ui->stipple_items->clear ();

  QSize icon_size = mp_ui->stipple_items->iconSize ();

  if (m_include_nil) {
    new QListWidgetItem (QObject::tr ("None"), mp_ui->stipple_items);
  }

  std::vector <lay::DitherPattern::iterator> iters; 
  for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
    iters.push_back (i);
  }
  std::sort (iters.begin (), iters.end (), display_order ());

  //  fill the list of stipple items
  for (lay::DitherPattern::iterator i = m_pattern.begin (); i != m_pattern.begin_custom (); ++i) {

    std::string name (i->name ());
    if (name.empty ()) {
      name = tl::sprintf ("#%d", std::distance (m_pattern.begin (), i));
    }

    const lay::DitherPatternInfo &dp_info = i->scaled (dpr);
    QBitmap bitmap = dp_info.get_bitmap (icon_size.width () * dpr, icon_size.height () * dpr, dpr);
#if QT_VERSION >= 0x050000
    bitmap.setDevicePixelRatio (dpr);
#endif
    new QListWidgetItem (QIcon (bitmap), tl::to_qstring (name), mp_ui->stipple_items);

  }

  for (std::vector <lay::DitherPattern::iterator>::const_iterator i = iters.begin (); i != iters.end (); ++i) {

    if ((*i)->order_index () > 0) {

      std::string name ((*i)->name ());
      if (name.empty ()) {
        name = tl::sprintf ("custom #%d", (*i)->order_index ());
      }

      const lay::DitherPatternInfo &dp_info = (*i)->scaled (dpr);
      QBitmap bitmap = dp_info.get_bitmap (icon_size.width () * dpr, icon_size.height () * dpr, dpr);
#if QT_VERSION >= 0x050000
      bitmap.setDevicePixelRatio (dpr);
#endif
      new QListWidgetItem (QIcon (bitmap), tl::to_qstring (name), mp_ui->stipple_items);

    }

  }
}

void 
SelectStippleForm::sel_changed (QListWidgetItem *citem, QListWidgetItem *)
{
  int row = mp_ui->stipple_items->row (citem);
  if (m_include_nil) {
    --row;
  }

  if (row >= int (std::distance (m_pattern.begin (), m_pattern.begin_custom ()))) {
    for (lay::DitherPattern::iterator i = m_pattern.begin_custom (); i != m_pattern.end (); ++i) {
      if (int (i->order_index ()) - 1 + std::distance (m_pattern.begin (), m_pattern.begin_custom ()) == row) {
        m_selected = std::distance (m_pattern.begin (), i);
        return;
      }
    }
  } else if (row >= 0) {
    m_selected = row;
  } else {
    m_selected = -1;
  }
}

}

#endif
