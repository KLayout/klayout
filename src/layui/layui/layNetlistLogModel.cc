
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

#include "layNetlistLogModel.h"

#include <QIcon>
#include <QFont>
#include <QColor>
#include <QWidget>

namespace lay
{

namespace {

  template <class Obj>
  struct sort_single_by_name
  {
    inline bool operator() (const Obj *a, const Obj *b) const
    {
      if ((a != 0) != (b != 0)) {
        return (a != 0) < (b != 0);
      }
      if (! a) {
        return false;
      } else {
        return a->name () < b->name ();
      }
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
    bool operator() (const std::pair<std::pair<const db::Circuit *, const db::Circuit *>, const db::NetlistCrossReference::PerCircuitData::log_entries_type *> &a,
                     const std::pair<std::pair<const db::Circuit *, const db::Circuit *>, const db::NetlistCrossReference::PerCircuitData::log_entries_type *> &b) const
    {
      return sort_pair<db::Circuit, sort_single_by_name<db::Circuit> > () (a.first, b.first);
    }
  };

}

const std::string var_sep (" \u21D4 ");

NetlistLogModel::NetlistLogModel (QWidget *parent, const db::NetlistCrossReference *cross_ref, const db::LayoutToNetlist *l2n)
  : QAbstractItemModel (parent), m_max_severity (db::NoSeverity)
{
  tl_assert (! cross_ref || cross_ref->netlist_a () != 0);
  tl_assert (! cross_ref || cross_ref->netlist_b () != 0);

  mp_lvsdb_messages = cross_ref ? &cross_ref->other_log_entries () : 0;
  if (mp_lvsdb_messages) {
    for (auto l = mp_lvsdb_messages->begin (); l != mp_lvsdb_messages->end (); ++l) {
      m_max_severity = std::max (m_max_severity, l->severity ());
    }
  }

  mp_l2n_messages = l2n ? &l2n->log_entries () : 0;
  if (mp_l2n_messages) {
    for (auto l = mp_l2n_messages->begin (); l != mp_l2n_messages->end (); ++l) {
      m_max_severity = std::max (m_max_severity, l->severity ());
    }
  }

  m_global_entries = int ((mp_lvsdb_messages ? mp_lvsdb_messages->size () : 0) + (mp_l2n_messages ? mp_l2n_messages->size () : 0));

  if (cross_ref) {
    for (auto i = cross_ref->begin_circuits (); i != cross_ref->end_circuits (); ++i) {
      const db::NetlistCrossReference::PerCircuitData *pcd = cross_ref->per_circuit_data_for (*i);
      if (pcd && (i->first || i->second) && ! pcd->log_entries.empty ()) {
        for (auto l = pcd->log_entries.begin (); l != pcd->log_entries.end (); ++l) {
          m_max_severity = std::max (m_max_severity, l->severity ());
        }
        m_circuits.push_back (std::make_pair (*i, &pcd->log_entries));
      }
    }
  }

  std::sort (m_circuits.begin (), m_circuits.end (), CircuitsCompareByName ());
}

bool
NetlistLogModel::hasChildren (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return m_global_entries > 0 || ! m_circuits.empty ();
  } else if (! parent.parent ().isValid ()) {
    return parent.row () >= m_global_entries;
  } else {
    return false;
  }
}

QModelIndex
NetlistLogModel::index (int row, int column, const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return createIndex (row, column, (void *) (0));
  } else {
    return createIndex (row, column, (void *) (& m_circuits [parent.row () - m_global_entries]));
  }
}

QModelIndex
NetlistLogModel::parent (const QModelIndex &child) const
{
  if (child.internalPointer () == (void *) 0) {
    return QModelIndex ();
  } else {
    const circuit_entry *ce = (const circuit_entry *) child.internalPointer ();
    return createIndex (int (ce - & m_circuits.front ()) + m_global_entries, child.column (), (void *) (0));
  }
}

int
NetlistLogModel::rowCount (const QModelIndex &parent) const
{
  if (! parent.isValid ()) {
    return int (m_circuits.size ()) + m_global_entries;
  } else if (parent.parent ().isValid ()) {
    return 0;
  } else if (parent.row () >= m_global_entries && parent.row () < int (m_circuits.size ()) + m_global_entries) {
    return int (m_circuits [parent.row () - m_global_entries].second->size ());
  } else {
    return 0;
  }
}

int
NetlistLogModel::columnCount (const QModelIndex & /*parent*/) const
{
  return 1;
}

QIcon
NetlistLogModel::icon_for_severity (db::Severity severity)
{
  if (severity == db::Error) {
    return QIcon (QString::fromUtf8 (":/error_16px.png"));
  } else if (severity == db::Warning) {
    return QIcon (QString::fromUtf8 (":/warn_16px.png"));
  } else if (severity == db::Info) {
    return QIcon (QString::fromUtf8 (":/info_16px.png"));
  } else {
    return QIcon ();
  }
}

const db::LogEntryData *
NetlistLogModel::log_entry (const QModelIndex &index) const
{
  const db::LogEntryData *le = 0;

  if (index.parent ().isValid ()) {
    const circuit_entry *ce = (const circuit_entry *) index.internalPointer ();
    if (ce) {
      le = (ce->second->begin () + index.row ()).operator-> ();
    }
  } else if (index.row () < m_global_entries) {
    int n_l2n = int (mp_l2n_messages ? mp_l2n_messages->size () : 0);
    if (index.row () < n_l2n) {
      le = (mp_l2n_messages->begin () + index.row ()).operator-> ();
    } else {
      le = (mp_lvsdb_messages->begin () + (index.row () - n_l2n)).operator-> ();
    }
  }

  return le;
}

QVariant
NetlistLogModel::data (const QModelIndex &index, int role) const
{
  const db::LogEntryData *le = log_entry (index);

  if (role == Qt::DecorationRole) {

    if (le) {
      return icon_for_severity (le->severity ());
    }

  } else if (role == Qt::DisplayRole) {

    if (le) {
      return QVariant (tl::to_qstring (le->to_string (false)));
    } else if (! index.parent ().isValid () && index.row () >= m_global_entries && index.row () < int (m_circuits.size ()) + m_global_entries) {
      const std::pair<const db::Circuit *, const db::Circuit *> &cp = m_circuits [index.row () - m_global_entries].first;
      if (! cp.first) {
        return QVariant (tr ("Circuit ") + tl::to_qstring (std::string ("-") + var_sep + cp.second->name ()));
      } else if (! cp.second) {
        return QVariant (tr ("Circuit ") + tl::to_qstring (cp.first->name () + var_sep + std::string ("-")));
      } else if (cp.first->name () != cp.second->name ()) {
        return QVariant (tr ("Circuit ") + tl::to_qstring (cp.first->name () + var_sep + cp.second->name ()));
      } else {
        return QVariant (tr ("Circuit ") + tl::to_qstring (cp.first->name ()));
      }
    }

  } else if (role == Qt::FontRole) {

    if (le) {
      QFont f;
      f.setBold (le->severity () == db::Error);
      return QVariant (f);
    } else if (! index.parent ().isValid () && index.row () >= m_global_entries && index.row () < int (m_circuits.size ()) + m_global_entries) {
      QFont f;
      f.setBold (true);
      return QVariant (f);
    }

  } else if (role == Qt::ForegroundRole) {

    if (! le) {
      //  ignore
    } else if (le->severity () == db::Error) {
      return QColor (255, 0, 0);
    } else if (le->severity () == db::Warning) {
      return QColor (0, 0, 255);
    }

  }

  return QVariant ();
}

QVariant
NetlistLogModel::headerData (int section, Qt::Orientation /*orientation*/, int role) const
{
  if (role == Qt::DisplayRole && section == 0) {
    return QVariant (tr ("Message"));
  } else {
    return QVariant ();
  }
}

}

#endif
