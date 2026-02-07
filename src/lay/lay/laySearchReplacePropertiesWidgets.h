
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


#ifndef HDR_SearchReplacePropertiesWidgets
#define HDR_SearchReplacePropertiesWidgets

#include <QWidget>
#include <QStackedWidget>

#include <string>

namespace db
{
  class Layout;
}

namespace lay
{

class Dispatcher;

/**
 *  @brief A base class for the search and replace properties widgets
 */
class SearchReplacePropertiesWidget
  : public QWidget
{
public:
  SearchReplacePropertiesWidget (QWidget *parent)
    : QWidget (parent)
  { }

  virtual void restore_state (const std::string &pfx, lay::Dispatcher *config_root) const = 0;
  virtual void save_state (const std::string &pfx, lay::Dispatcher *config_root) const = 0;
};

/**
 *  @brief A base class for a find properties widget
 */
class SearchPropertiesWidget
  : public SearchReplacePropertiesWidget
{
public:
  SearchPropertiesWidget (QWidget *parent)
    : SearchReplacePropertiesWidget (parent)
  { }

  virtual std::string search_expression (const std::string &cell_expr) const = 0;
  virtual std::string description () const = 0;
};

/**
 *  @brief A base class for a replace properties widget
 */
class ReplacePropertiesWidget
  : public SearchReplacePropertiesWidget
{
public:
  ReplacePropertiesWidget (QWidget *parent)
    : SearchReplacePropertiesWidget (parent)
  { }

  virtual std::string replace_expression () const = 0;
};

/**
 *  @brief Fill the given stack widget with pairs of items plus properties widget
 */
void fill_find_pages (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index);

/**
 *  @brief Fill the given stack widget with pairs of items plus properties widget
 */
void fill_replace_pages (QStackedWidget *sw, lay::LayoutViewBase *view, int cv_index);

/**
 *  @brief Get an object id from a page index
 *
 *  Converts the page index to an object id (used for persisting the state for example)
 */
std::string index_to_find_object_id (int index);

/**
 *  @brief Get index from object id
 *
 *  Converts the page index for a given object id (see index_to_object_id)
 */
int index_from_find_object_id (const std::string &id);

}

#endif

