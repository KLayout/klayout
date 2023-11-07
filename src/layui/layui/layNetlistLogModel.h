
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

#ifndef HDR_layNetlistCrossReference
#define HDR_layNetlistCrossReference

#include "layuiCommon.h"
#include "dbNetlistCrossReference.h"
#include "dbLayoutToNetlist.h"

#include <QAbstractItemModel>
#include <QIcon>

namespace lay
{

/**
 *  @brief An indexed netlist model for the netlist cross-reference log
 */
class LAYUI_PUBLIC NetlistLogModel
  : public QAbstractItemModel
{
public:
  NetlistLogModel (QWidget *parent, const db::NetlistCrossReference *cross_ref, const db::LayoutToNetlist *l2n);

  virtual bool hasChildren (const QModelIndex &parent) const;
  virtual QModelIndex index (int row, int column, const QModelIndex &parent) const;
  virtual QModelIndex parent (const QModelIndex &child) const;
  virtual int rowCount (const QModelIndex &parent) const;
  virtual int columnCount (const QModelIndex &parent) const;
  virtual QVariant data (const QModelIndex &index, int role) const;
  virtual QVariant headerData (int section, Qt::Orientation orientation, int role) const;

  const db::LogEntryData *log_entry (const QModelIndex &index) const;

  static QIcon icon_for_severity (db::Severity severity);

  db::Severity max_severity () const
  {
    return m_max_severity;
  }

private:
  typedef std::pair<std::pair<const db::Circuit *, const db::Circuit *>, const db::NetlistCrossReference::PerCircuitData::log_entries_type *> circuit_entry;
  std::vector<circuit_entry> m_circuits;
  const db::NetlistCrossReference::PerCircuitData::log_entries_type *mp_lvsdb_messages;
  const db::LayoutToNetlist::log_entries_type *mp_l2n_messages;
  int m_global_entries;
  db::Severity m_max_severity;
};

}

#endif

#endif  //  defined(HAVE_QT)
