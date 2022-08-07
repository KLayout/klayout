
/*

  KLayout Layout Viewer
  Copyright (C) 2006-2022 Matthias Koefferlein

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

#include "layNetlistLogModel.h"

#include <QIcon>
#include <QFont>
#include <QColor>

namespace lay
{

namespace {

  template <class Obj>
  struct sort_single_by_name
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      return a->name () < b->name ();
    }
  };

  template <class Obj, class SortBy>
  struct sort_pair
  {
    bool operator() (const std::pair<const Obj *, const Obj *> &a, const std::pair<const Obj *, const Obj *> &b) const
    {
      SortBy order;
      if (order (a.first, b.first)) {
        return true;
      } else if (order (b.first, a.first)) {
        return false;
      }
      return order (a.second, b.second);
    }
  };

  struct CircuitsCompareByName
  {
    bool operator() (const std::pair<std::pair<const db::Circuit *, const db::Circuit *>, const db::NetlistCrossReference::PerCircuitData *> &a,
                     const std::pair<std::pair<const db::Circuit *, const db::Circuit *>, const db::NetlistCrossReference::PerCircuitData *> &b) const
    {
      return sort_pair<db::Circuit, sort_single_by_name<db::Circuit> > () (a.first, b.first);
    }
  };

}

const std::string var_sep (" \u21D4 ");

NetlistLogModel::NetlistLogModel (const db::NetlistCrossReference *cross_ref)
{
  tl_assert (cross_ref->netlist_a () != 0);
  tl_assert (cross_ref->netlist_b () != 0);

  for (auto i = cross_ref->begin_circuits (); i != cross_ref->end_circuits (); ++i) {
    const db::NetlistCrossReference::PerCircuitData *pcd = cross_ref->per_circuit_data_for (*i);
    if (pcd && i->first && i->second && ! pcd->log_entries.empty ()) {
      m_circuits.push_back (std::make_pair (*i, pcd));
    }
  }

  std::sort (m_circuits.begin (), m_circuits.end (), CircuitsCompareByName ());
}

bool
NetlistLogModel::hasChildren (const QModelIndex &parent) const
{
  return (parent.isValid () || ! m_circuits.empty ());
}

QModelIndex
NetlistLogModel::index (int row, int column, const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return createIndex (row, column, quintptr (0));
  } else {
    return createIndex (row, column, quintptr (& m_circuits [parent.row ()].second->log_entries [row]));
  }
}

int
NetlistLogModel::rowCount (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return int (m_circuits.size ());
  } else if (parent.row () >= 0 && parent.row () < int (m_circuits.size ())) {
    return int (m_circuits [parent.row ()].second->log_entries.size ());
  } else {
    return 0;
  }
}

int
NetlistLogModel::columnCount (const QModelIndex & /*parent*/) const
{
  return 2;
}

QVariant
NetlistLogModel::data (const QModelIndex &index, int role) const
{
  if (role == Qt::DecorationRole) {

    if (index.parent ().isValid ()) {
      auto *le = (const db::NetlistCrossReference::LogEntryData *) index.internalPointer ();
      if (! le) {
        //  ignore
      } else if (le->severity == db::NetlistCrossReference::Error) {
        return QIcon (QString::fromUtf8 (":/error_16.png"));
      } else if (le->severity == db::NetlistCrossReference::Warning) {
        return QIcon (QString::fromUtf8 (":/warn_16.png"));
      } else if (le->severity == db::NetlistCrossReference::Info) {
        return QIcon (QString::fromUtf8 (":/info_16.png"));
      } else {
        return QIcon (QString::fromUtf8 (":/empty_16.png"));
      }
    }

  } else if (role == Qt::DisplayRole) {

    if (index.parent ().isValid ()) {
      auto *le = (const db::NetlistCrossReference::LogEntryData *) index.internalPointer ();
      if (le) {
        return QVariant (tl::to_qstring (le->msg));
      }
    } else if (index.row () >= 0 && index.row () < int (m_circuits.size ())) {
      const std::pair<const db::Circuit *, const db::Circuit *> &cp = m_circuits [index.row ()].first;
      if (cp.first->name () != cp.second->name ()) {
        return QVariant (tl::to_qstring (cp.first->name () + var_sep + cp.second->name ()));
      } else {
        return QVariant (tl::to_qstring (cp.first->name ()));
      }
    }

  } else if (role == Qt::FontRole) {

    if (index.parent ().isValid ()) {
      auto *le = (const db::NetlistCrossReference::LogEntryData *) index.internalPointer ();
      if (le && le->severity == db::NetlistCrossReference::Error) {
        QFont f;
        f.setBold (true);
        return QVariant (f);
      }
    }

  } else if (role == Qt::ForegroundRole) {

    if (index.parent ().isValid ()) {
      auto *le = (const db::NetlistCrossReference::LogEntryData *) index.internalPointer ();
      if (!le) {
        //  ignore
      } else if (le->severity == db::NetlistCrossReference::Error) {
        return QColor (255, 0, 0);
      } else if (le->severity == db::NetlistCrossReference::Warning) {
        return QColor (0, 0, 255);
      }
    }

  }

  return QVariant ();
}

QVariant
NetlistLogModel::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole && section == 1) {
    return QVariant (tr ("Message"));
  } else {
    return QVariant ();
  }
}

}

#endif
