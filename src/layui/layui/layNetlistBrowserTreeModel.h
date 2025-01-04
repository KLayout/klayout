
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

#ifndef HDR_layNetlistBrowserTreeModel
#define HDR_layNetlistBrowserTreeModel

#include "layColorPalette.h"
#include "layuiCommon.h"

#include "dbLayoutToNetlist.h"
#include "dbLayoutVsSchematic.h"

#include <QAbstractItemModel>
#include <QColor>

#include <map>
#include <memory>

class QTreeView;

namespace lay
{

class IndexedNetlistModel;
class NetlistObjectsPath;

// ----------------------------------------------------------------------------------
//  NetlistBrowserTreeModel definition

/**
 *  @brief The model for the circuit hierarchy tree
 */
class LAYUI_PUBLIC NetlistBrowserTreeModel
  : public QAbstractItemModel
{
Q_OBJECT

public:
  NetlistBrowserTreeModel (QWidget *parent, db::Netlist *netlist);
  NetlistBrowserTreeModel (QWidget *parent, db::LayoutToNetlist *l2ndb);
  NetlistBrowserTreeModel (QWidget *parent, db::LayoutVsSchematic *lvsdb);
  ~NetlistBrowserTreeModel ();

  virtual int columnCount (const QModelIndex &parent) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual Qt::ItemFlags flags (const QModelIndex &index) const;
  virtual bool hasChildren (const QModelIndex &parent) const;
  virtual QVariant headerData (int section, Qt::Orientation orientation, int role) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &index) const;
  virtual int rowCount (const QModelIndex &parent) const;

  QModelIndex index_from_id (void *id, int column) const;

  int status_column () const
  {
    return m_status_column;
  }

  std::pair<const db::Circuit *, const db::Circuit *> circuits_from_index (const QModelIndex &index) const;
  QModelIndex index_from_circuits (const std::pair<const db::Circuit *, const db::Circuit *> &circuits) const;
  QModelIndex index_from_netpath (const NetlistObjectsPath &path) const;

private:
  NetlistBrowserTreeModel (const NetlistBrowserTreeModel &);
  NetlistBrowserTreeModel &operator= (const NetlistBrowserTreeModel &);

  QString text (const QModelIndex &index) const;
  QVariant tooltip (const QModelIndex &index) const;
  QString search_text (const QModelIndex &index) const;
  db::NetlistCrossReference::Status status (const QModelIndex &index) const;
  std::pair<std::pair<const db::Circuit *, const db::Circuit *>, std::pair<db::NetlistCrossReference::Status, std::string> > cp_status_from_index(const QModelIndex &index, size_t &nprod, size_t &nlast, size_t &nnlast) const;
  void build_circuits_to_index (size_t nprod, const std::pair<const db::Circuit *, const db::Circuit *> &circuits, IndexedNetlistModel *model, const QModelIndex &index, std::map<std::pair<const db::Circuit *, const db::Circuit *>, QModelIndex> &map) const;

  db::LayoutToNetlist *mp_l2ndb;
  db::LayoutVsSchematic *mp_lvsdb;
  std::unique_ptr<IndexedNetlistModel> mp_indexer;
  mutable std::map<std::pair<const db::Circuit *, const db::Circuit *>, QModelIndex> m_circuits_to_index;
  int m_object_column;
  int m_status_column;
};

} // namespace lay

#endif

#endif  //  defined(HAVE_QT)
