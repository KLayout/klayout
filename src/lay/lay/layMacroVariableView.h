
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2024 Matthias Koefferlein

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


#ifndef HDR_layMacroVariableView
#define HDR_layMacroVariableView

#include "layCommon.h"
#include "gsiInspector.h"

#include <QTreeWidget>

#include <memory>

namespace tl
{
  class Variant;
}

namespace lay
{

/**
 *  @brief Utility: convert the variant into a nice string
 */
QString pretty_print (const tl::Variant &v);

/**
 *  @brief Provides a QTreeWidget that synchronizes with a gsi::Inspector object
 */
class LAY_PUBLIC MacroVariableView
  : public QTreeWidget
{
Q_OBJECT 

public:
  /**
   *  @brief Constructor
   */
  MacroVariableView (QWidget *parent = 0);

  /**
   *  @brief Attach an inspector to the view
   *
   *  The view will take over ownership over the inspector object.
   */
  void set_inspector (gsi::Inspector *inspector);

public slots:
  /**
   *  @brief Sets or resets the "show all" flag
   *
   *  If the "show all" flag is set, items with "IfRequested" visibility are shown too.
   */
  void set_show_all (bool show_all);

private slots:
  void expanded (QTreeWidgetItem *item);

private:
  void sync (QTreeWidgetItem *item, gsi::Inspector *inspector, bool fresh);
  void sync_item (QTreeWidgetItem *parent, gsi::Inspector *inspector, const QString &key, size_t index, int pos, bool fresh);
  void sync (bool fresh);

  std::unique_ptr<gsi::Inspector> mp_inspector;
  bool m_show_all;
};

}

#endif

