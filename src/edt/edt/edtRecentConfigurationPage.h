
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

#ifndef HDR_edtRecentConfigurationPage
#define HDR_edtRecentConfigurationPage

#include "layEditorOptionsPage.h"
#include "tlObject.h"
#include "tlDeferredExecution.h"

#include <list>
#include <QTreeWidget>

namespace lay
{
  class LayoutViewBase;
}

namespace edt
{

class PCellParametersPage;

class EditorOptionsPages;

/**
 *  @brief The base class for a object properties page
 */
class RecentConfigurationPage
  : public lay::EditorOptionsPage
{
Q_OBJECT

public:
  enum ConfigurationRendering
  {
    Text = 0,
    Bool = 1,
    Double = 2,
    Int = 3,
    Layer = 4,
    PCellParameters = 5,
    CellLibraryName = 6,
    CellDisplayName = 7,
    ArrayFlag = 8,
    DoubleIfArray = 9,
    IntIfArray = 10
  };

  struct ConfigurationDescriptor
  {
    ConfigurationDescriptor (const std::string &_cfg_name, const std::string &_title, ConfigurationRendering _rendering)
      : cfg_name (_cfg_name), title (_title), rendering (_rendering)
    { }

    std::string cfg_name, title;
    ConfigurationRendering rendering;
  };

  template <class Iter>
  RecentConfigurationPage (lay::LayoutViewBase *view, lay::Dispatcher *dispatcher, const std::string &recent_cfg_name, Iter begin_cfg, Iter end_cfg)
    : EditorOptionsPage (view, dispatcher), m_recent_cfg_name (recent_cfg_name), m_cfg (begin_cfg, end_cfg), dm_update_list (this, &RecentConfigurationPage::update_list)
  {
    init ();
  }

  virtual ~RecentConfigurationPage ();

  virtual std::string title () const;
  virtual int order () const;
  virtual void apply (lay::Dispatcher * /*root*/) { }
  virtual void setup (lay::Dispatcher * /*root*/) { }
  virtual void commit_recent (lay::Dispatcher *root);

private slots:
  void item_clicked (QTreeWidgetItem *item);

private:
  std::string m_recent_cfg_name;
  std::list<ConfigurationDescriptor> m_cfg;
  QTreeWidget *mp_tree_widget;
  tl::DeferredMethod<RecentConfigurationPage> dm_update_list;

  void init ();
  void update_list (const std::list<std::vector<std::string> > &stored_values);
  void update_list ();
  std::list<std::vector<std::string> > get_stored_values () const;
  void set_stored_values (const std::list<std::vector<std::string> > &values) const;
  void render_to (QTreeWidgetItem *item, int column, const std::vector<std::string> &values, RecentConfigurationPage::ConfigurationRendering rendering);
  void layers_changed (int);
  virtual void technology_changed (const std::string &);
};

}

#endif

#endif
