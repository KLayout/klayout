
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

static const size_t max_entries = 100;

void
RecentConfigurationPage::init ()
{
  QVBoxLayout *ly = new QVBoxLayout (this);
  ly->setMargin (0);

  mp_tree_widget = new QTreeWidget (this);
  mp_tree_widget->setRootIsDecorated (false);
  mp_tree_widget->setUniformRowHeights (true);
  ly->addWidget (mp_tree_widget);

  mp_tree_widget->setColumnCount (int (m_cfg.size ()));

  QStringList column_labels;
  for (std::list<ConfigurationDescriptor>::const_iterator c = m_cfg.begin (); c != m_cfg.end (); ++c) {
    column_labels << tl::to_qstring (c->title);
  }
  mp_tree_widget->setHeaderLabels (column_labels);

  update_list (get_stored_values ());
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
  tl::Extractor ex (serialized_list.c_str ());
  while (! ex.at_end ()) {

    values.push_back (std::vector<std::string> ());
    while (! ex.at_end () && ! ex.test (";")) {
      values.back ().push_back (std::string ());
      ex.read_word_or_quoted (values.back ().back ());
      ex.test (",");
    }

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

void
render_to (QTreeWidgetItem *item, int column, const std::string &v, RecentConfigurationPage::ConfigurationRendering rendering)
{
  //  store original value
  item->setData (column, Qt::UserRole, tl::to_qstring (v));

  // @@@ rendering
  item->setText (column, tl::to_qstring (v));

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

}
