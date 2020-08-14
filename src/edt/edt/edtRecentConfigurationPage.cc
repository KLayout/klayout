
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

#include "edtRecentConfigurationPage.h"
#include "layDispatcher.h"

#include <QVBoxLayout>
#include <QHeaderView>

namespace edt
{

void
RecentConfigurationPage::init ()
{
  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setMargin (0);

  mp_tree_widget = new QTreeWidget (this);
  ly->addWidget (mp_tree_widget);

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

void
render_to (QTreeWidgetItem *item, int column, const std::string &v, RecentConfigurationPage::ConfigurationRendering rendering)
{

  // @@@ rendering
  item->setText (column, tl::to_qstring (v));

}

void
RecentConfigurationPage::update_list ()
{
  int row = 0;
  for (std::list<std::vector<std::string> >::const_iterator v = m_stored_values.begin (); v != m_stored_values.end (); ++v, ++row) {

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
        render_to (item, column, (*v) [column], c->rendering);
      }
    }

  }

  while (mp_tree_widget->topLevelItemCount () > row) {
    delete mp_tree_widget->takeTopLevelItem (row);
  }

  mp_tree_widget->header ()->resizeSections (QHeaderView::ResizeToContents);
}

void
RecentConfigurationPage::commit_recent (lay::Dispatcher *root)
{
  std::vector<std::string> values;
  values.reserve (m_cfg.size ());
  for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c) {
    values.push_back (root->config_get (c->cfg_name));
  }

  for (std::list<std::vector<std::string> >::iterator v = m_stored_values.begin (); v != m_stored_values.end (); ++v) {
    if (*v == values) {
      m_stored_values.erase (v);
      break;
    }
  }

  m_stored_values.push_front (values);

  update_list ();
}

}
